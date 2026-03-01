#include "stockmaster/application/customer_service.h"

#include <QSqlError>
#include <QSqlQuery>
#include <QUuid>

#include "stockmaster/infra/db/database_service.h"

namespace stockmaster::application {

namespace {

int nextCustomerCodeSerial(const QVector<domain::Customer> &customers)
{
    int maxSerial = 0;
    for (const domain::Customer &customer : customers) {
        if (!customer.code.startsWith(QLatin1String("KH"))) {
            continue;
        }

        bool ok = false;
        const int serial = customer.code.mid(2).toInt(&ok);
        if (ok && serial > maxSerial) {
            maxSerial = serial;
        }
    }

    return maxSerial + 1;
}

}

CustomerService::CustomerService(stockmaster::infra::db::DatabaseService &databaseService)
    : m_databaseService(databaseService)
{
    if (!loadFromDatabase() || m_customers.isEmpty()) {
        seedSampleData();
    } else {
        recomputeNextCodeSerial();
    }
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

bool CustomerService::saveToDatabase(QString &errorMessage) const
{
    return persistCustomers(m_customers, errorMessage);
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

    int nextSerial = m_nextCodeSerial;
    QString code;
    do {
        code = generateDefaultCode(nextSerial++);
    } while (codeExists(code));

    domain::Customer customer;
    customer.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    customer.code = code;
    customer.name = name;
    customer.phone = input.phone.trimmed();
    customer.address = input.address.trimmed();
    customer.creditLimitVnd = input.creditLimitVnd;

    QVector<domain::Customer> updatedCustomers = m_customers;
    updatedCustomers.push_back(customer);

    if (!persistCustomers(updatedCustomers, errorMessage)) {
        return false;
    }

    m_customers = updatedCustomers;
    m_nextCodeSerial = nextSerial;
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

    QVector<domain::Customer> updatedCustomers = m_customers;
    for (domain::Customer &customer : updatedCustomers) {
        if (customer.id != normalizedId) {
            continue;
        }

        customer.name = name;
        customer.phone = input.phone.trimmed();
        customer.address = input.address.trimmed();
        customer.creditLimitVnd = input.creditLimitVnd;

        if (!persistCustomers(updatedCustomers, errorMessage)) {
            return false;
        }

        m_customers = updatedCustomers;
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

    QVector<domain::Customer> updatedCustomers = m_customers;
    for (qsizetype index = 0; index < updatedCustomers.size(); ++index) {
        if (updatedCustomers[index].id != normalizedId) {
            continue;
        }

        updatedCustomers.remove(index);
        if (!persistCustomers(updatedCustomers, errorMessage)) {
            return false;
        }

        m_customers = updatedCustomers;
        recomputeNextCodeSerial();
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

bool CustomerService::loadFromDatabase()
{
    m_customers.clear();
    m_nextCodeSerial = 1;

    if (!m_databaseService.isReady()) {
        return false;
    }

    QSqlQuery query(m_databaseService.database());
    if (!query.exec(QStringLiteral(
            "SELECT id, code, name, phone, address, credit_limit_vnd "
            "FROM customers ORDER BY code ASC;"))) {
        return false;
    }

    while (query.next()) {
        domain::Customer customer;
        customer.id = query.value(0).toString();
        customer.code = query.value(1).toString();
        customer.name = query.value(2).toString();
        customer.phone = query.value(3).toString();
        customer.address = query.value(4).toString();
        customer.creditLimitVnd = query.value(5).toLongLong();
        m_customers.push_back(customer);
    }

    recomputeNextCodeSerial();
    return true;
}

bool CustomerService::persistCustomers(const QVector<domain::Customer> &customers, QString &errorMessage) const
{
    if (!m_databaseService.isReady()) {
        errorMessage.clear();
        return true;
    }

    if (!m_databaseService.beginTransaction()) {
        errorMessage = m_databaseService.lastError();
        return false;
    }

    QSqlQuery deleteQuery(m_databaseService.database());
    if (!deleteQuery.exec(QStringLiteral("DELETE FROM customers;"))) {
        errorMessage = deleteQuery.lastError().text();
        m_databaseService.rollbackTransaction();
        return false;
    }

    QSqlQuery insertQuery(m_databaseService.database());
    if (!insertQuery.prepare(QStringLiteral(
            "INSERT INTO customers("
            "id, code, name, phone, address, credit_limit_vnd"
            ") VALUES("
            ":id, :code, :name, :phone, :address, :credit_limit_vnd"
            ");"))) {
        errorMessage = insertQuery.lastError().text();
        m_databaseService.rollbackTransaction();
        return false;
    }

    for (const domain::Customer &customer : customers) {
        insertQuery.bindValue(QStringLiteral(":id"), customer.id);
        insertQuery.bindValue(QStringLiteral(":code"), customer.code);
        insertQuery.bindValue(QStringLiteral(":name"), customer.name);
        insertQuery.bindValue(QStringLiteral(":phone"), customer.phone);
        insertQuery.bindValue(QStringLiteral(":address"), customer.address);
        insertQuery.bindValue(QStringLiteral(":credit_limit_vnd"), customer.creditLimitVnd);

        if (!insertQuery.exec()) {
            errorMessage = insertQuery.lastError().text();
            m_databaseService.rollbackTransaction();
            return false;
        }
    }

    if (!m_databaseService.commitTransaction()) {
        errorMessage = m_databaseService.lastError();
        m_databaseService.rollbackTransaction();
        return false;
    }

    errorMessage.clear();
    return true;
}

void CustomerService::recomputeNextCodeSerial()
{
    m_nextCodeSerial = nextCustomerCodeSerial(m_customers);
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
