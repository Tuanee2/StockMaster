#pragma once

#include <QString>
#include <QVector>

#include "stockmaster/domain/entities.h"

namespace stockmaster::application {

class CustomerService;
class OrderService;

struct CustomerReceivableSummary {
    QString customerId;
    QString code;
    QString name;
    core::Money receivableVnd = 0;
    int openOrderCount = 0;
    int paymentCount = 0;
};

class PaymentService {
public:
    PaymentService(OrderService &orderService,
                   const CustomerService &customerService);

    [[nodiscard]] QVector<CustomerReceivableSummary> findCustomerReceivables(const QString &keyword = {}) const;
    [[nodiscard]] QVector<domain::Order> findPayableOrdersByCustomer(const QString &customerId) const;
    [[nodiscard]] QVector<domain::Payment> findAllPayments() const;
    [[nodiscard]] QVector<domain::Payment> findPaymentsByCustomer(const QString &customerId) const;
    [[nodiscard]] QVector<domain::DebtLedger> findLedgerByCustomer(const QString &customerId) const;
    [[nodiscard]] QString orderNoById(const QString &orderId) const;

    [[nodiscard]] bool createReceipt(const QString &customerId,
                                     const QString &orderId,
                                     core::Money amountVnd,
                                     const QString &method,
                                     const QString &paidAt,
                                     QString &errorMessage);

private:
    [[nodiscard]] QString nextPaymentNo();

    OrderService &m_orderService;
    const CustomerService &m_customerService;
    QVector<domain::Payment> m_payments;
    int m_nextPaymentSerial = 1;
};

} // namespace stockmaster::application
