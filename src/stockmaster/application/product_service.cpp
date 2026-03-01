#include "stockmaster/application/product_service.h"

#include <QDate>
#include <QRegularExpression>
#include <QSet>
#include <QSqlError>
#include <QSqlQuery>
#include <QUuid>

#include <algorithm>

#include "stockmaster/infra/db/database_service.h"

namespace stockmaster::application {

namespace {

int nextProductSerial(const QVector<domain::Product> &products)
{
    int maxSerial = 0;
    for (const domain::Product &product : products) {
        if (!product.sku.startsWith(QLatin1String("SP"))) {
            continue;
        }

        bool ok = false;
        const int serial = product.sku.mid(2).toInt(&ok);
        if (ok && serial > maxSerial) {
            maxSerial = serial;
        }
    }

    return maxSerial + 1;
}

int nextLotSerial(const QVector<domain::ProductLot> &lots)
{
    int maxSerial = 0;
    for (const domain::ProductLot &lot : lots) {
        if (!lot.lotNo.startsWith(QLatin1Char('L'))) {
            continue;
        }

        bool ok = false;
        const int serial = lot.lotNo.mid(1).toInt(&ok);
        if (ok && serial > maxSerial) {
            maxSerial = serial;
        }
    }

    return maxSerial + 1;
}

}

ProductService::ProductService(stockmaster::infra::db::DatabaseService &databaseService)
    : m_databaseService(databaseService)
{
    if (!loadFromDatabase() || m_products.isEmpty()) {
        seedSampleData();
    } else {
        recomputeSerials();
    }
}

QVector<domain::Product> ProductService::findProducts(const QString &keyword) const
{
    const QString normalizedKeyword = keyword.trimmed();

    if (normalizedKeyword.isEmpty()) {
        return m_products;
    }

    QVector<domain::Product> filtered;
    filtered.reserve(m_products.size());

    for (const domain::Product &product : m_products) {
        const bool matched = product.sku.contains(normalizedKeyword, Qt::CaseInsensitive)
                             || product.name.contains(normalizedKeyword, Qt::CaseInsensitive)
                             || product.unit.contains(normalizedKeyword, Qt::CaseInsensitive);

        if (matched) {
            filtered.push_back(product);
        }
    }

    return filtered;
}

QVector<domain::ProductLot> ProductService::findLotsByProduct(const QString &productId) const
{
    QVector<domain::ProductLot> lots;
    const QString normalizedProductId = productId.trimmed();

    if (normalizedProductId.isEmpty()) {
        return lots;
    }

    for (const domain::ProductLot &lot : m_lots) {
        if (lot.productId == normalizedProductId) {
            lots.push_back(lot);
        }
    }

    return lots;
}

QVector<domain::InventoryMovement> ProductService::findAllInventoryMovements() const
{
    QVector<domain::InventoryMovement> movements = m_inventoryMovements;

    std::sort(movements.begin(),
              movements.end(),
              [](const domain::InventoryMovement &lhs, const domain::InventoryMovement &rhs) {
                  if (lhs.movementDate == rhs.movementDate) {
                      return lhs.id > rhs.id;
                  }

                  return lhs.movementDate > rhs.movementDate;
              });

    return movements;
}

QVector<domain::InventoryMovement> ProductService::findInventoryMovementsByProduct(const QString &productId) const
{
    QVector<domain::InventoryMovement> movements;
    const QString normalizedProductId = productId.trimmed();
    if (normalizedProductId.isEmpty()) {
        return movements;
    }

    const QVector<domain::InventoryMovement> allMovements = findAllInventoryMovements();
    for (const domain::InventoryMovement &movement : allMovements) {
        if (movement.productId == normalizedProductId) {
            movements.push_back(movement);
        }
    }

    return movements;
}

bool ProductService::productExists(const QString &productId) const
{
    const QString normalizedId = productId.trimmed();
    if (normalizedId.isEmpty()) {
        return false;
    }

    for (const domain::Product &product : m_products) {
        if (product.id == normalizedId) {
            return true;
        }
    }

    return false;
}

QString ProductService::productNameById(const QString &productId) const
{
    const QString normalizedId = productId.trimmed();
    if (normalizedId.isEmpty()) {
        return QString();
    }

    for (const domain::Product &product : m_products) {
        if (product.id == normalizedId) {
            return product.name;
        }
    }

    return QString();
}

core::Money ProductService::defaultPriceByProductId(const QString &productId) const
{
    const QString normalizedId = productId.trimmed();
    if (normalizedId.isEmpty()) {
        return 0;
    }

    for (const domain::Product &product : m_products) {
        if (product.id == normalizedId) {
            return product.defaultWholesalePriceVnd;
        }
    }

    return 0;
}

int ProductService::activeProductCount() const
{
    return m_products.size();
}

int ProductService::lowStockProductCount(int thresholdQty) const
{
    if (thresholdQty < 0) {
        thresholdQty = 0;
    }

    int count = 0;
    for (const domain::Product &product : m_products) {
        if (!product.isActive) {
            continue;
        }

        if (totalOnHandByProduct(product.id) <= thresholdQty) {
            ++count;
        }
    }

    return count;
}

int ProductService::expiringSoonProductCount(int warningDays) const
{
    if (warningDays < 0) {
        warningDays = 0;
    }

    const QDate today = QDate::currentDate();
    QSet<QString> activeProductIds;
    for (const domain::Product &product : m_products) {
        if (product.isActive) {
            activeProductIds.insert(product.id);
        }
    }

    QSet<QString> warnedProducts;

    for (const domain::ProductLot &lot : m_lots) {
        if (!activeProductIds.contains(lot.productId)) {
            continue;
        }

        if (lot.onHandQty <= 0) {
            continue;
        }

        const QDate expiryDate = QDate::fromString(lot.expiryDate, Qt::ISODate);
        if (!expiryDate.isValid()) {
            continue;
        }

        const int daysToExpiry = today.daysTo(expiryDate);
        if (daysToExpiry >= 0 && daysToExpiry <= warningDays) {
            warnedProducts.insert(lot.productId);
        }
    }

    return warnedProducts.size();
}

int ProductService::totalOnHandByProduct(const QString &productId) const
{
    const QString normalizedProductId = productId.trimmed();
    if (normalizedProductId.isEmpty()) {
        return 0;
    }

    int total = 0;
    for (const domain::ProductLot &lot : m_lots) {
        if (lot.productId == normalizedProductId) {
            total += lot.onHandQty;
        }
    }

    return total;
}

int ProductService::lotCountByProduct(const QString &productId) const
{
    const QString normalizedProductId = productId.trimmed();
    if (normalizedProductId.isEmpty()) {
        return 0;
    }

    int count = 0;
    for (const domain::ProductLot &lot : m_lots) {
        if (lot.productId == normalizedProductId) {
            ++count;
        }
    }

    return count;
}

bool ProductService::saveToDatabase(QString &errorMessage) const
{
    return persistState(m_products, m_lots, m_inventoryMovements, errorMessage);
}

bool ProductService::createProduct(const ProductInput &input, QString &errorMessage)
{
    const QString name = input.name.trimmed();
    if (name.isEmpty()) {
        errorMessage = QStringLiteral("Tên sản phẩm là bắt buộc.");
        return false;
    }

    if (input.defaultWholesalePriceVnd < 0) {
        errorMessage = QStringLiteral("Giá sỉ mặc định phải >= 0.");
        return false;
    }

    const QString unit = input.unit.trimmed().isEmpty() ? QStringLiteral("cai") : input.unit.trimmed();

    int nextSkuSerial = m_nextSkuSerial;
    QString sku = normalizedSku(input.sku);
    if (sku.isEmpty()) {
        do {
            sku = generateDefaultSku(nextSkuSerial++);
        } while (skuExists(sku));
    }

    if (skuExists(sku)) {
        errorMessage = QStringLiteral("SKU đã tồn tại.");
        return false;
    }

    domain::Product product;
    product.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    product.sku = sku;
    product.name = name;
    product.unit = unit;
    product.defaultWholesalePriceVnd = input.defaultWholesalePriceVnd;
    product.isActive = input.isActive;

    QVector<domain::Product> updatedProducts = m_products;
    updatedProducts.push_back(product);

    if (!persistState(updatedProducts, m_lots, m_inventoryMovements, errorMessage)) {
        return false;
    }

    m_products = updatedProducts;
    m_nextSkuSerial = nextSkuSerial;
    errorMessage.clear();
    return true;
}

bool ProductService::updateProduct(const QString &productId,
                                   const ProductInput &input,
                                   QString &errorMessage)
{
    const QString normalizedId = productId.trimmed();
    if (normalizedId.isEmpty()) {
        errorMessage = QStringLiteral("Không tìm thấy sản phẩm cần sửa.");
        return false;
    }

    const QString name = input.name.trimmed();
    if (name.isEmpty()) {
        errorMessage = QStringLiteral("Tên sản phẩm là bắt buộc.");
        return false;
    }

    if (input.defaultWholesalePriceVnd < 0) {
        errorMessage = QStringLiteral("Giá sỉ mặc định phải >= 0.");
        return false;
    }

    const QString sku = normalizedSku(input.sku);
    if (sku.isEmpty()) {
        errorMessage = QStringLiteral("SKU là bắt buộc.");
        return false;
    }

    if (skuExists(sku, normalizedId)) {
        errorMessage = QStringLiteral("SKU đã tồn tại.");
        return false;
    }

    const QString unit = input.unit.trimmed().isEmpty() ? QStringLiteral("cai") : input.unit.trimmed();

    QVector<domain::Product> updatedProducts = m_products;
    for (domain::Product &product : updatedProducts) {
        if (product.id != normalizedId) {
            continue;
        }

        product.sku = sku;
        product.name = name;
        product.unit = unit;
        product.defaultWholesalePriceVnd = input.defaultWholesalePriceVnd;
        product.isActive = input.isActive;

        if (!persistState(updatedProducts, m_lots, m_inventoryMovements, errorMessage)) {
            return false;
        }

        m_products = updatedProducts;
        recomputeSerials();
        errorMessage.clear();
        return true;
    }

    errorMessage = QStringLiteral("Không tìm thấy sản phẩm cần sửa.");
    return false;
}

bool ProductService::deleteProduct(const QString &productId, QString &errorMessage)
{
    const QString normalizedId = productId.trimmed();
    if (normalizedId.isEmpty()) {
        errorMessage = QStringLiteral("Không tìm thấy sản phẩm cần xóa.");
        return false;
    }

    if (totalOnHandByProduct(normalizedId) > 0) {
        errorMessage = QStringLiteral("Không thể xóa sản phẩm đang còn tồn kho.");
        return false;
    }

    QVector<domain::Product> updatedProducts = m_products;
    const auto productIt = std::find_if(updatedProducts.begin(),
                                        updatedProducts.end(),
                                        [&normalizedId](const domain::Product &product) {
                                            return product.id == normalizedId;
                                        });
    if (productIt == updatedProducts.end()) {
        errorMessage = QStringLiteral("Không tìm thấy sản phẩm cần xóa.");
        return false;
    }
    updatedProducts.erase(productIt);

    QVector<domain::ProductLot> updatedLots = m_lots;
    updatedLots.erase(
        std::remove_if(updatedLots.begin(),
                       updatedLots.end(),
                       [&normalizedId](const domain::ProductLot &lot) { return lot.productId == normalizedId; }),
        updatedLots.end());

    QVector<domain::InventoryMovement> updatedMovements = m_inventoryMovements;
    updatedMovements.erase(
        std::remove_if(updatedMovements.begin(),
                       updatedMovements.end(),
                       [&normalizedId](const domain::InventoryMovement &movement) {
                           return movement.productId == normalizedId;
                       }),
        updatedMovements.end());

    if (!persistState(updatedProducts, updatedLots, updatedMovements, errorMessage)) {
        return false;
    }

    m_products = updatedProducts;
    m_lots = updatedLots;
    m_inventoryMovements = updatedMovements;
    recomputeSerials();
    errorMessage.clear();
    return true;
}

bool ProductService::addLot(const QString &productId,
                            const ProductLotInput &input,
                            QString &errorMessage)
{
    const QString normalizedProductId = productId.trimmed();
    if (normalizedProductId.isEmpty()) {
        errorMessage = QStringLiteral("Bạn cần chọn sản phẩm trước khi thêm lô.");
        return false;
    }

    bool productFound = false;
    for (const domain::Product &product : m_products) {
        if (product.id == normalizedProductId) {
            productFound = true;
            break;
        }
    }

    if (!productFound) {
        errorMessage = QStringLiteral("Không tìm thấy sản phẩm để thêm lô.");
        return false;
    }

    if (input.initialQty <= 0) {
        errorMessage = QStringLiteral("Số lượng nhập lô phải > 0.");
        return false;
    }

    if (input.unitCostVnd < 0) {
        errorMessage = QStringLiteral("Giá vốn lô phải >= 0.");
        return false;
    }

    int nextLotSerial = m_nextLotSerial;
    QString lotNo = normalizedLotNo(input.lotNo);
    if (lotNo.isEmpty()) {
        do {
            lotNo = generateDefaultLotNo(nextLotSerial++);
        } while (lotNoExists(normalizedProductId, lotNo));
    }

    if (lotNoExists(normalizedProductId, lotNo)) {
        errorMessage = QStringLiteral("Mã lô đã tồn tại trong sản phẩm này.");
        return false;
    }

    QString receivedDate = input.receivedDate.trimmed();
    QDate receivedDateParsed;
    if (receivedDate.isEmpty()) {
        receivedDateParsed = QDate::currentDate();
        receivedDate = receivedDateParsed.toString(Qt::ISODate);
    } else {
        receivedDateParsed = QDate::fromString(receivedDate, Qt::ISODate);
        if (!receivedDateParsed.isValid()) {
            errorMessage = QStringLiteral("Ngày nhập lô không hợp lệ. Dùng định dạng YYYY-MM-DD.");
            return false;
        }
        receivedDate = receivedDateParsed.toString(Qt::ISODate);
    }

    QString expiryDate = input.expiryDate.trimmed();
    if (expiryDate.isEmpty()) {
        errorMessage = QStringLiteral("Ngày hết hạn là bắt buộc.");
        return false;
    }

    const QDate expiryDateParsed = QDate::fromString(expiryDate, Qt::ISODate);
    if (!expiryDateParsed.isValid()) {
        errorMessage = QStringLiteral("Ngày hết hạn không hợp lệ. Dùng định dạng YYYY-MM-DD.");
        return false;
    }

    expiryDate = expiryDateParsed.toString(Qt::ISODate);
    if (expiryDateParsed < receivedDateParsed) {
        errorMessage = QStringLiteral("Ngày hết hạn phải >= ngày nhập lô.");
        return false;
    }

    domain::ProductLot lot;
    lot.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    lot.productId = normalizedProductId;
    lot.lotNo = lotNo;
    lot.receivedDate = receivedDate;
    lot.expiryDate = expiryDate;
    lot.unitCostVnd = input.unitCostVnd;
    lot.onHandQty = input.initialQty;

    QVector<domain::ProductLot> updatedLots = m_lots;
    updatedLots.push_back(lot);

    QVector<domain::InventoryMovement> updatedMovements = m_inventoryMovements;
    appendInventoryMovement(updatedMovements,
                            lot,
                            lot.onHandQty,
                            QStringLiteral("LotCreated"),
                            QStringLiteral("Tạo lô ban đầu"),
                            receivedDate);

    if (!persistState(m_products, updatedLots, updatedMovements, errorMessage)) {
        return false;
    }

    m_lots = updatedLots;
    m_inventoryMovements = updatedMovements;
    m_nextLotSerial = nextLotSerial;
    errorMessage.clear();
    return true;
}

bool ProductService::stockIn(const QString &lotId,
                             int qty,
                             QString &errorMessage,
                             const QString &reason,
                             const QString &movementDate,
                             bool recordMovement,
                             bool persistChanges)
{
    const QString normalizedLotId = lotId.trimmed();
    if (normalizedLotId.isEmpty()) {
        errorMessage = QStringLiteral("Bạn cần chọn lô để nhập hàng.");
        return false;
    }

    if (qty <= 0) {
        errorMessage = QStringLiteral("Số lượng nhập phải > 0.");
        return false;
    }

    QVector<domain::ProductLot> updatedLots = m_lots;
    qsizetype lotIndex = -1;
    for (qsizetype index = 0; index < updatedLots.size(); ++index) {
        if (updatedLots[index].id == normalizedLotId) {
            lotIndex = index;
            break;
        }
    }

    if (lotIndex < 0) {
        errorMessage = QStringLiteral("Không tìm thấy lô để nhập hàng.");
        return false;
    }

    domain::ProductLot &lot = updatedLots[lotIndex];
    lot.onHandQty += qty;

    QVector<domain::InventoryMovement> updatedMovements = m_inventoryMovements;
    if (recordMovement) {
        appendInventoryMovement(updatedMovements,
                                lot,
                                qty,
                                QStringLiteral("StockIn"),
                                reason.trimmed().isEmpty() ? QStringLiteral("Nhập kho") : reason.trimmed(),
                                movementDate);
    }

    if (persistChanges && !persistState(m_products, updatedLots, updatedMovements, errorMessage)) {
        return false;
    }

    m_lots = updatedLots;
    m_inventoryMovements = updatedMovements;
    errorMessage.clear();
    return true;
}

bool ProductService::stockOut(const QString &lotId,
                              int qty,
                              QString &errorMessage,
                              const QString &reason,
                              const QString &movementDate,
                              bool recordMovement,
                              bool persistChanges)
{
    const QString normalizedLotId = lotId.trimmed();
    if (normalizedLotId.isEmpty()) {
        errorMessage = QStringLiteral("Bạn cần chọn lô để xuất hàng.");
        return false;
    }

    if (qty <= 0) {
        errorMessage = QStringLiteral("Số lượng xuất phải > 0.");
        return false;
    }

    QVector<domain::ProductLot> updatedLots = m_lots;
    qsizetype lotIndex = -1;
    for (qsizetype index = 0; index < updatedLots.size(); ++index) {
        if (updatedLots[index].id == normalizedLotId) {
            lotIndex = index;
            break;
        }
    }

    if (lotIndex < 0) {
        errorMessage = QStringLiteral("Không tìm thấy lô để xuất hàng.");
        return false;
    }

    domain::ProductLot &lot = updatedLots[lotIndex];
    if (lot.onHandQty < qty) {
        errorMessage = QStringLiteral("Không đủ tồn trong lô để xuất.");
        return false;
    }

    lot.onHandQty -= qty;

    QVector<domain::InventoryMovement> updatedMovements = m_inventoryMovements;
    if (recordMovement) {
        appendInventoryMovement(updatedMovements,
                                lot,
                                -qty,
                                QStringLiteral("StockOut"),
                                reason.trimmed().isEmpty() ? QStringLiteral("Xuất kho") : reason.trimmed(),
                                movementDate);
    }

    if (persistChanges && !persistState(m_products, updatedLots, updatedMovements, errorMessage)) {
        return false;
    }

    m_lots = updatedLots;
    m_inventoryMovements = updatedMovements;
    errorMessage.clear();
    return true;
}

bool ProductService::adjustLotQuantity(const QString &lotId,
                                       int qtyDelta,
                                       const QString &reason,
                                       const QString &movementDate,
                                       QString &errorMessage)
{
    const QString normalizedLotId = lotId.trimmed();
    if (normalizedLotId.isEmpty()) {
        errorMessage = QStringLiteral("Bạn cần chọn lô để điều chỉnh tồn.");
        return false;
    }

    if (qtyDelta == 0) {
        errorMessage = QStringLiteral("Số lượng điều chỉnh phải khác 0.");
        return false;
    }

    const QString normalizedReason = reason.trimmed();
    if (normalizedReason.isEmpty()) {
        errorMessage = QStringLiteral("Điều chỉnh tồn bắt buộc phải có lý do.");
        return false;
    }

    QString normalizedDate = movementDate.trimmed();
    if (normalizedDate.isEmpty()) {
        normalizedDate = QDate::currentDate().toString(Qt::ISODate);
    } else {
        const QDate parsedDate = QDate::fromString(normalizedDate, Qt::ISODate);
        if (!parsedDate.isValid()) {
            errorMessage = QStringLiteral("Ngày điều chỉnh không hợp lệ. Dùng định dạng YYYY-MM-DD.");
            return false;
        }
        normalizedDate = parsedDate.toString(Qt::ISODate);
    }

    QVector<domain::ProductLot> updatedLots = m_lots;
    qsizetype lotIndex = -1;
    for (qsizetype index = 0; index < updatedLots.size(); ++index) {
        if (updatedLots[index].id == normalizedLotId) {
            lotIndex = index;
            break;
        }
    }

    if (lotIndex < 0) {
        errorMessage = QStringLiteral("Không tìm thấy lô để điều chỉnh tồn.");
        return false;
    }

    domain::ProductLot &lot = updatedLots[lotIndex];
    const int nextOnHand = lot.onHandQty + qtyDelta;
    if (nextOnHand < 0) {
        errorMessage = QStringLiteral("Không thể điều chỉnh làm tồn kho âm.");
        return false;
    }

    lot.onHandQty = nextOnHand;

    QVector<domain::InventoryMovement> updatedMovements = m_inventoryMovements;
    appendInventoryMovement(updatedMovements,
                            lot,
                            qtyDelta,
                            qtyDelta > 0 ? QStringLiteral("AdjustUp") : QStringLiteral("AdjustDown"),
                            normalizedReason,
                            normalizedDate);

    if (!persistState(m_products, updatedLots, updatedMovements, errorMessage)) {
        return false;
    }

    m_lots = updatedLots;
    m_inventoryMovements = updatedMovements;
    errorMessage.clear();
    return true;
}

bool ProductService::skuExists(const QString &sku, const QString &excludeProductId) const
{
    const QString normalizedSkuValue = normalizedSku(sku);
    const QString normalizedExcludeId = excludeProductId.trimmed();

    for (const domain::Product &product : m_products) {
        if (!normalizedExcludeId.isEmpty() && product.id == normalizedExcludeId) {
            continue;
        }

        if (product.sku.compare(normalizedSkuValue, Qt::CaseInsensitive) == 0) {
            return true;
        }
    }

    return false;
}

bool ProductService::lotNoExists(const QString &productId,
                                 const QString &lotNo,
                                 const QString &excludeLotId) const
{
    const QString normalizedProductId = productId.trimmed();
    const QString normalizedLotNoValue = normalizedLotNo(lotNo);
    const QString normalizedExcludeId = excludeLotId.trimmed();

    for (const domain::ProductLot &lot : m_lots) {
        if (lot.productId != normalizedProductId) {
            continue;
        }

        if (!normalizedExcludeId.isEmpty() && lot.id == normalizedExcludeId) {
            continue;
        }

        if (lot.lotNo.compare(normalizedLotNoValue, Qt::CaseInsensitive) == 0) {
            return true;
        }
    }

    return false;
}

bool ProductService::findLotIndex(const QString &lotId, qsizetype &index) const
{
    const QString normalizedLotId = lotId.trimmed();
    if (normalizedLotId.isEmpty()) {
        return false;
    }

    for (qsizetype current = 0; current < m_lots.size(); ++current) {
        if (m_lots[current].id == normalizedLotId) {
            index = current;
            return true;
        }
    }

    return false;
}

QString ProductService::normalizedSku(const QString &sku)
{
    QString normalized = sku.trimmed().toUpper();
    normalized.replace(QRegularExpression(QStringLiteral("\\s+")), QString());
    return normalized;
}

QString ProductService::normalizedLotNo(const QString &lotNo)
{
    QString normalized = lotNo.trimmed().toUpper();
    normalized.replace(QRegularExpression(QStringLiteral("\\s+")), QString());
    return normalized;
}

QString ProductService::generateDefaultSku(int serial)
{
    return QStringLiteral("SP%1").arg(serial, 4, 10, QLatin1Char('0'));
}

QString ProductService::generateDefaultLotNo(int serial)
{
    return QStringLiteral("L%1").arg(serial, 5, 10, QLatin1Char('0'));
}

void ProductService::appendInventoryMovement(QVector<domain::InventoryMovement> &movements,
                                             const domain::ProductLot &lot,
                                             int qtyDelta,
                                             const QString &movementType,
                                             const QString &reason,
                                             const QString &movementDate)
{
    QString normalizedDate = movementDate.trimmed();
    if (normalizedDate.isEmpty()) {
        normalizedDate = QDate::currentDate().toString(Qt::ISODate);
    } else {
        const QDate parsedDate = QDate::fromString(normalizedDate, Qt::ISODate);
        normalizedDate = parsedDate.isValid() ? parsedDate.toString(Qt::ISODate)
                                              : QDate::currentDate().toString(Qt::ISODate);
    }

    domain::InventoryMovement movement;
    movement.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    movement.productId = lot.productId;
    movement.lotId = lot.id;
    movement.lotNo = lot.lotNo;
    movement.movementType = movementType;
    movement.reason = reason.trimmed();
    movement.movementDate = normalizedDate;
    movement.qtyDelta = qtyDelta;
    movement.lotBalanceAfter = lot.onHandQty;

    movements.push_back(movement);
}

bool ProductService::loadFromDatabase()
{
    m_products.clear();
    m_lots.clear();
    m_inventoryMovements.clear();
    recomputeSerials();

    if (!m_databaseService.isReady()) {
        return false;
    }

    QSqlQuery productQuery(m_databaseService.database());
    if (!productQuery.exec(QStringLiteral(
            "SELECT id, sku, name, unit, default_wholesale_price_vnd, is_active "
            "FROM products ORDER BY sku ASC;"))) {
        return false;
    }

    while (productQuery.next()) {
        domain::Product product;
        product.id = productQuery.value(0).toString();
        product.sku = productQuery.value(1).toString();
        product.name = productQuery.value(2).toString();
        product.unit = productQuery.value(3).toString();
        product.defaultWholesalePriceVnd = productQuery.value(4).toLongLong();
        product.isActive = productQuery.value(5).toInt() != 0;
        m_products.push_back(product);
    }

    QSqlQuery lotQuery(m_databaseService.database());
    if (!lotQuery.exec(QStringLiteral(
            "SELECT id, product_id, lot_no, received_date, expiry_date, unit_cost_vnd, on_hand_qty "
            "FROM product_lots ORDER BY product_id ASC, received_date ASC, lot_no ASC;"))) {
        return false;
    }

    while (lotQuery.next()) {
        domain::ProductLot lot;
        lot.id = lotQuery.value(0).toString();
        lot.productId = lotQuery.value(1).toString();
        lot.lotNo = lotQuery.value(2).toString();
        lot.receivedDate = lotQuery.value(3).toString();
        lot.expiryDate = lotQuery.value(4).toString();
        lot.unitCostVnd = lotQuery.value(5).toLongLong();
        lot.onHandQty = lotQuery.value(6).toInt();
        m_lots.push_back(lot);
    }

    QSqlQuery movementQuery(m_databaseService.database());
    if (!movementQuery.exec(QStringLiteral(
            "SELECT id, product_id, lot_id, lot_no, movement_type, reason, movement_date, qty_delta, lot_balance_after "
            "FROM inventory_movements ORDER BY movement_date ASC, id ASC;"))) {
        return false;
    }

    while (movementQuery.next()) {
        domain::InventoryMovement movement;
        movement.id = movementQuery.value(0).toString();
        movement.productId = movementQuery.value(1).toString();
        movement.lotId = movementQuery.value(2).toString();
        movement.lotNo = movementQuery.value(3).toString();
        movement.movementType = movementQuery.value(4).toString();
        movement.reason = movementQuery.value(5).toString();
        movement.movementDate = movementQuery.value(6).toString();
        movement.qtyDelta = movementQuery.value(7).toInt();
        movement.lotBalanceAfter = movementQuery.value(8).toInt();
        m_inventoryMovements.push_back(movement);
    }

    recomputeSerials();
    return true;
}

bool ProductService::persistState(const QVector<domain::Product> &products,
                                  const QVector<domain::ProductLot> &lots,
                                  const QVector<domain::InventoryMovement> &movements,
                                  QString &errorMessage) const
{
    if (!m_databaseService.isReady()) {
        errorMessage.clear();
        return true;
    }

    if (!m_databaseService.beginTransaction()) {
        errorMessage = m_databaseService.lastError();
        return false;
    }

    auto rollbackWith = [this, &errorMessage](const QString &message) {
        errorMessage = message;
        m_databaseService.rollbackTransaction();
        return false;
    };

    QSqlQuery clearMovementQuery(m_databaseService.database());
    if (!clearMovementQuery.exec(QStringLiteral("DELETE FROM inventory_movements;"))) {
        return rollbackWith(clearMovementQuery.lastError().text());
    }

    QSqlQuery clearLotQuery(m_databaseService.database());
    if (!clearLotQuery.exec(QStringLiteral("DELETE FROM product_lots;"))) {
        return rollbackWith(clearLotQuery.lastError().text());
    }

    QSqlQuery clearProductQuery(m_databaseService.database());
    if (!clearProductQuery.exec(QStringLiteral("DELETE FROM products;"))) {
        return rollbackWith(clearProductQuery.lastError().text());
    }

    QSqlQuery insertProductQuery(m_databaseService.database());
    if (!insertProductQuery.prepare(QStringLiteral(
            "INSERT INTO products("
            "id, sku, name, unit, default_wholesale_price_vnd, is_active"
            ") VALUES("
            ":id, :sku, :name, :unit, :default_wholesale_price_vnd, :is_active"
            ");"))) {
        return rollbackWith(insertProductQuery.lastError().text());
    }

    for (const domain::Product &product : products) {
        insertProductQuery.bindValue(QStringLiteral(":id"), product.id);
        insertProductQuery.bindValue(QStringLiteral(":sku"), product.sku);
        insertProductQuery.bindValue(QStringLiteral(":name"), product.name);
        insertProductQuery.bindValue(QStringLiteral(":unit"), product.unit);
        insertProductQuery.bindValue(QStringLiteral(":default_wholesale_price_vnd"),
                                     product.defaultWholesalePriceVnd);
        insertProductQuery.bindValue(QStringLiteral(":is_active"), product.isActive ? 1 : 0);
        if (!insertProductQuery.exec()) {
            return rollbackWith(insertProductQuery.lastError().text());
        }
    }

    QSqlQuery insertLotQuery(m_databaseService.database());
    if (!insertLotQuery.prepare(QStringLiteral(
            "INSERT INTO product_lots("
            "id, product_id, lot_no, received_date, expiry_date, unit_cost_vnd, on_hand_qty"
            ") VALUES("
            ":id, :product_id, :lot_no, :received_date, :expiry_date, :unit_cost_vnd, :on_hand_qty"
            ");"))) {
        return rollbackWith(insertLotQuery.lastError().text());
    }

    for (const domain::ProductLot &lot : lots) {
        insertLotQuery.bindValue(QStringLiteral(":id"), lot.id);
        insertLotQuery.bindValue(QStringLiteral(":product_id"), lot.productId);
        insertLotQuery.bindValue(QStringLiteral(":lot_no"), lot.lotNo);
        insertLotQuery.bindValue(QStringLiteral(":received_date"), lot.receivedDate);
        insertLotQuery.bindValue(QStringLiteral(":expiry_date"), lot.expiryDate);
        insertLotQuery.bindValue(QStringLiteral(":unit_cost_vnd"), lot.unitCostVnd);
        insertLotQuery.bindValue(QStringLiteral(":on_hand_qty"), lot.onHandQty);
        if (!insertLotQuery.exec()) {
            return rollbackWith(insertLotQuery.lastError().text());
        }
    }

    QSqlQuery insertMovementQuery(m_databaseService.database());
    if (!insertMovementQuery.prepare(QStringLiteral(
            "INSERT INTO inventory_movements("
            "id, product_id, lot_id, lot_no, movement_type, reason, movement_date, qty_delta, lot_balance_after"
            ") VALUES("
            ":id, :product_id, :lot_id, :lot_no, :movement_type, :reason, :movement_date, :qty_delta, :lot_balance_after"
            ");"))) {
        return rollbackWith(insertMovementQuery.lastError().text());
    }

    for (const domain::InventoryMovement &movement : movements) {
        insertMovementQuery.bindValue(QStringLiteral(":id"), movement.id);
        insertMovementQuery.bindValue(QStringLiteral(":product_id"), movement.productId);
        insertMovementQuery.bindValue(QStringLiteral(":lot_id"), movement.lotId);
        insertMovementQuery.bindValue(QStringLiteral(":lot_no"), movement.lotNo);
        insertMovementQuery.bindValue(QStringLiteral(":movement_type"), movement.movementType);
        insertMovementQuery.bindValue(QStringLiteral(":reason"), movement.reason);
        insertMovementQuery.bindValue(QStringLiteral(":movement_date"), movement.movementDate);
        insertMovementQuery.bindValue(QStringLiteral(":qty_delta"), movement.qtyDelta);
        insertMovementQuery.bindValue(QStringLiteral(":lot_balance_after"), movement.lotBalanceAfter);
        if (!insertMovementQuery.exec()) {
            return rollbackWith(insertMovementQuery.lastError().text());
        }
    }

    if (!m_databaseService.commitTransaction()) {
        return rollbackWith(m_databaseService.lastError());
    }

    errorMessage.clear();
    return true;
}

void ProductService::recomputeSerials()
{
    m_nextSkuSerial = nextProductSerial(m_products);
    m_nextLotSerial = nextLotSerial(m_lots);
}

void ProductService::seedSampleData()
{
    QString ignoredError;

    (void)createProduct(ProductInput{QStringLiteral("SP0001"),
                                     QStringLiteral("Gao ST25"),
                                     QStringLiteral("bao"),
                                     420000,
                                     true},
                        ignoredError);
    (void)createProduct(ProductInput{QStringLiteral("SP0002"),
                                     QStringLiteral("Duong Cat"),
                                     QStringLiteral("thung"),
                                     285000,
                                     true},
                        ignoredError);
    (void)createProduct(ProductInput{QStringLiteral("SP0003"),
                                     QStringLiteral("Nuoc Mam"),
                                     QStringLiteral("thung"),
                                     510000,
                                     true},
                        ignoredError);

    if (m_products.size() >= 3) {
        (void)addLot(m_products.at(0).id,
                     ProductLotInput{QStringLiteral("LGAO-2401"),
                                     QStringLiteral("2026-01-12"),
                                     QStringLiteral("2026-03-15"),
                                     120,
                                     395000},
                     ignoredError);
        (void)addLot(m_products.at(0).id,
                     ProductLotInput{QStringLiteral("LGAO-2402"),
                                     QStringLiteral("2026-02-18"),
                                     QStringLiteral("2026-10-15"),
                                     80,
                                     405000},
                     ignoredError);
        (void)addLot(m_products.at(1).id,
                     ProductLotInput{QStringLiteral("LDUONG-2501"),
                                     QStringLiteral("2026-02-02"),
                                     QStringLiteral("2026-03-01"),
                                     36,
                                     260000},
                     ignoredError);
        (void)addLot(m_products.at(2).id,
                     ProductLotInput{QStringLiteral("LNM-0101"),
                                     QStringLiteral("2026-01-20"),
                                     QStringLiteral("2027-01-31"),
                                     8,
                                     468000},
                     ignoredError);
    }
}

} // namespace stockmaster::application
