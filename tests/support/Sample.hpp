#pragma once

#include <chrono>
#include <optional>
#include <string_view>

#include "domain/Conversation.hpp"
#include "domain/ConversationId.hpp"
#include "domain/Message.hpp"
#include "domain/MessageBody.hpp"
#include "domain/MessageId.hpp"
#include "domain/OrderNumber.hpp"
#include "domain/PrincipalId.hpp"

namespace communication::testing {

/// A fixed instant, so that nothing in the suite depends on when it ran.
inline constexpr std::chrono::system_clock::time_point kNoon{std::chrono::seconds{1'758'000'000}};

/// These dereference without checking on purpose: they take literals the test author chose, so a
/// failure here means the literal is wrong, and crashing on the spot says so more plainly than a
/// test that carries on with a default value.
[[nodiscard]] inline domain::PrincipalId Principal(std::string_view raw) {
    return *domain::PrincipalId::From(raw);
}

[[nodiscard]] inline domain::ConversationId ConversationId(std::string_view raw) {
    return *domain::ConversationId::From(raw);
}

[[nodiscard]] inline domain::MessageId MessageId(std::string_view raw) {
    return *domain::MessageId::From(raw);
}

[[nodiscard]] inline domain::MessageBody Body(std::string_view raw) {
    return *domain::MessageBody::From(raw);
}

[[nodiscard]] inline domain::OrderNumber Order(std::string_view raw) {
    return *domain::OrderNumber::From(raw);
}

/// A conversation between two named principals, about nothing in particular.
[[nodiscard]] inline domain::Conversation Conversation(
    std::string_view id, std::string_view first, std::string_view second,
    std::optional<domain::OrderNumber> about_order = std::nullopt) {
    return *domain::Conversation::Between(ConversationId(id), Principal(first), Principal(second),
                                          std::move(about_order), kNoon);
}

}  // namespace communication::testing
