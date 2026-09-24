#include "adapters/persistence/PostgresMessageRepository.hpp"

#include <QVariant>
#include <algorithm>
#include <cstdint>
#include <exception>
#include <string>
#include <utility>

#include <orm/databaseconnection.hpp>
#include <orm/db.hpp>
#include <orm/exceptions/sqlerror.hpp>

#include "domain/CommunicationError.hpp"
#include "domain/ConversationId.hpp"
#include "domain/Message.hpp"
#include "domain/MessageBody.hpp"
#include "domain/MessageId.hpp"
#include "domain/PrincipalId.hpp"

#include "adapters/persistence/Sql.hpp"

namespace communication::adapters::persistence {
namespace {

using domain::CommunicationError;
using domain::Message;

constexpr auto kColumns =
    "id, conversation_id, sender, body, "
    "(EXTRACT(EPOCH FROM sent_at) * 1000)::bigint AS sent_at_ms";

[[nodiscard]] std::expected<Message, CommunicationError> ToMessage(Orm::SqlQuery& row) {
    const auto id = domain::MessageId::From(sql::Text(row, "id"));
    const auto conversation = domain::ConversationId::From(sql::Text(row, "conversation_id"));
    const auto sender = domain::PrincipalId::From(sql::Text(row, "sender"));
    const auto body = domain::MessageBody::From(sql::Text(row, "body"));
    if (!id || !conversation || !sender || !body) {
        sql::ReportCorruptRow("messages", sql::Text(row, "id"), "a column does not parse");
        return std::unexpected(CommunicationError::kCorruptRecord);
    }

    return Message(
        *id,
        *conversation,
        *sender,
        *body,
        sql::TimeFromMillis(sql::Integer(row, "sent_at_ms"))
    );
}

}  // namespace

PostgresMessageRepository::PostgresMessageRepository(QString connection)
    : connection_(std::move(connection)) {}

Orm::DatabaseConnection& PostgresMessageRepository::Connection() const {
    return Orm::DB::connection(connection_);
}

std::expected<void, CommunicationError> PostgresMessageRepository::Append(const Message& message) {
    const auto sent = sql::Millis(message.SentAt());
    auto& connection = Connection();
    try {
        connection.beginTransaction();

        connection.insert(
            "INSERT INTO messages (id, conversation_id, sender, body, sent_at) "
            "VALUES (?, ?, ?, ?, to_timestamp(? / 1000.0))",
            {
                sql::Bind(message.Id().Value()),
                sql::Bind(message.BelongsTo().Value()),
                sql::Bind(message.Sender().Value()),
                sql::Bind(message.Body().Value()),
                sql::Bind(sent),
            }
        );

        connection.update(
            "UPDATE conversations SET last_message_at = "
            "GREATEST(last_message_at, to_timestamp(? / 1000.0)) WHERE id = ?",
            {sql::Bind(sent), sql::Bind(message.BelongsTo().Value())}
        );

        connection.commit();
        return {};
    } catch (const Orm::Exceptions::SqlError& e) {
        try {
            connection.rollBack();
        } catch (const Orm::Exceptions::SqlError& rollback_failed) {
            sql::ReportUnavailable("rolling back a message", rollback_failed);
        }

        sql::ReportUnavailable("storing a message", e);
        return std::unexpected(CommunicationError::kDependencyUnavailable);
    }
}

std::expected<std::vector<Message>, CommunicationError> PostgresMessageRepository::ListIn(
    const domain::ConversationId& conversation,
    const std::optional<domain::MessageId>& before,
    std::size_t limit
) {
    const auto cursor = sql::Bind(before.has_value() ? before->Value() : std::string());
    try {
        auto rows =
            Connection().select(
                QString("SELECT %1 FROM messages WHERE conversation_id = ? AND ("
                    "  ? = '' OR (sent_at, id) < (SELECT m.sent_at, m.id FROM "
                    "messages m WHERE m.id = ?)"
                    ") ORDER BY sent_at DESC, id DESC LIMIT ?"
                ).arg(QLatin1String(kColumns)),
                {
                    sql::Bind(conversation.Value()),
                    cursor,
                    cursor,
                    sql::Bind(static_cast<std::int64_t>(limit)),
                }
            );

        std::vector<Message> out;
        while (rows.next()) {
            auto made = ToMessage(rows);
            if (!made) {
                return std::unexpected(made.error());
            }
            out.push_back(*made);
        }

        std::ranges::reverse(out);
        return out;
    } catch (const Orm::Exceptions::SqlError& e) {
        sql::ReportUnavailable("reading a conversation", e);
        return std::unexpected(CommunicationError::kDependencyUnavailable);
    }
}

}  // namespace communication::adapters::persistence
