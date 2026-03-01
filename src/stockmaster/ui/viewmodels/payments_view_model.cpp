#include "stockmaster/ui/viewmodels/payments_view_model.h"

#include <QLocale>
#include <QRegularExpression>

namespace stockmaster::ui::viewmodels {

PaymentsViewModel::PaymentsViewModel(application::PaymentService &paymentService,
                                     QObject *parent)
    : QAbstractListModel(parent)
    , m_paymentService(paymentService)
{
    refreshModel();
}

int PaymentsViewModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }

    return m_rows.size();
}

QVariant PaymentsViewModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_rows.size()) {
        return {};
    }

    const application::CustomerReceivableSummary &row = m_rows.at(index.row());
    switch (role) {
    case CustomerIdRole:
        return row.customerId;
    case CodeRole:
        return row.code;
    case NameRole:
        return row.name;
    case ReceivableVndRole:
        return row.receivableVnd;
    case ReceivableTextRole:
        return formatMoneyVnd(row.receivableVnd);
    case OpenOrderCountRole:
        return row.openOrderCount;
    case PaymentCountRole:
        return row.paymentCount;
    default:
        return {};
    }
}

QHash<int, QByteArray> PaymentsViewModel::roleNames() const
{
    return {
        {CustomerIdRole, QByteArrayLiteral("customerId")},
        {CodeRole, QByteArrayLiteral("code")},
        {NameRole, QByteArrayLiteral("name")},
        {ReceivableVndRole, QByteArrayLiteral("receivableVnd")},
        {ReceivableTextRole, QByteArrayLiteral("receivableText")},
        {OpenOrderCountRole, QByteArrayLiteral("openOrderCount")},
        {PaymentCountRole, QByteArrayLiteral("paymentCount")},
    };
}

QString PaymentsViewModel::filterText() const
{
    return m_filterText;
}

void PaymentsViewModel::setFilterText(const QString &value)
{
    if (m_filterText == value) {
        return;
    }

    m_filterText = value;
    emit filterTextChanged();
    refreshModel();
}

QString PaymentsViewModel::lastError() const
{
    return m_lastError;
}

int PaymentsViewModel::customerCount() const
{
    return m_rows.size();
}

bool PaymentsViewModel::hasSelectedCustomer() const
{
    return m_hasSelectedCustomer;
}

QString PaymentsViewModel::selectedCustomerId() const
{
    return m_selectedCustomerId;
}

QString PaymentsViewModel::selectedCustomerName() const
{
    return m_selectedCustomerName;
}

QString PaymentsViewModel::selectedReceivableText() const
{
    return formatMoneyVnd(m_selectedReceivableVnd);
}

int PaymentsViewModel::selectedOpenOrderCount() const
{
    return m_selectedOpenOrderCount;
}

QString PaymentsViewModel::preferredOrderId() const
{
    return m_preferredOrderId;
}

QVariantList PaymentsViewModel::payableOrders() const
{
    return m_payableOrders;
}

QVariantList PaymentsViewModel::paymentHistory() const
{
    return m_paymentHistory;
}

QVariantList PaymentsViewModel::customerLedger() const
{
    return m_customerLedger;
}

void PaymentsViewModel::reload()
{
    refreshModel();
}

void PaymentsViewModel::selectCustomer(const QString &customerId)
{
    const QString normalizedId = customerId.trimmed();
    if (m_hasSelectedCustomer && m_selectedCustomerId == normalizedId) {
        if (!m_preferredOrderId.isEmpty()) {
            m_preferredOrderId.clear();
            emit detailChanged();
        }
        return;
    }

    if (normalizedId.isEmpty()) {
        clearSelection();
        return;
    }

    m_preferredOrderId.clear();
    m_hasSelectedCustomer = true;
    m_selectedCustomerId = normalizedId;
    refreshSelectedData();
}

