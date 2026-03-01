#include "stockmaster/ui/viewmodels/app_view_model.h"

#include <QDate>
#include <QHash>
#include <QLocale>
#include <QVariantMap>

#include <algorithm>

#include "stockmaster/application/customer_service.h"
#include "stockmaster/application/order_service.h"
#include "stockmaster/application/payment_service.h"
#include "stockmaster/application/product_service.h"
#include "stockmaster/domain/entities.h"
#include "stockmaster/infra/db/database_service.h"

namespace stockmaster::ui::viewmodels {

namespace {

const QStringList kSections = {
    QStringLiteral("Dashboard"),
    QStringLiteral("Customers"),
    QStringLiteral("Products"),
    QStringLiteral("Orders"),
    QStringLiteral("Inventory"),
    QStringLiteral("Payments"),
    QStringLiteral("Reports"),
    QStringLiteral("Settings"),
};

}

AppViewModel::AppViewModel(infra::db::DatabaseService &databaseService,
                           application::OrderService &orderService,
                           application::PaymentService &paymentService,
                           const application::CustomerService &customerService,
                           const application::ProductService &productService,
                           QObject *parent)
    : QObject(parent)
    , m_databaseService(databaseService)
    , m_orderService(orderService)
    , m_paymentService(paymentService)
    , m_customerService(customerService)
    , m_productService(productService)
    , m_currentSection(kSections.first())
{
    refreshOverview();
}

QString AppViewModel::appTitle() const
{
    return QStringLiteral("StockMaster");
}

QStringList AppViewModel::sections() const
{
    return kSections;
}

QString AppViewModel::currentSection() const
{
    return m_currentSection;
}

void AppViewModel::setCurrentSection(const QString &value)
{
    if (m_currentSection == value || !kSections.contains(value)) {
        return;
    }

    m_currentSection = value;
    emit currentSectionChanged();
}

bool AppViewModel::databaseReady() const
{
    return m_databaseService.isReady();
}

QString AppViewModel::databaseBackend() const
{
    return m_databaseService.backendName();
}

int AppViewModel::customerCount() const
{
    return m_customerCount;
}

int AppViewModel::productCount() const
{
    return m_productCount;
}

int AppViewModel::openOrderCount() const
{
    return m_openOrderCount;
}

int AppViewModel::lowStockCount() const
{
    return m_lowStockCount;
}

int AppViewModel::expiringSoonProductCount() const
{
    return m_expiringSoonProductCount;
}

QString AppViewModel::receivableVndText() const
{
    return formatMoney(m_receivableVnd);
}

QVariantList AppViewModel::operationalAlerts() const
{
    return m_operationalAlerts;
}

QVariantList AppViewModel::monthlyTrends() const
{
    return m_monthlyTrends;
}

qint64 AppViewModel::monthlyTrendPeakVnd() const
{
    return m_monthlyTrendPeakVnd;
}

QVariantList AppViewModel::topCustomers() const
{
    return m_topCustomers;
}

QVariantList AppViewModel::topProducts() const
{
    return m_topProducts;
}

bool AppViewModel::hasMonthlyActivity() const
{
    return m_hasMonthlyActivity;
}

void AppViewModel::refreshOverview()
{
    const domain::DashboardMetrics metrics = m_orderService.loadDashboardMetrics();
    const QVector<domain::Order> orders = m_orderService.findOrders();
    const QVector<domain::Payment> payments = m_paymentService.findAllPayments();
    const QVector<domain::Customer> customers = m_customerService.findCustomers();
    const QVector<domain::Product> products = m_productService.findProducts();
    const QVector<application::CustomerReceivableSummary> customerReceivables = m_paymentService.findCustomerReceivables();

    m_customerCount = metrics.customerCount;
    m_productCount = metrics.productCount;
    m_openOrderCount = metrics.openOrderCount;
    m_lowStockCount = metrics.lowStockCount;
    m_expiringSoonProductCount = metrics.expiringSoonProductCount;
    m_receivableVnd = metrics.receivableVnd;

    QHash<QString, domain::Customer> customersById;
    customersById.reserve(customers.size());
    for (const domain::Customer &customer : customers) {
        customersById.insert(customer.id, customer);
    }

    QHash<QString, domain::Product> productsById;
    productsById.reserve(products.size());
    for (const domain::Product &product : products) {
        productsById.insert(product.id, product);
    }

    QHash<QString, core::Money> receivableByCustomer;
    for (const application::CustomerReceivableSummary &summary : customerReceivables) {
        receivableByCustomer.insert(summary.customerId, summary.receivableVnd);
    }

    constexpr int kMonthCount = 6;
    const QDate currentMonth = QDate::currentDate();
    const QDate firstVisibleMonth(currentMonth.year(), currentMonth.month(), 1);
    const QDate baseMonth = firstVisibleMonth.addMonths(-(kMonthCount - 1));

    QVector<core::Money> salesByMonth(kMonthCount, 0);
    QVector<core::Money> collectedByMonth(kMonthCount, 0);

    struct CustomerSalesRow {
        QString customerId;
        core::Money salesVnd = 0;
        int orderCount = 0;
    };
    QHash<QString, CustomerSalesRow> customerSalesById;

    struct ProductSalesRow {
        QString productId;
        core::Money salesVnd = 0;
        int soldQty = 0;
    };
    QHash<QString, ProductSalesRow> productSalesById;

    auto monthIndexForDate = [baseMonth, kMonthCount](const QDate &date) {
        if (!date.isValid()) {
            return -1;
        }

        const QDate monthStart(date.year(), date.month(), 1);
        const int delta = (monthStart.year() - baseMonth.year()) * 12
                          + (monthStart.month() - baseMonth.month());
        if (delta < 0 || delta >= kMonthCount) {
            return -1;
        }

        return delta;
    };

    for (const domain::Order &order : orders) {
        if (!countsAsRevenue(order.status)) {
            continue;
        }

        const QDate orderDate = QDate::fromString(order.orderDate, Qt::ISODate);
        const int monthIndex = monthIndexForDate(orderDate);
        if (monthIndex >= 0) {
            salesByMonth[monthIndex] += order.totalVnd;
        }

        CustomerSalesRow customerRow = customerSalesById.value(order.customerId);
        customerRow.customerId = order.customerId;
        customerRow.salesVnd += order.totalVnd;
        ++customerRow.orderCount;
        customerSalesById.insert(order.customerId, customerRow);

        const QVector<domain::OrderItem> items = m_orderService.findOrderItems(order.id);
        for (const domain::OrderItem &item : items) {
            ProductSalesRow productRow = productSalesById.value(item.productId);
            productRow.productId = item.productId;
            productRow.salesVnd += item.lineTotalVnd;
            productRow.soldQty += item.qty;
            productSalesById.insert(item.productId, productRow);
        }
    }

    for (const domain::Payment &payment : payments) {
        const QDate paidDate = QDate::fromString(payment.paidAt, Qt::ISODate);
        const int monthIndex = monthIndexForDate(paidDate);
        if (monthIndex >= 0) {
            collectedByMonth[monthIndex] += payment.amountVnd;
        }
    }

    QVariantList monthlyTrends;
    monthlyTrends.reserve(kMonthCount);
    m_monthlyTrendPeakVnd = 0;
    m_hasMonthlyActivity = false;

    for (int index = 0; index < kMonthCount; ++index) {
        const QDate monthDate = baseMonth.addMonths(index);
        const core::Money salesVnd = salesByMonth[index];
        const core::Money collectedVnd = collectedByMonth[index];

        m_monthlyTrendPeakVnd = std::max<qint64>(m_monthlyTrendPeakVnd,
                                                 std::max<core::Money>(salesVnd, collectedVnd));
        m_hasMonthlyActivity = m_hasMonthlyActivity || salesVnd > 0 || collectedVnd > 0;

        monthlyTrends.push_back(QVariantMap{
            {QStringLiteral("monthKey"), monthDate.toString(QStringLiteral("yyyy-MM"))},
            {QStringLiteral("monthLabel"), monthDate.toString(QStringLiteral("MM/yy"))},
            {QStringLiteral("salesVnd"), salesVnd},
            {QStringLiteral("salesText"), formatMoney(salesVnd)},
            {QStringLiteral("collectedVnd"), collectedVnd},
            {QStringLiteral("collectedText"), formatMoney(collectedVnd)},
        });
    }
    m_monthlyTrends = monthlyTrends;

    QVector<CustomerSalesRow> rankedCustomers;
    rankedCustomers.reserve(customerSalesById.size());
    for (auto it = customerSalesById.cbegin(); it != customerSalesById.cend(); ++it) {
        rankedCustomers.push_back(it.value());
    }
    std::sort(rankedCustomers.begin(), rankedCustomers.end(), [&customersById](const CustomerSalesRow &lhs,
                                                                               const CustomerSalesRow &rhs) {
        if (lhs.salesVnd == rhs.salesVnd) {
            const QString lhsName = customersById.value(lhs.customerId).name;
            const QString rhsName = customersById.value(rhs.customerId).name;
            return lhsName < rhsName;
        }

        return lhs.salesVnd > rhs.salesVnd;
    });

    QVariantList topCustomers;
    for (qsizetype index = 0; index < rankedCustomers.size() && index < 5; ++index) {
        const CustomerSalesRow &row = rankedCustomers.at(index);
        const domain::Customer customer = customersById.value(row.customerId);
        topCustomers.push_back(QVariantMap{
            {QStringLiteral("customerId"), row.customerId},
            {QStringLiteral("code"), customer.code},
            {QStringLiteral("name"), customer.name.isEmpty() ? row.customerId : customer.name},
            {QStringLiteral("orderCount"), row.orderCount},
            {QStringLiteral("salesVnd"), row.salesVnd},
            {QStringLiteral("salesText"), formatMoney(row.salesVnd)},
            {QStringLiteral("receivableVnd"), receivableByCustomer.value(row.customerId)},
            {QStringLiteral("receivableText"), formatMoney(receivableByCustomer.value(row.customerId))},
        });
    }
    m_topCustomers = topCustomers;

    QVector<ProductSalesRow> rankedProducts;
    rankedProducts.reserve(productSalesById.size());
    for (auto it = productSalesById.cbegin(); it != productSalesById.cend(); ++it) {
        rankedProducts.push_back(it.value());
    }
    std::sort(rankedProducts.begin(), rankedProducts.end(), [&productsById](const ProductSalesRow &lhs,
                                                                            const ProductSalesRow &rhs) {
        if (lhs.salesVnd == rhs.salesVnd) {
            if (lhs.soldQty == rhs.soldQty) {
                const QString lhsName = productsById.value(lhs.productId).name;
                const QString rhsName = productsById.value(rhs.productId).name;
                return lhsName < rhsName;
            }

            return lhs.soldQty > rhs.soldQty;
        }

        return lhs.salesVnd > rhs.salesVnd;
    });

    QVariantList topProducts;
    for (qsizetype index = 0; index < rankedProducts.size() && index < 5; ++index) {
        const ProductSalesRow &row = rankedProducts.at(index);
        const domain::Product product = productsById.value(row.productId);
        topProducts.push_back(QVariantMap{
            {QStringLiteral("productId"), row.productId},
            {QStringLiteral("sku"), product.sku},
            {QStringLiteral("name"), product.name.isEmpty() ? row.productId : product.name},
            {QStringLiteral("soldQty"), row.soldQty},
            {QStringLiteral("salesVnd"), row.salesVnd},
            {QStringLiteral("salesText"), formatMoney(row.salesVnd)},
        });
    }
    m_topProducts = topProducts;

    QVariantList operationalAlerts;

    int receivableAlerts = 0;
    for (const application::CustomerReceivableSummary &summary : customerReceivables) {
        if (summary.receivableVnd <= 0 || receivableAlerts >= 2) {
            continue;
        }

        const domain::Customer customer = customersById.value(summary.customerId);
        const bool overLimit = customer.creditLimitVnd > 0 && summary.receivableVnd >= customer.creditLimitVnd;
        operationalAlerts.push_back(QVariantMap{
            {QStringLiteral("badge"), QStringLiteral("Công nợ")},
            {QStringLiteral("severity"), overLimit ? QStringLiteral("danger") : QStringLiteral("warning")},
            {QStringLiteral("title"), summary.name},
            {QStringLiteral("detail"),
             QStringLiteral("%1 | %2 đơn chờ thu")
                 .arg(formatMoney(summary.receivableVnd))
                 .arg(summary.openOrderCount)},
        });
        ++receivableAlerts;
    }

    struct LowStockRow {
        QString productId;
        int onHandQty = 0;
    };
    QVector<LowStockRow> lowStockRows;
    for (const domain::Product &product : products) {
        if (!product.isActive) {
            continue;
        }

        const int onHandQty = m_productService.totalOnHandByProduct(product.id);
        if (onHandQty <= 20) {
            lowStockRows.push_back(LowStockRow{product.id, onHandQty});
        }
    }

    std::sort(lowStockRows.begin(), lowStockRows.end(), [&productsById](const LowStockRow &lhs, const LowStockRow &rhs) {
        if (lhs.onHandQty == rhs.onHandQty) {
            return productsById.value(lhs.productId).name < productsById.value(rhs.productId).name;
        }

        return lhs.onHandQty < rhs.onHandQty;
    });

    for (qsizetype index = 0; index < lowStockRows.size() && index < 2; ++index) {
        const LowStockRow &row = lowStockRows.at(index);
        const domain::Product product = productsById.value(row.productId);
        operationalAlerts.push_back(QVariantMap{
            {QStringLiteral("badge"), QStringLiteral("Tồn thấp")},
            {QStringLiteral("severity"), row.onHandQty <= 5 ? QStringLiteral("danger") : QStringLiteral("warning")},
            {QStringLiteral("title"), product.sku + QStringLiteral(" - ") + product.name},
            {QStringLiteral("detail"),
             QStringLiteral("Tồn còn %1 %2").arg(row.onHandQty).arg(product.unit)},
        });
    }

    struct ExpiringLotRow {
        QString productId;
        QString lotNo;
        int daysToExpiry = 0;
        int onHandQty = 0;
    };
    QVector<ExpiringLotRow> expiringLots;
    const QDate today = QDate::currentDate();

    for (const domain::Product &product : products) {
        if (!product.isActive) {
            continue;
        }

        const QVector<domain::ProductLot> lots = m_productService.findLotsByProduct(product.id);
        for (const domain::ProductLot &lot : lots) {
            if (lot.onHandQty <= 0) {
                continue;
            }

            const QDate expiryDate = QDate::fromString(lot.expiryDate, Qt::ISODate);
            if (!expiryDate.isValid()) {
                continue;
            }

            const int daysToExpiry = today.daysTo(expiryDate);
            if (daysToExpiry < 0 || daysToExpiry > 30) {
                continue;
            }

            expiringLots.push_back(ExpiringLotRow{product.id, lot.lotNo, daysToExpiry, lot.onHandQty});
        }
    }

    std::sort(expiringLots.begin(), expiringLots.end(), [&productsById](const ExpiringLotRow &lhs, const ExpiringLotRow &rhs) {
        if (lhs.daysToExpiry == rhs.daysToExpiry) {
            return productsById.value(lhs.productId).name < productsById.value(rhs.productId).name;
        }

        return lhs.daysToExpiry < rhs.daysToExpiry;
    });

    for (qsizetype index = 0; index < expiringLots.size() && index < 2; ++index) {
        const ExpiringLotRow &row = expiringLots.at(index);
        const domain::Product product = productsById.value(row.productId);
        const QString detail = row.daysToExpiry == 0
            ? QStringLiteral("Hết hạn hôm nay | Tồn lô: %1").arg(row.onHandQty)
            : QStringLiteral("HSD còn %1 ngày | Tồn lô: %2").arg(row.daysToExpiry).arg(row.onHandQty);

        operationalAlerts.push_back(QVariantMap{
            {QStringLiteral("badge"), QStringLiteral("Hạn dùng")},
            {QStringLiteral("severity"), row.daysToExpiry <= 7 ? QStringLiteral("danger") : QStringLiteral("warning")},
            {QStringLiteral("title"), product.name + QStringLiteral(" | ") + row.lotNo},
            {QStringLiteral("detail"), detail},
        });
    }

    m_operationalAlerts = operationalAlerts;

    emit databaseReadyChanged();
    emit metricsChanged();
    emit dashboardDataChanged();
}

QString AppViewModel::formatMoneyVnd(qint64 amountVnd) const
{
    return formatMoney(amountVnd);
}

QString AppViewModel::formatMoney(core::Money amountVnd)
{
    const QLocale locale(QLocale::Vietnamese, QLocale::Vietnam);
    return locale.toString(amountVnd) + QStringLiteral(" VND");
}

bool AppViewModel::countsAsRevenue(domain::OrderStatus status)
{
    return status == domain::OrderStatus::Confirmed
           || status == domain::OrderStatus::PartiallyPaid
           || status == domain::OrderStatus::Paid;
}

} // namespace stockmaster::ui::viewmodels
