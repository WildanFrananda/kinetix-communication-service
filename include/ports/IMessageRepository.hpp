#pragma once

#include <cstddef>
#include <expected>
#include <optional>
#include <vector>

#include "domain/CommunicationError.hpp"
#include "domain/ConversationId.hpp"
#include "domain/Message.hpp"
#include "domain/MessageId.hpp"

namespace communication::ports {

class IMessageRepository {
  public:
    virtual ~IMessageRepository() = default;

    IMessageRepository(const IMessageRepository&) = delete;
    IMessageRepository& operator=(const IMessageRepository&) = delete;
    IMessageRepository(IMessageRepository&&) = delete;
    IMessageRepository& operator=(IMessageRepository&&) = delete;

    [[nodiscard]] virtual std::expected<void, domain::CommunicationError> Append(
        const domain::Message& message
    ) = 0;

    [[nodiscard]] virtual std::expected<std::vector<domain::Message>, domain::CommunicationError> ListIn(
        const domain::ConversationId& conversation,
        const std::optional<domain::MessageId>& before,
        std::size_t limit
    ) = 0;

  protected:
    IMessageRepository() = default;
};

}  // namespace communication::ports
