#include "app/ListConversations.hpp"

#include <algorithm>
#include <utility>

#include "domain/PrincipalId.hpp"

namespace communication::app {

using domain::CommunicationError;
using domain::Conversation;
using domain::PrincipalId;

ListConversations::ListConversations(std::shared_ptr<ports::IConversationRepository> conversations)
    : conversations_(std::move(conversations)) {}

std::expected<std::vector<Conversation>, CommunicationError> ListConversations::Execute(
    std::string_view requester,
    std::size_t limit
) {
    const auto asked_by = PrincipalId::From(requester);
    if (!asked_by) {
        return std::unexpected(CommunicationError::kMalformedInput);
    }
    const std::size_t page = limit == 0 ? kDefaultPage : std::min(limit, kMaxPage);
    return conversations_->ListFor(*asked_by, page);
}

}  // namespace communication::app
