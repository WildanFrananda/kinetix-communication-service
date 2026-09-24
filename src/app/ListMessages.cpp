#include "app/ListMessages.hpp"

#include <algorithm>
#include <optional>
#include <utility>

#include "domain/ConversationId.hpp"
#include "domain/MessageId.hpp"
#include "domain/PrincipalId.hpp"

namespace communication::app {

using domain::CommunicationError;
using domain::ConversationId;
using domain::Message;
using domain::MessageId;
using domain::PrincipalId;

ListMessages::ListMessages(
    std::shared_ptr<ports::IConversationRepository> conversations,
    std::shared_ptr<ports::IMessageRepository> messages
) : conversations_(std::move(conversations)), messages_(std::move(messages)) {}

std::expected<std::vector<Message>, CommunicationError> ListMessages::Execute(
    std::string_view requester,
    std::string_view conversation,
    std::string_view before,
    std::size_t limit
) {
    const auto asked_by = PrincipalId::From(requester);
    const auto where = ConversationId::From(conversation);
    if (!asked_by || !where) {
        return std::unexpected(CommunicationError::kMalformedInput);
    }

    std::optional<MessageId> cursor;
    if (!before.empty()) {
        cursor = MessageId::From(before);
        if (!cursor) {
            return std::unexpected(CommunicationError::kMalformedInput);
        }
    }

    auto found = conversations_->FindById(*where);
    if (!found) {
        return std::unexpected(found.error());
    }
    if (!found->has_value()) {
        return std::unexpected(CommunicationError::kConversationNotFound);
    }
    if (!(*found)->Includes(*asked_by)) {
        return std::unexpected(CommunicationError::kNotAParticipant);
    }

    const std::size_t page = limit == 0 ? kDefaultPage : std::min(limit, kMaxPage);
    return messages_->ListIn(*where, cursor, page);
}

}  // namespace communication::app
