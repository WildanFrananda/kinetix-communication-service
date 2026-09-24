#pragma once

#include <expected>

#include <gmock/gmock.h>

#include "ports/IMessageBroadcaster.hpp"

namespace communication::testing {

// Not `final`: NiceMock<T> derives from T, and a final mock cannot be quietened.
class MockMessageBroadcaster : public ports::IMessageBroadcaster {
  public:
    MOCK_METHOD((std::expected<void, domain::CommunicationError>), Publish,
                (const domain::Conversation&, const domain::Message&), (override));
};

}  // namespace communication::testing
