#include "stockmaster/infra/db/database_service.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QUuid>

namespace stockmaster::infra::db {

namespace {

constexpr int kSchemaVersion = 1;

}

DatabaseService::DatabaseService(const QString &databaseFilePath)
    : m_requestedDatabaseFilePath(databaseFilePath.trimmed())
    , m_connectionName(QUuid::createUuid().toString(QUuid::WithoutBraces))
{
}

DatabaseService::~DatabaseService()
{
    if (!m_connectionName.isEmpty()) {
        if (m_database.isValid()) {
            m_database.close();
            m_database = QSqlDatabase();
        }

        QSqlDatabase::removeDatabase(m_connectionName);
    }
}

bool DatabaseService::initialize()
{
    if (m_ready) {
        return true;
    }

    if (!openConnection()) {
        return false;
    }

    m_resolvedDatabaseFilePath = resolveDatabaseFilePath();
    if (!ensureSchema()) {
        m_database.close();
        m_database = QSqlDatabase();
        QSqlDatabase::removeDatabase(m_connectionName);
        m_connectionName.clear();
        return false;
    }

    m_ready = true;
    m_lastError.clear();
    return true;
}

bool DatabaseService::isReady() const
{
    return m_ready;
}

QString DatabaseService::backendName() const
{
    return QStringLiteral("SQLite (local-first)");
}

QString DatabaseService::databaseFilePath() const
{
    return m_resolvedDatabaseFilePath;
}

QString DatabaseService::lastError() const
{
    return m_lastError;
}

QSqlDatabase DatabaseService::database() const
{
    return m_database;
}

bool DatabaseService::beginTransaction()
{
    if (!m_ready || !m_database.isOpen()) {
        m_lastError = QStringLiteral("Database chưa sẵn sàng.");
        return false;
    }

    if (m_transactionDepth == 0 && !m_database.transaction()) {
        m_lastError = m_database.lastError().text();
        return false;
    }

    ++m_transactionDepth;
    return true;
}

bool DatabaseService::commitTransaction()
{
    if (m_transactionDepth <= 0) {
        return true;
    }

    --m_transactionDepth;
    if (m_transactionDepth > 0) {
        return true;
    }

    if (!m_database.commit()) {
        m_lastError = m_database.lastError().text();
        m_transactionDepth = 0;
        return false;
    }

    m_lastError.clear();
    return true;
}

void DatabaseService::rollbackTransaction()
{
    if (m_transactionDepth <= 0) {
        return;
    }

    m_transactionDepth = 0;
    if (m_database.isOpen()) {
        if (!m_database.rollback()) {
            m_lastError = m_database.lastError().text();
        }
    }
}

QString DatabaseService::resolveDatabaseFilePath() const
{
    if (!m_requestedDatabaseFilePath.isEmpty()) {
        return m_requestedDatabaseFilePath;
    }

    const QString envPath = qEnvironmentVariable("STOCKMASTER_DB_PATH").trimmed();
    if (!envPath.isEmpty()) {
        return envPath;
    }

    QString appDataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (appDataDir.isEmpty()) {
        appDataDir = QDir::currentPath();
    }

    return QDir(appDataDir).filePath(QStringLiteral("stockmaster.sqlite"));
}

bool DatabaseService::openConnection()
{
    if (!m_connectionName.isEmpty() && QSqlDatabase::contains(m_connectionName)) {
        m_database = QSqlDatabase::database(m_connectionName);
        if (m_database.isOpen()) {
            return true;
        }
    } else if (m_connectionName.isEmpty()) {
        m_connectionName = QUuid::createUuid().toString(QUuid::WithoutBraces);
    }

    const QString filePath = resolveDatabaseFilePath();
    if (filePath != QLatin1String(":memory:")) {
        const QFileInfo dbInfo(filePath);
        QDir directory = dbInfo.dir();
        if (!directory.exists() && !directory.mkpath(QStringLiteral("."))) {
            m_lastError = QStringLiteral("Không thể tạo thư mục DB local.");
            return false;
        }
    }

    if (!m_database.isValid()) {
        m_database = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), m_connectionName);
    }

    m_database.setDatabaseName(filePath);
    if (!m_database.open()) {
        m_lastError = m_database.lastError().text();
        return false;
    }

    m_lastError.clear();
    return true;
}

