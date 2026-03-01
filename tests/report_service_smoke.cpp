#include <cassert>

#include <QDate>
#include <QDir>
#include <QFileInfo>
#include <QGuiApplication>

#include "stockmaster/application/customer_service.h"
#include "stockmaster/application/order_service.h"
#include "stockmaster/application/payment_service.h"
#include "stockmaster/application/product_service.h"
#include "stockmaster/application/report_service.h"
#include "stockmaster/infra/db/database_service.h"

int main(int argc, char *argv[])
{
    using namespace stockmaster;

    QGuiApplication app(argc, argv);

    infra::db::DatabaseService databaseService;
    assert(databaseService.initialize());

    application::CustomerService customerService;
    application::ProductService productService;
    application::OrderService orderService(databaseService, customerService, productService);
    application::PaymentService paymentService(orderService, customerService);
    application::ReportService reportService(orderService,
                                             paymentService,
                                             customerService,
                                             productService);

    const QVector<domain::Customer> customers = customerService.findCustomers();
    const QVector<domain::Product> products = productService.findProducts();
    assert(!customers.isEmpty());
    assert(!products.isEmpty());

    QString errorMessage;
    QString orderId;
    const QString reportDate = QDate::currentDate().toString(Qt::ISODate);

    assert(orderService.createDraftOrder(customers.first().id,
                                         reportDate,
                                         orderId,
                                         errorMessage));
    assert(orderService.upsertDraftItem(orderId,
                                        application::DraftOrderItemInput{products.first().id,
                                                                         2,
                                                                         products.first().defaultWholesalePriceVnd,
                                                                         0},
                                        errorMessage));
    assert(orderService.confirmOrder(orderId, errorMessage));
    assert(paymentService.createReceipt(customers.first().id,
                                        orderId,
                                        products.first().defaultWholesalePriceVnd,
                                        QStringLiteral("Tien mat"),
                                        reportDate,
                                        errorMessage));

    application::SalesSummaryReport salesReport;
    assert(reportService.buildSalesSummaryReport(reportDate, reportDate, salesReport, errorMessage));
    assert(salesReport.orderCount >= 1);
    assert(salesReport.totalSalesVnd > 0);
    assert(salesReport.totalCollectedVnd > 0);
    assert(!salesReport.buckets.isEmpty());

    const application::DebtAgingReport debtReport = reportService.buildDebtAgingReport();
    assert(debtReport.totalVnd >= 0);
    assert(!debtReport.rows.isEmpty());

    QVector<application::InventoryMovementReportRow> movementRows;
    assert(reportService.buildInventoryMovementReport(QString(),
                                                     reportDate,
                                                     reportDate,
                                                     movementRows,
                                                     errorMessage));
    assert(!movementRows.isEmpty());

    const QString tempDir = QDir::tempPath();
    const QString salesCsvPath = tempDir + QStringLiteral("/stockmaster_sales_summary_smoke.csv");
    const QString debtPdfPath = tempDir + QStringLiteral("/stockmaster_debt_aging_smoke.pdf");
    const QString movementCsvPath = tempDir + QStringLiteral("/stockmaster_inventory_movement_smoke.csv");

    assert(reportService.exportSalesSummaryCsv(salesReport, salesCsvPath, errorMessage));
    assert(QFileInfo::exists(salesCsvPath));
    assert(QFileInfo(salesCsvPath).size() > 0);

    assert(reportService.exportDebtAgingPdf(debtReport, debtPdfPath, errorMessage));
    assert(QFileInfo::exists(debtPdfPath));
    assert(QFileInfo(debtPdfPath).size() > 0);

    assert(reportService.exportInventoryMovementCsv(movementRows, movementCsvPath, errorMessage));
    assert(QFileInfo::exists(movementCsvPath));
    assert(QFileInfo(movementCsvPath).size() > 0);

    return 0;
}
