#pragma once

#include <json/json.h>

#include "domain/Conversation.hpp"
#include "domain/Message.hpp"

namespace communication::adapters::wire {

[[nodiscard]] Json::Value ToJson(const domain::Message& message);

[[nodiscard]] Json::Value ToJson(
    const domain::Conversation& conversation,
    const domain::PrincipalId& seen_by
);

}  // namespace communication::adapters::wire
