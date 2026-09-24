#pragma once

#include <drogon/HttpTypes.h>

#include "domain/CommunicationError.hpp"

namespace communication::adapters::http {

[[nodiscard]] constexpr drogon::HttpStatusCode HttpStatusFor(domain::CommunicationError error) noexcept {
    switch (error) {
        case domain::CommunicationError::kMalformedInput:
        case domain::CommunicationError::kConversationWithSelf:
            return drogon::k400BadRequest;
        case domain::CommunicationError::kNotAuthenticated:
            return drogon::k401Unauthorized;
        case domain::CommunicationError::kNotAParticipant:
        case domain::CommunicationError::kConversationNotFound:
        case domain::CommunicationError::kUnknownPrincipal:
            return drogon::k404NotFound;
        case domain::CommunicationError::kConversationAlreadyExists:
            return drogon::k409Conflict;
        case domain::CommunicationError::kDependencyUnavailable:
            return drogon::k503ServiceUnavailable;
        case domain::CommunicationError::kCorruptRecord:
            return drogon::k500InternalServerError;
    }

    return drogon::k500InternalServerError;
}

}  // namespace communication::adapters::http