void PaymentsViewModel::focusOrder(const QString &customerId, const QString &orderId)
{
    const QString normalizedCustomerId = customerId.trimmed();
    const QString normalizedOrderId = orderId.trimmed();
    if (normalizedCustomerId.isEmpty() || normalizedOrderId.isEmpty()) {
        return;
    }

    m_hasSelectedCustomer = true;
    m_selectedCustomerId = normalizedCustomerId;
    m_preferredOrderId = normalizedOrderId;
    refreshSelectedData();
}

bool PaymentsViewModel::createReceipt(const QString &orderId,
                                      const QString &amountText,
                                      const QString &method,
                                      const QString &paidAt)
{
    if (!m_hasSelectedCustomer) {
        setLastError(QStringLiteral("Chưa chọn khách hàng để lập phiếu thu."));
        return false;
    }

    core::Money amountVnd = 0;
    if (!parseMoneyInput(amountText, amountVnd) || amountVnd <= 0) {
        setLastError(QStringLiteral("Số tiền thu không hợp lệ. Chỉ nhập số nguyên > 0."));
        return false;
    }

    QString errorMessage;
    const bool success = m_paymentService.createReceipt(m_selectedCustomerId,
                                                        orderId,
                                                        amountVnd,
                                                        method,
                                                        paidAt,
                                                        errorMessage);
    if (!success) {
        setLastError(errorMessage);
        return false;
    }

    setLastError(QString());
    refreshModel();
    refreshSelectedData();
    return true;
}

bool PaymentsViewModel::parseMoneyInput(const QString &input, core::Money &value) const
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

QString PaymentsViewModel::formatMoneyVnd(core::Money amountVnd)
{
    const QLocale locale(QLocale::Vietnamese, QLocale::Vietnam);
    return locale.toString(amountVnd) + QStringLiteral(" VND");
}

void PaymentsViewModel::setLastError(const QString &message)
{
    if (m_lastError == message) {
        return;
    }

    m_lastError = message;
    emit lastErrorChanged();
}

void PaymentsViewModel::refreshModel()
{
    const int previousCount = m_rows.size();

    beginResetModel();
    m_rows = m_paymentService.findCustomerReceivables(m_filterText);
    endResetModel();

    if (previousCount != m_rows.size()) {
        emit customerCountChanged();
    }

    if (!m_hasSelectedCustomer) {
        if (!m_rows.isEmpty()) {
            m_hasSelectedCustomer = true;
            m_selectedCustomerId = m_rows.first().customerId;
            refreshSelectedData();
        } else {
            clearSelection();
        }
        return;
    }

    bool selectedVisible = false;
    for (const application::CustomerReceivableSummary &row : m_rows) {
        if (row.customerId == m_selectedCustomerId) {
            selectedVisible = true;
            break;
        }
    }

    if (!selectedVisible && !m_rows.isEmpty()) {
        m_selectedCustomerId = m_rows.first().customerId;
    } else if (!selectedVisible) {
        clearSelection();
        return;
    }

    refreshSelectedData();
}

