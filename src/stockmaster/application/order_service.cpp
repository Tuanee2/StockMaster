#include "stockmaster/application/order_service.h"

#include <QDate>
#include <QUuid>

#include <algorithm>

#include "stockmaster/application/customer_service.h"
#include "stockmaster/application/product_service.h"
#include "stockmaster/infra/db/database_service.h"

namespace stockmaster::application {

OrderService::OrderService(const infra::db::DatabaseService &databaseService,
                           const CustomerService &customerService,
                           ProductService &productService)
    : m_databaseService(databaseService)
    , m_customerService(customerService)
    , m_productService(productService)
{
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

    domain::Order order;
    order.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    order.orderNo = nextOrderNo();
    order.customerId = normalizedCustomerId;
    order.orderDate = orderDate.trimmed().isEmpty()
        ? QDate::currentDate().toString(Qt::ISODate)
        : orderDate.trimmed();
    order.status = domain::OrderStatus::Draft;
    order.subtotalVnd = 0;
    order.discountVnd = 0;
    order.totalVnd = 0;
    order.paidVnd = 0;
    order.balanceVnd = 0;

    m_orders.push_back(order);
    m_orderItemsByOrder.insert(order.id, {});

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

    domain::Order &order = m_orders[index];
    if (!canEditDraft(order, errorMessage)) {
        return false;
    }

    const QString normalizedCustomerId = customerId.trimmed();
    if (!ensureCustomerExists(normalizedCustomerId)) {
        errorMessage = QStringLiteral("Khách hàng không hợp lệ.");
        return false;
    }

    order.customerId = normalizedCustomerId;
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

    domain::Order &order = m_orders[index];
    if (!canEditDraft(order, errorMessage)) {
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

    QVector<domain::OrderItem> &items = m_orderItemsByOrder[order.id];

    for (domain::OrderItem &item : items) {
        if (item.productId != normalizedProductId) {
            continue;
        }

        item.qty = input.qty;
        item.unitPriceVnd = input.unitPriceVnd;
        item.discountVnd = input.discountVnd;
        item.lineTotalVnd = lineSubtotal - input.discountVnd;

        recomputeOrderTotals(order);
        errorMessage.clear();
        return true;
    }

    domain::OrderItem item;
    item.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    item.productId = normalizedProductId;
    item.qty = input.qty;
    item.unitPriceVnd = input.unitPriceVnd;
    item.discountVnd = input.discountVnd;
    item.lineTotalVnd = lineSubtotal - input.discountVnd;

    items.push_back(item);
    recomputeOrderTotals(order);

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

    domain::Order &order = m_orders[index];
    if (!canEditDraft(order, errorMessage)) {
        return false;
    }

    QVector<domain::OrderItem> &items = m_orderItemsByOrder[order.id];
    const QString normalizedOrderItemId = orderItemId.trimmed();

    for (qsizetype itemIndex = 0; itemIndex < items.size(); ++itemIndex) {
        if (items[itemIndex].id != normalizedOrderItemId) {
            continue;
        }

        items.remove(itemIndex);
        recomputeOrderTotals(order);

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

    QVector<StockAllocation> plannedAllocations;
    if (!planStockAllocations(order.id, items, plannedAllocations, errorMessage)) {
        return false;
    }

    QVector<StockAllocation> appliedAllocations;
    appliedAllocations.reserve(plannedAllocations.size());
    const QString confirmReason = QStringLiteral("Xác nhận đơn %1").arg(order.orderNo);

    for (const StockAllocation &allocation : plannedAllocations) {
        QString stockError;
        if (!m_productService.stockOut(allocation.lotId,
                                       allocation.qty,
                                       stockError,
                                       confirmReason,
                                       order.orderDate,
                                       true)) {
            for (qsizetype rollbackIndex = appliedAllocations.size() - 1; rollbackIndex >= 0; --rollbackIndex) {
                QString rollbackError;
                (void)m_productService.stockIn(appliedAllocations[rollbackIndex].lotId,
                                               appliedAllocations[rollbackIndex].qty,
                                               rollbackError,
                                               QString(),
                                               QString(),
                                               false);

                if (rollbackIndex == 0) {
                    break;
                }
            }

            errorMessage = QStringLiteral("Xác nhận đơn thất bại khi trừ kho: %1").arg(stockError);
            return false;
        }

        appliedAllocations.push_back(allocation);
    }

    for (const StockAllocation &allocation : plannedAllocations) {
        m_stockAllocations.push_back(allocation);
    }

    order.status = domain::OrderStatus::Confirmed;
    recomputeOrderTotals(order);

    errorMessage.clear();
    return true;
}

bool OrderService::applyPayment(const QString &orderId,
                                core::Money amountVnd,
                                QString &errorMessage)
{
    qsizetype index = -1;
    if (!findOrderIndex(orderId, index)) {
        errorMessage = QStringLiteral("Không tìm thấy đơn hàng.");
        return false;
    }

    domain::Order &order = m_orders[index];

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

    if (order.status == domain::OrderStatus::Confirmed) {
        QVector<StockAllocation> allocations;
        for (const StockAllocation &allocation : m_stockAllocations) {
            if (allocation.orderId == order.id) {
                allocations.push_back(allocation);
            }
        }

        QVector<StockAllocation> restoredAllocations;
        restoredAllocations.reserve(allocations.size());
        const QString voidReason = QStringLiteral("Void đơn %1").arg(order.orderNo);

        for (const StockAllocation &allocation : allocations) {
            QString stockError;
            if (!m_productService.stockIn(allocation.lotId,
                                          allocation.qty,
                                          stockError,
                                          voidReason,
                                          QDate::currentDate().toString(Qt::ISODate),
                                          true)) {
                for (qsizetype rollbackIndex = restoredAllocations.size() - 1; rollbackIndex >= 0; --rollbackIndex) {
                    QString rollbackError;
                    (void)m_productService.stockOut(restoredAllocations[rollbackIndex].lotId,
                                                    restoredAllocations[rollbackIndex].qty,
                                                    rollbackError,
                                                    QString(),
                                                    QString(),
                                                    false);

                    if (rollbackIndex == 0) {
                        break;
                    }
                }

                errorMessage = QStringLiteral("Void đơn thất bại khi hoàn kho: %1").arg(stockError);
                return false;
            }

            restoredAllocations.push_back(allocation);
        }

        m_stockAllocations.erase(
            std::remove_if(m_stockAllocations.begin(), m_stockAllocations.end(), [&order](const StockAllocation &allocation) {
                return allocation.orderId == order.id;
            }),
            m_stockAllocations.end());
    }

    order.status = domain::OrderStatus::Voided;
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

bool OrderService::planStockAllocations(const QString &orderId,
                                        const QVector<domain::OrderItem> &items,
                                        QVector<StockAllocation> &planned,
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

            planned.push_back(StockAllocation{orderId, item.id, lot.id, allocatedQty});
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

core::Money OrderService::calculateLineSubtotal(const domain::OrderItem &item)
{
    return static_cast<core::Money>(item.qty) * item.unitPriceVnd;
}

void OrderService::recomputeOrderTotals(domain::Order &order)
{
    const QVector<domain::OrderItem> items = m_orderItemsByOrder.value(order.id);

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

QString OrderService::nextOrderNo()
{
    return QStringLiteral("ORD%1").arg(m_nextOrderSerial++, 5, 10, QLatin1Char('0'));
}

} // namespace stockmaster::application
