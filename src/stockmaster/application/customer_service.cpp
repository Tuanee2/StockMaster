#include "stockmaster/application/customer_service.h"

#include <QUuid>

namespace stockmaster::application {

CustomerService::CustomerService()
{
    seedSampleData();
}

QVector<domain::Customer> CustomerService::findCustomers(const QString &keyword) const
{
    const QString normalizedKeyword = keyword.trimmed();

    if (normalizedKeyword.isEmpty()) {
        return m_customers;
    }

    QVector<domain::Customer> filtered;
    filtered.reserve(m_customers.size());

    for (const domain::Customer &customer : m_customers) {
        const bool matched = customer.code.contains(normalizedKeyword, Qt::CaseInsensitive)
                             || customer.name.contains(normalizedKeyword, Qt::CaseInsensitive)
                             || customer.phone.contains(normalizedKeyword, Qt::CaseInsensitive);

        if (matched) {
            filtered.push_back(customer);
        }
    }

    return filtered;
}

bool CustomerService::customerExists(const QString &customerId) const
{
    const QString normalizedId = customerId.trimmed();
    if (normalizedId.isEmpty()) {
        return false;
    }

    for (const domain::Customer &customer : m_customers) {
        if (customer.id == normalizedId) {
            return true;
        }
    }

    return false;
}

QString CustomerService::customerNameById(const QString &customerId) const
{
    const QString normalizedId = customerId.trimmed();
    if (normalizedId.isEmpty()) {
        return QString();
    }

    for (const domain::Customer &customer : m_customers) {
        if (customer.id == normalizedId) {
            return customer.name;
        }
    }

    return QString();
}

int CustomerService::activeCustomerCount() const
{
    return m_customers.size();
}

bool CustomerService::createCustomer(const CustomerInput &input, QString &errorMessage)
{
    const QString name = input.name.trimmed();
    if (name.isEmpty()) {
        errorMessage = QStringLiteral("Tên khách hàng là bắt buộc.");
        return false;
    }

    if (input.creditLimitVnd < 0) {
        errorMessage = QStringLiteral("Hạn mức công nợ phải >= 0.");
        return false;
    }

    QString code;
    do {
        code = generateDefaultCode(m_nextCodeSerial++);
    } while (codeExists(code));

    domain::Customer customer;
    customer.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    customer.code = code;
    customer.name = name;
    customer.phone = input.phone.trimmed();
    customer.address = input.address.trimmed();
    customer.creditLimitVnd = input.creditLimitVnd;

    m_customers.push_back(customer);
    errorMessage.clear();
    return true;
}

bool CustomerService::updateCustomer(const QString &customerId,
                                     const CustomerInput &input,
                                     QString &errorMessage)
{
    const QString normalizedId = customerId.trimmed();
    if (normalizedId.isEmpty()) {
        errorMessage = QStringLiteral("Không tìm thấy khách hàng cần sửa.");
        return false;
    }

    const QString name = input.name.trimmed();
    if (name.isEmpty()) {
        errorMessage = QStringLiteral("Tên khách hàng là bắt buộc.");
        return false;
    }

    if (input.creditLimitVnd < 0) {
        errorMessage = QStringLiteral("Hạn mức công nợ phải >= 0.");
        return false;
    }

    for (domain::Customer &customer : m_customers) {
        if (customer.id != normalizedId) {
            continue;
        }

        customer.name = name;
        customer.phone = input.phone.trimmed();
        customer.address = input.address.trimmed();
        customer.creditLimitVnd = input.creditLimitVnd;

        errorMessage.clear();
        return true;
    }

    errorMessage = QStringLiteral("Không tìm thấy khách hàng cần sửa.");
    return false;
}

bool CustomerService::deleteCustomer(const QString &customerId, QString &errorMessage)
{
    const QString normalizedId = customerId.trimmed();
    if (normalizedId.isEmpty()) {
        errorMessage = QStringLiteral("Không tìm thấy khách hàng cần xóa.");
        return false;
    }

    for (qsizetype index = 0; index < m_customers.size(); ++index) {
        if (m_customers[index].id != normalizedId) {
            continue;
        }

        m_customers.remove(index);
        errorMessage.clear();
        return true;
    }

    errorMessage = QStringLiteral("Không tìm thấy khách hàng cần xóa.");
    return false;
}

bool CustomerService::codeExists(const QString &code) const
{
    for (const domain::Customer &customer : m_customers) {
        if (customer.code.compare(code, Qt::CaseInsensitive) == 0) {
            return true;
        }
    }

    return false;
}

QString CustomerService::generateDefaultCode(int serial)
{
    return QStringLiteral("KH%1").arg(serial, 4, 10, QLatin1Char('0'));
}

void CustomerService::seedSampleData()
{
    QString ignoredError;
    (void)createCustomer(CustomerInput{QStringLiteral("Cua Hang Minh Chau"),
                                       QStringLiteral("0905123456"),
                                       QStringLiteral("Da Nang"),
                                       50000000},
                         ignoredError);
    (void)createCustomer(CustomerInput{QStringLiteral("Nha Phan Phoi An Phat"),
                                       QStringLiteral("0911987654"),
                                       QStringLiteral("Ho Chi Minh"),
                                       120000000},
                         ignoredError);
    (void)createCustomer(CustomerInput{QStringLiteral("Dai Ly Hoang Gia"),
                                       QStringLiteral("0988654321"),
                                       QStringLiteral("Ha Noi"),
                                       80000000},
                         ignoredError);
}

} // namespace stockmaster::application
