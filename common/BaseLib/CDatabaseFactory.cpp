#include "CDatabaseFactory.h"
#include "CMariaDatabase.h"
#include "CPostgresDatabase.h"

namespace BaseLib
{
    std::unique_ptr<IDatabase> CreateDatabase(const std::string& driver)
    {
        if (driver == "postgresql" || driver == "postgres" || driver == "pg")
            return std::make_unique<CPostgresDatabase>();

        return std::make_unique<CMariaDatabase>();
    }
}
