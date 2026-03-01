#pragma once

#include <QAbstractListModel>
#include <QHash>
#include <QVariantMap>
#include <QVector>

#include "stockmaster/application/customer_service.h"

namespace stockmaster::ui::viewmodels {

class CustomersViewModel : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(QString filterText READ filterText WRITE setFilterText NOTIFY filterTextChanged)
    Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged)
    Q_PROPERTY(int customerCount READ customerCount NOTIFY customerCountChanged)

public:
    enum CustomerRole {
        CustomerIdRole = Qt::UserRole + 1,
        CodeRole,
        NameRole,
        PhoneRole,
        AddressRole,
        CreditLimitVndRole,
        CreditLimitTextRole
    };

    explicit CustomersViewModel(application::CustomerService &customerService,
                                QObject *parent = nullptr);

    [[nodiscard]] int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    [[nodiscard]] QVariant data(const QModelIndex &index, int role) const override;
    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;

    [[nodiscard]] QString filterText() const;
    void setFilterText(const QString &value);

    [[nodiscard]] QString lastError() const;
    [[nodiscard]] int customerCount() const;

    Q_INVOKABLE bool createCustomer(const QString &name,
                                    const QString &phone,
                                    const QString &address,
                                    const QString &creditLimitText);
    Q_INVOKABLE bool updateCustomer(const QString &customerId,
                                    const QString &name,
                                    const QString &phone,
                                    const QString &address,
                                    const QString &creditLimitText);
    Q_INVOKABLE bool deleteCustomer(const QString &customerId);
    Q_INVOKABLE QVariantMap getCustomer(int row) const;
    Q_INVOKABLE void reload();

signals:
    void filterTextChanged();
    void lastErrorChanged();
    void customerCountChanged();

private:
    [[nodiscard]] bool parseMoneyInput(const QString &input, core::Money &value) const;
    [[nodiscard]] static QString formatMoneyVnd(core::Money amountVnd);
    void setLastError(const QString &message);
    void refreshModel();

    application::CustomerService &m_customerService;
    QVector<domain::Customer> m_rows;
    QString m_filterText;
    QString m_lastError;
};

} // namespace stockmaster::ui::viewmodels
