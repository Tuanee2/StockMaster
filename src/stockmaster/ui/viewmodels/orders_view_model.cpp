#include "stockmaster/ui/viewmodels/orders_view_model.h"

#include <QDate>
#include <QLocale>
#include <QRegularExpression>
#include <QStringList>

#include <algorithm>

#include "stockmaster/application/customer_service.h"
#include "stockmaster/application/product_service.h"

namespace stockmaster::ui::viewmodels {

OrdersViewModel::OrdersViewModel(application::OrderService &orderService,
                                 const application::CustomerService &customerService,
                                 const application::ProductService &productService,
                                 QObject *parent)
    : QAbstractListModel(parent)
    , m_orderService(orderService)
    , m_customerService(customerService)
    , m_productService(productService)
{
    refreshLookups();
    refreshOrders();
}

int OrdersViewModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }

    return m_orders.size();
}

QVariant OrdersViewModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_orders.size()) {
        return {};
    }

    const domain::Order &order = m_orders.at(index.row());

    switch (role) {
    case OrderIdRole:
        return order.id;
    case OrderNoRole:
        return order.orderNo;
    case OrderDateRole:
        return order.orderDate;
    case StatusRole:
        return statusText(order.status);
    case CustomerNameRole: {
        const QString name = m_customerService.customerNameById(order.customerId);
        return name.isEmpty() ? QStringLiteral("<Unknown customer>") : name;
    }
    case TotalTextRole:
        return formatMoneyVnd(order.totalVnd);
    case ItemCountRole:
        return m_orderService.findOrderItems(order.id).size();
    default:
        return {};
    }
}

QHash<int, QByteArray> OrdersViewModel::roleNames() const
{
    return {
        {OrderIdRole, QByteArrayLiteral("orderId")},
        {OrderNoRole, QByteArrayLiteral("orderNo")},
        {OrderDateRole, QByteArrayLiteral("orderDate")},
        {StatusRole, QByteArrayLiteral("status")},
        {CustomerNameRole, QByteArrayLiteral("customerName")},
        {TotalTextRole, QByteArrayLiteral("totalText")},
        {ItemCountRole, QByteArrayLiteral("itemCount")},
    };
}

QString OrdersViewModel::lastError() const
{
    return m_lastError;
}

int OrdersViewModel::orderCount() const
{
    return m_orders.size();
}

QString OrdersViewModel::queryOrderNo() const
{
    return m_queryOrderNo;
}

QString OrdersViewModel::queryFromDate() const
{
    return m_queryFromDate;
}

QString OrdersViewModel::queryToDate() const
{
    return m_queryToDate;
}

bool OrdersViewModel::hasSelectedOrder() const
{
    return m_hasSelectedOrder;
}

QString OrdersViewModel::selectedOrderId() const
{
    return m_hasSelectedOrder ? m_selectedOrder.id : QString();
}

QString OrdersViewModel::selectedOrderNo() const
{
    return m_hasSelectedOrder ? m_selectedOrder.orderNo : QString();
}

QString OrdersViewModel::selectedOrderDate() const
{
    return m_hasSelectedOrder ? m_selectedOrder.orderDate : QString();
}

QString OrdersViewModel::selectedOrderStatus() const
{
    return m_hasSelectedOrder ? statusText(m_selectedOrder.status) : QString();
}

QString OrdersViewModel::selectedOrderCustomerId() const
{
    return m_hasSelectedOrder ? m_selectedOrder.customerId : QString();
}

QString OrdersViewModel::selectedOrderTotalText() const
{
    return m_hasSelectedOrder ? formatMoneyVnd(m_selectedOrder.totalVnd) : QStringLiteral("0 VND");
}

bool OrdersViewModel::canEditDraft() const
{
    return selectedOrderInStatus(domain::OrderStatus::Draft);
}

bool OrdersViewModel::canConfirm() const
{
    return canEditDraft() && !m_selectedOrderItems.isEmpty();
}

bool OrdersViewModel::canVoid() const
{
    return selectedOrderInStatus(domain::OrderStatus::Draft)
           || selectedOrderInStatus(domain::OrderStatus::Confirmed);
}

