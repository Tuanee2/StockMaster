#include "stockmaster/ui/viewmodels/products_view_model.h"

#include <QDate>
#include <QLocale>
#include <QRegularExpression>

namespace stockmaster::ui::viewmodels {

ProductsViewModel::ProductsViewModel(application::ProductService &productService,
                                     QObject *parent)
    : QAbstractListModel(parent)
    , m_productService(productService)
{
    refreshModel();
}

int ProductsViewModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }

    return m_rows.size();
}

QVariant ProductsViewModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_rows.size()) {
        return {};
    }

    const domain::Product &product = m_rows.at(index.row());

    switch (role) {
    case ProductIdRole:
        return product.id;
    case SkuRole:
        return product.sku;
    case NameRole:
        return product.name;
    case UnitRole:
        return product.unit;
    case DefaultWholesalePriceVndRole:
        return product.defaultWholesalePriceVnd;
    case DefaultWholesalePriceTextRole:
        return formatMoneyVnd(product.defaultWholesalePriceVnd);
    case IsActiveRole:
        return product.isActive;
    case TotalOnHandRole:
        return m_productService.totalOnHandByProduct(product.id);
    case LotCountRole:
        return m_productService.lotCountByProduct(product.id);
    default:
        return {};
    }
}

QHash<int, QByteArray> ProductsViewModel::roleNames() const
{
    return {
        {ProductIdRole, QByteArrayLiteral("productId")},
        {SkuRole, QByteArrayLiteral("sku")},
        {NameRole, QByteArrayLiteral("name")},
        {UnitRole, QByteArrayLiteral("unit")},
        {DefaultWholesalePriceVndRole, QByteArrayLiteral("defaultWholesalePriceVnd")},
        {DefaultWholesalePriceTextRole, QByteArrayLiteral("defaultWholesalePriceText")},
        {IsActiveRole, QByteArrayLiteral("isActive")},
        {TotalOnHandRole, QByteArrayLiteral("totalOnHand")},
        {LotCountRole, QByteArrayLiteral("lotCount")},
    };
}

QString ProductsViewModel::filterText() const
{
    return m_filterText;
}

void ProductsViewModel::setFilterText(const QString &value)
{
    if (m_filterText == value) {
        return;
    }

    m_filterText = value;
    emit filterTextChanged();
    refreshModel();
}

QString ProductsViewModel::lastError() const
{
    return m_lastError;
}

int ProductsViewModel::productCount() const
{
    return m_rows.size();
}

QString ProductsViewModel::selectedProductId() const
{
    return m_selectedProductId;
}

QString ProductsViewModel::selectedProductName() const
{
    if (m_selectedProductId.isEmpty()) {
        return QString();
    }

    for (const domain::Product &product : m_rows) {
        if (product.id == m_selectedProductId) {
            return product.name;
        }
    }

    const QVector<domain::Product> allProducts = m_productService.findProducts();
    for (const domain::Product &product : allProducts) {
        if (product.id == m_selectedProductId) {
            return product.name;
        }
    }

    return QString();
}

int ProductsViewModel::selectedProductTotalOnHand() const
{
    if (m_selectedProductId.isEmpty()) {
        return 0;
    }

    return m_productService.totalOnHandByProduct(m_selectedProductId);
}

int ProductsViewModel::selectedProductExpiringSoonLotCount() const
{
    return m_selectedProductExpiringSoonLotCount;
}

QVariantList ProductsViewModel::selectedProductLots() const
{
    return m_selectedProductLots;
}

bool ProductsViewModel::hasSelectedProduct() const
{
    return !m_selectedProductId.isEmpty();
}

bool ProductsViewModel::createProduct(const QString &sku,
                                      const QString &name,
                                      const QString &unit,
                                      const QString &defaultPriceText,
                                      bool isActive)
{
    core::Money defaultPriceVnd = 0;
    if (!parseMoneyInput(defaultPriceText, defaultPriceVnd)) {
        setLastError(QStringLiteral("Giá sỉ mặc định không hợp lệ. Chỉ nhập số nguyên >= 0."));
        return false;
    }

    QString errorMessage;
    const bool success = m_productService.createProduct(
        application::ProductInput{sku, name, unit, defaultPriceVnd, isActive},
        errorMessage);

    if (!success) {
        setLastError(errorMessage);
        return false;
    }

    setLastError(QString());
    refreshModel();
    return true;
}

