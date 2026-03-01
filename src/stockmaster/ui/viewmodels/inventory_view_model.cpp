#include "stockmaster/ui/viewmodels/inventory_view_model.h"

#include <QRegularExpression>

namespace stockmaster::ui::viewmodels {

namespace {
constexpr int kLowStockThreshold = 20;
}

InventoryViewModel::InventoryViewModel(application::ProductService &productService,
                                       QObject *parent)
    : QAbstractListModel(parent)
    , m_productService(productService)
{
    refreshModel();
}

int InventoryViewModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }

    return m_rows.size();
}

QVariant InventoryViewModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_rows.size()) {
        return {};
    }

    const domain::Product &product = m_rows.at(index.row());
    const int totalOnHand = m_productService.totalOnHandByProduct(product.id);

    switch (role) {
    case ProductIdRole:
        return product.id;
    case SkuRole:
        return product.sku;
    case NameRole:
        return product.name;
    case UnitRole:
        return product.unit;
    case TotalOnHandRole:
        return totalOnHand;
    case LotCountRole:
        return m_productService.lotCountByProduct(product.id);
    case IsLowStockRole:
        return product.isActive && totalOnHand <= kLowStockThreshold;
    default:
        return {};
    }
}

QHash<int, QByteArray> InventoryViewModel::roleNames() const
{
    return {
        {ProductIdRole, QByteArrayLiteral("productId")},
        {SkuRole, QByteArrayLiteral("sku")},
        {NameRole, QByteArrayLiteral("name")},
        {UnitRole, QByteArrayLiteral("unit")},
        {TotalOnHandRole, QByteArrayLiteral("totalOnHand")},
        {LotCountRole, QByteArrayLiteral("lotCount")},
        {IsLowStockRole, QByteArrayLiteral("isLowStock")},
    };
}

QString InventoryViewModel::filterText() const
{
    return m_filterText;
}

void InventoryViewModel::setFilterText(const QString &value)
{
    if (m_filterText == value) {
        return;
    }

    m_filterText = value;
    emit filterTextChanged();
    refreshModel();
}

QString InventoryViewModel::lastError() const
{
    return m_lastError;
}

int InventoryViewModel::productCount() const
{
    return m_rows.size();
}

int InventoryViewModel::lowStockCount() const
{
    int count = 0;
    for (const domain::Product &product : m_rows) {
        if (product.isActive && m_productService.totalOnHandByProduct(product.id) <= kLowStockThreshold) {
            ++count;
        }
    }

    return count;
}

bool InventoryViewModel::hasSelectedProduct() const
{
    return m_hasSelectedProduct;
}

QString InventoryViewModel::selectedProductId() const
{
    return m_selectedProductId;
}

QString InventoryViewModel::selectedProductName() const
{
    return m_selectedProductName;
}

int InventoryViewModel::selectedProductTotalOnHand() const
{
    return m_selectedProductTotalOnHand;
}

bool InventoryViewModel::selectedProductLowStock() const
{
    return m_selectedProductLowStock;
}

QVariantList InventoryViewModel::selectedProductLots() const
{
    return m_selectedProductLots;
}

QVariantList InventoryViewModel::selectedProductMovements() const
{
    return m_selectedProductMovements;
}

void InventoryViewModel::reload()
{
    refreshModel();
}

void InventoryViewModel::selectProduct(const QString &productId)
{
    const QString normalizedId = productId.trimmed();
    if (m_hasSelectedProduct && m_selectedProductId == normalizedId) {
        return;
    }

    if (normalizedId.isEmpty()) {
        clearSelection();
        return;
    }

    m_hasSelectedProduct = true;
    m_selectedProductId = normalizedId;
    refreshSelectedData();
}

bool InventoryViewModel::adjustSelectedLot(const QString &lotId,
                                           const QString &qtyDeltaText,
                                           const QString &reason,
                                           const QString &adjustmentDate)
{
    if (!m_hasSelectedProduct) {
        setLastError(QStringLiteral("Chưa chọn sản phẩm để điều chỉnh tồn."));
        return false;
    }

    int qtyDelta = 0;
    if (!parseSignedInt(qtyDeltaText, qtyDelta) || qtyDelta == 0) {
        setLastError(QStringLiteral("Số lượng điều chỉnh không hợp lệ. Dùng số nguyên khác 0, có thể âm hoặc dương."));
        return false;
    }

    QString errorMessage;
    const bool success = m_productService.adjustLotQuantity(lotId,
                                                            qtyDelta,
                                                            reason,
                                                            adjustmentDate,
                                                            errorMessage);
    if (!success) {
        setLastError(errorMessage);
        return false;
    }

    setLastError(QString());
    refreshModel();
    refreshSelectedData();
    return true;
}

bool InventoryViewModel::parseSignedInt(const QString &input, int &value) const
{
    QString normalized = input.trimmed();
    if (normalized.isEmpty()) {
        return false;
    }

    normalized.remove(QRegularExpression(QStringLiteral("[\\s,_]")));

    bool ok = false;
    const int parsed = normalized.toInt(&ok);
    if (!ok) {
        return false;
    }

    value = parsed;
    return true;
}