bool OrdersViewModel::canOpenPayment() const
{
    return m_hasSelectedOrder
           && !m_selectedOrder.customerId.trimmed().isEmpty()
           && (m_selectedOrder.status == domain::OrderStatus::Confirmed
               || m_selectedOrder.status == domain::OrderStatus::PartiallyPaid);
}

QVariantList OrdersViewModel::selectedOrderItems() const
{
    return m_selectedOrderItems;
}

QVariantList OrdersViewModel::customers() const
{
    return m_customers;
}

QVariantList OrdersViewModel::products() const
{
    return m_products;
}

void OrdersViewModel::reload()
{
    refreshLookups();
    refreshOrders();
}

bool OrdersViewModel::createDraftOrder(const QString &customerId, const QString &orderDate)
{
    QString resolvedCustomerId = customerId.trimmed();

    if (resolvedCustomerId.isEmpty() && !m_customers.isEmpty()) {
        const QVariantMap firstCustomer = m_customers.first().toMap();
        resolvedCustomerId = firstCustomer.value(QStringLiteral("id")).toString();
    }

    if (resolvedCustomerId.isEmpty()) {
        setLastError(QStringLiteral("Chưa có khách hàng để tạo đơn."));
        return false;
    }

    QString orderId;
    QString errorMessage;
    const bool success = m_orderService.createDraftOrder(resolvedCustomerId,
                                                          orderDate,
                                                          orderId,
                                                          errorMessage);

    if (!success) {
        setLastError(errorMessage);
        return false;
    }

    setLastError(QString());
    refreshOrders();
    selectOrder(orderId);
    return true;
}

void OrdersViewModel::selectOrder(const QString &orderId)
{
    const QString normalizedId = orderId.trimmed();

    if (normalizedId.isEmpty()) {
        clearSelection();
        return;
    }

    if (m_hasSelectedOrder && m_selectedOrder.id == normalizedId) {
        return;
    }

    m_hasSelectedOrder = true;
    m_selectedOrder.id = normalizedId;
    refreshSelectedOrder();
}

bool OrdersViewModel::updateSelectedOrderCustomer(const QString &customerId)
{
    if (!m_hasSelectedOrder) {
        setLastError(QStringLiteral("Chưa chọn đơn hàng."));
        return false;
    }

    QString errorMessage;
    const bool success = m_orderService.updateDraftOrderCustomer(m_selectedOrder.id,
                                                                  customerId,
                                                                  errorMessage);

    if (!success) {
        setLastError(errorMessage);
        return false;
    }

    setLastError(QString());
    refreshOrders();
    refreshSelectedOrder();
    return true;
}

bool OrdersViewModel::addOrUpdateDraftItem(const QString &productId,
                                           const QString &preferredLotId,
                                           const QString &qtyText,
                                           const QString &unitPriceText,
                                           const QString &discountText)
{
    if (!m_hasSelectedOrder) {
        setLastError(QStringLiteral("Chưa chọn đơn hàng."));
        return false;
    }

    int qty = 0;
    if (!parsePositiveInt(qtyText, qty)) {
        setLastError(QStringLiteral("Số lượng không hợp lệ."));
        return false;
    }

    core::Money unitPriceVnd = 0;
    if (unitPriceText.trimmed().isEmpty()) {
        unitPriceVnd = m_productService.defaultPriceByProductId(productId);
    } else if (!parseMoneyInput(unitPriceText, unitPriceVnd)) {
        setLastError(QStringLiteral("Đơn giá không hợp lệ."));
        return false;
    }

    core::Money discountVnd = 0;
    if (!discountText.trimmed().isEmpty() && !parseMoneyInput(discountText, discountVnd)) {
        setLastError(QStringLiteral("Giảm giá không hợp lệ."));
        return false;
    }

    QString errorMessage;
    const bool success = m_orderService.upsertDraftItem(
        m_selectedOrder.id,
        application::DraftOrderItemInput{productId, qty, unitPriceVnd, discountVnd, preferredLotId},
        errorMessage);

    if (!success) {
        setLastError(errorMessage);
        return false;
    }

    setLastError(QString());
    refreshOrders();
    refreshSelectedOrder();
    return true;
}

