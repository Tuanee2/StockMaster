#include <cassert>

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>

#include "stockmaster/application/customer_service.h"
#include "stockmaster/application/order_service.h"
#include "stockmaster/application/payment_service.h"
#include "stockmaster/application/product_service.h"
#include "stockmaster/infra/db/database_service.h"

int main(int argc, char *argv[])
{
    using namespace stockmaster;

    QCoreApplication app(argc, argv);

    const QString dbPath = QDir::temp().filePath(QStringLiteral("stockmaster_sqlite_persistence_smoke.sqlite"));
    QFile::remove(dbPath);

    QString createdCustomerId;
    QString createdOrderId;
    QString createdProductId;

    {
        infra::db::DatabaseService databaseService(dbPath);
        assert(databaseService.initialize());

        application::CustomerService customerService(databaseService);
        application::ProductService productService(databaseService);
        application::OrderService orderService(databaseService, customerService, productService);
        application::PaymentService paymentService(databaseService, orderService, customerService);

        QString errorMessage;
        assert(customerService.createCustomer(application::CustomerInput{
                                                  QStringLiteral("Khach Luu SQLite"),
                                                  QStringLiteral("0909000000"),
                                                  QStringLiteral("Can Tho"),
                                                  25000000,
                                              },
                                              errorMessage));

        const QVector<domain::Customer> customers = customerService.findCustomers(QStringLiteral("Khach Luu SQLite"));
        assert(customers.size() == 1);
        createdCustomerId = customers.first().id;

        assert(productService.createProduct(application::ProductInput{
                                                QString(),
                                                QStringLiteral("Hang Luu SQLite"),
                                                QStringLiteral("hop"),
                                                150000,
                                                true,
                                            },
                                            errorMessage));

        const QVector<domain::Product> persistedProducts = productService.findProducts(QStringLiteral("Hang Luu SQLite"));
        assert(persistedProducts.size() == 1);
        createdProductId = persistedProducts.first().id;

        assert(productService.addLot(createdProductId,
                                     application::ProductLotInput{
                                         QString(),
                                         QStringLiteral("2026-03-01"),
                                         QStringLiteral("2026-12-31"),
                                         20,
                                         100000,
                                     },
                                     errorMessage));

        assert(orderService.createDraftOrder(createdCustomerId,
                                             QStringLiteral("2026-03-05"),
                                             createdOrderId,
                                             errorMessage));
        assert(orderService.upsertDraftItem(createdOrderId,
                                            application::DraftOrderItemInput{
                                                createdProductId,
                                                2,
                                                150000,
                                                0,
                                            },
                                            errorMessage));
        assert(orderService.confirmOrder(createdOrderId, errorMessage));
        assert(paymentService.createReceipt(createdCustomerId,
                                            createdOrderId,
                                            150000,
                                            QStringLiteral("Tien mat"),
                                            QStringLiteral("2026-03-06"),
                                            errorMessage));
    }

    assert(QFileInfo::exists(dbPath));
    assert(QFileInfo(dbPath).size() > 0);

    {
        infra::db::DatabaseService databaseService(dbPath);
        assert(databaseService.initialize());

        application::CustomerService customerService(databaseService);
        application::ProductService productService(databaseService);
        application::OrderService orderService(databaseService, customerService, productService);
        application::PaymentService paymentService(databaseService, orderService, customerService);

        const QVector<domain::Customer> customers = customerService.findCustomers(QStringLiteral("Khach Luu SQLite"));
        assert(customers.size() == 1);
        assert(customers.first().id == createdCustomerId);

        const QVector<domain::Product> products = productService.findProducts(QStringLiteral("Hang Luu SQLite"));
        assert(products.size() == 1);
        assert(products.first().id == createdProductId);

        domain::Order restoredOrder;
        assert(orderService.getOrderById(createdOrderId, restoredOrder));
        assert(restoredOrder.status == domain::OrderStatus::PartiallyPaid);
        assert(restoredOrder.balanceVnd == 150000);

        const QVector<domain::Payment> payments = paymentService.findPaymentsByCustomer(createdCustomerId);
        assert(payments.size() == 1);
        assert(payments.first().amountVnd == 150000);
    }

    return 0;
}
