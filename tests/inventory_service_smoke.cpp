#include <cassert>

#include <QCoreApplication>
#include <QDir>
#include <QFile>

#include "stockmaster/application/product_service.h"
#include "stockmaster/infra/db/database_service.h"

int main(int argc, char *argv[])
{
    using namespace stockmaster;

    QCoreApplication app(argc, argv);

    const QString dbPath = QDir::temp().filePath(QStringLiteral("stockmaster_inventory_service_smoke.sqlite"));
    QFile::remove(dbPath);

    infra::db::DatabaseService databaseService(dbPath);
    assert(databaseService.initialize());

    application::ProductService productService(databaseService);
    const QVector<domain::Product> products = productService.findProducts();
    assert(!products.isEmpty());

    const domain::Product product = products.first();
    const QVector<domain::ProductLot> initialLots = productService.findLotsByProduct(product.id);
    assert(!initialLots.isEmpty());

    const domain::ProductLot firstLot = initialLots.first();
    const int initialOnHand = firstLot.onHandQty;

    QString errorMessage;
    assert(!productService.adjustLotQuantity(firstLot.id,
                                             -(initialOnHand + 1),
                                             QStringLiteral("Kiem ke sai"),
                                             QStringLiteral("2026-03-01"),
                                             errorMessage));

    assert(productService.adjustLotQuantity(firstLot.id,
                                            7,
                                            QStringLiteral("Bu kiem ke"),
                                            QStringLiteral("2026-03-02"),
                                            errorMessage));
    assert(productService.stockOut(firstLot.id,
                                   5,
                                   errorMessage,
                                   QStringLiteral("Xuat kiem tra"),
                                   QStringLiteral("2026-03-03")));

    const QVector<domain::ProductLot> updatedLots = productService.findLotsByProduct(product.id);
    bool foundUpdatedLot = false;
    for (const domain::ProductLot &lot : updatedLots) {
        if (lot.id == firstLot.id) {
            foundUpdatedLot = true;
            assert(lot.onHandQty == initialOnHand + 2);
            break;
        }
    }
    assert(foundUpdatedLot);

    const QVector<domain::InventoryMovement> movements = productService.findInventoryMovementsByProduct(product.id);
    assert(movements.size() >= 2);
    assert(movements.first().movementType == QStringLiteral("StockOut"));
    assert(movements.first().movementDate == QStringLiteral("2026-03-03"));
    assert(movements.first().reason == QStringLiteral("Xuat kiem tra"));
    assert(movements.first().qtyDelta == -5);
    assert(movements.first().lotBalanceAfter == initialOnHand + 2);

    assert(movements.at(1).movementType == QStringLiteral("AdjustUp"));
    assert(movements.at(1).movementDate == QStringLiteral("2026-03-02"));
    assert(movements.at(1).reason == QStringLiteral("Bu kiem ke"));
    assert(movements.at(1).qtyDelta == 7);

    return 0;
}