bool OrdersViewModel::removeDraftItem(const QString &orderItemId)
{
    if (!m_hasSelectedOrder) {
        setLastError(QStringLiteral("Chưa chọn đơn hàng."));
        return false;
    }

    QString errorMessage;
    const bool success = m_orderService.removeDraftItem(m_selectedOrder.id,
                                                         orderItemId,
                                                         errorMessage);

    if (!success) {
        setLastError(errorMessage);
        return false;
    }

    setLastError(QString());
    refreshOrders();
    refreshSelectedOrder();
    return true;
}

bool OrdersViewModel::confirmSelectedOrder()
{
    if (!m_hasSelectedOrder) {
        setLastError(QStringLiteral("Chưa chọn đơn hàng."));
        return false;
    }

    QString errorMessage;
    const bool success = m_orderService.confirmOrder(m_selectedOrder.id, errorMessage);

    if (!success) {
        setLastError(errorMessage);
        return false;
    }

    setLastError(QString());
    refreshOrders();
    refreshSelectedOrder();
    return true;
}

bool OrdersViewModel::voidSelectedOrder()
{
    if (!m_hasSelectedOrder) {
        setLastError(QStringLiteral("Chưa chọn đơn hàng."));
        return false;
    }

    QString errorMessage;
    const bool success = m_orderService.voidOrder(m_selectedOrder.id, errorMessage);

    if (!success) {
        setLastError(errorMessage);
        return false;
    }

    setLastError(QString());
    refreshOrders();
    refreshSelectedOrder();
    return true;
}

QString OrdersViewModel::defaultPriceTextForProduct(const QString &productId) const
{
    return QString::number(m_productService.defaultPriceByProductId(productId));
}

QVariantList OrdersViewModel::lotsForProduct(const QString &productId) const
{
    const QString normalizedProductId = productId.trimmed();
    if (normalizedProductId.isEmpty()) {
        return {};
    }

    QVector<domain::ProductLot> lots = m_productService.findLotsByProduct(normalizedProductId);
    std::sort(lots.begin(), lots.end(), [](const domain::ProductLot &lhs, const domain::ProductLot &rhs) {
        if (lhs.receivedDate == rhs.receivedDate) {
            return lhs.lotNo < rhs.lotNo;
        }

        return lhs.receivedDate < rhs.receivedDate;
    });

    QVariantList result;
    result.reserve(lots.size());
    for (const domain::ProductLot &lot : lots) {
        if (lot.onHandQty <= 0) {
            continue;
        }

        result.push_back(QVariantMap{
            {QStringLiteral("id"), lot.id},
            {QStringLiteral("lotNo"), lot.lotNo},
            {QStringLiteral("onHandQty"), lot.onHandQty},
            {QStringLiteral("receivedDate"), lot.receivedDate},
            {QStringLiteral("expiryDate"), lot.expiryDate},
            {QStringLiteral("label"),
             QStringLiteral("%1 | Còn %2 | HSD %3")
                 .arg(lot.lotNo)
                 .arg(lot.onHandQty)
                 .arg(lot.expiryDate)},
        });
    }

    return result;
}

QVariantList OrdersViewModel::previewAllocations(const QString &productId,
                                                 const QString &preferredLotId,
                                                 const QString &qtyText) const
{
    int qty = 0;
    if (!parsePositiveInt(qtyText, qty)) {
        return {};
    }

    return buildAllocationPreview(productId, preferredLotId, qty);
}

QString OrdersViewModel::orderIdAt(int row) const
{
    if (row < 0 || row >= m_orders.size()) {
        return QString();
    }

    return m_orders.at(row).id;
}

bool OrdersViewModel::applyOrderQuery(const QString &orderNo,
                                      const QString &fromDate,
                                      const QString &toDate)
{
    QString normalizedOrderNo;
    QString normalizedFromDate;
    QString normalizedToDate;
    QString errorMessage;
    if (!validateAndNormalizeQuery(orderNo,
                                   fromDate,
                                   toDate,
                                   normalizedOrderNo,
                                   normalizedFromDate,
                                   normalizedToDate,
                                   errorMessage)) {
        setLastError(errorMessage);
        return false;
    }

    m_queryOrderNo = normalizedOrderNo;
    m_queryFromDate = normalizedFromDate;
    m_queryToDate = normalizedToDate;
    emit queryChanged();

    setLastError(QString());
    rebuildFilteredOrders();
    return true;
}

