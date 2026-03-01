#include <cassert>

#include <QCoreApplication>
#include <QDir>
#include <QFile>

#include "stockmaster/application/customer_service.h"
#include "stockmaster/application/order_service.h"
#include "stockmaster/application/payment_service.h"
#include "stockmaster/application/product_service.h"
#include "stockmaster/infra/db/database_service.h"

int main(int argc, char *argv[])
{
    using namespace stockmaster;

    QCoreApplication app(argc, argv);

    const QString dbPath = QDir::temp().filePath(QStringLiteral("stockmaster_payment_service_smoke.sqlite"));
    QFile::remove(dbPath);

    infra::db::DatabaseService databaseService(dbPath);
    assert(databaseService.initialize());

    application::CustomerService customerService(databaseService);
    application::ProductService productService(databaseService);
    application::OrderService orderService(databaseService, customerService, productService);
    application::PaymentService paymentService(databaseService, orderService, customerService);

    const QVector<domain::Customer> customers = customerService.findCustomers();
    const QVector<domain::Product> products = productService.findProducts();
    assert(!customers.isEmpty());
    assert(!products.isEmpty());

    QString orderId;
    QString errorMessage;
    assert(orderService.createDraftOrder(customers.first().id,
                                         QStringLiteral("2026-03-01"),
                                         orderId,
                                         errorMessage));

    const core::Money unitPriceVnd = products.first().defaultWholesalePriceVnd;
    assert(orderService.upsertDraftItem(orderId,
                                        application::DraftOrderItemInput{products.first().id, 1, unitPriceVnd, 0},
                                        errorMessage));
    assert(orderService.confirmOrder(orderId, errorMessage));

    domain::Order order;
    assert(orderService.getOrderById(orderId, order));
    assert(order.status == domain::OrderStatus::Confirmed);
    assert(order.balanceVnd == unitPriceVnd);

    const core::Money partialAmount = unitPriceVnd / 2;
    assert(partialAmount > 0);
    assert(paymentService.createReceipt(customers.first().id,
                                        orderId,
                                        partialAmount,
                                        QStringLiteral("Tien mat"),
                                        QStringLiteral("2026-03-02"),
                                        errorMessage));

    assert(orderService.getOrderById(orderId, order));
    assert(order.status == domain::OrderStatus::PartiallyPaid);
    assert(order.balanceVnd == unitPriceVnd - partialAmount);

    assert(!paymentService.createReceipt(customers.first().id,
                                         orderId,
                                         order.balanceVnd + 1,
                                         QStringLiteral("Tien mat"),
                                         QStringLiteral("2026-03-03"),
                                         errorMessage));

    assert(paymentService.createReceipt(customers.first().id,
                                        orderId,
                                        order.balanceVnd,
                                        QStringLiteral("Chuyen khoan"),
                                        QStringLiteral("2026-03-03"),
                                        errorMessage));

    assert(orderService.getOrderById(orderId, order));
    assert(order.status == domain::OrderStatus::Paid);
    assert(order.balanceVnd == 0);

    const QVector<domain::Payment> payments = paymentService.findPaymentsByCustomer(customers.first().id);
    assert(payments.size() == 2);

    const QVector<domain::DebtLedger> ledger = paymentService.findLedgerByCustomer(customers.first().id);
    assert(ledger.size() == 3);
    assert(ledger.last().balanceAfterVnd == 0);

    return 0;
}
