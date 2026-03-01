#include "stockmaster/application/report_service.h"

#include <QFile>
#include <QHash>
#include <QPainter>
#include <QPageSize>
#include <QPdfWriter>
#include <QTextStream>

#include <algorithm>

#include "stockmaster/application/customer_service.h"
#include "stockmaster/application/order_service.h"
#include "stockmaster/application/payment_service.h"
#include "stockmaster/application/product_service.h"

namespace stockmaster::application {

namespace {

QString formatMoney(core::Money amountVnd)
{
    return QString::number(amountVnd);
}

}

ReportService::ReportService(const OrderService &orderService,
                             const PaymentService &paymentService,
                             const CustomerService &customerService,
                             const ProductService &productService)
    : m_orderService(orderService)
    , m_paymentService(paymentService)
    , m_customerService(customerService)
    , m_productService(productService)
{
}

bool ReportService::buildSalesSummaryReport(const QString &fromDate,
                                            const QString &toDate,
                                            SalesSummaryReport &report,
                                            QString &errorMessage) const
{
    QDate startDate;
    QDate endDate;
    if (!parseDateRange(fromDate, toDate, startDate, endDate, errorMessage)) {
        return false;
    }

    report = SalesSummaryReport{};
    report.fromDate = startDate.toString(Qt::ISODate);
    report.toDate = endDate.toString(Qt::ISODate);

    const int dayCount = startDate.daysTo(endDate) + 1;
    report.buckets.reserve(std::max(0, dayCount));

    QHash<QString, qsizetype> bucketByDate;
    for (int offset = 0; offset < dayCount; ++offset) {
        const QDate day = startDate.addDays(offset);
        SalesSummaryBucket bucket;
        bucket.bucketDate = day.toString(Qt::ISODate);
        bucket.bucketLabel = day.toString(QStringLiteral("dd/MM"));
        bucketByDate.insert(bucket.bucketDate, report.buckets.size());
        report.buckets.push_back(bucket);
    }

    const QVector<domain::Order> orders = m_orderService.findOrders();
    for (const domain::Order &order : orders) {
        if (!countsAsSales(order.status)) {
            continue;
        }

        const QDate orderDate = QDate::fromString(order.orderDate, Qt::ISODate);
        if (!orderDate.isValid() || orderDate < startDate || orderDate > endDate) {
            continue;
        }

        const QString bucketKey = orderDate.toString(Qt::ISODate);
        const qsizetype bucketIndex = bucketByDate.value(bucketKey, -1);
        if (bucketIndex >= 0) {
            SalesSummaryBucket &bucket = report.buckets[bucketIndex];
            ++bucket.orderCount;
            bucket.salesVnd += order.totalVnd;
        }

        ++report.orderCount;
        report.totalSalesVnd += order.totalVnd;
        report.outstandingVnd += order.balanceVnd;
    }

    const QVector<domain::Payment> payments = m_paymentService.findAllPayments();
    for (const domain::Payment &payment : payments) {
        const QDate paidDate = QDate::fromString(payment.paidAt, Qt::ISODate);
        if (!paidDate.isValid() || paidDate < startDate || paidDate > endDate) {
            continue;
        }

        const QString bucketKey = paidDate.toString(Qt::ISODate);
        const qsizetype bucketIndex = bucketByDate.value(bucketKey, -1);
        if (bucketIndex >= 0) {
            report.buckets[bucketIndex].collectedVnd += payment.amountVnd;
        }

        report.totalCollectedVnd += payment.amountVnd;
    }

    errorMessage.clear();
    return true;
}

DebtAgingReport ReportService::buildDebtAgingReport() const
{
    const QVector<domain::Customer> customers = m_customerService.findCustomers();
    const QVector<domain::Order> orders = m_orderService.findOrders();
    const QDate today = QDate::currentDate();

    QHash<QString, domain::Customer> customersById;
    customersById.reserve(customers.size());
    for (const domain::Customer &customer : customers) {
        customersById.insert(customer.id, customer);
    }

    QHash<QString, DebtAgingRow> rowsByCustomer;
    DebtAgingReport report;

    for (const domain::Order &order : orders) {
        if ((order.status != domain::OrderStatus::Confirmed
             && order.status != domain::OrderStatus::PartiallyPaid)
            || order.balanceVnd <= 0) {
            continue;
        }

        const QDate orderDate = QDate::fromString(order.orderDate, Qt::ISODate);
        const int ageDays = orderDate.isValid() ? orderDate.daysTo(today) : 0;

        DebtAgingRow row = rowsByCustomer.value(order.customerId);
        row.customerId = order.customerId;

        const domain::Customer customer = customersById.value(order.customerId);
        row.code = customer.code;
        row.name = customer.name.isEmpty() ? order.customerId : customer.name;

        if (ageDays <= 30) {
            row.currentBucketVnd += order.balanceVnd;
            report.currentBucketVnd += order.balanceVnd;
        } else if (ageDays <= 60) {
            row.bucket31To60Vnd += order.balanceVnd;
            report.bucket31To60Vnd += order.balanceVnd;
        } else {
            row.bucket61PlusVnd += order.balanceVnd;
            report.bucket61PlusVnd += order.balanceVnd;
        }

        row.totalVnd += order.balanceVnd;
        report.totalVnd += order.balanceVnd;
        rowsByCustomer.insert(order.customerId, row);
    }

    report.rows.reserve(rowsByCustomer.size());
    for (auto it = rowsByCustomer.cbegin(); it != rowsByCustomer.cend(); ++it) {
        report.rows.push_back(it.value());
    }

    std::sort(report.rows.begin(), report.rows.end(), [](const DebtAgingRow &lhs, const DebtAgingRow &rhs) {
        if (lhs.totalVnd == rhs.totalVnd) {
            return lhs.name < rhs.name;
        }

        return lhs.totalVnd > rhs.totalVnd;
    });

    return report;
}

bool ReportService::buildInventoryMovementReport(const QString &keyword,
                                                 const QString &fromDate,
                                                 const QString &toDate,
                                                 QVector<InventoryMovementReportRow> &rows,
                                                 QString &errorMessage) const
{
    QDate startDate;
    QDate endDate;
    if (!parseDateRange(fromDate, toDate, startDate, endDate, errorMessage)) {
        return false;
    }

    rows.clear();

    const QString normalizedKeyword = keyword.trimmed();
    const QVector<domain::Product> products = m_productService.findProducts();
    QHash<QString, domain::Product> productsById;
    productsById.reserve(products.size());
    for (const domain::Product &product : products) {
        productsById.insert(product.id, product);
    }

    const QVector<domain::InventoryMovement> movements = m_productService.findAllInventoryMovements();
    rows.reserve(movements.size());

    for (const domain::InventoryMovement &movement : movements) {
        const QDate movementDate = QDate::fromString(movement.movementDate, Qt::ISODate);
        if (!movementDate.isValid() || movementDate < startDate || movementDate > endDate) {
            continue;
        }

        const domain::Product product = productsById.value(movement.productId);
        const QString productName = product.name.isEmpty() ? movement.productId : product.name;

        const bool matched = normalizedKeyword.isEmpty()
                             || productName.contains(normalizedKeyword, Qt::CaseInsensitive)
                             || product.sku.contains(normalizedKeyword, Qt::CaseInsensitive)
                             || movement.lotNo.contains(normalizedKeyword, Qt::CaseInsensitive)
                             || movement.reason.contains(normalizedKeyword, Qt::CaseInsensitive);
        if (!matched) {
            continue;
        }

        rows.push_back(InventoryMovementReportRow{
            movement.id,
            movement.movementDate,
            movement.productId,
            product.sku,
            productName,
            movement.lotNo,
            movementTypeLabel(movement.movementType),
            movement.reason,
            movement.qtyDelta,
            movement.lotBalanceAfter,
        });
    }

    errorMessage.clear();
    return true;
}

bool ReportService::exportSalesSummaryCsv(const SalesSummaryReport &report,
                                          const QString &filePath,
                                          QString &errorMessage) const
{
    QStringList lines;
    lines << QStringLiteral("Sales Summary Theo Ky");
    lines << QStringLiteral("Tu ngay,%1").arg(csvCell(report.fromDate));
    lines << QStringLiteral("Den ngay,%1").arg(csvCell(report.toDate));
    lines << QStringLiteral("So don,%1").arg(report.orderCount);
    lines << QStringLiteral("Tong doanh so,%1").arg(report.totalSalesVnd);
    lines << QStringLiteral("Tong thu tien,%1").arg(report.totalCollectedVnd);
    lines << QStringLiteral("Cong no con lai,%1").arg(report.outstandingVnd);
    lines << QString();
    lines << QStringLiteral("Ngay,So don,Doanh so,Thu tien");

    for (const SalesSummaryBucket &bucket : report.buckets) {
        lines << QStringLiteral("%1,%2,%3,%4")
                     .arg(csvCell(bucket.bucketDate))
                     .arg(bucket.orderCount)
                     .arg(bucket.salesVnd)
                     .arg(bucket.collectedVnd);
    }

    return writeTextFile(filePath, lines, errorMessage);
}

bool ReportService::exportSalesSummaryPdf(const SalesSummaryReport &report,
                                          const QString &filePath,
                                          QString &errorMessage) const
{
    QStringList lines;
    lines << QStringLiteral("Ky bao cao: %1 -> %2").arg(report.fromDate, report.toDate);
    lines << QStringLiteral("So don: %1").arg(report.orderCount);
    lines << QStringLiteral("Tong doanh so: %1 VND").arg(formatMoney(report.totalSalesVnd));
    lines << QStringLiteral("Tong thu tien: %1 VND").arg(formatMoney(report.totalCollectedVnd));
    lines << QStringLiteral("Cong no con lai: %1 VND").arg(formatMoney(report.outstandingVnd));
    lines << QString();
    lines << QStringLiteral("Chi tiet theo ngay:");

    for (const SalesSummaryBucket &bucket : report.buckets) {
        lines << QStringLiteral("%1 | don: %2 | doanh so: %3 | thu: %4")
                     .arg(bucket.bucketDate)
                     .arg(bucket.orderCount)
                     .arg(formatMoney(bucket.salesVnd))
                     .arg(formatMoney(bucket.collectedVnd));
    }

    return writePdfFile(filePath, QStringLiteral("Sales Summary Theo Ky"), lines, errorMessage);
}

bool ReportService::exportDebtAgingCsv(const DebtAgingReport &report,
                                       const QString &filePath,
                                       QString &errorMessage) const
{
    QStringList lines;
    lines << QStringLiteral("Debt Aging Report");
    lines << QStringLiteral("0-30 ngay,%1").arg(report.currentBucketVnd);
    lines << QStringLiteral("31-60 ngay,%1").arg(report.bucket31To60Vnd);
    lines << QStringLiteral(">60 ngay,%1").arg(report.bucket61PlusVnd);
    lines << QStringLiteral("Tong,%1").arg(report.totalVnd);
    lines << QString();
    lines << QStringLiteral("Ma khach,Ten khach,0-30 ngay,31-60 ngay,>60 ngay,Tong");

    for (const DebtAgingRow &row : report.rows) {
        lines << QStringLiteral("%1,%2,%3,%4,%5,%6")
                     .arg(csvCell(row.code))
                     .arg(csvCell(row.name))
                     .arg(row.currentBucketVnd)
                     .arg(row.bucket31To60Vnd)
                     .arg(row.bucket61PlusVnd)
                     .arg(row.totalVnd);
    }

    return writeTextFile(filePath, lines, errorMessage);
}

bool ReportService::exportDebtAgingPdf(const DebtAgingReport &report,
                                       const QString &filePath,
                                       QString &errorMessage) const
{
    QStringList lines;
    lines << QStringLiteral("0-30 ngay: %1 VND").arg(formatMoney(report.currentBucketVnd));
    lines << QStringLiteral("31-60 ngay: %1 VND").arg(formatMoney(report.bucket31To60Vnd));
    lines << QStringLiteral(">60 ngay: %1 VND").arg(formatMoney(report.bucket61PlusVnd));
    lines << QStringLiteral("Tong cong no: %1 VND").arg(formatMoney(report.totalVnd));
    lines << QString();
    lines << QStringLiteral("Chi tiet theo khach:");

    for (const DebtAgingRow &row : report.rows) {
        lines << QStringLiteral("%1 - %2 | 0-30: %3 | 31-60: %4 | >60: %5 | Tong: %6")
                     .arg(row.code)
                     .arg(row.name)
                     .arg(formatMoney(row.currentBucketVnd))
                     .arg(formatMoney(row.bucket31To60Vnd))
                     .arg(formatMoney(row.bucket61PlusVnd))
                     .arg(formatMoney(row.totalVnd));
    }

    return writePdfFile(filePath, QStringLiteral("Debt Aging Report"), lines, errorMessage);
}

bool ReportService::exportInventoryMovementCsv(const QVector<InventoryMovementReportRow> &rows,
                                               const QString &filePath,
                                               QString &errorMessage) const
{
    QStringList lines;
    lines << QStringLiteral("Inventory Movement");
    lines << QStringLiteral("Ngay,SKU,San pham,Lo,Loai,Qty delta,Ton sau,Ly do");

    for (const InventoryMovementReportRow &row : rows) {
        lines << QStringLiteral("%1,%2,%3,%4,%5,%6,%7,%8")
                     .arg(csvCell(row.movementDate))
                     .arg(csvCell(row.sku))
                     .arg(csvCell(row.productName))
                     .arg(csvCell(row.lotNo))
                     .arg(csvCell(row.movementType))
                     .arg(row.qtyDelta)
                     .arg(row.lotBalanceAfter)
                     .arg(csvCell(row.reason));
    }

    return writeTextFile(filePath, lines, errorMessage);
}

bool ReportService::exportInventoryMovementPdf(const QVector<InventoryMovementReportRow> &rows,
                                               const QString &filePath,
                                               QString &errorMessage) const
{
    QStringList lines;
    lines << QStringLiteral("Tong dong bien dong: %1").arg(rows.size());
    lines << QString();

    for (const InventoryMovementReportRow &row : rows) {
        const QString qtyDeltaText = (row.qtyDelta > 0 ? QStringLiteral("+") : QString())
                                     + QString::number(row.qtyDelta);

        lines << QStringLiteral("%1 | %2 - %3 | %4 | %5 | %6 | Ton: %7 | %8")
                     .arg(row.movementDate)
                     .arg(row.sku)
                     .arg(row.productName)
                     .arg(row.lotNo)
                     .arg(row.movementType)
                     .arg(qtyDeltaText)
                     .arg(row.lotBalanceAfter)
                     .arg(row.reason);
    }

    return writePdfFile(filePath, QStringLiteral("Inventory Movement"), lines, errorMessage);
}

bool ReportService::parseDateRange(const QString &fromDate,
                                   const QString &toDate,
                                   QDate &startDate,
                                   QDate &endDate,
                                   QString &errorMessage)
{
    QString normalizedFrom = fromDate.trimmed();
    QString normalizedTo = toDate.trimmed();

    if (normalizedFrom.isEmpty() && normalizedTo.isEmpty()) {
        endDate = QDate::currentDate();
        startDate = QDate(endDate.year(), endDate.month(), 1);
        errorMessage.clear();
        return true;
    }

    if (normalizedFrom.isEmpty()) {
        normalizedFrom = normalizedTo;
    }

    if (normalizedTo.isEmpty()) {
        normalizedTo = normalizedFrom;
    }

    startDate = QDate::fromString(normalizedFrom, Qt::ISODate);
    endDate = QDate::fromString(normalizedTo, Qt::ISODate);

    if (!startDate.isValid() || !endDate.isValid()) {
        errorMessage = QStringLiteral("Ngày lọc không hợp lệ. Dùng định dạng YYYY-MM-DD.");
        return false;
    }

    if (startDate > endDate) {
        errorMessage = QStringLiteral("Khoảng ngày không hợp lệ: từ ngày phải <= đến ngày.");
        return false;
    }

    errorMessage.clear();
    return true;
}

bool ReportService::countsAsSales(domain::OrderStatus status)
{
    return status == domain::OrderStatus::Confirmed
           || status == domain::OrderStatus::PartiallyPaid
           || status == domain::OrderStatus::Paid;
}

QString ReportService::movementTypeLabel(const QString &movementType)
{
    if (movementType == QLatin1String("LotCreated")) {
        return QStringLiteral("Tạo lô");
    }
    if (movementType == QLatin1String("StockIn")) {
        return QStringLiteral("Nhập kho");
    }
    if (movementType == QLatin1String("StockOut")) {
        return QStringLiteral("Xuất kho");
    }
    if (movementType == QLatin1String("AdjustUp")) {
        return QStringLiteral("Điều chỉnh +");
    }
    if (movementType == QLatin1String("AdjustDown")) {
        return QStringLiteral("Điều chỉnh -");
    }

    return movementType;
}

bool ReportService::writeTextFile(const QString &filePath,
                                  const QStringList &lines,
                                  QString &errorMessage)
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        errorMessage = QStringLiteral("Không thể ghi file export: %1").arg(file.errorString());
        return false;
    }

    QTextStream out(&file);
    for (const QString &line : lines) {
        out << line << '\n';
    }

    file.close();
    errorMessage.clear();
    return true;
}