void PaymentsViewModel::refreshSelectedData()
{
    if (!m_hasSelectedCustomer || m_selectedCustomerId.isEmpty()) {
        clearSelection();
        return;
    }

    application::CustomerReceivableSummary selectedSummary;
    bool found = false;

    QVector<application::CustomerReceivableSummary> allSummaries = m_paymentService.findCustomerReceivables();
    for (const application::CustomerReceivableSummary &summary : allSummaries) {
        if (summary.customerId == m_selectedCustomerId) {
            selectedSummary = summary;
            found = true;
            break;
        }
    }

    if (!found) {
        clearSelection();
        return;
    }

    m_hasSelectedCustomer = true;
    m_selectedCustomerName = selectedSummary.code + QStringLiteral(" - ") + selectedSummary.name;
    m_selectedReceivableVnd = selectedSummary.receivableVnd;
    m_selectedOpenOrderCount = selectedSummary.openOrderCount;

    QVariantList newPayableOrders;
    const QVector<domain::Order> orders = m_paymentService.findPayableOrdersByCustomer(m_selectedCustomerId);
    newPayableOrders.reserve(orders.size());
    for (const domain::Order &order : orders) {
        newPayableOrders.push_back(QVariantMap{
            {QStringLiteral("id"), order.id},
            {QStringLiteral("orderNo"), order.orderNo},
            {QStringLiteral("orderDate"), order.orderDate},
            {QStringLiteral("balanceVnd"), order.balanceVnd},
            {QStringLiteral("balanceText"), formatMoneyVnd(order.balanceVnd)},
            {QStringLiteral("label"), order.orderNo + QStringLiteral(" | ")
                                          + order.orderDate + QStringLiteral(" | Còn: ")
                                          + formatMoneyVnd(order.balanceVnd)},
        });
    }

    QVariantList newPaymentHistory;
    const QVector<domain::Payment> payments = m_paymentService.findPaymentsByCustomer(m_selectedCustomerId);
    newPaymentHistory.reserve(payments.size());
    for (const domain::Payment &payment : payments) {
        const QString orderNo = m_paymentService.orderNoById(payment.orderId);
        newPaymentHistory.push_back(QVariantMap{
            {QStringLiteral("paymentId"), payment.id},
            {QStringLiteral("paymentNo"), payment.paymentNo},
            {QStringLiteral("orderNo"), orderNo.isEmpty() ? payment.orderId : orderNo},
            {QStringLiteral("amountText"), formatMoneyVnd(payment.amountVnd)},
            {QStringLiteral("method"), payment.method},
            {QStringLiteral("paidAt"), payment.paidAt},
        });
    }

    QVariantList newCustomerLedger;
    const QVector<domain::DebtLedger> ledger = m_paymentService.findLedgerByCustomer(m_selectedCustomerId);
    newCustomerLedger.reserve(ledger.size());
    for (const domain::DebtLedger &entry : ledger) {
        newCustomerLedger.push_back(QVariantMap{
            {QStringLiteral("entryId"), entry.id},
            {QStringLiteral("refType"), entry.refType},
            {QStringLiteral("refId"), entry.refId},
            {QStringLiteral("deltaText"), formatMoneyVnd(entry.deltaVnd)},
            {QStringLiteral("balanceAfterText"), formatMoneyVnd(entry.balanceAfterVnd)},
            {QStringLiteral("createdAt"), entry.createdAt},
            {QStringLiteral("isDebit"), entry.deltaVnd >= 0},
        });
    }

    m_payableOrders = newPayableOrders;
    m_paymentHistory = newPaymentHistory;
    m_customerLedger = newCustomerLedger;

    bool preferredStillVisible = false;
    if (!m_preferredOrderId.isEmpty()) {
        for (const QVariant &orderValue : m_payableOrders) {
            const QVariantMap orderData = orderValue.toMap();
            if (orderData.value(QStringLiteral("id")).toString() == m_preferredOrderId) {
                preferredStillVisible = true;
                break;
            }
        }
    }
    if (!preferredStillVisible) {
        m_preferredOrderId.clear();
    }

    emit selectedCustomerChanged();
    emit detailChanged();
}

void PaymentsViewModel::clearSelection()
{
    const bool hadSelection = m_hasSelectedCustomer || !m_payableOrders.isEmpty()
        || !m_paymentHistory.isEmpty() || !m_customerLedger.isEmpty();

    m_hasSelectedCustomer = false;
    m_selectedCustomerId.clear();
    m_selectedCustomerName.clear();
    m_preferredOrderId.clear();
    m_selectedReceivableVnd = 0;
    m_selectedOpenOrderCount = 0;
    m_payableOrders.clear();
    m_paymentHistory.clear();
    m_customerLedger.clear();

    if (hadSelection) {
        emit selectedCustomerChanged();
        emit detailChanged();
    }
}

} // namespace stockmaster::ui::viewmodels
