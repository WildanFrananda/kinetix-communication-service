#include "adapters/persistence/PostgresConversationRepository.hpp"

#include <QVariant>
#include <chrono>
#include <cstdint>
#include <exception>
#include <string>
#include <utility>

#include <orm/databaseconnection.hpp>
#include <orm/db.hpp>
#include <orm/exceptions/sqlerror.hpp>

#include "domain/CommunicationError.hpp"
#include "domain/Conversation.hpp"
#include "domain/ConversationId.hpp"
#include "domain/OrderNumber.hpp"
#include "domain/PrincipalId.hpp"

#include "adapters/persistence/Sql.hpp"

namespace communication::adapters::persistence {
namespace {

using domain::CommunicationError;
using domain::Conversation;

constexpr auto kColumns =
    "id, first_principal, second_principal, about_order, "
    "(EXTRACT(EPOCH FROM started_at) * 1000)::bigint AS started_at_ms";

[[nodiscard]] std::expected<Conversation, CommunicationError> ToConversation(Orm::SqlQuery& row) {
    const auto id = domain::ConversationId::From(sql::Text(row, "id"));
    const auto first = domain::PrincipalId::From(sql::Text(row, "first_principal"));
    const auto second = domain::PrincipalId::From(sql::Text(row, "second_principal"));
    if (!id || !first || !second) {
        sql::ReportCorruptRow(
            "conversations", sql::Text(row, "id"),
            "an identifier does not parse"
        );
        return std::unexpected(CommunicationError::kCorruptRecord);
    }

    std::optional<domain::OrderNumber> order;
    if (const auto raw = sql::Text(row, "about_order"); !raw.empty()) {
        order = domain::OrderNumber::From(raw);
        if (!order) {
            sql::ReportCorruptRow("conversations", id->Value(), "the order number does not parse");
            return std::unexpected(CommunicationError::kCorruptRecord);
        }
    }

    auto made = Conversation::Between(
        *id,
        *first,
        *second,
        std::move(order),
        sql::TimeFromMillis(sql::Integer(row, "started_at_ms"))
    );

    if (!made) {
        sql::ReportCorruptRow("conversations", id->Value(), "the row breaks a domain rule");
        return std::unexpected(CommunicationError::kCorruptRecord);
    }
    return *made;
}

}  // namespace

PostgresConversationRepository::PostgresConversationRepository(QString connection)
    : connection_(std::move(connection)) {}

Orm::DatabaseConnection& PostgresConversationRepository::Connection() const {
    return Orm::DB::connection(connection_);
}

std::expected<void, CommunicationError> PostgresConversationRepository::Save(
    const Conversation& conversation
) {
    const auto started = sql::Millis(conversation.StartedAt());
    try {
        const auto [affected, _] = Connection().affectingStatement(
            "INSERT INTO conversations "
            "(id, first_principal, second_principal, about_order, started_at, last_message_at) "
            "VALUES (?, ?, ?, NULLIF(?, ''), to_timestamp(? / 1000.0), to_timestamp(? / 1000.0)) "
            "ON CONFLICT DO NOTHING",
            {
                sql::Bind(conversation.Id().Value()),
                sql::Bind(conversation.First().Value()),
                sql::Bind(conversation.Second().Value()),
                sql::Bind(conversation.AboutOrder().has_value()
                    ? conversation.AboutOrder()->Value()
                    : std::string()
                ),
                sql::Bind(started), sql::Bind(started),
            }
        );

        if (affected == 0) {
            return std::unexpected(CommunicationError::kConversationAlreadyExists);
        }
        return {};
    } catch (const Orm::Exceptions::SqlError& e) {
        sql::ReportUnavailable("storing a conversation", e);
        return std::unexpected(CommunicationError::kDependencyUnavailable);
    }
}

std::expected<std::optional<Conversation>, CommunicationError> PostgresConversationRepository::FindById(
    const domain::ConversationId& id
) {
    try {
        auto rows = Connection().select(
            QString("SELECT %1 FROM conversations WHERE id = ?").arg(QLatin1String(kColumns)),
            {sql::Bind(id.Value())}
        );
        if (!rows.next()) {
            return std::optional<Conversation>{};
        }
        auto made = ToConversation(rows);
        if (!made) {
            return std::unexpected(made.error());
        }
        return std::optional<Conversation>{*made};
    } catch (const Orm::Exceptions::SqlError& e) {
        sql::ReportUnavailable("reading a conversation", e);
        return std::unexpected(CommunicationError::kDependencyUnavailable);
    }
}

std::expected<std::optional<Conversation>, CommunicationError> PostgresConversationRepository::FindBetween(
    const domain::PrincipalId& first,
    const domain::PrincipalId& second,
    const std::optional<domain::OrderNumber>& about_order
) {
    const auto left = sql::Bind(first.Value());
    const auto right = sql::Bind(second.Value());
    try {
        auto rows = Connection().select(
            QString(
                "SELECT %1 FROM conversations WHERE "
                "LEAST(first_principal, second_principal) = LEAST(?, ?) AND "
                "GREATEST(first_principal, second_principal) = GREATEST(?, ?) AND "
                "COALESCE(about_order, '') = ?"
            ).arg(QLatin1String(kColumns)),
            {
                left, right, left, right,
                sql::Bind(about_order.has_value() ? about_order->Value() : std::string()),
            }
        );

        if (!rows.next()) {
            return std::optional<Conversation>{};
        }
        auto made = ToConversation(rows);
        if (!made) {
            return std::unexpected(made.error());
        }
        return std::optional<Conversation>{*made};
    } catch (const Orm::Exceptions::SqlError& e) {
        sql::ReportUnavailable("looking for a conversation", e);
        return std::unexpected(CommunicationError::kDependencyUnavailable);
    }
}

std::expected<std::vector<Conversation>, CommunicationError> PostgresConversationRepository::ListFor(
    const domain::PrincipalId& principal,
    std::size_t limit
) {
    const auto who = sql::Bind(principal.Value());
    try {
        auto rows = Connection().select(
            QString(
                "SELECT %1 FROM conversations WHERE first_principal = ? OR second_principal = ? "
                "ORDER BY last_message_at DESC, id DESC LIMIT ?"
            ).arg(QLatin1String(kColumns)),
            {who, who, sql::Bind(static_cast<std::int64_t>(limit))}
        );

        std::vector<Conversation> out;
        while (rows.next()) {
            auto made = ToConversation(rows);
            if (!made) {
                return std::unexpected(made.error());
            }
            out.push_back(*made);
        }
        return out;
    } catch (const Orm::Exceptions::SqlError& e) {
        sql::ReportUnavailable("listing conversations", e);
        return std::unexpected(CommunicationError::kDependencyUnavailable);
    }
}

}  // namespace communication::adapters::persistence
