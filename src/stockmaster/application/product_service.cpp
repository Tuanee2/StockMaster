#include "stockmaster/application/product_service.h"

#include <QDate>
#include <QRegularExpression>
#include <QSet>
#include <QUuid>

#include <algorithm>

namespace stockmaster::application {

ProductService::ProductService()
{
    seedSampleData();
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

    QString sku = normalizedSku(input.sku);
    if (sku.isEmpty()) {
        do {
            sku = generateDefaultSku(m_nextSkuSerial++);
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

    m_products.push_back(product);

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

    for (domain::Product &product : m_products) {
        if (product.id != normalizedId) {
            continue;
        }

        product.sku = sku;
        product.name = name;
        product.unit = unit;
        product.defaultWholesalePriceVnd = input.defaultWholesalePriceVnd;
        product.isActive = input.isActive;

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

    for (qsizetype index = 0; index < m_products.size(); ++index) {
        if (m_products[index].id != normalizedId) {
            continue;
        }

        m_products.remove(index);

        for (qsizetype lotIndex = m_lots.size() - 1; lotIndex >= 0; --lotIndex) {
            if (m_lots[lotIndex].productId == normalizedId) {
                m_lots.remove(lotIndex);
            }

            if (lotIndex == 0) {
                break;
            }
        }

        m_inventoryMovements.erase(
            std::remove_if(m_inventoryMovements.begin(),
                           m_inventoryMovements.end(),
                           [&normalizedId](const domain::InventoryMovement &movement) {
                               return movement.productId == normalizedId;
                           }),
            m_inventoryMovements.end());

        errorMessage.clear();
        return true;
    }

    errorMessage = QStringLiteral("Không tìm thấy sản phẩm cần xóa.");
    return false;
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

    QString lotNo = normalizedLotNo(input.lotNo);
    if (lotNo.isEmpty()) {
        do {
            lotNo = generateDefaultLotNo(m_nextLotSerial++);
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

    m_lots.push_back(lot);
    appendInventoryMovement(lot,
                            lot.onHandQty,
                            QStringLiteral("LotCreated"),
                            QStringLiteral("Tạo lô ban đầu"),
                            receivedDate);

    errorMessage.clear();
    return true;
}

bool ProductService::stockIn(const QString &lotId,
                             int qty,
                             QString &errorMessage,
                             const QString &reason,
                             const QString &movementDate,
                             bool recordMovement)
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

    qsizetype lotIndex = -1;
    if (!findLotIndex(normalizedLotId, lotIndex)) {
        errorMessage = QStringLiteral("Không tìm thấy lô để nhập hàng.");
        return false;
    }

    domain::ProductLot &lot = m_lots[lotIndex];
    lot.onHandQty += qty;

    if (recordMovement) {
        appendInventoryMovement(lot,
                                qty,
                                QStringLiteral("StockIn"),
                                reason.trimmed().isEmpty() ? QStringLiteral("Nhập kho") : reason.trimmed(),
                                movementDate);
    }

    errorMessage.clear();
    return true;
}

bool ProductService::stockOut(const QString &lotId,
                              int qty,
                              QString &errorMessage,
                              const QString &reason,
                              const QString &movementDate,
                              bool recordMovement)
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

    qsizetype lotIndex = -1;
    if (!findLotIndex(normalizedLotId, lotIndex)) {
        errorMessage = QStringLiteral("Không tìm thấy lô để xuất hàng.");
        return false;
    }

    domain::ProductLot &lot = m_lots[lotIndex];
    if (lot.onHandQty < qty) {
        errorMessage = QStringLiteral("Không đủ tồn trong lô để xuất.");
        return false;
    }

    lot.onHandQty -= qty;

    if (recordMovement) {
        appendInventoryMovement(lot,
                                -qty,
                                QStringLiteral("StockOut"),
                                reason.trimmed().isEmpty() ? QStringLiteral("Xuất kho") : reason.trimmed(),
                                movementDate);
    }

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

    qsizetype lotIndex = -1;
    if (!findLotIndex(normalizedLotId, lotIndex)) {
        errorMessage = QStringLiteral("Không tìm thấy lô để điều chỉnh tồn.");
        return false;
    }

    domain::ProductLot &lot = m_lots[lotIndex];
    const int nextOnHand = lot.onHandQty + qtyDelta;
    if (nextOnHand < 0) {
        errorMessage = QStringLiteral("Không thể điều chỉnh làm tồn kho âm.");
        return false;
    }

    lot.onHandQty = nextOnHand;
    appendInventoryMovement(lot,
                            qtyDelta,
                            qtyDelta > 0 ? QStringLiteral("AdjustUp") : QStringLiteral("AdjustDown"),
                            normalizedReason,
                            normalizedDate);

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

void ProductService::appendInventoryMovement(const domain::ProductLot &lot,
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

    m_inventoryMovements.push_back(movement);
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

    m_nextSkuSerial = m_products.size() + 1;
}

} // namespace stockmaster::application
