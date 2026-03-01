#pragma once

#include <QString>

namespace stockmaster::infra::db {

class DatabaseService {
public:
    bool initialize();

    [[nodiscard]] bool isReady() const;
    [[nodiscard]] QString backendName() const;

private:
    bool m_ready = false;
};

} // namespace stockmaster::infra::db
