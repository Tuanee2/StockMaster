#include "stockmaster/application/payment_service.h"

#include <QDate>
#include <QUuid>

#include <algorithm>

#include "stockmaster/application/customer_service.h"
#include "stockmaster/application/order_service.h"

namespace stockmaster::application {

PaymentService::PaymentService(OrderService &orderService,
                               const CustomerService &customerService)
    : m_orderService(orderService)
    , m_customerService(customerService)
{
}

QVector<CustomerReceivableSummary> PaymentService::findCustomerReceivables(const QString &keyword) const
{
    const QVector<domain::Customer> customers = m_customerService.findCustomers(keyword);
    const QVector<domain::Order> orders = m_orderService.findOrders();

    QVector<CustomerReceivableSummary> summaries;
    summaries.reserve(customers.size());

    for (const domain::Customer &customer : customers) {
        CustomerReceivableSummary summary;
        summary.customerId = customer.id;
        summary.code = customer.code;
        summary.name = customer.name;

        for (const domain::Order &order : orders) {
            if (order.customerId != customer.id) {
                continue;
            }

            if (order.status == domain::OrderStatus::Confirmed
                || order.status == domain::OrderStatus::PartiallyPaid) {
                summary.receivableVnd += order.balanceVnd;
                ++summary.openOrderCount;
            }
        }

        for (const domain::Payment &payment : m_payments) {
            if (payment.customerId == customer.id) {
                ++summary.paymentCount;
            }
        }

        summaries.push_back(summary);
    }

    std::sort(summaries.begin(), summaries.end(), [](const CustomerReceivableSummary &lhs,
                                                     const CustomerReceivableSummary &rhs) {
        if (lhs.receivableVnd == rhs.receivableVnd) {
            return lhs.name < rhs.name;
        }

        return lhs.receivableVnd > rhs.receivableVnd;
    });

    return summaries;
}

QVector<domain::Order> PaymentService::findPayableOrdersByCustomer(const QString &customerId) const
{
    QVector<domain::Order> payableOrders;
    const QString normalizedCustomerId = customerId.trimmed();
    if (normalizedCustomerId.isEmpty()) {
        return payableOrders;
    }

    const QVector<domain::Order> orders = m_orderService.findOrders();
    for (const domain::Order &order : orders) {
        if (order.customerId != normalizedCustomerId) {
            continue;
        }

        if ((order.status == domain::OrderStatus::Confirmed
             || order.status == domain::OrderStatus::PartiallyPaid)
            && order.balanceVnd > 0) {
            payableOrders.push_back(order);
        }
    }

    return payableOrders;
}

QVector<domain::Payment> PaymentService::findAllPayments() const
{
    QVector<domain::Payment> payments = m_payments;

    std::sort(payments.begin(), payments.end(), [](const domain::Payment &lhs, const domain::Payment &rhs) {
        if (lhs.paidAt == rhs.paidAt) {
            return lhs.paymentNo > rhs.paymentNo;
        }

        return lhs.paidAt > rhs.paidAt;
    });

    return payments;
}

QVector<domain::Payment> PaymentService::findPaymentsByCustomer(const QString &customerId) const
{
    QVector<domain::Payment> payments;
    const QString normalizedCustomerId = customerId.trimmed();
    if (normalizedCustomerId.isEmpty()) {
        return payments;
    }

    const QVector<domain::Payment> allPayments = findAllPayments();
    for (const domain::Payment &payment : allPayments) {
        if (payment.customerId == normalizedCustomerId) {
            payments.push_back(payment);
        }
    }

    return payments;
}

QVector<domain::DebtLedger> PaymentService::findLedgerByCustomer(const QString &customerId) const
{
    struct LedgerEvent {
        QString date;
        bool isOrder = false;
        QString refNo;
        core::Money deltaVnd = 0;
    };

    QVector<domain::DebtLedger> ledger;
    const QString normalizedCustomerId = customerId.trimmed();
    if (normalizedCustomerId.isEmpty()) {
        return ledger;
    }

    QVector<LedgerEvent> events;

    const QVector<domain::Order> orders = m_orderService.findOrders();
    for (const domain::Order &order : orders) {
        if (order.customerId != normalizedCustomerId) {
            continue;
        }

        if (order.status == domain::OrderStatus::Draft || order.status == domain::OrderStatus::Voided) {
            continue;
        }

        if (order.totalVnd <= 0) {
            continue;
        }

        events.push_back(LedgerEvent{order.orderDate, true, order.orderNo, order.totalVnd});
    }

    for (const domain::Payment &payment : m_payments) {
        if (payment.customerId != normalizedCustomerId) {
            continue;
        }

        events.push_back(LedgerEvent{payment.paidAt, false, payment.paymentNo, -payment.amountVnd});
    }

    std::sort(events.begin(), events.end(), [](const LedgerEvent &lhs, const LedgerEvent &rhs) {
        if (lhs.date == rhs.date) {
            if (lhs.isOrder == rhs.isOrder) {
                return lhs.refNo < rhs.refNo;
            }

            return lhs.isOrder && !rhs.isOrder;
        }

        return lhs.date < rhs.date;
    });

    core::Money runningBalance = 0;
    ledger.reserve(events.size());
    for (const LedgerEvent &event : events) {
        runningBalance += event.deltaVnd;

        domain::DebtLedger entry;
        entry.id = QStringLiteral("%1-%2").arg(event.isOrder ? QStringLiteral("ORDER") : QStringLiteral("PAY"),
                                               event.refNo);
        entry.customerId = normalizedCustomerId;
        entry.refType = event.isOrder ? QStringLiteral("Order") : QStringLiteral("Payment");
        entry.refId = event.refNo;
        entry.deltaVnd = event.deltaVnd;
        entry.balanceAfterVnd = runningBalance;
        entry.createdAt = event.date;
        ledger.push_back(entry);
    }

    return ledger;
}

QString PaymentService::orderNoById(const QString &orderId) const
{
    domain::Order order;
    if (!m_orderService.getOrderById(orderId, order)) {
        return QString();
    }

    return order.orderNo;
}

bool PaymentService::createReceipt(const QString &customerId,
                                   const QString &orderId,
                                   core::Money amountVnd,
                                   const QString &method,
                                   const QString &paidAt,
                                   QString &errorMessage)
{
    const QString normalizedCustomerId = customerId.trimmed();
    if (!m_customerService.customerExists(normalizedCustomerId)) {
        errorMessage = QStringLiteral("Khách hàng không hợp lệ.");
        return false;
    }

    const QString normalizedOrderId = orderId.trimmed();
    if (normalizedOrderId.isEmpty()) {
        errorMessage = QStringLiteral("Bạn cần chọn đơn để lập phiếu thu.");
        return false;
    }

    domain::Order order;
    if (!m_orderService.getOrderById(normalizedOrderId, order)) {
        errorMessage = QStringLiteral("Không tìm thấy đơn hàng cần thu tiền.");
        return false;
    }

    if (order.customerId != normalizedCustomerId) {
        errorMessage = QStringLiteral("Đơn hàng không thuộc khách đã chọn.");
        return false;
    }

    if (order.status != domain::OrderStatus::Confirmed
        && order.status != domain::OrderStatus::PartiallyPaid) {
        errorMessage = QStringLiteral("Chỉ đơn Confirmed/PartiallyPaid mới được lập phiếu thu.");
        return false;
    }

    if (order.balanceVnd <= 0) {
        errorMessage = QStringLiteral("Đơn không còn công nợ để thu.");
        return false;
    }

    if (amountVnd <= 0) {
        errorMessage = QStringLiteral("Số tiền thu phải > 0.");
        return false;
    }

    if (amountVnd > order.balanceVnd) {
        errorMessage = QStringLiteral("Không được thu vượt công nợ còn lại của đơn.");
        return false;
    }

    QString normalizedPaidAt = paidAt.trimmed();
    if (normalizedPaidAt.isEmpty()) {
        normalizedPaidAt = QDate::currentDate().toString(Qt::ISODate);
    } else {
        const QDate paidDate = QDate::fromString(normalizedPaidAt, Qt::ISODate);
        if (!paidDate.isValid()) {
            errorMessage = QStringLiteral("Ngày thu không hợp lệ. Dùng định dạng YYYY-MM-DD.");
            return false;
        }
        normalizedPaidAt = paidDate.toString(Qt::ISODate);
    }

    QString applyError;
    if (!m_orderService.applyPayment(normalizedOrderId, amountVnd, applyError)) {
        errorMessage = applyError;
        return false;
    }

    domain::Payment payment;
    payment.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    payment.paymentNo = nextPaymentNo();
    payment.customerId = normalizedCustomerId;
    payment.orderId = normalizedOrderId;
    payment.amountVnd = amountVnd;
    payment.method = method.trimmed().isEmpty() ? QStringLiteral("Tien mat") : method.trimmed();
    payment.paidAt = normalizedPaidAt;

    m_payments.push_back(payment);
    errorMessage.clear();
    return true;
}

QString PaymentService::nextPaymentNo()
{
    return QStringLiteral("PT%1").arg(m_nextPaymentSerial++, 5, 10, QLatin1Char('0'));
}

} // namespace stockmaster::application
