#pragma once

#include <expected>

#include "domain/CommunicationError.hpp"
#include "domain/Conversation.hpp"
#include "domain/Message.hpp"

namespace communication::ports {

class IMessageBroadcaster {
  public:
    virtual ~IMessageBroadcaster() = default;

    IMessageBroadcaster(const IMessageBroadcaster&) = delete;
    IMessageBroadcaster& operator=(const IMessageBroadcaster&) = delete;
    IMessageBroadcaster(IMessageBroadcaster&&) = delete;
    IMessageBroadcaster& operator=(IMessageBroadcaster&&) = delete;

    [[nodiscard]] virtual std::expected<void, domain::CommunicationError> Publish(
        const domain::Conversation& conversation,
        const domain::Message& message
    ) = 0;

  protected:
    IMessageBroadcaster() = default;
};

}  // namespace communication::ports
