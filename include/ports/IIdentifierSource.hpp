#pragma once

#include "domain/ConversationId.hpp"
#include "domain/MessageId.hpp"

namespace communication::ports {

class IIdentifierSource {
  public:
    virtual ~IIdentifierSource() = default;

    IIdentifierSource(const IIdentifierSource&) = delete;
    IIdentifierSource& operator=(const IIdentifierSource&) = delete;
    IIdentifierSource(IIdentifierSource&&) = delete;
    IIdentifierSource& operator=(IIdentifierSource&&) = delete;

    [[nodiscard]] virtual domain::ConversationId NewConversationId() = 0;
    [[nodiscard]] virtual domain::MessageId NewMessageId() = 0;

  protected:
    IIdentifierSource() = default;
};

}  // namespace communication::ports
