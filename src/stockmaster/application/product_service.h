#pragma once

#include <QString>
#include <QVector>

#include "stockmaster/domain/entities.h"

namespace stockmaster::infra::db {
class DatabaseService;
}

namespace stockmaster::application {

struct ProductInput {
    QString sku;
    QString name;
    QString unit;
    core::Money defaultWholesalePriceVnd = 0;
    bool isActive = true;
};

struct ProductLotInput {
    QString lotNo;
    QString receivedDate;
    QString expiryDate;
    int initialQty = 0;
    core::Money unitCostVnd = 0;
};

class ProductService {
public:
    explicit ProductService(stockmaster::infra::db::DatabaseService &databaseService);

    [[nodiscard]] QVector<domain::Product> findProducts(const QString &keyword = {}) const;
    [[nodiscard]] QVector<domain::InventoryMovement> findAllInventoryMovements() const;
    [[nodiscard]] QVector<domain::ProductLot> findLotsByProduct(const QString &productId) const;
    [[nodiscard]] QVector<domain::InventoryMovement> findInventoryMovementsByProduct(const QString &productId) const;
    [[nodiscard]] bool productExists(const QString &productId) const;
    [[nodiscard]] QString productNameById(const QString &productId) const;
    [[nodiscard]] core::Money defaultPriceByProductId(const QString &productId) const;

    [[nodiscard]] int activeProductCount() const;
    [[nodiscard]] int lowStockProductCount(int thresholdQty = 20) const;
    [[nodiscard]] int expiringSoonProductCount(int warningDays = 30) const;
    [[nodiscard]] int totalOnHandByProduct(const QString &productId) const;
    [[nodiscard]] int lotCountByProduct(const QString &productId) const;
    [[nodiscard]] bool saveToDatabase(QString &errorMessage) const;

    [[nodiscard]] bool createProduct(const ProductInput &input, QString &errorMessage);
    [[nodiscard]] bool updateProduct(const QString &productId,
                                     const ProductInput &input,
                                     QString &errorMessage);
    [[nodiscard]] bool deleteProduct(const QString &productId, QString &errorMessage);

    [[nodiscard]] bool addLot(const QString &productId,
                              const ProductLotInput &input,
                              QString &errorMessage);
    [[nodiscard]] bool stockIn(const QString &lotId,
                               int qty,
                               QString &errorMessage,
                               const QString &reason = {},
                               const QString &movementDate = {},
                               bool recordMovement = true,
                               bool persistState = true);
    [[nodiscard]] bool stockOut(const QString &lotId,
                                int qty,
                                QString &errorMessage,
                                const QString &reason = {},
                                const QString &movementDate = {},
                                bool recordMovement = true,
                                bool persistState = true);
    [[nodiscard]] bool adjustLotQuantity(const QString &lotId,
                                         int qtyDelta,
                                         const QString &reason,
                                         const QString &movementDate,
                                         QString &errorMessage);

private:
    [[nodiscard]] bool skuExists(const QString &sku,
                                 const QString &excludeProductId = {}) const;
    [[nodiscard]] bool lotNoExists(const QString &productId,
                                   const QString &lotNo,
                                   const QString &excludeLotId = {}) const;
    [[nodiscard]] bool findLotIndex(const QString &lotId, qsizetype &index) const;

    [[nodiscard]] static QString normalizedSku(const QString &sku);
    [[nodiscard]] static QString normalizedLotNo(const QString &lotNo);
    [[nodiscard]] static QString generateDefaultSku(int serial);
    [[nodiscard]] static QString generateDefaultLotNo(int serial);
    static void appendInventoryMovement(QVector<domain::InventoryMovement> &movements,
                                        const domain::ProductLot &lot,
                                        int qtyDelta,
                                        const QString &movementType,
                                        const QString &reason,
                                        const QString &movementDate);
    bool loadFromDatabase();
    bool persistState(const QVector<domain::Product> &products,
                      const QVector<domain::ProductLot> &lots,
                      const QVector<domain::InventoryMovement> &movements,
                      QString &errorMessage) const;
    void recomputeSerials();

    void seedSampleData();

    stockmaster::infra::db::DatabaseService &m_databaseService;
    QVector<domain::Product> m_products;
    QVector<domain::ProductLot> m_lots;
    QVector<domain::InventoryMovement> m_inventoryMovements;
    int m_nextSkuSerial = 1;
    int m_nextLotSerial = 1;
};

} // namespace stockmaster::application
