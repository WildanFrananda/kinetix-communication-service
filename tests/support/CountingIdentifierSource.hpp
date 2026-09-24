#pragma once

#include <string>

#include "ports/IIdentifierSource.hpp"

namespace communication::testing {

/// Identifiers a test can predict: conv-1, conv-2, msg-1, msg-2.
///
/// A fake rather than a mock. What a test needs from this port is a value it can compare against
/// later, not a record of how many times it was called.
class CountingIdentifierSource final : public ports::IIdentifierSource {
  public:
    [[nodiscard]] domain::ConversationId NewConversationId() override {
        ++conversations_;
        return *domain::ConversationId::From("conv-" + std::to_string(conversations_));
    }

    [[nodiscard]] domain::MessageId NewMessageId() override {
        ++messages_;
        return *domain::MessageId::From("msg-" + std::to_string(messages_));
    }

  private:
    int conversations_ = 0;
    int messages_ = 0;
};

}  // namespace communication::testing