void OrdersViewModel::clearOrderQuery()
{
    if (m_queryOrderNo.isEmpty() && m_queryFromDate.isEmpty() && m_queryToDate.isEmpty()) {
        return;
    }

    m_queryOrderNo.clear();
    m_queryFromDate.clear();
    m_queryToDate.clear();
    emit queryChanged();

    setLastError(QString());
    rebuildFilteredOrders();
}

QString OrdersViewModel::statusText(domain::OrderStatus status)
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

    return QStringLiteral("Unknown");
}

QString OrdersViewModel::formatMoneyVnd(core::Money amountVnd)
{
    const QLocale locale(QLocale::Vietnamese, QLocale::Vietnam);
    return locale.toString(amountVnd) + QStringLiteral(" VND");
}

bool OrdersViewModel::parseMoneyInput(const QString &input, core::Money &value) const
{
    QString normalized = input.trimmed();
    if (normalized.isEmpty()) {
        value = 0;
        return true;
    }

    normalized.remove(QRegularExpression(QStringLiteral("[\\s.,_]")));

    if (normalized.startsWith(QLatin1Char('-'))) {
        return false;
    }

    bool ok = false;
    const qlonglong parsed = normalized.toLongLong(&ok);
    if (!ok || parsed < 0) {
        return false;
    }

    value = parsed;
    return true;
}

bool OrdersViewModel::parsePositiveInt(const QString &input, int &value) const
{
    QString normalized = input.trimmed();
    normalized.remove(QRegularExpression(QStringLiteral("[\\s.,_]")));

    bool ok = false;
    const int parsed = normalized.toInt(&ok);
    if (!ok || parsed <= 0) {
        return false;
    }

    value = parsed;
    return true;
}

bool OrdersViewModel::selectedOrderInStatus(domain::OrderStatus status) const
{
    return m_hasSelectedOrder && m_selectedOrder.status == status;
}

QVariantList OrdersViewModel::buildAllocationPreview(const QString &productId,
                                                     const QString &preferredLotId,
                                                     int qty) const
{
    const QString normalizedProductId = productId.trimmed();
    if (normalizedProductId.isEmpty() || qty <= 0) {
        return {};
    }

    QVector<domain::ProductLot> lots = m_productService.findLotsByProduct(normalizedProductId);
    if (lots.isEmpty()) {
        return {};
    }

    std::sort(lots.begin(), lots.end(), [](const domain::ProductLot &lhs, const domain::ProductLot &rhs) {
        if (lhs.receivedDate == rhs.receivedDate) {
            return lhs.lotNo < rhs.lotNo;
        }

        return lhs.receivedDate < rhs.receivedDate;
    });

    const QString normalizedPreferredLotId = preferredLotId.trimmed();
    if (!normalizedPreferredLotId.isEmpty()) {
        auto preferredIt = std::find_if(lots.begin(), lots.end(), [&normalizedPreferredLotId](const domain::ProductLot &lot) {
            return lot.id == normalizedPreferredLotId;
        });
        if (preferredIt != lots.end()) {
            std::rotate(lots.begin(), preferredIt, preferredIt + 1);
        }
    }

    int remainingQty = qty;
    QVariantList preview;

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

        preview.push_back(QVariantMap{
            {QStringLiteral("lotId"), lot.id},
            {QStringLiteral("lotNo"), lot.lotNo},
            {QStringLiteral("allocatedQty"), allocatedQty},
            {QStringLiteral("onHandQty"), lot.onHandQty},
            {QStringLiteral("label"),
             QStringLiteral("%1: lấy %2 / còn %3")
                 .arg(lot.lotNo)
                 .arg(allocatedQty)
                 .arg(lot.onHandQty)},
        });
        remainingQty -= allocatedQty;
    }

    if (remainingQty > 0) {
        preview.push_back(QVariantMap{
            {QStringLiteral("lotId"), QString()},
            {QStringLiteral("lotNo"), QStringLiteral("Thiếu tồn")},
            {QStringLiteral("allocatedQty"), -remainingQty},
            {QStringLiteral("onHandQty"), 0},
            {QStringLiteral("label"),
             QStringLiteral("Thiếu %1 để đủ số lượng yêu cầu").arg(remainingQty)},
        });
    }

    return preview;
}

