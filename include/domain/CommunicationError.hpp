#pragma once

#include <cstdint>
#include <string_view>

using namespace std;

namespace communication::domain {

enum class CommunicationError : uint8_t {
    kMalformedInput,
    kNotAuthenticated,
    kConversationNotFound,
    kNotAParticipant,
    kConversationWithSelf,
    kUnknownPrincipal,
    kDependencyUnavailable,
    kConversationAlreadyExists,
    kCorruptRecord,
};

[[nodiscard]] constexpr string_view Describe(CommunicationError error) noexcept {
    switch (error) {
        case CommunicationError::kMalformedInput:
            return "malformed_input";
        case CommunicationError::kNotAuthenticated:
            return "not_authenticated";
        case CommunicationError::kConversationNotFound:
            return "conversation_not_found";
        case CommunicationError::kNotAParticipant:
            return "not_a_participant";
        case CommunicationError::kConversationWithSelf:
            return "conversation_with_self";
        case CommunicationError::kUnknownPrincipal:
            return "unknown_principal";
        case CommunicationError::kDependencyUnavailable:
            return "dependency_unavailable";
        case CommunicationError::kConversationAlreadyExists:
            return "conversation_already_exists";
        case CommunicationError::kCorruptRecord:
            return "corrupt_record";
    }
    return "unknown";
}

}  // namespace communication::domain
