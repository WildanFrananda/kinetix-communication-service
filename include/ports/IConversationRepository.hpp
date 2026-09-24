#pragma once

#include <cstddef>
#include <expected>
#include <optional>
#include <vector>

#include "domain/CommunicationError.hpp"
#include "domain/Conversation.hpp"
#include "domain/ConversationId.hpp"
#include "domain/OrderNumber.hpp"
#include "domain/PrincipalId.hpp"

using namespace std;

namespace communication::ports {

class IConversationRepository {
  public:
    virtual ~IConversationRepository() = default;

    IConversationRepository(const IConversationRepository&) = delete;
    IConversationRepository& operator=(const IConversationRepository&) = delete;
    IConversationRepository(IConversationRepository&&) = delete;
    IConversationRepository& operator=(IConversationRepository&&) = delete;

    [[nodiscard]] virtual expected<void, domain::CommunicationError> Save(
        const domain::Conversation& conversation
    ) = 0;

    [[nodiscard]] virtual expected<optional<domain::Conversation>, domain::CommunicationError>
    FindById(const domain::ConversationId& id) = 0;

    [[nodiscard]] virtual expected<optional<domain::Conversation>, domain::CommunicationError>
    FindBetween(
        const domain::PrincipalId& first,
        const domain::PrincipalId& second,
        const optional<domain::OrderNumber>& about_order
    ) = 0;

    [[nodiscard]] virtual expected<vector<domain::Conversation>, domain::CommunicationError>
    ListFor(const domain::PrincipalId& principal, size_t limit) = 0;

  protected:
    IConversationRepository() = default;
};

}  // namespace communication::ports