bool OrdersViewModel::validateAndNormalizeQuery(const QString &orderNo,
                                                const QString &fromDate,
                                                const QString &toDate,
                                                QString &normalizedOrderNo,
                                                QString &normalizedFromDate,
                                                QString &normalizedToDate,
                                                QString &errorMessage) const
{
    normalizedOrderNo = orderNo.trimmed();
    normalizedFromDate = fromDate.trimmed();
    normalizedToDate = toDate.trimmed();

    QDate fromParsed;
    QDate toParsed;
    const bool hasFromDate = !normalizedFromDate.isEmpty();
    const bool hasToDate = !normalizedToDate.isEmpty();

    if (hasFromDate) {
        fromParsed = QDate::fromString(normalizedFromDate, Qt::ISODate);
        if (!fromParsed.isValid()) {
            errorMessage = QStringLiteral("Ngày bắt đầu không hợp lệ. Dùng định dạng YYYY-MM-DD.");
            return false;
        }
        normalizedFromDate = fromParsed.toString(Qt::ISODate);
    }

    if (hasToDate) {
        toParsed = QDate::fromString(normalizedToDate, Qt::ISODate);
        if (!toParsed.isValid()) {
            errorMessage = QStringLiteral("Ngày kết thúc không hợp lệ. Dùng định dạng YYYY-MM-DD.");
            return false;
        }
        normalizedToDate = toParsed.toString(Qt::ISODate);
    }

    if (hasFromDate && hasToDate && fromParsed > toParsed) {
        errorMessage = QStringLiteral("Khoảng ngày không hợp lệ: ngày bắt đầu phải <= ngày kết thúc.");
        return false;
    }

    return true;
}

void OrdersViewModel::rebuildFilteredOrders()
{
    QVector<domain::Order> filtered;
    filtered.reserve(m_allOrders.size());

    const bool hasOrderNoFilter = !m_queryOrderNo.isEmpty();
    const bool hasFromDate = !m_queryFromDate.isEmpty();
    const bool hasToDate = !m_queryToDate.isEmpty();
    const QDate fromDate = hasFromDate ? QDate::fromString(m_queryFromDate, Qt::ISODate) : QDate{};
    const QDate toDate = hasToDate ? QDate::fromString(m_queryToDate, Qt::ISODate) : QDate{};

    for (const domain::Order &order : m_allOrders) {
        if (hasOrderNoFilter && !order.orderNo.contains(m_queryOrderNo, Qt::CaseInsensitive)) {
            continue;
        }

        if (hasFromDate || hasToDate) {
            const QDate orderDate = QDate::fromString(order.orderDate, Qt::ISODate);
            if (!orderDate.isValid()) {
                continue;
            }

            if (hasFromDate && hasToDate) {
                if (orderDate < fromDate || orderDate > toDate) {
                    continue;
                }
            } else if (hasFromDate) {
                if (orderDate != fromDate) {
                    continue;
                }
            } else if (hasToDate) {
                if (orderDate != toDate) {
                    continue;
                }
            }
        }

        filtered.push_back(order);
    }

    const int previousCount = m_orders.size();
    beginResetModel();
    m_orders = filtered;
    endResetModel();

    if (previousCount != m_orders.size()) {
        emit orderCountChanged();
    }

    if (m_hasSelectedOrder) {
        bool selectedVisible = false;
        for (const domain::Order &order : m_orders) {
            if (order.id == m_selectedOrder.id) {
                selectedVisible = true;
                break;
            }
        }

        if (!selectedVisible) {
            clearSelection();
        } else {
            refreshSelectedOrder();
        }
    }
}

void OrdersViewModel::setLastError(const QString &message)
{
    if (m_lastError == message) {
        return;
    }

    m_lastError = message;
    emit lastErrorChanged();
}

void OrdersViewModel::refreshOrders()
{
    m_allOrders = m_orderService.findOrders();
    rebuildFilteredOrders();
}

