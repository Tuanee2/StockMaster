#pragma once

#include <QSqlDatabase>
#include <QString>

namespace stockmaster::infra::db {

class DatabaseService {
public:
    explicit DatabaseService(const QString &databaseFilePath = {});
    ~DatabaseService();

    bool initialize();

    [[nodiscard]] bool isReady() const;
    [[nodiscard]] QString backendName() const;
    [[nodiscard]] QString databaseFilePath() const;
    [[nodiscard]] QString lastError() const;
    [[nodiscard]] QSqlDatabase database() const;

    bool beginTransaction();
    bool commitTransaction();
    void rollbackTransaction();

private:
    [[nodiscard]] QString resolveDatabaseFilePath() const;
    bool openConnection();
    bool ensureSchema();
    bool executeStatement(const QString &sql);
    bool setSchemaVersion(int version);

    QString m_requestedDatabaseFilePath;
    QString m_resolvedDatabaseFilePath;
    QString m_connectionName;
    QString m_lastError;
    QSqlDatabase m_database;
    bool m_ready = false;
    int m_transactionDepth = 0;
};

} // namespace stockmaster::infra::db