bool DatabaseService::ensureSchema()
{
    if (!executeStatement(QStringLiteral("PRAGMA journal_mode = WAL;"))) {
        return false;
    }

    if (!executeStatement(QStringLiteral(
            "CREATE TABLE IF NOT EXISTS schema_meta ("
            "key TEXT PRIMARY KEY,"
            "value TEXT NOT NULL"
            ");"))) {
        return false;
    }

    if (!executeStatement(QStringLiteral(
            "CREATE TABLE IF NOT EXISTS customers ("
            "id TEXT PRIMARY KEY,"
            "code TEXT NOT NULL UNIQUE,"
            "name TEXT NOT NULL,"
            "phone TEXT NOT NULL DEFAULT '',"
            "address TEXT NOT NULL DEFAULT '',"
            "credit_limit_vnd INTEGER NOT NULL DEFAULT 0"
            ");"))) {
        return false;
    }

    if (!executeStatement(QStringLiteral(
            "CREATE TABLE IF NOT EXISTS products ("
            "id TEXT PRIMARY KEY,"
            "sku TEXT NOT NULL UNIQUE,"
            "name TEXT NOT NULL,"
            "unit TEXT NOT NULL DEFAULT '',"
            "default_wholesale_price_vnd INTEGER NOT NULL DEFAULT 0,"
            "is_active INTEGER NOT NULL DEFAULT 1"
            ");"))) {
        return false;
    }

    if (!executeStatement(QStringLiteral(
            "CREATE TABLE IF NOT EXISTS product_lots ("
            "id TEXT PRIMARY KEY,"
            "product_id TEXT NOT NULL,"
            "lot_no TEXT NOT NULL,"
            "received_date TEXT NOT NULL,"
            "expiry_date TEXT NOT NULL,"
            "unit_cost_vnd INTEGER NOT NULL DEFAULT 0,"
            "on_hand_qty INTEGER NOT NULL DEFAULT 0,"
            "UNIQUE(product_id, lot_no)"
            ");"))) {
        return false;
    }

    if (!executeStatement(QStringLiteral(
            "CREATE TABLE IF NOT EXISTS inventory_movements ("
            "id TEXT PRIMARY KEY,"
            "product_id TEXT NOT NULL,"
            "lot_id TEXT NOT NULL,"
            "lot_no TEXT NOT NULL,"
            "movement_type TEXT NOT NULL,"
            "reason TEXT NOT NULL DEFAULT '',"
            "movement_date TEXT NOT NULL,"
            "qty_delta INTEGER NOT NULL DEFAULT 0,"
            "lot_balance_after INTEGER NOT NULL DEFAULT 0"
            ");"))) {
        return false;
    }

    if (!executeStatement(QStringLiteral(
            "CREATE TABLE IF NOT EXISTS orders ("
            "id TEXT PRIMARY KEY,"
            "order_no TEXT NOT NULL UNIQUE,"
            "customer_id TEXT NOT NULL,"
            "order_date TEXT NOT NULL,"
            "status TEXT NOT NULL,"
            "subtotal_vnd INTEGER NOT NULL DEFAULT 0,"
            "discount_vnd INTEGER NOT NULL DEFAULT 0,"
            "total_vnd INTEGER NOT NULL DEFAULT 0,"
            "paid_vnd INTEGER NOT NULL DEFAULT 0,"
            "balance_vnd INTEGER NOT NULL DEFAULT 0"
            ");"))) {
        return false;
    }

    if (!executeStatement(QStringLiteral(
            "CREATE TABLE IF NOT EXISTS order_items ("
            "id TEXT PRIMARY KEY,"
            "order_id TEXT NOT NULL,"
            "product_id TEXT NOT NULL,"
            "qty INTEGER NOT NULL DEFAULT 0,"
            "unit_price_vnd INTEGER NOT NULL DEFAULT 0,"
            "discount_vnd INTEGER NOT NULL DEFAULT 0,"
            "line_total_vnd INTEGER NOT NULL DEFAULT 0"
            ");"))) {
        return false;
    }

    if (!executeStatement(QStringLiteral(
            "CREATE TABLE IF NOT EXISTS stock_allocations ("
            "order_id TEXT NOT NULL,"
            "order_item_id TEXT NOT NULL,"
            "lot_id TEXT NOT NULL,"
            "qty INTEGER NOT NULL DEFAULT 0,"
            "PRIMARY KEY (order_id, order_item_id, lot_id)"
            ");"))) {
        return false;
    }

    if (!executeStatement(QStringLiteral(
            "CREATE TABLE IF NOT EXISTS payments ("
            "id TEXT PRIMARY KEY,"
            "payment_no TEXT NOT NULL UNIQUE,"
            "customer_id TEXT NOT NULL,"
            "order_id TEXT NOT NULL,"
            "amount_vnd INTEGER NOT NULL DEFAULT 0,"
            "method TEXT NOT NULL DEFAULT '',"
            "paid_at TEXT NOT NULL"
            ");"))) {
        return false;
    }

    if (!executeStatement(QStringLiteral(
            "CREATE INDEX IF NOT EXISTS idx_product_lots_product_id "
            "ON product_lots(product_id);"))) {
        return false;
    }

    if (!executeStatement(QStringLiteral(
            "CREATE INDEX IF NOT EXISTS idx_inventory_movements_product_date "
            "ON inventory_movements(product_id, movement_date DESC);"))) {
        return false;
    }

    if (!executeStatement(QStringLiteral(
            "CREATE INDEX IF NOT EXISTS idx_orders_customer_status "
            "ON orders(customer_id, status);"))) {
        return false;
    }

    if (!executeStatement(QStringLiteral(
            "CREATE INDEX IF NOT EXISTS idx_orders_order_date "
            "ON orders(order_date DESC);"))) {
        return false;
    }

    if (!executeStatement(QStringLiteral(
            "CREATE INDEX IF NOT EXISTS idx_order_items_order_id "
            "ON order_items(order_id);"))) {
        return false;
    }

    if (!executeStatement(QStringLiteral(
            "CREATE INDEX IF NOT EXISTS idx_stock_allocations_order_id "
            "ON stock_allocations(order_id);"))) {
        return false;
    }

    if (!executeStatement(QStringLiteral(
            "CREATE INDEX IF NOT EXISTS idx_payments_customer_paid_at "
            "ON payments(customer_id, paid_at DESC);"))) {
        return false;
    }

    if (!executeStatement(QStringLiteral(
            "CREATE INDEX IF NOT EXISTS idx_payments_order_id "
            "ON payments(order_id);"))) {
        return false;
    }

    return setSchemaVersion(kSchemaVersion);
}

bool DatabaseService::executeStatement(const QString &sql)
{
    QSqlQuery query(m_database);
    if (!query.exec(sql)) {
        m_lastError = query.lastError().text();
        return false;
    }

    return true;
}

bool DatabaseService::setSchemaVersion(int version)
{
    QSqlQuery query(m_database);
    query.prepare(QStringLiteral(
        "INSERT INTO schema_meta(key, value) VALUES('schema_version', :value) "
        "ON CONFLICT(key) DO UPDATE SET value = excluded.value;"));
    query.bindValue(QStringLiteral(":value"), QString::number(version));
    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return false;
    }

    m_lastError.clear();
    return true;
}

} // namespace stockmaster::infra::db
