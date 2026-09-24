#include "adapters/wire/Json.hpp"

#include <chrono>
#include <cstdint>

namespace communication::adapters::wire {
namespace {

[[nodiscard]] Json::Int64 Millis(std::chrono::system_clock::time_point at) {
    return static_cast<Json::Int64>(
        std::chrono::duration_cast<std::chrono::milliseconds>(at.time_since_epoch()).count()
    );
}

}  // namespace

Json::Value ToJson(const domain::Message& message) {
    Json::Value out;
    out["id"] = message.Id().Value();
    out["conversation_id"] = message.BelongsTo().Value();
    out["sender"] = message.Sender().Value();
    out["body"] = message.Body().Value();
    out["sent_at_ms"] = Millis(message.SentAt());
    return out;
}

Json::Value ToJson(const domain::Conversation& conversation, const domain::PrincipalId& seen_by) {
    Json::Value out;
    out["id"] = conversation.Id().Value();
    out["started_at_ms"] = Millis(conversation.StartedAt());

    if (const auto other = conversation.CounterpartOf(seen_by)) {
        out["counterpart"] = other->Value();
    } else {
        out["counterpart"] = Json::Value::nullSingleton();
    }

    if (conversation.AboutOrder().has_value()) {
        out["order_number"] = conversation.AboutOrder()->Value();
    } else {
        out["order_number"] = Json::Value::nullSingleton();
    }
    return out;
}

}  // namespace communication::adapters::wire
