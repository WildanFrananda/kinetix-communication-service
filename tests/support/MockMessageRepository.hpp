#pragma once

#include <cstddef>
#include <expected>
#include <optional>
#include <vector>

#include <gmock/gmock.h>

#include "ports/IMessageRepository.hpp"

namespace communication::testing {

// Not `final`: NiceMock<T> derives from T, and a final mock cannot be quietened.
class MockMessageRepository : public ports::IMessageRepository {
  public:
    MOCK_METHOD((std::expected<void, domain::CommunicationError>), Append, (const domain::Message&),
                (override));
    MOCK_METHOD((std::expected<std::vector<domain::Message>, domain::CommunicationError>), ListIn,
                (const domain::ConversationId&, (const std::optional<domain::MessageId>&),
                 std::size_t),
                (override));
};

}  // namespace communication::testing
