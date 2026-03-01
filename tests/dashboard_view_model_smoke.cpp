#include <cassert>

#include <QCoreApplication>
#include <QDate>
#include <QDir>
#include <QFile>
#include <QVariantList>
#include <QVariantMap>

#include "stockmaster/application/customer_service.h"
#include "stockmaster/application/order_service.h"
#include "stockmaster/application/payment_service.h"
#include "stockmaster/application/product_service.h"
#include "stockmaster/infra/db/database_service.h"
#include "stockmaster/ui/viewmodels/app_view_model.h"

int main(int argc, char *argv[])
{
    using namespace stockmaster;

    QCoreApplication app(argc, argv);

    const QString dbPath = QDir::temp().filePath(QStringLiteral("stockmaster_dashboard_view_model_smoke.sqlite"));
    QFile::remove(dbPath);

    infra::db::DatabaseService databaseService(dbPath);
    assert(databaseService.initialize());

    application::CustomerService customerService(databaseService);
    application::ProductService productService(databaseService);
    application::OrderService orderService(databaseService, customerService, productService);
    application::PaymentService paymentService(databaseService, orderService, customerService);
    ui::viewmodels::AppViewModel appViewModel(databaseService,
                                              orderService,
                                              paymentService,
                                              customerService,
                                              productService);

    const QVector<domain::Customer> customers = customerService.findCustomers();
    const QVector<domain::Product> products = productService.findProducts();
    assert(customers.size() >= 2);
    assert(products.size() >= 2);

    QString errorMessage;
    QString firstOrderId;
    const QDate currentMonth = QDate::currentDate();
    const QDate lastMonth = currentMonth.addMonths(-1);

    assert(orderService.createDraftOrder(customers.at(0).id,
                                         lastMonth.toString(Qt::ISODate),
                                         firstOrderId,
                                         errorMessage));
    assert(orderService.upsertDraftItem(firstOrderId,
                                        application::DraftOrderItemInput{products.at(0).id,
                                                                         2,
                                                                         products.at(0).defaultWholesalePriceVnd,
                                                                         0},
                                        errorMessage));
    assert(orderService.confirmOrder(firstOrderId, errorMessage));
    assert(paymentService.createReceipt(customers.at(0).id,
                                        firstOrderId,
                                        products.at(0).defaultWholesalePriceVnd,
                                        QStringLiteral("Tien mat"),
                                        currentMonth.toString(Qt::ISODate),
                                        errorMessage));

    QString secondOrderId;
    assert(orderService.createDraftOrder(customers.at(1).id,
                                         currentMonth.toString(Qt::ISODate),
                                         secondOrderId,
                                         errorMessage));
    assert(orderService.upsertDraftItem(secondOrderId,
                                        application::DraftOrderItemInput{products.at(1).id,
                                                                         3,
                                                                         products.at(1).defaultWholesalePriceVnd,
                                                                         0},
                                        errorMessage));
    assert(orderService.confirmOrder(secondOrderId, errorMessage));

    appViewModel.refreshOverview();

    const QVariantList monthlyTrends = appViewModel.monthlyTrends();
    assert(monthlyTrends.size() == 6);

    bool foundCurrentMonth = false;
    bool foundLastMonth = false;
    for (const QVariant &item : monthlyTrends) {
        const QVariantMap row = item.toMap();
        const QString monthKey = row.value(QStringLiteral("monthKey")).toString();
        if (monthKey == currentMonth.toString(QStringLiteral("yyyy-MM"))) {
            foundCurrentMonth = true;
            assert(row.value(QStringLiteral("salesVnd")).toLongLong() > 0);
            assert(row.value(QStringLiteral("collectedVnd")).toLongLong() > 0);
        }

        if (monthKey == lastMonth.toString(QStringLiteral("yyyy-MM"))) {
            foundLastMonth = true;
            assert(row.value(QStringLiteral("salesVnd")).toLongLong() > 0);
        }
    }

    assert(foundCurrentMonth);
    assert(foundLastMonth);
    assert(appViewModel.monthlyTrendPeakVnd() > 0);
    assert(appViewModel.hasMonthlyActivity());

    const QVariantList topCustomers = appViewModel.topCustomers();
    assert(!topCustomers.isEmpty());
    const QVariantMap firstCustomer = topCustomers.first().toMap();
    assert(firstCustomer.value(QStringLiteral("salesVnd")).toLongLong() > 0);

    const QVariantList topProducts = appViewModel.topProducts();
    assert(!topProducts.isEmpty());
    const QVariantMap firstProduct = topProducts.first().toMap();
    assert(firstProduct.value(QStringLiteral("salesVnd")).toLongLong() > 0);

    const QVariantList operationalAlerts = appViewModel.operationalAlerts();
    assert(!operationalAlerts.isEmpty());

    return 0;
}
