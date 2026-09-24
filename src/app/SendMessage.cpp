#include "app/SendMessage.hpp"

#include <utility>

#include "domain/ConversationId.hpp"
#include "domain/Message.hpp"
#include "domain/MessageBody.hpp"
#include "domain/PrincipalId.hpp"

namespace communication::app {

using domain::CommunicationError;
using domain::ConversationId;
using domain::Message;
using domain::MessageBody;
using domain::PrincipalId;

SendMessage::SendMessage(
    std::shared_ptr<ports::IConversationRepository> conversations,
    std::shared_ptr<ports::IMessageRepository> messages,
    std::shared_ptr<ports::IMessageBroadcaster> broadcaster,
    std::shared_ptr<ports::IIdentifierSource> identifiers,
    std::shared_ptr<ports::IClock> clock
) : conversations_(std::move(conversations)),
    messages_(std::move(messages)),
    broadcaster_(std::move(broadcaster)),
    identifiers_(std::move(identifiers)),
    clock_(std::move(clock)) {}

std::expected<SentMessage, CommunicationError> SendMessage::Execute(
    std::string_view sender,
    std::string_view conversation,
    std::string_view body
) {
    const auto from = PrincipalId::From(sender);
    const auto where = ConversationId::From(conversation);
    const auto what = MessageBody::From(body);
    if (!from || !where || !what) {
        return std::unexpected(CommunicationError::kMalformedInput);
    }

    auto found = conversations_->FindById(*where);

    if (!found) {
        return std::unexpected(found.error());
    }
    if (!found->has_value()) {
        return std::unexpected(CommunicationError::kConversationNotFound);
    }
    const auto& thread = **found;

    if (!thread.Includes(*from)) {
        return std::unexpected(CommunicationError::kNotAParticipant);
    }

    const Message message(identifiers_->NewMessageId(), thread.Id(), *from, *what, clock_->Now());

    auto appended = messages_->Append(message);
    if (!appended) {
        return std::unexpected(appended.error());
    }

    const auto published = broadcaster_->Publish(thread, message);
    return SentMessage{.message = message, .broadcast = published.has_value()};
}

}  // namespace communication::app
