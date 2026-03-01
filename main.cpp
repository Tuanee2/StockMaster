#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>

#include "stockmaster/application/customer_service.h"
#include "stockmaster/application/order_service.h"
#include "stockmaster/application/payment_service.h"
#include "stockmaster/application/product_service.h"
#include "stockmaster/application/report_service.h"
#include "stockmaster/infra/db/database_service.h"
#include "stockmaster/ui/viewmodels/app_view_model.h"
#include "stockmaster/ui/viewmodels/customers_view_model.h"
#include "stockmaster/ui/viewmodels/inventory_view_model.h"
#include "stockmaster/ui/viewmodels/orders_view_model.h"
#include "stockmaster/ui/viewmodels/payments_view_model.h"
#include "stockmaster/ui/viewmodels/products_view_model.h"
#include "stockmaster/ui/viewmodels/reports_view_model.h"
#include "stockmaster/ui/viewmodels/settings_view_model.h"

int main(int argc, char *argv[])
{
    QQuickStyle::setStyle(QStringLiteral("Basic"));

    QGuiApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("StockMaster"));
    app.setApplicationVersion(QStringLiteral(STOCKMASTER_APP_VERSION));

    stockmaster::infra::db::DatabaseService databaseService;
    databaseService.initialize();

    stockmaster::application::CustomerService customerService(databaseService);
    stockmaster::application::ProductService productService(databaseService);
    stockmaster::application::OrderService orderService(databaseService, customerService, productService);
    stockmaster::application::PaymentService paymentService(databaseService, orderService, customerService);
    stockmaster::application::ReportService reportService(orderService,
                                                          paymentService,
                                                          customerService,
                                                          productService);
    stockmaster::ui::viewmodels::AppViewModel appViewModel(databaseService,
                                                           orderService,
                                                           paymentService,
                                                           customerService,
                                                           productService);
    stockmaster::ui::viewmodels::CustomersViewModel customersViewModel(customerService);
    stockmaster::ui::viewmodels::InventoryViewModel inventoryViewModel(productService);
    stockmaster::ui::viewmodels::OrdersViewModel ordersViewModel(orderService, customerService, productService);
    stockmaster::ui::viewmodels::PaymentsViewModel paymentsViewModel(paymentService);
    stockmaster::ui::viewmodels::ProductsViewModel productsViewModel(productService);
    stockmaster::ui::viewmodels::ReportsViewModel reportsViewModel(reportService);
    stockmaster::ui::viewmodels::SettingsViewModel settingsViewModel(databaseService);

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("appViewModel"), &appViewModel);
    engine.rootContext()->setContextProperty(QStringLiteral("customersViewModel"), &customersViewModel);
    engine.rootContext()->setContextProperty(QStringLiteral("inventoryViewModel"), &inventoryViewModel);
    engine.rootContext()->setContextProperty(QStringLiteral("ordersViewModel"), &ordersViewModel);
    engine.rootContext()->setContextProperty(QStringLiteral("paymentsViewModel"), &paymentsViewModel);
    engine.rootContext()->setContextProperty(QStringLiteral("productsViewModel"), &productsViewModel);
    engine.rootContext()->setContextProperty(QStringLiteral("reportsViewModel"), &reportsViewModel);
    engine.rootContext()->setContextProperty(QStringLiteral("settingsViewModel"), &settingsViewModel);

    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);

    engine.loadFromModule("StockMaster", "Main");

    return app.exec();
}
