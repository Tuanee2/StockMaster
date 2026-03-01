#pragma once

#include <QString>
#include <QVector>

#include "stockmaster/domain/entities.h"

namespace stockmaster::infra::db {
class DatabaseService;
}

namespace stockmaster::application {

struct CustomerInput {
    QString name;
    QString phone;
    QString address;
    core::Money creditLimitVnd = 0;
};

class CustomerService {
public:
    explicit CustomerService(stockmaster::infra::db::DatabaseService &databaseService);

    [[nodiscard]] QVector<domain::Customer> findCustomers(const QString &keyword = {}) const;
    [[nodiscard]] bool customerExists(const QString &customerId) const;
    [[nodiscard]] QString customerNameById(const QString &customerId) const;
    [[nodiscard]] int activeCustomerCount() const;
    [[nodiscard]] bool saveToDatabase(QString &errorMessage) const;

    [[nodiscard]] bool createCustomer(const CustomerInput &input, QString &errorMessage);
    [[nodiscard]] bool updateCustomer(const QString &customerId,
                                      const CustomerInput &input,
                                      QString &errorMessage);
    [[nodiscard]] bool deleteCustomer(const QString &customerId, QString &errorMessage);

private:
    [[nodiscard]] bool codeExists(const QString &code) const;
    [[nodiscard]] static QString generateDefaultCode(int serial);
    bool loadFromDatabase();
    bool persistCustomers(const QVector<domain::Customer> &customers, QString &errorMessage) const;
    void recomputeNextCodeSerial();
    void seedSampleData();

    stockmaster::infra::db::DatabaseService &m_databaseService;
    QVector<domain::Customer> m_customers;
    int m_nextCodeSerial = 1;
};

} // namespace stockmaster::application