bool ReportService::writePdfFile(const QString &filePath,
                                 const QString &title,
                                 const QStringList &lines,
                                 QString &errorMessage)
{
    QPdfWriter writer(filePath);
    writer.setPageSize(QPageSize(QPageSize::A4));
    writer.setResolution(120);
    writer.setTitle(title);

    QPainter painter(&writer);
    if (!painter.isActive()) {
        errorMessage = QStringLiteral("Không thể tạo file PDF.");
        return false;
    }

    QFont titleFont = painter.font();
    titleFont.setPixelSize(18);
    titleFont.setBold(true);

    QFont bodyFont = painter.font();
    bodyFont.setPixelSize(11);
    bodyFont.setBold(false);

    const int left = 72;
    const int top = 72;
    const int lineHeight = 22;
    const int bottomLimit = writer.height() - 72;
    int y = top;

    painter.setFont(titleFont);
    painter.drawText(left, y, title);
    y += 36;

    painter.setFont(bodyFont);
    for (const QString &line : lines) {
        if (y > bottomLimit) {
            writer.newPage();
            y = top;
            painter.setFont(bodyFont);
        }

        painter.drawText(left, y, line);
        y += lineHeight;
    }

    painter.end();
    errorMessage.clear();
    return true;
}

QString ReportService::csvCell(const QString &value)
{
    QString escaped = value;
    escaped.replace(QLatin1Char('"'), QStringLiteral("\"\""));
    return QStringLiteral("\"%1\"").arg(escaped);
}

} // namespace stockmaster::application
