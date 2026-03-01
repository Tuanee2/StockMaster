#pragma once

#include <QAbstractListModel>
#include <QHash>
#include <QVariantList>
#include <QVector>

#include "stockmaster/application/product_service.h"

namespace stockmaster::ui::viewmodels {

class InventoryViewModel : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(QString filterText READ filterText WRITE setFilterText NOTIFY filterTextChanged)
    Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged)
    Q_PROPERTY(int productCount READ productCount NOTIFY productCountChanged)
    Q_PROPERTY(int lowStockCount READ lowStockCount NOTIFY productCountChanged)
    Q_PROPERTY(bool hasSelectedProduct READ hasSelectedProduct NOTIFY selectedProductChanged)
    Q_PROPERTY(QString selectedProductId READ selectedProductId NOTIFY selectedProductChanged)
    Q_PROPERTY(QString selectedProductName READ selectedProductName NOTIFY selectedProductChanged)
    Q_PROPERTY(int selectedProductTotalOnHand READ selectedProductTotalOnHand NOTIFY selectedProductChanged)
    Q_PROPERTY(bool selectedProductLowStock READ selectedProductLowStock NOTIFY selectedProductChanged)
    Q_PROPERTY(QVariantList selectedProductLots READ selectedProductLots NOTIFY detailChanged)
    Q_PROPERTY(QVariantList selectedProductMovements READ selectedProductMovements NOTIFY detailChanged)

public:
    enum InventoryRole {
        ProductIdRole = Qt::UserRole + 1,
        SkuRole,
        NameRole,
        UnitRole,
        TotalOnHandRole,
        LotCountRole,
        IsLowStockRole,
    };

    explicit InventoryViewModel(application::ProductService &productService,
                                QObject *parent = nullptr);

    [[nodiscard]] int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    [[nodiscard]] QVariant data(const QModelIndex &index, int role) const override;
    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;

    [[nodiscard]] QString filterText() const;
    void setFilterText(const QString &value);

    [[nodiscard]] QString lastError() const;
    [[nodiscard]] int productCount() const;
    [[nodiscard]] int lowStockCount() const;
    [[nodiscard]] bool hasSelectedProduct() const;
    [[nodiscard]] QString selectedProductId() const;
    [[nodiscard]] QString selectedProductName() const;
    [[nodiscard]] int selectedProductTotalOnHand() const;
    [[nodiscard]] bool selectedProductLowStock() const;
    [[nodiscard]] QVariantList selectedProductLots() const;
    [[nodiscard]] QVariantList selectedProductMovements() const;

    Q_INVOKABLE void reload();
    Q_INVOKABLE void selectProduct(const QString &productId);
    Q_INVOKABLE bool adjustSelectedLot(const QString &lotId,
                                       const QString &qtyDeltaText,
                                       const QString &reason,
                                       const QString &adjustmentDate);

signals:
    void filterTextChanged();
    void lastErrorChanged();
    void productCountChanged();
    void selectedProductChanged();
    void detailChanged();

private:
    [[nodiscard]] bool parseSignedInt(const QString &input, int &value) const;
    [[nodiscard]] QString movementTypeLabel(const QString &movementType) const;
    void setLastError(const QString &message);
    void refreshModel();
    void refreshSelectedData();
    void clearSelection();

    application::ProductService &m_productService;
    QVector<domain::Product> m_rows;
    QString m_filterText;
    QString m_lastError;

    bool m_hasSelectedProduct = false;
    QString m_selectedProductId;
    QString m_selectedProductName;
    int m_selectedProductTotalOnHand = 0;
    bool m_selectedProductLowStock = false;
    QVariantList m_selectedProductLots;
    QVariantList m_selectedProductMovements;
};

} // namespace stockmaster::ui::viewmodels
