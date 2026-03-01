#pragma once

#include <QObject>
#include <QVariantList>

#include "stockmaster/application/report_service.h"

namespace stockmaster::ui::viewmodels {

class ReportsViewModel : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged)
    Q_PROPERTY(QString lastExportPath READ lastExportPath NOTIFY lastExportPathChanged)

    Q_PROPERTY(QString salesFromDate READ salesFromDate NOTIFY salesChanged)
    Q_PROPERTY(QString salesToDate READ salesToDate NOTIFY salesChanged)
    Q_PROPERTY(int salesOrderCount READ salesOrderCount NOTIFY salesChanged)
    Q_PROPERTY(QString salesTotalVndText READ salesTotalVndText NOTIFY salesChanged)
    Q_PROPERTY(QString salesCollectedVndText READ salesCollectedVndText NOTIFY salesChanged)
    Q_PROPERTY(QString salesOutstandingVndText READ salesOutstandingVndText NOTIFY salesChanged)
    Q_PROPERTY(QVariantList salesBuckets READ salesBuckets NOTIFY salesChanged)

    Q_PROPERTY(QString debtCurrentVndText READ debtCurrentVndText NOTIFY debtChanged)
    Q_PROPERTY(QString debt31To60VndText READ debt31To60VndText NOTIFY debtChanged)
    Q_PROPERTY(QString debt61PlusVndText READ debt61PlusVndText NOTIFY debtChanged)
    Q_PROPERTY(QString debtTotalVndText READ debtTotalVndText NOTIFY debtChanged)
    Q_PROPERTY(QVariantList debtRows READ debtRows NOTIFY debtChanged)

    Q_PROPERTY(QString movementFromDate READ movementFromDate NOTIFY movementChanged)
    Q_PROPERTY(QString movementToDate READ movementToDate NOTIFY movementChanged)
    Q_PROPERTY(QVariantList inventoryMovementRows READ inventoryMovementRows NOTIFY movementChanged)

public:
    explicit ReportsViewModel(application::ReportService &reportService,
                              QObject *parent = nullptr);

    [[nodiscard]] QString lastError() const;
    [[nodiscard]] QString lastExportPath() const;

    [[nodiscard]] QString salesFromDate() const;
    [[nodiscard]] QString salesToDate() const;
    [[nodiscard]] int salesOrderCount() const;
    [[nodiscard]] QString salesTotalVndText() const;
    [[nodiscard]] QString salesCollectedVndText() const;
    [[nodiscard]] QString salesOutstandingVndText() const;
    [[nodiscard]] QVariantList salesBuckets() const;

    [[nodiscard]] QString debtCurrentVndText() const;
    [[nodiscard]] QString debt31To60VndText() const;
    [[nodiscard]] QString debt61PlusVndText() const;
    [[nodiscard]] QString debtTotalVndText() const;
    [[nodiscard]] QVariantList debtRows() const;

    [[nodiscard]] QString movementFromDate() const;
    [[nodiscard]] QString movementToDate() const;
    [[nodiscard]] QVariantList inventoryMovementRows() const;

    Q_INVOKABLE void reloadAll();
    Q_INVOKABLE bool loadSalesSummary(const QString &fromDate, const QString &toDate);
    Q_INVOKABLE void loadDebtAging();
    Q_INVOKABLE bool loadInventoryMovement(const QString &keyword,
                                           const QString &fromDate,
                                           const QString &toDate);
    Q_INVOKABLE bool exportSalesSummary(const QString &format);
    Q_INVOKABLE bool exportDebtAging(const QString &format);
    Q_INVOKABLE bool exportInventoryMovement(const QString &format);

signals:
    void lastErrorChanged();
    void lastExportPathChanged();
    void salesChanged();
    void debtChanged();
    void movementChanged();

private:
    [[nodiscard]] static QString formatMoney(core::Money amountVnd);
    [[nodiscard]] QString buildExportPath(const QString &baseName,
                                          const QString &extension,
                                          QString &errorMessage) const;
    void setLastError(const QString &message);
    void setLastExportPath(const QString &path);

    application::ReportService &m_reportService;

    QString m_lastError;
    QString m_lastExportPath;

    application::SalesSummaryReport m_salesReport;
    application::DebtAgingReport m_debtReport;
    QVector<application::InventoryMovementReportRow> m_inventoryMovementRows;
    QString m_movementKeyword;
    QString m_movementFromDate;
    QString m_movementToDate;
};

} // namespace stockmaster::ui::viewmodels
