#include "stockmaster/infra/db/database_service.h"

namespace stockmaster::infra::db {

bool DatabaseService::initialize()
{
    // Placeholder: migration + connection bootstrap will be added in infra/db.
    m_ready = true;
    return m_ready;
}

bool DatabaseService::isReady() const
{
    return m_ready;
}

QString DatabaseService::backendName() const
{
    return QStringLiteral("SQLite (local-first)");
}

} // namespace stockmaster::infra::db
