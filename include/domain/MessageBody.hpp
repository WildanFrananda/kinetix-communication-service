#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>

#include "domain/Text.hpp"

using namespace std;

namespace communication::domain {

class MessageBody {
  public:
    static constexpr size_t kMaxLength = 4000;

    [[nodiscard]] static optional<MessageBody> From(string_view raw) {
        auto trimmed = text::Trim(raw);
        if (
            trimmed.empty() ||
            trimmed.size() > kMaxLength ||
            !text::IsPrintableWithLayout(trimmed)
        ) {
            return nullopt;
        }

        return MessageBody(move(trimmed));
    }

    [[nodiscard]] const string& Value() const noexcept {
        return value_;
    }

    [[nodiscard]] bool operator==(const MessageBody& other) const noexcept = default;

  private:
    explicit MessageBody(string value) : value_(move(value)) {}

    string value_;
};

}  // namespace communication::domain
