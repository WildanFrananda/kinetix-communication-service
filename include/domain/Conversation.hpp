#pragma once

#include <array>
#include <chrono>
#include <expected>
#include <optional>
#include <utility>

#include "domain/CommunicationError.hpp"
#include "domain/ConversationId.hpp"
#include "domain/OrderNumber.hpp"
#include "domain/PrincipalId.hpp"

using namespace std;

using time_point = chrono::system_clock::time_point;

namespace communication::domain {

class Conversation {
  public:
    [[nodiscard]] static expected<Conversation, CommunicationError> Between(
        ConversationId id,
        PrincipalId first,
        PrincipalId second,
        optional<OrderNumber> about_order,
        time_point started_at
    ) {
        if (first == second) {
            return unexpected(CommunicationError::kConversationWithSelf);
        }

        return Conversation(
            move(id),
            move(first),
            move(second),
            move(about_order),
            started_at
        );
    }

    [[nodiscard]] const ConversationId& Id() const noexcept {
        return id_;
    }

    [[nodiscard]] const PrincipalId& First() const noexcept {
        return first_;
    }

    [[nodiscard]] const PrincipalId& Second() const noexcept {
        return second_;
    }

    [[nodiscard]] const optional<OrderNumber>& AboutOrder() const noexcept {
        return about_order_;
    }

    [[nodiscard]] time_point StartedAt() const noexcept {
        return started_at_;
    }

    [[nodiscard]] bool Includes(const PrincipalId& principal) const noexcept {
        return principal == first_ || principal == second_;
    }

    [[nodiscard]] optional<PrincipalId> CounterpartOf(const PrincipalId& principal) const {
        if (principal == first_) {
            return second_;
        }

        if (principal == second_) {
            return first_;
        }

        return nullopt;
    }

    [[nodiscard]] static array<PrincipalId, 2> InCanonicalOrder(
        const PrincipalId& first,
        const PrincipalId& second
    ) {
        if (first.Value() <= second.Value()) {
            return {first, second};
        }

        return {second, first};
    }

  private:
    Conversation(
        ConversationId id,
        PrincipalId first,
        PrincipalId second,
        optional<OrderNumber> about_order,
        time_point started_at
    ) : id_(move(id)),
        first_(move(first)),
        second_(move(second)),
        about_order_(move(about_order)),
        started_at_(started_at) {}

    ConversationId id_;
    PrincipalId first_;
    PrincipalId second_;
    optional<OrderNumber> about_order_;
    time_point started_at_;
};

}  // namespace communication::domain
