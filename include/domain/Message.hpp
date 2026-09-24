#pragma once

#include <chrono>
#include <utility>

#include "domain/ConversationId.hpp"
#include "domain/MessageBody.hpp"
#include "domain/MessageId.hpp"
#include "domain/PrincipalId.hpp"

using namespace std;

using time_point = chrono::system_clock::time_point;

namespace communication::domain {

class Message {
  public:
    Message(
        MessageId id,
        ConversationId conversation,
        PrincipalId sender,
        MessageBody body,
        time_point sent_at
    ) : id_(move(id)),
        conversation_(move(conversation)),
        sender_(move(sender)),
        body_(move(body)),
        sent_at_(sent_at) {}

    [[nodiscard]] const MessageId& Id() const noexcept {
        return id_;
    }

    [[nodiscard]] const ConversationId& BelongsTo() const noexcept {
        return conversation_;
    }

    [[nodiscard]] const PrincipalId& Sender() const noexcept {
        return sender_;
    }

    [[nodiscard]] const MessageBody& Body() const noexcept {
        return body_;
    }

    [[nodiscard]] time_point SentAt() const noexcept {
        return sent_at_;
    }

    [[nodiscard]] bool operator==(const Message& other) const noexcept = default;

  private:
    MessageId id_;
    ConversationId conversation_;
    PrincipalId sender_;
    MessageBody body_;
    time_point sent_at_;
};

}  // namespace communication::domain
