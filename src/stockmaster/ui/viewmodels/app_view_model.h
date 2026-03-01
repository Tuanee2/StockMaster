#pragma once

#include <QObject>
#include <QVariantList>
#include <QString>
#include <QStringList>

#include "stockmaster/domain/entities.h"

namespace stockmaster::application {
class CustomerService;
class OrderService;
class PaymentService;
class ProductService;
}

namespace stockmaster::infra::db {
class DatabaseService;
}

namespace stockmaster::ui::viewmodels {

class AppViewModel : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString appTitle READ appTitle CONSTANT)
    Q_PROPERTY(QStringList sections READ sections CONSTANT)
    Q_PROPERTY(QString currentSection READ currentSection WRITE setCurrentSection NOTIFY currentSectionChanged)
    Q_PROPERTY(bool databaseReady READ databaseReady NOTIFY databaseReadyChanged)
    Q_PROPERTY(QString databaseBackend READ databaseBackend CONSTANT)
    Q_PROPERTY(int customerCount READ customerCount NOTIFY metricsChanged)
    Q_PROPERTY(int productCount READ productCount NOTIFY metricsChanged)
    Q_PROPERTY(int openOrderCount READ openOrderCount NOTIFY metricsChanged)
    Q_PROPERTY(int lowStockCount READ lowStockCount NOTIFY metricsChanged)
    Q_PROPERTY(int expiringSoonProductCount READ expiringSoonProductCount NOTIFY metricsChanged)
    Q_PROPERTY(QString receivableVndText READ receivableVndText NOTIFY metricsChanged)
    Q_PROPERTY(QVariantList operationalAlerts READ operationalAlerts NOTIFY dashboardDataChanged)
    Q_PROPERTY(QVariantList monthlyTrends READ monthlyTrends NOTIFY dashboardDataChanged)
    Q_PROPERTY(qint64 monthlyTrendPeakVnd READ monthlyTrendPeakVnd NOTIFY dashboardDataChanged)
    Q_PROPERTY(QVariantList topCustomers READ topCustomers NOTIFY dashboardDataChanged)
    Q_PROPERTY(QVariantList topProducts READ topProducts NOTIFY dashboardDataChanged)
    Q_PROPERTY(bool hasMonthlyActivity READ hasMonthlyActivity NOTIFY dashboardDataChanged)

public:
    AppViewModel(infra::db::DatabaseService &databaseService,
                 application::OrderService &orderService,
                 application::PaymentService &paymentService,
                 const application::CustomerService &customerService,
                 const application::ProductService &productService,
                 QObject *parent = nullptr);

    [[nodiscard]] QString appTitle() const;
    [[nodiscard]] QStringList sections() const;

    [[nodiscard]] QString currentSection() const;
    void setCurrentSection(const QString &value);

    [[nodiscard]] bool databaseReady() const;
    [[nodiscard]] QString databaseBackend() const;

    [[nodiscard]] int customerCount() const;
    [[nodiscard]] int productCount() const;
    [[nodiscard]] int openOrderCount() const;
    [[nodiscard]] int lowStockCount() const;
    [[nodiscard]] int expiringSoonProductCount() const;
    [[nodiscard]] QString receivableVndText() const;
    [[nodiscard]] QVariantList operationalAlerts() const;
    [[nodiscard]] QVariantList monthlyTrends() const;
    [[nodiscard]] qint64 monthlyTrendPeakVnd() const;
    [[nodiscard]] QVariantList topCustomers() const;
    [[nodiscard]] QVariantList topProducts() const;
    [[nodiscard]] bool hasMonthlyActivity() const;

    Q_INVOKABLE void refreshOverview();
    Q_INVOKABLE QString formatMoneyVnd(qint64 amountVnd) const;

signals:
    void currentSectionChanged();
    void databaseReadyChanged();
    void metricsChanged();
    void dashboardDataChanged();

private:
    static QString formatMoney(core::Money amountVnd);
    static bool countsAsRevenue(domain::OrderStatus status);

    infra::db::DatabaseService &m_databaseService;
    application::OrderService &m_orderService;
    application::PaymentService &m_paymentService;
    const application::CustomerService &m_customerService;
    const application::ProductService &m_productService;

    QString m_currentSection;
    int m_customerCount = 0;
    int m_productCount = 0;
    int m_openOrderCount = 0;
    int m_lowStockCount = 0;
    int m_expiringSoonProductCount = 0;
    core::Money m_receivableVnd = 0;
    QVariantList m_operationalAlerts;
    QVariantList m_monthlyTrends;
    qint64 m_monthlyTrendPeakVnd = 0;
    QVariantList m_topCustomers;
    QVariantList m_topProducts;
    bool m_hasMonthlyActivity = false;
};

} // namespace stockmaster::ui::viewmodels
