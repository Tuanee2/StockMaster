#include "stockmaster/application/order_service.h"

#include <QDate>
#include <QSqlError>
#include <QSqlQuery>
#include <QUuid>

#include <algorithm>

#include "stockmaster/application/customer_service.h"
#include "stockmaster/application/product_service.h"
#include "stockmaster/infra/db/database_service.h"

namespace stockmaster::application {

namespace {

int nextOrderSerial(const QVector<domain::Order> &orders)
{
    int maxSerial = 0;
    for (const domain::Order &order : orders) {
        if (!order.orderNo.startsWith(QLatin1String("ORD"))) {
            continue;
        }

        bool ok = false;
        const int serial = order.orderNo.mid(3).toInt(&ok);
        if (ok && serial > maxSerial) {
            maxSerial = serial;
        }
    }

    return maxSerial + 1;
}

}

OrderService::OrderService(infra::db::DatabaseService &databaseService,
                           const CustomerService &customerService,
                           ProductService &productService)
    : m_databaseService(databaseService)
    , m_customerService(customerService)
    , m_productService(productService)
{
    (void)loadFromDatabase();
}

domain::DashboardMetrics OrderService::loadDashboardMetrics() const
{
    domain::DashboardMetrics metrics;

    if (!m_databaseService.isReady()) {
        return metrics;
    }

    metrics.customerCount = m_customerService.activeCustomerCount();
    metrics.productCount = m_productService.activeProductCount();
    metrics.openOrderCount = openOrderCount();
    metrics.lowStockCount = m_productService.lowStockProductCount();
    metrics.expiringSoonProductCount = m_productService.expiringSoonProductCount();

    for (const domain::Order &order : m_orders) {
        if (order.status == domain::OrderStatus::Confirmed
            || order.status == domain::OrderStatus::PartiallyPaid) {
            metrics.receivableVnd += order.balanceVnd;
        }
    }

    return metrics;
}

QVector<domain::Order> OrderService::findOrders() const
{
    QVector<domain::Order> result = m_orders;

    std::sort(result.begin(), result.end(), [](const domain::Order &lhs, const domain::Order &rhs) {
        if (lhs.orderDate == rhs.orderDate) {
            return lhs.orderNo > rhs.orderNo;
        }

        return lhs.orderDate > rhs.orderDate;
    });

    return result;
}

QVector<domain::OrderItem> OrderService::findOrderItems(const QString &orderId) const
{
    return m_orderItemsByOrder.value(orderId.trimmed());
}

QVector<StockAllocationInfo> OrderService::findAllocations(const QString &orderId,
                                                           const QString &orderItemId) const
{
    const QString normalizedOrderId = orderId.trimmed();
    const QString normalizedOrderItemId = orderItemId.trimmed();

    QVector<StockAllocationInfo> result;
    result.reserve(m_stockAllocations.size());

    for (const StockAllocationInfo &allocation : m_stockAllocations) {
        if (allocation.orderId != normalizedOrderId) {
            continue;
        }

        if (!normalizedOrderItemId.isEmpty() && allocation.orderItemId != normalizedOrderItemId) {
            continue;
        }

        result.push_back(allocation);
    }

    return result;
}

bool OrderService::getOrderById(const QString &orderId, domain::Order &order) const
{
    qsizetype index = -1;
    if (!findOrderIndex(orderId, index)) {
        return false;
    }

    order = m_orders.at(index);
    return true;
}

int OrderService::openOrderCount() const
{
    int count = 0;

    for (const domain::Order &order : m_orders) {
        if (order.status == domain::OrderStatus::Draft
            || order.status == domain::OrderStatus::Confirmed
            || order.status == domain::OrderStatus::PartiallyPaid) {
            ++count;
        }
    }

    return count;
}

bool OrderService::saveToDatabase(QString &errorMessage) const
{
    return persistState(m_orders, m_orderItemsByOrder, m_stockAllocations, errorMessage);
}

void OrderService::restoreOrderSnapshot(const domain::Order &orderSnapshot)
{
    for (domain::Order &order : m_orders) {
        if (order.id == orderSnapshot.id) {
            order = orderSnapshot;
            return;
        }
    }
}

bool OrderService::createDraftOrder(const QString &customerId,
                                    const QString &orderDate,
                                    QString &orderIdOut,
                                    QString &errorMessage)
{
    if (!m_databaseService.isReady()) {
        errorMessage = QStringLiteral("Database chưa sẵn sàng.");
        return false;
    }

    const QString normalizedCustomerId = customerId.trimmed();
    if (!ensureCustomerExists(normalizedCustomerId)) {
        errorMessage = QStringLiteral("Khách hàng không hợp lệ.");
        return false;
    }

    int nextOrderSerial = m_nextOrderSerial;

    domain::Order order;
    order.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    order.orderNo = QStringLiteral("ORD%1").arg(nextOrderSerial++, 5, 10, QLatin1Char('0'));
    order.customerId = normalizedCustomerId;
    order.orderDate = orderDate.trimmed().isEmpty()
        ? QDate::currentDate().toString(Qt::ISODate)
        : orderDate.trimmed();
    order.status = domain::OrderStatus::Draft;

    QVector<domain::Order> updatedOrders = m_orders;
    updatedOrders.push_back(order);

    QHash<QString, QVector<domain::OrderItem>> updatedItemsByOrder = m_orderItemsByOrder;
    updatedItemsByOrder.insert(order.id, {});

    if (!persistState(updatedOrders, updatedItemsByOrder, m_stockAllocations, errorMessage)) {
        return false;
    }

    m_orders = updatedOrders;
    m_orderItemsByOrder = updatedItemsByOrder;
    m_nextOrderSerial = nextOrderSerial;

    orderIdOut = order.id;
    errorMessage.clear();
    return true;
}

bool OrderService::updateDraftOrderCustomer(const QString &orderId,
                                            const QString &customerId,
                                            QString &errorMessage)
{
    qsizetype index = -1;
    if (!findOrderIndex(orderId, index)) {
        errorMessage = QStringLiteral("Không tìm thấy đơn hàng.");
        return false;
    }

    const domain::Order &currentOrder = m_orders[index];
    if (!canEditDraft(currentOrder, errorMessage)) {
        return false;
    }

    const QString normalizedCustomerId = customerId.trimmed();
    if (!ensureCustomerExists(normalizedCustomerId)) {
        errorMessage = QStringLiteral("Khách hàng không hợp lệ.");
        return false;
    }

    QVector<domain::Order> updatedOrders = m_orders;
    updatedOrders[index].customerId = normalizedCustomerId;

    if (!persistState(updatedOrders, m_orderItemsByOrder, m_stockAllocations, errorMessage)) {
        return false;
    }

    m_orders = updatedOrders;
    errorMessage.clear();
    return true;
}

bool OrderService::upsertDraftItem(const QString &orderId,
                                   const DraftOrderItemInput &input,
                                   QString &errorMessage)
{
    qsizetype index = -1;
    if (!findOrderIndex(orderId, index)) {
        errorMessage = QStringLiteral("Không tìm thấy đơn hàng.");
        return false;
    }

    const domain::Order &currentOrder = m_orders[index];
    if (!canEditDraft(currentOrder, errorMessage)) {
        return false;
    }

    const QString normalizedProductId = input.productId.trimmed();
    if (!ensureProductExists(normalizedProductId)) {
        errorMessage = QStringLiteral("Sản phẩm không hợp lệ.");
        return false;
    }

    if (input.qty <= 0) {
        errorMessage = QStringLiteral("Số lượng phải > 0.");
        return false;
    }

    if (input.unitPriceVnd < 0) {
        errorMessage = QStringLiteral("Đơn giá phải >= 0.");
        return false;
    }

    if (input.discountVnd < 0) {
        errorMessage = QStringLiteral("Giảm giá phải >= 0.");
        return false;
    }

    const core::Money lineSubtotal = static_cast<core::Money>(input.qty) * input.unitPriceVnd;
    if (input.discountVnd > lineSubtotal) {
        errorMessage = QStringLiteral("Giảm giá không thể lớn hơn thành tiền.");
        return false;
    }

    QVector<domain::Order> updatedOrders = m_orders;
    QHash<QString, QVector<domain::OrderItem>> updatedItemsByOrder = m_orderItemsByOrder;
    QVector<domain::OrderItem> &items = updatedItemsByOrder[orderId.trimmed()];

    QString targetOrderItemId;
    bool foundExisting = false;
    for (domain::OrderItem &item : items) {
        if (item.productId != normalizedProductId) {
            continue;
        }

        targetOrderItemId = item.id;
        item.qty = input.qty;
        item.unitPriceVnd = input.unitPriceVnd;
        item.discountVnd = input.discountVnd;
        item.lineTotalVnd = lineSubtotal - input.discountVnd;
        foundExisting = true;
        break;
    }

    if (!foundExisting) {
        domain::OrderItem item;
        item.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
        item.productId = normalizedProductId;
        item.qty = input.qty;
        item.unitPriceVnd = input.unitPriceVnd;
        item.discountVnd = input.discountVnd;
        item.lineTotalVnd = lineSubtotal - input.discountVnd;
        targetOrderItemId = item.id;
        items.push_back(item);
    }

    QVector<StockAllocationInfo> plannedAllocations;
    if (!planDraftAllocationsForItem(orderId.trimmed(),
                                     targetOrderItemId,
                                     input,
                                     plannedAllocations,
                                     errorMessage)) {
        return false;
    }

    QVector<StockAllocationInfo> updatedAllocations = m_stockAllocations;
    updatedAllocations.erase(
        std::remove_if(updatedAllocations.begin(),
                       updatedAllocations.end(),
                       [&orderId, &targetOrderItemId](const StockAllocationInfo &allocation) {
                           return allocation.orderId == orderId.trimmed()
                                  && allocation.orderItemId == targetOrderItemId;
                       }),
        updatedAllocations.end());
    for (const StockAllocationInfo &allocation : plannedAllocations) {
        updatedAllocations.push_back(allocation);
    }

    recomputeOrderTotals(updatedOrders[index], items);

    if (!persistState(updatedOrders, updatedItemsByOrder, updatedAllocations, errorMessage)) {
        return false;
    }

    m_orders = updatedOrders;
    m_orderItemsByOrder = updatedItemsByOrder;
    m_stockAllocations = updatedAllocations;
    errorMessage.clear();
    return true;
}

bool OrderService::removeDraftItem(const QString &orderId,
                                   const QString &orderItemId,
                                   QString &errorMessage)
{
    qsizetype index = -1;
    if (!findOrderIndex(orderId, index)) {
        errorMessage = QStringLiteral("Không tìm thấy đơn hàng.");
        return false;
    }

    const domain::Order &currentOrder = m_orders[index];
    if (!canEditDraft(currentOrder, errorMessage)) {
        return false;
    }

    QHash<QString, QVector<domain::OrderItem>> updatedItemsByOrder = m_orderItemsByOrder;
    QVector<domain::OrderItem> &items = updatedItemsByOrder[orderId.trimmed()];
    const QString normalizedOrderItemId = orderItemId.trimmed();

    for (qsizetype itemIndex = 0; itemIndex < items.size(); ++itemIndex) {
        if (items[itemIndex].id != normalizedOrderItemId) {
            continue;
        }

        items.remove(itemIndex);
        QVector<StockAllocationInfo> updatedAllocations = m_stockAllocations;
        updatedAllocations.erase(
            std::remove_if(updatedAllocations.begin(),
                           updatedAllocations.end(),
                           [&orderId, &normalizedOrderItemId](const StockAllocationInfo &allocation) {
                               return allocation.orderId == orderId.trimmed()
                                      && allocation.orderItemId == normalizedOrderItemId;
                           }),
            updatedAllocations.end());
        QVector<domain::Order> updatedOrders = m_orders;
        recomputeOrderTotals(updatedOrders[index], items);

        if (!persistState(updatedOrders, updatedItemsByOrder, updatedAllocations, errorMessage)) {
            return false;
        }

        m_orders = updatedOrders;
        m_orderItemsByOrder = updatedItemsByOrder;
        m_stockAllocations = updatedAllocations;
        errorMessage.clear();
        return true;
    }

    errorMessage = QStringLiteral("Không tìm thấy dòng sản phẩm trong đơn.");
    return false;
}

bool OrderService::confirmOrder(const QString &orderId, QString &errorMessage)
{
    qsizetype index = -1;
    if (!findOrderIndex(orderId, index)) {
        errorMessage = QStringLiteral("Không tìm thấy đơn hàng.");
        return false;
    }

    domain::Order &order = m_orders[index];
    if (!canEditDraft(order, errorMessage)) {
        return false;
    }

    const QVector<domain::OrderItem> items = m_orderItemsByOrder.value(order.id);
    if (items.isEmpty()) {
        errorMessage = QStringLiteral("Đơn Draft chưa có sản phẩm.");
        return false;
    }

    QVector<StockAllocationInfo> plannedAllocations;
    if (!planStockAllocations(order.id, items, plannedAllocations, errorMessage)) {
        return false;
    }

    QVector<StockAllocationInfo> appliedAllocations;
    appliedAllocations.reserve(plannedAllocations.size());
    const QString confirmReason = QStringLiteral("Xác nhận đơn %1").arg(order.orderNo);

    auto revertAllocatedStock = [this, &appliedAllocations]() {
        for (qsizetype rollbackIndex = appliedAllocations.size() - 1; rollbackIndex >= 0; --rollbackIndex) {
            QString rollbackError;
            (void)m_productService.stockIn(appliedAllocations[rollbackIndex].lotId,
                                           appliedAllocations[rollbackIndex].qty,
                                           rollbackError,
                                           QString(),
                                           QString(),
                                           false,
                                           false);

            if (rollbackIndex == 0) {
                break;
            }
        }
    };

    for (const StockAllocationInfo &allocation : plannedAllocations) {
        QString stockError;
        if (!m_productService.stockOut(allocation.lotId,
                                       allocation.qty,
                                       stockError,
                                       confirmReason,
                                       order.orderDate,
                                       true,
                                       false)) {
            revertAllocatedStock();
            errorMessage = QStringLiteral("Xác nhận đơn thất bại khi trừ kho: %1").arg(stockError);
            return false;
        }

        appliedAllocations.push_back(allocation);
    }

    const domain::Order previousOrder = order;
    const QVector<StockAllocationInfo> previousAllocations = m_stockAllocations;

    m_stockAllocations.erase(
        std::remove_if(m_stockAllocations.begin(),
                       m_stockAllocations.end(),
                       [&order](const StockAllocationInfo &allocation) {
                           return allocation.orderId == order.id;
                       }),
        m_stockAllocations.end());
    for (const StockAllocationInfo &allocation : plannedAllocations) {
        m_stockAllocations.push_back(allocation);
    }

    order.status = domain::OrderStatus::Confirmed;
    recomputeOrderTotals(order, items);

    if (!m_databaseService.beginTransaction()) {
        errorMessage = m_databaseService.lastError();
        order = previousOrder;
        m_stockAllocations = previousAllocations;
        revertAllocatedStock();
        return false;
    }

    QString persistenceError;
    if (!m_productService.saveToDatabase(persistenceError) || !saveToDatabase(persistenceError)) {
        m_databaseService.rollbackTransaction();
        order = previousOrder;
        m_stockAllocations = previousAllocations;
        revertAllocatedStock();
        errorMessage = persistenceError;
        return false;
    }

    if (!m_databaseService.commitTransaction()) {
        errorMessage = m_databaseService.lastError();
        m_databaseService.rollbackTransaction();
        order = previousOrder;
        m_stockAllocations = previousAllocations;
        revertAllocatedStock();
        return false;
    }

    errorMessage.clear();
    return true;
}

bool OrderService::applyPayment(const QString &orderId,
                                core::Money amountVnd,
                                QString &errorMessage,
                                bool persistChanges)
{
    qsizetype index = -1;
    if (!findOrderIndex(orderId, index)) {
        errorMessage = QStringLiteral("Không tìm thấy đơn hàng.");
        return false;
    }

    QVector<domain::Order> updatedOrders = m_orders;
    domain::Order &order = updatedOrders[index];

    if (amountVnd <= 0) {
        errorMessage = QStringLiteral("Số tiền thu phải > 0.");
        return false;
    }

    if (order.status == domain::OrderStatus::Draft) {
        errorMessage = QStringLiteral("Chỉ đơn đã Confirmed/PartiallyPaid mới được thu tiền.");
        return false;
    }

    if (order.status == domain::OrderStatus::Voided) {
        errorMessage = QStringLiteral("Đơn đã Voided, không thể thu tiền.");
        return false;
    }

    if (order.status == domain::OrderStatus::Paid || order.balanceVnd <= 0) {
        errorMessage = QStringLiteral("Đơn đã thanh toán đủ.");
        return false;
    }

    if (amountVnd > order.balanceVnd) {
        errorMessage = QStringLiteral("Không được thu vượt công nợ còn lại của đơn.");
        return false;
    }

    order.paidVnd += amountVnd;
    order.balanceVnd = std::max<core::Money>(0, order.totalVnd - order.paidVnd);
    order.status = order.balanceVnd == 0
        ? domain::OrderStatus::Paid
        : domain::OrderStatus::PartiallyPaid;

    if (persistChanges && !persistState(updatedOrders, m_orderItemsByOrder, m_stockAllocations, errorMessage)) {
        return false;
    }

    m_orders = updatedOrders;
    errorMessage.clear();
    return true;
}

bool OrderService::voidOrder(const QString &orderId, QString &errorMessage)
{
    qsizetype index = -1;
    if (!findOrderIndex(orderId, index)) {
        errorMessage = QStringLiteral("Không tìm thấy đơn hàng.");
        return false;
    }

    domain::Order &order = m_orders[index];
    if (!canVoid(order, errorMessage)) {
        return false;
    }

    QVector<StockAllocationInfo> allocations;
    if (order.status == domain::OrderStatus::Confirmed) {
        for (const StockAllocationInfo &allocation : m_stockAllocations) {
            if (allocation.orderId == order.id) {
                allocations.push_back(allocation);
            }
        }
    }

    QVector<StockAllocationInfo> restoredAllocations;
    restoredAllocations.reserve(allocations.size());
    const QString voidReason = QStringLiteral("Void đơn %1").arg(order.orderNo);

    auto undoRestoredStock = [this, &restoredAllocations]() {
        for (qsizetype rollbackIndex = restoredAllocations.size() - 1; rollbackIndex >= 0; --rollbackIndex) {
            QString rollbackError;
            (void)m_productService.stockOut(restoredAllocations[rollbackIndex].lotId,
                                            restoredAllocations[rollbackIndex].qty,
                                            rollbackError,
                                            QString(),
                                            QString(),
                                            false,
                                            false);

            if (rollbackIndex == 0) {
                break;
            }
        }
    };

    for (const StockAllocationInfo &allocation : allocations) {
        QString stockError;
        if (!m_productService.stockIn(allocation.lotId,
                                      allocation.qty,
                                      stockError,
                                      voidReason,
                                      QDate::currentDate().toString(Qt::ISODate),
                                      true,
                                      false)) {
            undoRestoredStock();
            errorMessage = QStringLiteral("Void đơn thất bại khi hoàn kho: %1").arg(stockError);
            return false;
        }

        restoredAllocations.push_back(allocation);
    }

    const domain::Order previousOrder = order;
    const QVector<StockAllocationInfo> previousAllocations = m_stockAllocations;

    if (!allocations.isEmpty()) {
        m_stockAllocations.erase(
            std::remove_if(m_stockAllocations.begin(), m_stockAllocations.end(), [&order](const StockAllocationInfo &allocation) {
                return allocation.orderId == order.id;
            }),
            m_stockAllocations.end());
    }

    order.status = domain::OrderStatus::Voided;

    if (!m_databaseService.beginTransaction()) {
        errorMessage = m_databaseService.lastError();
        order = previousOrder;
        m_stockAllocations = previousAllocations;
        undoRestoredStock();
        return false;
    }

    QString persistenceError;
    if (!m_productService.saveToDatabase(persistenceError) || !saveToDatabase(persistenceError)) {
        m_databaseService.rollbackTransaction();
        order = previousOrder;
        m_stockAllocations = previousAllocations;
        undoRestoredStock();
        errorMessage = persistenceError;
        return false;
    }

    if (!m_databaseService.commitTransaction()) {
        errorMessage = m_databaseService.lastError();
        m_databaseService.rollbackTransaction();
        order = previousOrder;
        m_stockAllocations = previousAllocations;
        undoRestoredStock();
        return false;
    }

    errorMessage.clear();
    return true;
}

bool OrderService::findOrderIndex(const QString &orderId, qsizetype &index) const
{
    const QString normalizedOrderId = orderId.trimmed();
    if (normalizedOrderId.isEmpty()) {
        return false;
    }

    for (qsizetype current = 0; current < m_orders.size(); ++current) {
        if (m_orders[current].id == normalizedOrderId) {
            index = current;
            return true;
        }
    }

    return false;
}

bool OrderService::canEditDraft(const domain::Order &order, QString &errorMessage) const
{
    if (order.status != domain::OrderStatus::Draft) {
        errorMessage = QStringLiteral("Chỉ đơn Draft mới được chỉnh sửa.");
        return false;
    }

    return true;
}

bool OrderService::canVoid(const domain::Order &order, QString &errorMessage) const
{
    if (order.status == domain::OrderStatus::Voided) {
        errorMessage = QStringLiteral("Đơn đã ở trạng thái Voided.");
        return false;
    }

    if (order.status != domain::OrderStatus::Draft
        && order.status != domain::OrderStatus::Confirmed) {
        errorMessage = QStringLiteral("Chỉ đơn Draft/Confirmed mới được Void.");
        return false;
    }

    return true;
}

bool OrderService::ensureCustomerExists(const QString &customerId) const
{
    return m_customerService.customerExists(customerId);
}

bool OrderService::ensureProductExists(const QString &productId) const
{
    return m_productService.productExists(productId);
}

bool OrderService::planDraftAllocationsForItem(const QString &orderId,
                                               const QString &orderItemId,
                                               const DraftOrderItemInput &input,
                                               QVector<StockAllocationInfo> &planned,
                                               QString &errorMessage) const
{
    planned.clear();

    QVector<domain::ProductLot> lots = m_productService.findLotsByProduct(input.productId);
    if (lots.isEmpty()) {
        errorMessage = QStringLiteral("Sản phẩm chưa có lô tồn kho để chọn.");
        return false;
    }

    qsizetype preferredIndex = -1;
    const QString preferredLotId = input.preferredLotId.trimmed();
    if (!preferredLotId.isEmpty()) {
        for (qsizetype index = 0; index < lots.size(); ++index) {
            if (lots[index].id == preferredLotId) {
                preferredIndex = index;
                break;
            }
        }

        if (preferredIndex < 0) {
            errorMessage = QStringLiteral("Lô được chọn không hợp lệ cho sản phẩm này.");
            return false;
        }
    }

    std::sort(lots.begin(), lots.end(), [](const domain::ProductLot &lhs, const domain::ProductLot &rhs) {
        if (lhs.receivedDate == rhs.receivedDate) {
            return lhs.lotNo < rhs.lotNo;
        }

        return lhs.receivedDate < rhs.receivedDate;
    });

    if (preferredIndex >= 0) {
        auto preferredIt = std::find_if(lots.begin(), lots.end(), [&preferredLotId](const domain::ProductLot &lot) {
            return lot.id == preferredLotId;
        });
        if (preferredIt != lots.end()) {
            std::rotate(lots.begin(), preferredIt, preferredIt + 1);
        }
    }

    int remainingQty = input.qty;
    for (const domain::ProductLot &lot : lots) {
        if (remainingQty <= 0) {
            break;
        }

        if (lot.onHandQty <= 0) {
            continue;
        }

        const int allocatedQty = std::min(remainingQty, lot.onHandQty);
        if (allocatedQty <= 0) {
            continue;
        }

        planned.push_back(StockAllocationInfo{orderId, orderItemId, lot.id, allocatedQty});
        remainingQty -= allocatedQty;
    }

    if (remainingQty > 0) {
        const QString productName = m_productService.productNameById(input.productId);
        errorMessage = QStringLiteral("Không đủ tồn theo các lô hiện có cho sản phẩm: %1")
                           .arg(productName.isEmpty() ? input.productId : productName);
        return false;
    }

    return true;
}

bool OrderService::planStockAllocations(const QString &orderId,
                                        const QVector<domain::OrderItem> &items,
                                        QVector<StockAllocationInfo> &planned,
                                        QString &errorMessage) const
{
    planned.clear();

    for (const domain::OrderItem &item : items) {
        int remainingQty = item.qty;
        if (remainingQty <= 0) {
            errorMessage = QStringLiteral("Đơn có dòng sản phẩm với số lượng không hợp lệ.");
            return false;
        }

        QVector<domain::ProductLot> lots = m_productService.findLotsByProduct(item.productId);
        std::sort(lots.begin(), lots.end(), [](const domain::ProductLot &lhs, const domain::ProductLot &rhs) {
            if (lhs.receivedDate == rhs.receivedDate) {
                return lhs.lotNo < rhs.lotNo;
            }

            return lhs.receivedDate < rhs.receivedDate;
        });

        QHash<QString, int> availableByLotId;
        availableByLotId.reserve(lots.size());
        for (const domain::ProductLot &lot : lots) {
            availableByLotId.insert(lot.id, lot.onHandQty);
        }

        const QVector<StockAllocationInfo> preferredAllocations = findAllocations(orderId, item.id);
        for (const StockAllocationInfo &allocation : preferredAllocations) {
            if (remainingQty <= 0) {
                break;
            }

            if (!availableByLotId.contains(allocation.lotId) || allocation.qty <= 0) {
                continue;
            }

            const int availableQty = availableByLotId.value(allocation.lotId);
            const int allocatedQty = std::min({allocation.qty, availableQty, remainingQty});
            if (allocatedQty <= 0) {
                continue;
            }

            planned.push_back(StockAllocationInfo{orderId, item.id, allocation.lotId, allocatedQty});
            availableByLotId.insert(allocation.lotId, availableQty - allocatedQty);
            remainingQty -= allocatedQty;
        }

        for (const domain::ProductLot &lot : lots) {
            if (remainingQty <= 0) {
                break;
            }

            const int availableQty = availableByLotId.value(lot.id);
            if (availableQty <= 0) {
                continue;
            }

            const int allocatedQty = std::min(remainingQty, availableQty);
            if (allocatedQty <= 0) {
                continue;
            }

            planned.push_back(StockAllocationInfo{orderId, item.id, lot.id, allocatedQty});
            availableByLotId.insert(lot.id, availableQty - allocatedQty);
            remainingQty -= allocatedQty;
        }

        if (remainingQty > 0) {
            const QString productName = m_productService.productNameById(item.productId);
            errorMessage = QStringLiteral("Không đủ tồn để xác nhận đơn cho sản phẩm: %1")
                               .arg(productName.isEmpty() ? item.productId : productName);
            return false;
        }
    }

    return true;
}

void OrderService::recomputeOrderTotals(domain::Order &order, const QVector<domain::OrderItem> &items)
{
    core::Money subtotal = 0;
    core::Money discount = 0;

    for (const domain::OrderItem &item : items) {
        subtotal += calculateLineSubtotal(item);
        discount += item.discountVnd;
    }

    order.subtotalVnd = subtotal;
    order.discountVnd = discount;
    order.totalVnd = std::max<core::Money>(0, subtotal - discount);
    order.balanceVnd = std::max<core::Money>(0, order.totalVnd - order.paidVnd);
}

core::Money OrderService::calculateLineSubtotal(const domain::OrderItem &item)
{
    return static_cast<core::Money>(item.qty) * item.unitPriceVnd;
}

QString OrderService::statusToString(domain::OrderStatus status)
{
    switch (status) {
    case domain::OrderStatus::Draft:
        return QStringLiteral("Draft");
    case domain::OrderStatus::Confirmed:
        return QStringLiteral("Confirmed");
    case domain::OrderStatus::PartiallyPaid:
        return QStringLiteral("PartiallyPaid");
    case domain::OrderStatus::Paid:
        return QStringLiteral("Paid");
    case domain::OrderStatus::Voided:
        return QStringLiteral("Voided");
    }

    return QStringLiteral("Draft");
}

domain::OrderStatus OrderService::statusFromString(const QString &value)
{
    if (value == QLatin1String("Confirmed")) {
        return domain::OrderStatus::Confirmed;
    }
    if (value == QLatin1String("PartiallyPaid")) {
        return domain::OrderStatus::PartiallyPaid;
    }
    if (value == QLatin1String("Paid")) {
        return domain::OrderStatus::Paid;
    }
    if (value == QLatin1String("Voided")) {
        return domain::OrderStatus::Voided;
    }

    return domain::OrderStatus::Draft;
}

bool OrderService::loadFromDatabase()
{
    m_orders.clear();
    m_orderItemsByOrder.clear();
    m_stockAllocations.clear();
    m_nextOrderSerial = 1;

    if (!m_databaseService.isReady()) {
        return false;
    }

    QSqlQuery orderQuery(m_databaseService.database());
    if (!orderQuery.exec(QStringLiteral(
            "SELECT id, order_no, customer_id, order_date, status, subtotal_vnd, discount_vnd, total_vnd, paid_vnd, balance_vnd "
            "FROM orders ORDER BY order_date ASC, order_no ASC;"))) {
        return false;
    }

    while (orderQuery.next()) {
        domain::Order order;
        order.id = orderQuery.value(0).toString();
        order.orderNo = orderQuery.value(1).toString();
        order.customerId = orderQuery.value(2).toString();
        order.orderDate = orderQuery.value(3).toString();
        order.status = statusFromString(orderQuery.value(4).toString());
        order.subtotalVnd = orderQuery.value(5).toLongLong();
        order.discountVnd = orderQuery.value(6).toLongLong();
        order.totalVnd = orderQuery.value(7).toLongLong();
        order.paidVnd = orderQuery.value(8).toLongLong();
        order.balanceVnd = orderQuery.value(9).toLongLong();
        m_orders.push_back(order);
        m_orderItemsByOrder.insert(order.id, {});
    }

    QSqlQuery itemQuery(m_databaseService.database());
    if (!itemQuery.exec(QStringLiteral(
            "SELECT id, order_id, product_id, qty, unit_price_vnd, discount_vnd, line_total_vnd "
            "FROM order_items ORDER BY order_id ASC, id ASC;"))) {
        return false;
    }

    while (itemQuery.next()) {
        domain::OrderItem item;
        item.id = itemQuery.value(0).toString();
        const QString orderId = itemQuery.value(1).toString();
        item.productId = itemQuery.value(2).toString();
        item.qty = itemQuery.value(3).toInt();
        item.unitPriceVnd = itemQuery.value(4).toLongLong();
        item.discountVnd = itemQuery.value(5).toLongLong();
        item.lineTotalVnd = itemQuery.value(6).toLongLong();
        m_orderItemsByOrder[orderId].push_back(item);
    }

    QSqlQuery allocationQuery(m_databaseService.database());
    if (!allocationQuery.exec(QStringLiteral(
            "SELECT order_id, order_item_id, lot_id, qty "
            "FROM stock_allocations ORDER BY order_id ASC, order_item_id ASC, lot_id ASC;"))) {
        return false;
    }

    while (allocationQuery.next()) {
        m_stockAllocations.push_back(StockAllocationInfo{
            allocationQuery.value(0).toString(),
            allocationQuery.value(1).toString(),
            allocationQuery.value(2).toString(),
            allocationQuery.value(3).toInt(),
        });
    }

    recomputeNextOrderSerial();
    return true;
}

bool OrderService::persistState(const QVector<domain::Order> &orders,
                                const QHash<QString, QVector<domain::OrderItem>> &orderItemsByOrder,
                                const QVector<StockAllocationInfo> &stockAllocations,
                                QString &errorMessage) const
{
    if (!m_databaseService.isReady()) {
        errorMessage.clear();
        return true;
    }

    if (!m_databaseService.beginTransaction()) {
        errorMessage = m_databaseService.lastError();
        return false;
    }

    auto rollbackWith = [this, &errorMessage](const QString &message) {
        errorMessage = message;
        m_databaseService.rollbackTransaction();
        return false;
    };

    QSqlQuery clearAllocationsQuery(m_databaseService.database());
    if (!clearAllocationsQuery.exec(QStringLiteral("DELETE FROM stock_allocations;"))) {
        return rollbackWith(clearAllocationsQuery.lastError().text());
    }

    QSqlQuery clearItemsQuery(m_databaseService.database());
    if (!clearItemsQuery.exec(QStringLiteral("DELETE FROM order_items;"))) {
        return rollbackWith(clearItemsQuery.lastError().text());
    }

    QSqlQuery clearOrdersQuery(m_databaseService.database());
    if (!clearOrdersQuery.exec(QStringLiteral("DELETE FROM orders;"))) {
        return rollbackWith(clearOrdersQuery.lastError().text());
    }

    QSqlQuery insertOrderQuery(m_databaseService.database());
    if (!insertOrderQuery.prepare(QStringLiteral(
            "INSERT INTO orders("
            "id, order_no, customer_id, order_date, status, subtotal_vnd, discount_vnd, total_vnd, paid_vnd, balance_vnd"
            ") VALUES("
            ":id, :order_no, :customer_id, :order_date, :status, :subtotal_vnd, :discount_vnd, :total_vnd, :paid_vnd, :balance_vnd"
            ");"))) {
        return rollbackWith(insertOrderQuery.lastError().text());
    }

    for (const domain::Order &order : orders) {
        insertOrderQuery.bindValue(QStringLiteral(":id"), order.id);
        insertOrderQuery.bindValue(QStringLiteral(":order_no"), order.orderNo);
        insertOrderQuery.bindValue(QStringLiteral(":customer_id"), order.customerId);
        insertOrderQuery.bindValue(QStringLiteral(":order_date"), order.orderDate);
        insertOrderQuery.bindValue(QStringLiteral(":status"), statusToString(order.status));
        insertOrderQuery.bindValue(QStringLiteral(":subtotal_vnd"), order.subtotalVnd);
        insertOrderQuery.bindValue(QStringLiteral(":discount_vnd"), order.discountVnd);
        insertOrderQuery.bindValue(QStringLiteral(":total_vnd"), order.totalVnd);
        insertOrderQuery.bindValue(QStringLiteral(":paid_vnd"), order.paidVnd);
        insertOrderQuery.bindValue(QStringLiteral(":balance_vnd"), order.balanceVnd);
        if (!insertOrderQuery.exec()) {
            return rollbackWith(insertOrderQuery.lastError().text());
        }
    }

    QSqlQuery insertItemQuery(m_databaseService.database());
    if (!insertItemQuery.prepare(QStringLiteral(
            "INSERT INTO order_items("
            "id, order_id, product_id, qty, unit_price_vnd, discount_vnd, line_total_vnd"
            ") VALUES("
            ":id, :order_id, :product_id, :qty, :unit_price_vnd, :discount_vnd, :line_total_vnd"
            ");"))) {
        return rollbackWith(insertItemQuery.lastError().text());
    }

    for (auto it = orderItemsByOrder.cbegin(); it != orderItemsByOrder.cend(); ++it) {
        for (const domain::OrderItem &item : it.value()) {
            insertItemQuery.bindValue(QStringLiteral(":id"), item.id);
            insertItemQuery.bindValue(QStringLiteral(":order_id"), it.key());
            insertItemQuery.bindValue(QStringLiteral(":product_id"), item.productId);
            insertItemQuery.bindValue(QStringLiteral(":qty"), item.qty);
            insertItemQuery.bindValue(QStringLiteral(":unit_price_vnd"), item.unitPriceVnd);
            insertItemQuery.bindValue(QStringLiteral(":discount_vnd"), item.discountVnd);
            insertItemQuery.bindValue(QStringLiteral(":line_total_vnd"), item.lineTotalVnd);
            if (!insertItemQuery.exec()) {
                return rollbackWith(insertItemQuery.lastError().text());
            }
        }
    }

    QSqlQuery insertAllocationQuery(m_databaseService.database());
    if (!insertAllocationQuery.prepare(QStringLiteral(
            "INSERT INTO stock_allocations("
            "order_id, order_item_id, lot_id, qty"
            ") VALUES("
            ":order_id, :order_item_id, :lot_id, :qty"
            ");"))) {
        return rollbackWith(insertAllocationQuery.lastError().text());
    }

    for (const StockAllocationInfo &allocation : stockAllocations) {
        insertAllocationQuery.bindValue(QStringLiteral(":order_id"), allocation.orderId);
        insertAllocationQuery.bindValue(QStringLiteral(":order_item_id"), allocation.orderItemId);
        insertAllocationQuery.bindValue(QStringLiteral(":lot_id"), allocation.lotId);
        insertAllocationQuery.bindValue(QStringLiteral(":qty"), allocation.qty);
        if (!insertAllocationQuery.exec()) {
            return rollbackWith(insertAllocationQuery.lastError().text());
        }
    }

    if (!m_databaseService.commitTransaction()) {
        return rollbackWith(m_databaseService.lastError());
    }

    errorMessage.clear();
    return true;
}

void OrderService::recomputeNextOrderSerial()
{
    m_nextOrderSerial = nextOrderSerial(m_orders);
}

QString OrderService::nextOrderNo()
{
    return QStringLiteral("ORD%1").arg(m_nextOrderSerial++, 5, 10, QLatin1Char('0'));
}

} // namespace stockmaster::application