bool ProductsViewModel::updateProduct(const QString &productId,
                                      const QString &sku,
                                      const QString &name,
                                      const QString &unit,
                                      const QString &defaultPriceText,
                                      bool isActive)
{
    core::Money defaultPriceVnd = 0;
    if (!parseMoneyInput(defaultPriceText, defaultPriceVnd)) {
        setLastError(QStringLiteral("Giá sỉ mặc định không hợp lệ. Chỉ nhập số nguyên >= 0."));
        return false;
    }

    QString errorMessage;
    const bool success = m_productService.updateProduct(
        productId,
        application::ProductInput{sku, name, unit, defaultPriceVnd, isActive},
        errorMessage);

    if (!success) {
        setLastError(errorMessage);
        return false;
    }

    setLastError(QString());
    refreshModel();
    refreshSelectedLots();
    return true;
}

bool ProductsViewModel::deleteProduct(const QString &productId)
{
    QString errorMessage;
    const bool success = m_productService.deleteProduct(productId, errorMessage);

    if (!success) {
        setLastError(errorMessage);
        return false;
    }

    if (m_selectedProductId == productId) {
        m_selectedProductId.clear();
        emit selectedProductChanged();
    }

    setLastError(QString());
    refreshModel();
    refreshSelectedLots();
    return true;
}

bool ProductsViewModel::addLotToSelectedProduct(const QString &lotNo,
                                                const QString &receivedDate,
                                                const QString &expiryDate,
                                                const QString &qtyText,
                                                const QString &unitCostText)
{
    if (m_selectedProductId.isEmpty()) {
        setLastError(QStringLiteral("Bạn cần chọn sản phẩm trước khi thêm lô."));
        return false;
    }

    int qty = 0;
    if (!parsePositiveInt(qtyText, qty)) {
        setLastError(QStringLiteral("Số lượng nhập lô không hợp lệ."));
        return false;
    }

    core::Money unitCostVnd = 0;
    if (!parseMoneyInput(unitCostText, unitCostVnd)) {
        setLastError(QStringLiteral("Giá vốn lô không hợp lệ."));
        return false;
    }

    QString errorMessage;
    const bool success = m_productService.addLot(
        m_selectedProductId,
        application::ProductLotInput{lotNo, receivedDate, expiryDate, qty, unitCostVnd},
        errorMessage);

    if (!success) {
        setLastError(errorMessage);
        return false;
    }

    setLastError(QString());
    refreshModel();
    refreshSelectedLots();
    return true;
}

bool ProductsViewModel::stockInLot(const QString &lotId, const QString &qtyText)
{
    int qty = 0;
    if (!parsePositiveInt(qtyText, qty)) {
        setLastError(QStringLiteral("Số lượng nhập không hợp lệ."));
        return false;
    }

    QString errorMessage;
    const bool success = m_productService.stockIn(lotId, qty, errorMessage);

    if (!success) {
        setLastError(errorMessage);
        return false;
    }

    setLastError(QString());
    refreshModel();
    refreshSelectedLots();
    return true;
}

bool ProductsViewModel::stockOutLot(const QString &lotId, const QString &qtyText)
{
    int qty = 0;
    if (!parsePositiveInt(qtyText, qty)) {
        setLastError(QStringLiteral("Số lượng xuất không hợp lệ."));
        return false;
    }

    QString errorMessage;
    const bool success = m_productService.stockOut(lotId, qty, errorMessage);

    if (!success) {
        setLastError(errorMessage);
        return false;
    }

    setLastError(QString());
    refreshModel();
    refreshSelectedLots();
    return true;
}

QVariantMap ProductsViewModel::getProduct(int row) const
{
    if (row < 0 || row >= m_rows.size()) {
        return {};
    }

    const domain::Product &product = m_rows.at(row);

    return {
        {QStringLiteral("productId"), product.id},
        {QStringLiteral("sku"), product.sku},
        {QStringLiteral("name"), product.name},
        {QStringLiteral("unit"), product.unit},
        {QStringLiteral("defaultWholesalePriceVnd"), product.defaultWholesalePriceVnd},
        {QStringLiteral("defaultWholesalePriceText"), QString::number(product.defaultWholesalePriceVnd)},
        {QStringLiteral("isActive"), product.isActive},
    };
}

void ProductsViewModel::selectProduct(const QString &productId)
{
    const QString normalizedId = productId.trimmed();
    if (m_selectedProductId == normalizedId) {
        return;
    }

    m_selectedProductId = normalizedId;
    emit selectedProductChanged();
    refreshSelectedLots();
}

