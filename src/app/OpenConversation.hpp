#pragma once

#include <expected>
#include <memory>
#include <string_view>

#include "domain/CommunicationError.hpp"
#include "domain/Conversation.hpp"
#include "ports/IClock.hpp"
#include "ports/IConversationRepository.hpp"
#include "ports/IIdentifierSource.hpp"
#include "ports/IPrincipalDirectory.hpp"

namespace communication::app {

class OpenConversation {
  public:
    OpenConversation(
        std::shared_ptr<ports::IConversationRepository> conversations,
        std::shared_ptr<ports::IPrincipalDirectory> directory,
        std::shared_ptr<ports::IIdentifierSource> identifiers,
        std::shared_ptr<ports::IClock> clock
    );

    [[nodiscard]] std::expected<domain::Conversation, domain::CommunicationError> Execute(
        std::string_view requester,
        std::string_view counterpart,
        std::string_view about_order
    );

  private:
    std::shared_ptr<ports::IConversationRepository> conversations_;
    std::shared_ptr<ports::IPrincipalDirectory> directory_;
    std::shared_ptr<ports::IIdentifierSource> identifiers_;
    std::shared_ptr<ports::IClock> clock_;
};

}  // namespace communication::app
