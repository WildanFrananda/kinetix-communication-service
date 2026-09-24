#include "app/OpenConversation.hpp"

#include <optional>
#include <utility>

#include "domain/OrderNumber.hpp"
#include "domain/PrincipalId.hpp"

namespace communication::app {

using domain::CommunicationError;
using domain::Conversation;
using domain::OrderNumber;
using domain::PrincipalId;

OpenConversation::OpenConversation(
    std::shared_ptr<ports::IConversationRepository> conversations,
    std::shared_ptr<ports::IPrincipalDirectory> directory,
    std::shared_ptr<ports::IIdentifierSource> identifiers,
    std::shared_ptr<ports::IClock> clock
) : conversations_(std::move(conversations)),
    directory_(std::move(directory)),
    identifiers_(std::move(identifiers)),
    clock_(std::move(clock)) {}

std::expected<Conversation, CommunicationError> OpenConversation::Execute(
    std::string_view requester,
    std::string_view counterpart,
    std::string_view about_order
) {
    const auto asked_by = PrincipalId::From(requester);
    const auto other = PrincipalId::From(counterpart);
    if (!asked_by || !other) {
        return std::unexpected(CommunicationError::kMalformedInput);
    }

    std::optional<OrderNumber> order;
    if (!about_order.empty()) {
        order = OrderNumber::From(about_order);
        if (!order) {
            return std::unexpected(CommunicationError::kMalformedInput);
        }
    }

    auto existing = conversations_->FindBetween(*asked_by, *other, order);
    if (!existing) {
        return std::unexpected(existing.error());
    }
    if (existing->has_value()) {
        return **existing;
    }

    for (const auto& principal : {*asked_by, *other}) {
        auto known = directory_->Exists(principal);
        if (!known) {
            return std::unexpected(known.error());
        }
        if (!*known) {
            return std::unexpected(CommunicationError::kUnknownPrincipal);
        }
    }

    auto conversation = Conversation::Between(
        identifiers_->NewConversationId(),
        *asked_by,
        *other,
        std::move(order), clock_->Now()
    );

    if (!conversation) {
        return std::unexpected(conversation.error());
    }

    auto saved = conversations_->Save(*conversation);
    if (saved) {
        return *conversation;
    }

    if (saved.error() != CommunicationError::kConversationAlreadyExists) {
        return std::unexpected(saved.error());
    }

    auto theirs = conversations_->FindBetween(*asked_by, *other, conversation->AboutOrder());
    if (!theirs) {
        return std::unexpected(theirs.error());
    }
    if (!theirs->has_value()) {
        return std::unexpected(CommunicationError::kDependencyUnavailable);
    }
    return **theirs;
}

}  // namespace communication::app