QString InventoryViewModel::movementTypeLabel(const QString &movementType) const
{
    if (movementType == QLatin1String("LotCreated")) {
        return QStringLiteral("Tạo lô");
    }
    if (movementType == QLatin1String("StockIn")) {
        return QStringLiteral("Nhập kho");
    }
    if (movementType == QLatin1String("StockOut")) {
        return QStringLiteral("Xuất kho");
    }
    if (movementType == QLatin1String("AdjustUp")) {
        return QStringLiteral("Điều chỉnh +");
    }
    if (movementType == QLatin1String("AdjustDown")) {
        return QStringLiteral("Điều chỉnh -");
    }

    return movementType;
}

void InventoryViewModel::setLastError(const QString &message)
{
    if (m_lastError == message) {
        return;
    }

    m_lastError = message;
    emit lastErrorChanged();
}

void InventoryViewModel::refreshModel()
{
    beginResetModel();
    m_rows = m_productService.findProducts(m_filterText);
    endResetModel();

    emit productCountChanged();

    if (!m_hasSelectedProduct) {
        if (!m_rows.isEmpty()) {
            m_hasSelectedProduct = true;
            m_selectedProductId = m_rows.first().id;
            refreshSelectedData();
        } else {
            clearSelection();
        }
        return;
    }

    bool selectedVisible = false;
    for (const domain::Product &product : m_rows) {
        if (product.id == m_selectedProductId) {
            selectedVisible = true;
            break;
        }
    }

    if (!selectedVisible && !m_rows.isEmpty()) {
        m_selectedProductId = m_rows.first().id;
        refreshSelectedData();
    } else if (!selectedVisible) {
        clearSelection();
    } else {
        refreshSelectedData();
    }
}

void InventoryViewModel::refreshSelectedData()
{
    if (!m_hasSelectedProduct || m_selectedProductId.isEmpty()) {
        clearSelection();
        return;
    }

    domain::Product selectedProduct;
    bool found = false;

    QVector<domain::Product> allProducts = m_productService.findProducts();
    for (const domain::Product &product : allProducts) {
        if (product.id == m_selectedProductId) {
            selectedProduct = product;
            found = true;
            break;
        }
    }

    if (!found) {
        clearSelection();
        return;
    }

    m_hasSelectedProduct = true;
    m_selectedProductName = selectedProduct.sku + QStringLiteral(" - ") + selectedProduct.name;
    m_selectedProductTotalOnHand = m_productService.totalOnHandByProduct(selectedProduct.id);
    m_selectedProductLowStock = selectedProduct.isActive && m_selectedProductTotalOnHand <= kLowStockThreshold;

    QVariantList lots;
    const QVector<domain::ProductLot> productLots = m_productService.findLotsByProduct(selectedProduct.id);
    lots.reserve(productLots.size());
    for (const domain::ProductLot &lot : productLots) {
        lots.push_back(QVariantMap{
            {QStringLiteral("lotId"), lot.id},
            {QStringLiteral("lotNo"), lot.lotNo},
            {QStringLiteral("receivedDate"), lot.receivedDate},
            {QStringLiteral("expiryDate"), lot.expiryDate},
            {QStringLiteral("onHandQty"), lot.onHandQty},
            {QStringLiteral("unitCostVnd"), lot.unitCostVnd},
            {QStringLiteral("label"), lot.lotNo + QStringLiteral(" | Tồn: ") + QString::number(lot.onHandQty)},
        });
    }

    QVariantList movements;
    const QVector<domain::InventoryMovement> movementRows = m_productService.findInventoryMovementsByProduct(selectedProduct.id);
    movements.reserve(movementRows.size());
    for (const domain::InventoryMovement &movement : movementRows) {
        movements.push_back(QVariantMap{
            {QStringLiteral("movementId"), movement.id},
            {QStringLiteral("lotId"), movement.lotId},
            {QStringLiteral("lotNo"), movement.lotNo},
            {QStringLiteral("movementType"), movementTypeLabel(movement.movementType)},
            {QStringLiteral("reason"), movement.reason},
            {QStringLiteral("movementDate"), movement.movementDate},
            {QStringLiteral("qtyDelta"), movement.qtyDelta},
            {QStringLiteral("lotBalanceAfter"), movement.lotBalanceAfter},
            {QStringLiteral("isPositive"), movement.qtyDelta > 0},
        });
    }

    m_selectedProductLots = lots;
    m_selectedProductMovements = movements;

    emit selectedProductChanged();
    emit detailChanged();
}

void InventoryViewModel::clearSelection()
{
    const bool hadSelection = m_hasSelectedProduct || !m_selectedProductLots.isEmpty() || !m_selectedProductMovements.isEmpty();

    m_hasSelectedProduct = false;
    m_selectedProductId.clear();
    m_selectedProductName.clear();
    m_selectedProductTotalOnHand = 0;
    m_selectedProductLowStock = false;
    m_selectedProductLots.clear();
    m_selectedProductMovements.clear();

    if (hadSelection) {
        emit selectedProductChanged();
        emit detailChanged();
    }
}

} // namespace stockmaster::ui::viewmodels
