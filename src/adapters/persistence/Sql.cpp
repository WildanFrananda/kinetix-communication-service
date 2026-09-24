#include "adapters/persistence/Sql.hpp"

#include <trantor/utils/Logger.h>

namespace communication::adapters::persistence::sql {

void ReportCorruptRow(std::string_view table, std::string_view id, std::string_view why) {
    LOG_ERROR << "corrupt row in " << std::string(table) << " (id=" << std::string(id) << "): " << std::string(why) << " — this will not fix itself by being retried";
}

void ReportUnavailable(std::string_view doing, const Orm::Exceptions::SqlError& error) {
    LOG_ERROR << "database refused while " << std::string(doing) << ": " << error.what();
}

}  // namespace communication::adapters::persistence::sql
