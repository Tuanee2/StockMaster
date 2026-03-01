#pragma once

#include <QAbstractListModel>
#include <QHash>
#include <QVariantList>
#include <QVariantMap>
#include <QVector>

#include "stockmaster/application/product_service.h"

namespace stockmaster::ui::viewmodels {

class ProductsViewModel : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(QString filterText READ filterText WRITE setFilterText NOTIFY filterTextChanged)
    Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged)
    Q_PROPERTY(int productCount READ productCount NOTIFY productCountChanged)
    Q_PROPERTY(QString selectedProductId READ selectedProductId NOTIFY selectedProductChanged)
    Q_PROPERTY(QString selectedProductName READ selectedProductName NOTIFY selectedProductChanged)
    Q_PROPERTY(int selectedProductTotalOnHand READ selectedProductTotalOnHand NOTIFY selectedProductLotsChanged)
    Q_PROPERTY(int selectedProductExpiringSoonLotCount READ selectedProductExpiringSoonLotCount NOTIFY selectedProductLotsChanged)
    Q_PROPERTY(QVariantList selectedProductLots READ selectedProductLots NOTIFY selectedProductLotsChanged)
    Q_PROPERTY(bool hasSelectedProduct READ hasSelectedProduct NOTIFY selectedProductChanged)

public:
    enum ProductRole {
        ProductIdRole = Qt::UserRole + 1,
        SkuRole,
        NameRole,
        UnitRole,
        DefaultWholesalePriceVndRole,
        DefaultWholesalePriceTextRole,
        IsActiveRole,
        TotalOnHandRole,
        LotCountRole
    };

    explicit ProductsViewModel(application::ProductService &productService,
                               QObject *parent = nullptr);

    [[nodiscard]] int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    [[nodiscard]] QVariant data(const QModelIndex &index, int role) const override;
    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;

    [[nodiscard]] QString filterText() const;
    void setFilterText(const QString &value);

    [[nodiscard]] QString lastError() const;
    [[nodiscard]] int productCount() const;
    [[nodiscard]] QString selectedProductId() const;
    [[nodiscard]] QString selectedProductName() const;
    [[nodiscard]] int selectedProductTotalOnHand() const;
    [[nodiscard]] int selectedProductExpiringSoonLotCount() const;
    [[nodiscard]] QVariantList selectedProductLots() const;
    [[nodiscard]] bool hasSelectedProduct() const;

    Q_INVOKABLE bool createProduct(const QString &sku,
                                   const QString &name,
                                   const QString &unit,
                                   const QString &defaultPriceText,
                                   bool isActive);
    Q_INVOKABLE bool updateProduct(const QString &productId,
                                   const QString &sku,
                                   const QString &name,
                                   const QString &unit,
                                   const QString &defaultPriceText,
                                   bool isActive);
    Q_INVOKABLE bool deleteProduct(const QString &productId);

    Q_INVOKABLE bool addLotToSelectedProduct(const QString &lotNo,
                                             const QString &receivedDate,
                                             const QString &expiryDate,
                                             const QString &qtyText,
                                             const QString &unitCostText);
    Q_INVOKABLE bool stockInLot(const QString &lotId, const QString &qtyText);
    Q_INVOKABLE bool stockOutLot(const QString &lotId, const QString &qtyText);

    Q_INVOKABLE QVariantMap getProduct(int row) const;
    Q_INVOKABLE void selectProduct(const QString &productId);
    Q_INVOKABLE void reload();

signals:
    void filterTextChanged();
    void lastErrorChanged();
    void productCountChanged();
    void selectedProductChanged();
    void selectedProductLotsChanged();

private:
    [[nodiscard]] bool parseMoneyInput(const QString &input, core::Money &value) const;
    [[nodiscard]] bool parsePositiveInt(const QString &input, int &value) const;
    [[nodiscard]] static QString formatMoneyVnd(core::Money amountVnd);

    void setLastError(const QString &message);
    void refreshModel();
    void refreshSelectedLots();
    [[nodiscard]] bool selectedProductExists() const;

    application::ProductService &m_productService;
    QVector<domain::Product> m_rows;
    QString m_filterText;
    QString m_lastError;
    QString m_selectedProductId;
    QVariantList m_selectedProductLots;
    int m_selectedProductExpiringSoonLotCount = 0;
};

} // namespace stockmaster::ui::viewmodels
