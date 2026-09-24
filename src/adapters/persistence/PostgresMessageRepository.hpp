#pragma once

#include <QString>
#include <cstddef>
#include <expected>
#include <optional>
#include <vector>

#include "ports/IMessageRepository.hpp"

namespace Orm {
class DatabaseConnection;
}  // namespace Orm

namespace communication::adapters::persistence {

class PostgresMessageRepository final : public ports::IMessageRepository {
  public:
    explicit PostgresMessageRepository(QString connection);

    [[nodiscard]] std::expected<void, domain::CommunicationError> Append(
        const domain::Message& message
    ) override;

    [[nodiscard]] std::expected<std::vector<domain::Message>, domain::CommunicationError> ListIn(
        const domain::ConversationId& conversation,
        const std::optional<domain::MessageId>& before,
        std::size_t limit
    ) override;

  private:
    [[nodiscard]] Orm::DatabaseConnection& Connection() const;

    QString connection_;
};

}  // namespace communication::adapters::persistence
