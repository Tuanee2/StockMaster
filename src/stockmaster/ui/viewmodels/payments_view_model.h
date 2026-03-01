#pragma once

#include <QAbstractListModel>
#include <QHash>
#include <QVariantList>
#include <QVector>

#include "stockmaster/application/payment_service.h"

namespace stockmaster::ui::viewmodels {

class PaymentsViewModel : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(QString filterText READ filterText WRITE setFilterText NOTIFY filterTextChanged)
    Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged)
    Q_PROPERTY(int customerCount READ customerCount NOTIFY customerCountChanged)
    Q_PROPERTY(bool hasSelectedCustomer READ hasSelectedCustomer NOTIFY selectedCustomerChanged)
    Q_PROPERTY(QString selectedCustomerId READ selectedCustomerId NOTIFY selectedCustomerChanged)
    Q_PROPERTY(QString selectedCustomerName READ selectedCustomerName NOTIFY selectedCustomerChanged)
    Q_PROPERTY(QString selectedReceivableText READ selectedReceivableText NOTIFY selectedCustomerChanged)
    Q_PROPERTY(int selectedOpenOrderCount READ selectedOpenOrderCount NOTIFY selectedCustomerChanged)
    Q_PROPERTY(QString preferredOrderId READ preferredOrderId NOTIFY detailChanged)
    Q_PROPERTY(QVariantList payableOrders READ payableOrders NOTIFY detailChanged)
    Q_PROPERTY(QVariantList paymentHistory READ paymentHistory NOTIFY detailChanged)
    Q_PROPERTY(QVariantList customerLedger READ customerLedger NOTIFY detailChanged)

public:
    enum CustomerRole {
        CustomerIdRole = Qt::UserRole + 1,
        CodeRole,
        NameRole,
        ReceivableVndRole,
        ReceivableTextRole,
        OpenOrderCountRole,
        PaymentCountRole,
    };

    explicit PaymentsViewModel(application::PaymentService &paymentService,
                               QObject *parent = nullptr);

    [[nodiscard]] int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    [[nodiscard]] QVariant data(const QModelIndex &index, int role) const override;
    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;

    [[nodiscard]] QString filterText() const;
    void setFilterText(const QString &value);

    [[nodiscard]] QString lastError() const;
    [[nodiscard]] int customerCount() const;
    [[nodiscard]] bool hasSelectedCustomer() const;
    [[nodiscard]] QString selectedCustomerId() const;
    [[nodiscard]] QString selectedCustomerName() const;
    [[nodiscard]] QString selectedReceivableText() const;
    [[nodiscard]] int selectedOpenOrderCount() const;
    [[nodiscard]] QString preferredOrderId() const;
    [[nodiscard]] QVariantList payableOrders() const;
    [[nodiscard]] QVariantList paymentHistory() const;
    [[nodiscard]] QVariantList customerLedger() const;

    Q_INVOKABLE void reload();
    Q_INVOKABLE void selectCustomer(const QString &customerId);
    Q_INVOKABLE void focusOrder(const QString &customerId, const QString &orderId);
    Q_INVOKABLE bool createReceipt(const QString &orderId,
                                   const QString &amountText,
                                   const QString &method,
                                   const QString &paidAt);

signals:
    void filterTextChanged();
    void lastErrorChanged();
    void customerCountChanged();
    void selectedCustomerChanged();
    void detailChanged();

private:
    [[nodiscard]] bool parseMoneyInput(const QString &input, core::Money &value) const;
    [[nodiscard]] static QString formatMoneyVnd(core::Money amountVnd);
    void setLastError(const QString &message);
    void refreshModel();
    void refreshSelectedData();
    void clearSelection();

    application::PaymentService &m_paymentService;
    QVector<application::CustomerReceivableSummary> m_rows;
    QString m_filterText;
    QString m_lastError;

    bool m_hasSelectedCustomer = false;
    QString m_selectedCustomerId;
    QString m_selectedCustomerName;
    QString m_preferredOrderId;
    core::Money m_selectedReceivableVnd = 0;
    int m_selectedOpenOrderCount = 0;

    QVariantList m_payableOrders;
    QVariantList m_paymentHistory;
    QVariantList m_customerLedger;
};

} // namespace stockmaster::ui::viewmodels
