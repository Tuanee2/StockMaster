#include "stockmaster/ui/viewmodels/reports_view_model.h"

#include <QDate>
#include <QDateTime>
#include <QDir>
#include <QLocale>
#include <QStandardPaths>
#include <QVariantMap>

namespace stockmaster::ui::viewmodels {

ReportsViewModel::ReportsViewModel(application::ReportService &reportService, QObject *parent)
    : QObject(parent)
    , m_reportService(reportService)
{
    reloadAll();
}

QString ReportsViewModel::lastError() const
{
    return m_lastError;
}

QString ReportsViewModel::lastExportPath() const
{
    return m_lastExportPath;
}

QString ReportsViewModel::salesFromDate() const
{
    return m_salesReport.fromDate;
}

QString ReportsViewModel::salesToDate() const
{
    return m_salesReport.toDate;
}

int ReportsViewModel::salesOrderCount() const
{
    return m_salesReport.orderCount;
}

QString ReportsViewModel::salesTotalVndText() const
{
    return formatMoney(m_salesReport.totalSalesVnd);
}

QString ReportsViewModel::salesCollectedVndText() const
{
    return formatMoney(m_salesReport.totalCollectedVnd);
}

QString ReportsViewModel::salesOutstandingVndText() const
{
    return formatMoney(m_salesReport.outstandingVnd);
}

QVariantList ReportsViewModel::salesBuckets() const
{
    QVariantList rows;
    rows.reserve(m_salesReport.buckets.size());

    for (const application::SalesSummaryBucket &bucket : m_salesReport.buckets) {
        rows.push_back(QVariantMap{
            {QStringLiteral("bucketDate"), bucket.bucketDate},
            {QStringLiteral("bucketLabel"), bucket.bucketLabel},
            {QStringLiteral("orderCount"), bucket.orderCount},
            {QStringLiteral("salesVnd"), bucket.salesVnd},
            {QStringLiteral("salesText"), formatMoney(bucket.salesVnd)},
            {QStringLiteral("collectedVnd"), bucket.collectedVnd},
            {QStringLiteral("collectedText"), formatMoney(bucket.collectedVnd)},
        });
    }

    return rows;
}

QString ReportsViewModel::debtCurrentVndText() const
{
    return formatMoney(m_debtReport.currentBucketVnd);
}

QString ReportsViewModel::debt31To60VndText() const
{
    return formatMoney(m_debtReport.bucket31To60Vnd);
}

QString ReportsViewModel::debt61PlusVndText() const
{
    return formatMoney(m_debtReport.bucket61PlusVnd);
}

QString ReportsViewModel::debtTotalVndText() const
{
    return formatMoney(m_debtReport.totalVnd);
}

QVariantList ReportsViewModel::debtRows() const
{
    QVariantList rows;
    rows.reserve(m_debtReport.rows.size());

    for (const application::DebtAgingRow &row : m_debtReport.rows) {
        rows.push_back(QVariantMap{
            {QStringLiteral("customerId"), row.customerId},
            {QStringLiteral("code"), row.code},
            {QStringLiteral("name"), row.name},
            {QStringLiteral("currentVnd"), row.currentBucketVnd},
            {QStringLiteral("currentText"), formatMoney(row.currentBucketVnd)},
            {QStringLiteral("bucket31To60Vnd"), row.bucket31To60Vnd},
            {QStringLiteral("bucket31To60Text"), formatMoney(row.bucket31To60Vnd)},
            {QStringLiteral("bucket61PlusVnd"), row.bucket61PlusVnd},
            {QStringLiteral("bucket61PlusText"), formatMoney(row.bucket61PlusVnd)},
            {QStringLiteral("totalVnd"), row.totalVnd},
            {QStringLiteral("totalText"), formatMoney(row.totalVnd)},
        });
    }

    return rows;
}

QString ReportsViewModel::movementFromDate() const
{
    return m_movementFromDate;
}

QString ReportsViewModel::movementToDate() const
{
    return m_movementToDate;
}

QVariantList ReportsViewModel::inventoryMovementRows() const
{
    QVariantList rows;
    rows.reserve(m_inventoryMovementRows.size());

    for (const application::InventoryMovementReportRow &row : m_inventoryMovementRows) {
        rows.push_back(QVariantMap{
            {QStringLiteral("movementId"), row.movementId},
            {QStringLiteral("movementDate"), row.movementDate},
            {QStringLiteral("sku"), row.sku},
            {QStringLiteral("productName"), row.productName},
            {QStringLiteral("lotNo"), row.lotNo},
            {QStringLiteral("movementType"), row.movementType},
            {QStringLiteral("reason"), row.reason},
            {QStringLiteral("qtyDelta"), row.qtyDelta},
            {QStringLiteral("qtyDeltaText"),
             (row.qtyDelta > 0 ? QStringLiteral("+") : QString()) + QString::number(row.qtyDelta)},
            {QStringLiteral("lotBalanceAfter"), row.lotBalanceAfter},
        });
    }

    return rows;
}

void ReportsViewModel::reloadAll()
{
    const QDate today = QDate::currentDate();
    const QString defaultFrom = QDate(today.year(), today.month(), 1).toString(Qt::ISODate);
    const QString defaultTo = today.toString(Qt::ISODate);

    (void)loadSalesSummary(defaultFrom, defaultTo);
    loadDebtAging();
    (void)loadInventoryMovement(QString(), defaultFrom, defaultTo);
}

bool ReportsViewModel::loadSalesSummary(const QString &fromDate, const QString &toDate)
{
    application::SalesSummaryReport report;
    QString errorMessage;
    if (!m_reportService.buildSalesSummaryReport(fromDate, toDate, report, errorMessage)) {
        setLastError(errorMessage);
        return false;
    }

    m_salesReport = report;
    setLastError(QString());
    emit salesChanged();
    return true;
}

void ReportsViewModel::loadDebtAging()
{
    m_debtReport = m_reportService.buildDebtAgingReport();
    setLastError(QString());
    emit debtChanged();
}

bool ReportsViewModel::loadInventoryMovement(const QString &keyword,
                                             const QString &fromDate,
                                             const QString &toDate)
{
    QVector<application::InventoryMovementReportRow> rows;
    QString errorMessage;
    if (!m_reportService.buildInventoryMovementReport(keyword, fromDate, toDate, rows, errorMessage)) {
        setLastError(errorMessage);
        return false;
    }

    m_movementKeyword = keyword.trimmed();
    QString normalizedFrom = fromDate.trimmed();
    QString normalizedTo = toDate.trimmed();
    if (normalizedFrom.isEmpty() && normalizedTo.isEmpty()) {
        const QDate today = QDate::currentDate();
        normalizedTo = today.toString(Qt::ISODate);
        normalizedFrom = QDate(today.year(), today.month(), 1).toString(Qt::ISODate);
    } else if (normalizedFrom.isEmpty()) {
        normalizedFrom = normalizedTo;
    } else if (normalizedTo.isEmpty()) {
        normalizedTo = normalizedFrom;
    }

    m_movementFromDate = normalizedFrom;
    m_movementToDate = normalizedTo;
    m_inventoryMovementRows = rows;
    setLastError(QString());
    emit movementChanged();
    return true;
}

bool ReportsViewModel::exportSalesSummary(const QString &format)
{
    QString errorMessage;
    const QString extension = format.compare(QStringLiteral("pdf"), Qt::CaseInsensitive) == 0
        ? QStringLiteral("pdf")
        : QStringLiteral("csv");
    const QString filePath = buildExportPath(QStringLiteral("sales_summary"), extension, errorMessage);
    if (filePath.isEmpty()) {
        setLastError(errorMessage);
        return false;
    }

    const bool success = extension == QLatin1String("pdf")
        ? m_reportService.exportSalesSummaryPdf(m_salesReport, filePath, errorMessage)
        : m_reportService.exportSalesSummaryCsv(m_salesReport, filePath, errorMessage);

    if (!success) {
        setLastError(errorMessage);
        return false;
    }

    setLastError(QString());
    setLastExportPath(filePath);
    return true;
}

bool ReportsViewModel::exportDebtAging(const QString &format)
{
    QString errorMessage;
    const QString extension = format.compare(QStringLiteral("pdf"), Qt::CaseInsensitive) == 0
        ? QStringLiteral("pdf")
        : QStringLiteral("csv");
    const QString filePath = buildExportPath(QStringLiteral("debt_aging"), extension, errorMessage);
    if (filePath.isEmpty()) {
        setLastError(errorMessage);
        return false;
    }

    const bool success = extension == QLatin1String("pdf")
        ? m_reportService.exportDebtAgingPdf(m_debtReport, filePath, errorMessage)
        : m_reportService.exportDebtAgingCsv(m_debtReport, filePath, errorMessage);

    if (!success) {
        setLastError(errorMessage);
        return false;
    }

    setLastError(QString());
    setLastExportPath(filePath);
    return true;
}

bool ReportsViewModel::exportInventoryMovement(const QString &format)
{
    QString errorMessage;
    const QString extension = format.compare(QStringLiteral("pdf"), Qt::CaseInsensitive) == 0
        ? QStringLiteral("pdf")
        : QStringLiteral("csv");
    const QString filePath = buildExportPath(QStringLiteral("inventory_movement"), extension, errorMessage);
    if (filePath.isEmpty()) {
        setLastError(errorMessage);
        return false;
    }

    const bool success = extension == QLatin1String("pdf")
        ? m_reportService.exportInventoryMovementPdf(m_inventoryMovementRows, filePath, errorMessage)
        : m_reportService.exportInventoryMovementCsv(m_inventoryMovementRows, filePath, errorMessage);

    if (!success) {
        setLastError(errorMessage);
        return false;
    }

    setLastError(QString());
    setLastExportPath(filePath);
    return true;
}

QString ReportsViewModel::formatMoney(core::Money amountVnd)
{
    const QLocale locale(QLocale::Vietnamese, QLocale::Vietnam);
    return locale.toString(amountVnd) + QStringLiteral(" VND");
}

QString ReportsViewModel::buildExportPath(const QString &baseName,
                                          const QString &extension,
                                          QString &errorMessage) const
{
    QString rootDir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    if (rootDir.isEmpty()) {
        rootDir = QDir::currentPath();
    }

    QDir exportDir(rootDir);
    if (!exportDir.mkpath(QStringLiteral("StockMasterExports"))) {
        errorMessage = QStringLiteral("Không thể tạo thư mục export.");
        return QString();
    }

    exportDir.cd(QStringLiteral("StockMasterExports"));
    const QString timeStamp = QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd_HHmmss"));
    errorMessage.clear();
    return exportDir.filePath(QStringLiteral("%1_%2.%3")
                                  .arg(baseName)
                                  .arg(timeStamp)
                                  .arg(extension));
}

void ReportsViewModel::setLastError(const QString &message)
{
    if (m_lastError == message) {
        return;
    }

    m_lastError = message;
    emit lastErrorChanged();
}

void ReportsViewModel::setLastExportPath(const QString &path)
{
    if (m_lastExportPath == path) {
        return;
    }

    m_lastExportPath = path;
    emit lastExportPathChanged();
}

} // namespace stockmaster::ui::viewmodels