void ProductsViewModel::reload()
{
    refreshModel();
    refreshSelectedLots();
}

bool ProductsViewModel::parseMoneyInput(const QString &input, core::Money &value) const
{
    QString normalized = input.trimmed();
    if (normalized.isEmpty()) {
        value = 0;
        return true;
    }

    normalized.remove(QRegularExpression(QStringLiteral("[\\s.,_]")));

    if (normalized.startsWith(QLatin1Char('-'))) {
        return false;
    }

    bool ok = false;
    const qlonglong parsed = normalized.toLongLong(&ok);
    if (!ok || parsed < 0) {
        return false;
    }

    value = parsed;
    return true;
}

bool ProductsViewModel::parsePositiveInt(const QString &input, int &value) const
{
    QString normalized = input.trimmed();
    normalized.remove(QRegularExpression(QStringLiteral("[\\s.,_]")));

    bool ok = false;
    const int parsed = normalized.toInt(&ok);
    if (!ok || parsed <= 0) {
        return false;
    }

    value = parsed;
    return true;
}

QString ProductsViewModel::formatMoneyVnd(core::Money amountVnd)
{
    const QLocale locale(QLocale::Vietnamese, QLocale::Vietnam);
    return locale.toString(amountVnd) + QStringLiteral(" VND");
}

void ProductsViewModel::setLastError(const QString &message)
{
    if (m_lastError == message) {
        return;
    }

    m_lastError = message;
    emit lastErrorChanged();
}

void ProductsViewModel::refreshModel()
{
    const int previousCount = m_rows.size();

    beginResetModel();
    m_rows = m_productService.findProducts(m_filterText);
    endResetModel();

    if (previousCount != m_rows.size()) {
        emit productCountChanged();
    }

    if (!m_selectedProductId.isEmpty() && !selectedProductExists()) {
        m_selectedProductId.clear();
        emit selectedProductChanged();
    } else if (!m_selectedProductId.isEmpty()) {
        emit selectedProductChanged();
    }
}

void ProductsViewModel::refreshSelectedLots()
{
    QVariantList newLots;
    int expiringSoonCount = 0;
    const QDate today = QDate::currentDate();

    if (!m_selectedProductId.isEmpty()) {
        const QVector<domain::ProductLot> lots = m_productService.findLotsByProduct(m_selectedProductId);
        newLots.reserve(lots.size());

        for (const domain::ProductLot &lot : lots) {
            bool isExpired = false;
            bool isExpiringSoon = false;
            int daysToExpiry = -1;

            const QDate expiryDate = QDate::fromString(lot.expiryDate, Qt::ISODate);
            if (expiryDate.isValid() && lot.onHandQty > 0) {
                daysToExpiry = today.daysTo(expiryDate);
                isExpired = daysToExpiry < 0;
                isExpiringSoon = !isExpired && daysToExpiry <= 30;
            }

            if (isExpiringSoon) {
                ++expiringSoonCount;
            }

            newLots.push_back(QVariantMap{
                {QStringLiteral("lotId"), lot.id},
                {QStringLiteral("lotNo"), lot.lotNo},
                {QStringLiteral("receivedDate"), lot.receivedDate},
                {QStringLiteral("expiryDate"), lot.expiryDate},
                {QStringLiteral("onHandQty"), lot.onHandQty},
                {QStringLiteral("unitCostVnd"), lot.unitCostVnd},
                {QStringLiteral("unitCostText"), formatMoneyVnd(lot.unitCostVnd)},
                {QStringLiteral("daysToExpiry"), daysToExpiry},
                {QStringLiteral("isExpiringSoon"), isExpiringSoon},
                {QStringLiteral("isExpired"), isExpired},
            });
        }
    }

    m_selectedProductLots = newLots;
    m_selectedProductExpiringSoonLotCount = expiringSoonCount;
    emit selectedProductLotsChanged();
}

bool ProductsViewModel::selectedProductExists() const
{
    if (m_selectedProductId.isEmpty()) {
        return false;
    }

    for (const domain::Product &product : m_rows) {
        if (product.id == m_selectedProductId) {
            return true;
        }
    }

    const QVector<domain::Product> allProducts = m_productService.findProducts();
    for (const domain::Product &product : allProducts) {
        if (product.id == m_selectedProductId) {
            return true;
        }
    }

    return false;
}

} // namespace stockmaster::ui::viewmodels
