#pragma once

#include <QString>
#include <cstddef>
#include <expected>
#include <optional>
#include <vector>

#include "ports/IConversationRepository.hpp"

namespace Orm {
class DatabaseConnection;
}  // namespace Orm

namespace communication::adapters::persistence {

class PostgresConversationRepository final : public ports::IConversationRepository {
  public:
    explicit PostgresConversationRepository(QString connection);

    [[nodiscard]] std::expected<void, domain::CommunicationError> Save(
        const domain::Conversation& conversation
    ) override;

    [[nodiscard]] std::expected<std::optional<domain::Conversation>, domain::CommunicationError> FindById(
        const domain::ConversationId& id
    ) override;

    [[nodiscard]] std::expected<std::optional<domain::Conversation>, domain::CommunicationError> FindBetween(
        const domain::PrincipalId& first, const domain::PrincipalId& second,
        const std::optional<domain::OrderNumber>& about_order
    ) override;

    [[nodiscard]] std::expected<std::vector<domain::Conversation>, domain::CommunicationError> ListFor(
        const domain::PrincipalId& principal, std::size_t limit
    ) override;

  private:
    [[nodiscard]] Orm::DatabaseConnection& Connection() const;

    QString connection_;
};

}  // namespace communication::adapters::persistence