void OrdersViewModel::refreshSelectedOrder()
{
    if (!m_hasSelectedOrder) {
        if (!m_selectedOrderItems.isEmpty()) {
            m_selectedOrderItems.clear();
            emit selectedOrderItemsChanged();
        }
        return;
    }

    domain::Order order;
    if (!m_orderService.getOrderById(m_selectedOrder.id, order)) {
        clearSelection();
        return;
    }

    m_selectedOrder = order;

    QVariantList items;
    const QVector<domain::OrderItem> orderItems = m_orderService.findOrderItems(order.id);
    items.reserve(orderItems.size());

    for (const domain::OrderItem &item : orderItems) {
        const QString productName = m_productService.productNameById(item.productId);
        const QVector<domain::ProductLot> productLots = m_productService.findLotsByProduct(item.productId);
        QStringList allocationParts;
        const QVector<application::StockAllocationInfo> allocations = m_orderService.findAllocations(order.id, item.id);
        const QString primaryLotId = allocations.isEmpty() ? QString() : allocations.first().lotId;
        for (const application::StockAllocationInfo &allocation : allocations) {
            QString lotLabel = allocation.lotId;
            for (const domain::ProductLot &lot : productLots) {
                if (lot.id == allocation.lotId) {
                    lotLabel = lot.lotNo;
                    break;
                }
            }

            allocationParts.push_back(QStringLiteral("%1 (%2)").arg(lotLabel).arg(allocation.qty));
        }

        items.push_back(QVariantMap{
            {QStringLiteral("orderItemId"), item.id},
            {QStringLiteral("productId"), item.productId},
            {QStringLiteral("productName"), productName.isEmpty() ? item.productId : productName},
            {QStringLiteral("qty"), item.qty},
            {QStringLiteral("unitPriceVnd"), item.unitPriceVnd},
            {QStringLiteral("unitPriceText"), formatMoneyVnd(item.unitPriceVnd)},
            {QStringLiteral("discountVnd"), item.discountVnd},
            {QStringLiteral("discountText"), formatMoneyVnd(item.discountVnd)},
            {QStringLiteral("lineTotalVnd"), item.lineTotalVnd},
            {QStringLiteral("lineTotalText"), formatMoneyVnd(item.lineTotalVnd)},
            {QStringLiteral("primaryLotId"), primaryLotId},
            {QStringLiteral("lotSummary"), allocationParts.join(QStringLiteral(", "))},
        });
    }

    m_selectedOrderItems = items;
    emit selectedOrderChanged();
    emit selectedOrderItemsChanged();
}

void OrdersViewModel::refreshLookups()
{
    QVariantList customerList;
    const QVector<domain::Customer> customers = m_customerService.findCustomers();
    customerList.reserve(customers.size());
    for (const domain::Customer &customer : customers) {
        customerList.push_back(QVariantMap{
            {QStringLiteral("id"), customer.id},
            {QStringLiteral("code"), customer.code},
            {QStringLiteral("name"), customer.name},
            {QStringLiteral("phone"), customer.phone},
            {QStringLiteral("label"), customer.code + QStringLiteral(" - ") + customer.name},
        });
    }

    QVariantList productList;
    const QVector<domain::Product> products = m_productService.findProducts();
    productList.reserve(products.size());
    for (const domain::Product &product : products) {
        productList.push_back(QVariantMap{
            {QStringLiteral("id"), product.id},
            {QStringLiteral("sku"), product.sku},
            {QStringLiteral("name"), product.name},
            {QStringLiteral("label"), product.sku + QStringLiteral(" - ") + product.name},
            {QStringLiteral("defaultPriceVnd"), product.defaultWholesalePriceVnd},
        });
    }

    m_customers = customerList;
    m_products = productList;
    emit lookupChanged();
}

void OrdersViewModel::clearSelection()
{
    if (!m_hasSelectedOrder && m_selectedOrderItems.isEmpty()) {
        return;
    }

    m_hasSelectedOrder = false;
    m_selectedOrder = domain::Order{};
    m_selectedOrderItems.clear();

    emit selectedOrderChanged();
    emit selectedOrderItemsChanged();
}

} // namespace stockmaster::ui::viewmodels
