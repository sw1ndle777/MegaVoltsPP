#pragma once
#include "IDatabase.h"
#include <string>
#include <memory>

namespace BaseLib
{
    /// @brief create a database implementation based on the driver name.
    /// @param driver "mariadb" (default), "postgresql", "postgres", or "pg".
    /// @return a unique_ptr to the selected IDatabase implementation.
    std::unique_ptr<IDatabase> CreateDatabase(const std::string& driver);
}
