#pragma once

#include <expected>
#include <memory>
#include <string_view>

#include "domain/CommunicationError.hpp"
#include "ports/IClock.hpp"
#include "ports/IConversationRepository.hpp"
#include "ports/IIdentifierSource.hpp"
#include "ports/IMessageBroadcaster.hpp"
#include "ports/IMessageRepository.hpp"

#include "app/SentMessage.hpp"

namespace communication::app {

class SendMessage {
  public:
    SendMessage(
        std::shared_ptr<ports::IConversationRepository> conversations,
        std::shared_ptr<ports::IMessageRepository> messages,
        std::shared_ptr<ports::IMessageBroadcaster> broadcaster,
        std::shared_ptr<ports::IIdentifierSource> identifiers,
        std::shared_ptr<ports::IClock> clock
    );

    [[nodiscard]] std::expected<SentMessage, domain::CommunicationError> Execute(
        std::string_view sender,
        std::string_view conversation,
        std::string_view body
    );

  private:
    std::shared_ptr<ports::IConversationRepository> conversations_;
    std::shared_ptr<ports::IMessageRepository> messages_;
    std::shared_ptr<ports::IMessageBroadcaster> broadcaster_;
    std::shared_ptr<ports::IIdentifierSource> identifiers_;
    std::shared_ptr<ports::IClock> clock_;
};

}  // namespace communication::app
