#include "stockmaster/ui/viewmodels/customers_view_model.h"

#include <QLocale>
#include <QRegularExpression>

namespace stockmaster::ui::viewmodels {

CustomersViewModel::CustomersViewModel(application::CustomerService &customerService,
                                       QObject *parent)
    : QAbstractListModel(parent)
    , m_customerService(customerService)
{
    refreshModel();
}

int CustomersViewModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }

    return m_rows.size();
}

QVariant CustomersViewModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_rows.size()) {
        return {};
    }

    const domain::Customer &customer = m_rows.at(index.row());
    switch (role) {
    case CustomerIdRole:
        return customer.id;
    case CodeRole:
        return customer.code;
    case NameRole:
        return customer.name;
    case PhoneRole:
        return customer.phone;
    case AddressRole:
        return customer.address;
    case CreditLimitVndRole:
        return customer.creditLimitVnd;
    case CreditLimitTextRole:
        return formatMoneyVnd(customer.creditLimitVnd);
    default:
        return {};
    }
}

QHash<int, QByteArray> CustomersViewModel::roleNames() const
{
    return {
        {CustomerIdRole, QByteArrayLiteral("customerId")},
        {CodeRole, QByteArrayLiteral("code")},
        {NameRole, QByteArrayLiteral("name")},
        {PhoneRole, QByteArrayLiteral("phone")},
        {AddressRole, QByteArrayLiteral("address")},
        {CreditLimitVndRole, QByteArrayLiteral("creditLimitVnd")},
        {CreditLimitTextRole, QByteArrayLiteral("creditLimitText")},
    };
}

QString CustomersViewModel::filterText() const
{
    return m_filterText;
}

void CustomersViewModel::setFilterText(const QString &value)
{
    const QString normalized = value;
    if (m_filterText == normalized) {
        return;
    }

    m_filterText = normalized;
    emit filterTextChanged();
    refreshModel();
}

QString CustomersViewModel::lastError() const
{
    return m_lastError;
}

int CustomersViewModel::customerCount() const
{
    return m_rows.size();
}

bool CustomersViewModel::createCustomer(const QString &name,
                                        const QString &phone,
                                        const QString &address,
                                        const QString &creditLimitText)
{
    core::Money creditLimitVnd = 0;
    if (!parseMoneyInput(creditLimitText, creditLimitVnd)) {
        setLastError(QStringLiteral("Hạn mức công nợ không hợp lệ. Chỉ nhập số nguyên >= 0."));
        return false;
    }

    QString errorMessage;
    const bool success = m_customerService.createCustomer(
        application::CustomerInput{name, phone, address, creditLimitVnd}, errorMessage);

    if (!success) {
        setLastError(errorMessage);
        return false;
    }

    setLastError(QString());
    refreshModel();
    return true;
}

bool CustomersViewModel::updateCustomer(const QString &customerId,
                                        const QString &name,
                                        const QString &phone,
                                        const QString &address,
                                        const QString &creditLimitText)
{
    core::Money creditLimitVnd = 0;
    if (!parseMoneyInput(creditLimitText, creditLimitVnd)) {
        setLastError(QStringLiteral("Hạn mức công nợ không hợp lệ. Chỉ nhập số nguyên >= 0."));
        return false;
    }

    QString errorMessage;
    const bool success = m_customerService.updateCustomer(
        customerId,
        application::CustomerInput{name, phone, address, creditLimitVnd},
        errorMessage);

    if (!success) {
        setLastError(errorMessage);
        return false;
    }

    setLastError(QString());
    refreshModel();
    return true;
}

bool CustomersViewModel::deleteCustomer(const QString &customerId)
{
    QString errorMessage;
    const bool success = m_customerService.deleteCustomer(customerId, errorMessage);

    if (!success) {
        setLastError(errorMessage);
        return false;
    }

    setLastError(QString());
    refreshModel();
    return true;
}

QVariantMap CustomersViewModel::getCustomer(int row) const
{
    if (row < 0 || row >= m_rows.size()) {
        return {};
    }

    const domain::Customer &customer = m_rows.at(row);

    return {
        {QStringLiteral("customerId"), customer.id},
        {QStringLiteral("code"), customer.code},
        {QStringLiteral("name"), customer.name},
        {QStringLiteral("phone"), customer.phone},
        {QStringLiteral("address"), customer.address},
        {QStringLiteral("creditLimitVnd"), customer.creditLimitVnd},
        {QStringLiteral("creditLimitText"), QString::number(customer.creditLimitVnd)},
    };
}

void CustomersViewModel::reload()
{
    refreshModel();
}

bool CustomersViewModel::parseMoneyInput(const QString &input, core::Money &value) const
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

QString CustomersViewModel::formatMoneyVnd(core::Money amountVnd)
{
    const QLocale locale(QLocale::Vietnamese, QLocale::Vietnam);
    return locale.toString(amountVnd) + QStringLiteral(" VND");
}

void CustomersViewModel::setLastError(const QString &message)
{
    if (m_lastError == message) {
        return;
    }

    m_lastError = message;
    emit lastErrorChanged();
}

void CustomersViewModel::refreshModel()
{
    const int previousCount = m_rows.size();

    beginResetModel();
    m_rows = m_customerService.findCustomers(m_filterText);
    endResetModel();

    if (previousCount != m_rows.size()) {
        emit customerCountChanged();
    }
}

} // namespace stockmaster::ui::viewmodels
