#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>

#include "domain/Text.hpp"

using namespace std;

namespace communication::domain {

class MessageId {
  public:
    static constexpr size_t kMaxLength = 64;

    [[nodiscard]] static optional<MessageId> From(string_view raw) {
        auto trimmed = text::Trim(raw);
        if (trimmed.empty() || trimmed.size() > kMaxLength || !text::IsPrintable(trimmed)) {
            return nullopt;
        }

        return MessageId(move(trimmed));
    }

    [[nodiscard]] const string& Value() const noexcept {
        return value_;
    }

    [[nodiscard]] bool operator==(const MessageId& other) const noexcept = default;

  private:
    explicit MessageId(string value) : value_(move(value)) {}

    string value_;
};

}  // namespace communication::domain
