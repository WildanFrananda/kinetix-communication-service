#pragma once

#include <cstddef>
#include <expected>
#include <optional>
#include <vector>

#include <gmock/gmock.h>

#include "ports/IConversationRepository.hpp"

namespace communication::testing {

// Not `final`: NiceMock<T> derives from T, and a final mock cannot be quietened.
class MockConversationRepository : public ports::IConversationRepository {
  public:
    MOCK_METHOD((std::expected<void, domain::CommunicationError>), Save, (const domain::Conversation&),
                (override));
    MOCK_METHOD((std::expected<std::optional<domain::Conversation>, domain::CommunicationError>), FindById,
                (const domain::ConversationId&), (override));
    MOCK_METHOD((std::expected<std::optional<domain::Conversation>, domain::CommunicationError>),
                FindBetween,
                (const domain::PrincipalId&, const domain::PrincipalId&,
                 (const std::optional<domain::OrderNumber>&)),
                (override));
    MOCK_METHOD((std::expected<std::vector<domain::Conversation>, domain::CommunicationError>), ListFor,
                (const domain::PrincipalId&, std::size_t), (override));
};

}  // namespace communication::testing
