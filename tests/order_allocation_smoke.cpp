#include <cassert>

#include <QCoreApplication>
#include <QDir>
#include <QFile>

#include "stockmaster/application/customer_service.h"
#include "stockmaster/application/order_service.h"
#include "stockmaster/application/product_service.h"
#include "stockmaster/infra/db/database_service.h"

int main(int argc, char *argv[])
{
    using namespace stockmaster;

    QCoreApplication app(argc, argv);

    const QString dbPath = QDir::temp().filePath(
        QStringLiteral("stockmaster_order_allocation_smoke_%1.sqlite")
            .arg(QCoreApplication::applicationPid()));
    QFile::remove(dbPath);

    infra::db::DatabaseService databaseService(dbPath);
    assert(databaseService.initialize());

    application::CustomerService customerService(databaseService);
    application::ProductService productService(databaseService);
    application::OrderService orderService(databaseService, customerService, productService);

    QString errorMessage;
    assert(productService.createProduct(application::ProductInput{
                                           QString(),
                                           QStringLiteral("San pham phan lo test"),
                                           QStringLiteral("thung"),
                                           120000,
                                           true,
                                       },
                                       errorMessage));

    domain::Product createdProduct;
    bool foundProduct = false;
    for (const domain::Product &product : productService.findProducts()) {
        if (product.name == QStringLiteral("San pham phan lo test")) {
            createdProduct = product;
            foundProduct = true;
            break;
        }
    }
    assert(foundProduct);
    assert(!createdProduct.sku.trimmed().isEmpty());

    assert(productService.addLot(createdProduct.id,
                                 application::ProductLotInput{
                                     QStringLiteral("LTEST-01"),
                                     QStringLiteral("2026-03-01"),
                                     QStringLiteral("2026-09-01"),
                                     3,
                                     70000,
                                 },
                                 errorMessage));
    assert(productService.addLot(createdProduct.id,
                                 application::ProductLotInput{
                                     QStringLiteral("LTEST-02"),
                                     QStringLiteral("2026-03-02"),
                                     QStringLiteral("2026-10-01"),
                                     5,
                                     71000,
                                 },
                                 errorMessage));

    const QVector<domain::ProductLot> lots = productService.findLotsByProduct(createdProduct.id);
    assert(lots.size() == 2);

    QString firstLotId;
    QString secondLotId;
    for (const domain::ProductLot &lot : lots) {
        if (lot.lotNo == QStringLiteral("LTEST-01")) {
            firstLotId = lot.id;
        } else if (lot.lotNo == QStringLiteral("LTEST-02")) {
            secondLotId = lot.id;
        }
    }

    assert(!firstLotId.isEmpty());
    assert(!secondLotId.isEmpty());

    const QVector<domain::Customer> customers = customerService.findCustomers();
    assert(!customers.isEmpty());

    QString orderId;
    assert(orderService.createDraftOrder(customers.first().id,
                                         QStringLiteral("2026-03-05"),
                                         orderId,
                                         errorMessage));

    assert(orderService.upsertDraftItem(orderId,
                                        application::DraftOrderItemInput{
                                            createdProduct.id,
                                            5,
                                            createdProduct.defaultWholesalePriceVnd,
                                            0,
                                            firstLotId,
                                        },
                                        errorMessage));

    const QVector<application::StockAllocationInfo> allocations = orderService.findAllocations(orderId);
    assert(allocations.size() == 2);
    assert(allocations.at(0).lotId == firstLotId);
    assert(allocations.at(0).qty == 3);
    assert(allocations.at(1).lotId == secondLotId);
    assert(allocations.at(1).qty == 2);

    assert(orderService.confirmOrder(orderId, errorMessage));

    const QVector<domain::ProductLot> updatedLots = productService.findLotsByProduct(createdProduct.id);
    for (const domain::ProductLot &lot : updatedLots) {
        if (lot.id == firstLotId) {
            assert(lot.onHandQty == 0);
        } else if (lot.id == secondLotId) {
            assert(lot.onHandQty == 3);
        }
    }

    return 0;
}
