#pragma once

#include <QDate>
#include <QString>
#include <QVector>

#include "stockmaster/domain/entities.h"

namespace stockmaster::application {

class CustomerService;
class OrderService;
class PaymentService;
class ProductService;

struct SalesSummaryBucket {
    QString bucketDate;
    QString bucketLabel;
    int orderCount = 0;
    core::Money salesVnd = 0;
    core::Money collectedVnd = 0;
};

struct SalesSummaryReport {
    QString fromDate;
    QString toDate;
    int orderCount = 0;
    core::Money totalSalesVnd = 0;
    core::Money totalCollectedVnd = 0;
    core::Money outstandingVnd = 0;
    QVector<SalesSummaryBucket> buckets;
};

struct DebtAgingRow {
    QString customerId;
    QString code;
    QString name;
    core::Money currentBucketVnd = 0;
    core::Money bucket31To60Vnd = 0;
    core::Money bucket61PlusVnd = 0;
    core::Money totalVnd = 0;
};

struct DebtAgingReport {
    core::Money currentBucketVnd = 0;
    core::Money bucket31To60Vnd = 0;
    core::Money bucket61PlusVnd = 0;
    core::Money totalVnd = 0;
    QVector<DebtAgingRow> rows;
};

struct InventoryMovementReportRow {
    QString movementId;
    QString movementDate;
    QString productId;
    QString sku;
    QString productName;
    QString lotNo;
    QString movementType;
    QString reason;
    int qtyDelta = 0;
    int lotBalanceAfter = 0;
};

class ReportService {
public:
    ReportService(const OrderService &orderService,
                  const PaymentService &paymentService,
                  const CustomerService &customerService,
                  const ProductService &productService);

    [[nodiscard]] bool buildSalesSummaryReport(const QString &fromDate,
                                               const QString &toDate,
                                               SalesSummaryReport &report,
                                               QString &errorMessage) const;
    [[nodiscard]] DebtAgingReport buildDebtAgingReport() const;
    [[nodiscard]] bool buildInventoryMovementReport(const QString &keyword,
                                                    const QString &fromDate,
                                                    const QString &toDate,
                                                    QVector<InventoryMovementReportRow> &rows,
                                                    QString &errorMessage) const;

    [[nodiscard]] bool exportSalesSummaryCsv(const SalesSummaryReport &report,
                                             const QString &filePath,
                                             QString &errorMessage) const;
    [[nodiscard]] bool exportSalesSummaryPdf(const SalesSummaryReport &report,
                                             const QString &filePath,
                                             QString &errorMessage) const;
    [[nodiscard]] bool exportDebtAgingCsv(const DebtAgingReport &report,
                                          const QString &filePath,
                                          QString &errorMessage) const;
    [[nodiscard]] bool exportDebtAgingPdf(const DebtAgingReport &report,
                                          const QString &filePath,
                                          QString &errorMessage) const;
    [[nodiscard]] bool exportInventoryMovementCsv(const QVector<InventoryMovementReportRow> &rows,
                                                  const QString &filePath,
                                                  QString &errorMessage) const;
    [[nodiscard]] bool exportInventoryMovementPdf(const QVector<InventoryMovementReportRow> &rows,
                                                  const QString &filePath,
                                                  QString &errorMessage) const;

private:
    [[nodiscard]] static bool parseDateRange(const QString &fromDate,
                                             const QString &toDate,
                                             QDate &startDate,
                                             QDate &endDate,
                                             QString &errorMessage);
    [[nodiscard]] static bool countsAsSales(domain::OrderStatus status);
    [[nodiscard]] static QString movementTypeLabel(const QString &movementType);
    [[nodiscard]] static bool writeTextFile(const QString &filePath,
                                            const QStringList &lines,
                                            QString &errorMessage);
    [[nodiscard]] static bool writePdfFile(const QString &filePath,
                                           const QString &title,
                                           const QStringList &lines,
                                           QString &errorMessage);
    [[nodiscard]] static QString csvCell(const QString &value);

    const OrderService &m_orderService;
    const PaymentService &m_paymentService;
    const CustomerService &m_customerService;
    const ProductService &m_productService;
};

} // namespace stockmaster::application
