#pragma once

#include <QString>
#include <QVariant>
#include <chrono>
#include <cstdint>
#include <exception>
#include <string>
#include <string_view>

#include <orm/exceptions/sqlerror.hpp>
#include <orm/types/sqlquery.hpp>

namespace communication::adapters::persistence::sql {

[[nodiscard]] inline std::string Text(Orm::SqlQuery& row, const char* column) {
    const QVariant value = row.value(QLatin1String(column));

    if (value.isNull()) {
        return {};
    }

    return value.toString().toStdString();
}

[[nodiscard]] inline std::int64_t Integer(Orm::SqlQuery& row, const char* column) {
    return row.value(QLatin1String(column)).toLongLong();
}

[[nodiscard]] inline QVariant Bind(const std::string& value) {
    return QVariant(QString::fromStdString(value));
}

[[nodiscard]] inline QVariant Bind(std::int64_t value) {
    return QVariant(static_cast<qlonglong>(value));
}

[[nodiscard]] inline std::int64_t Millis(std::chrono::system_clock::time_point at) {
    return std::chrono::duration_cast<std::chrono::milliseconds>(at.time_since_epoch()).count();
}

[[nodiscard]] inline std::chrono::system_clock::time_point TimeFromMillis(std::int64_t millis) {
    return std::chrono::system_clock::time_point(std::chrono::milliseconds(millis));
}

void ReportCorruptRow(std::string_view table, std::string_view id, std::string_view why);

void ReportUnavailable(std::string_view doing, const Orm::Exceptions::SqlError& error);

}  // namespace communication::adapters::persistence::sql
