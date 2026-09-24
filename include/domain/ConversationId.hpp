#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>

#include "domain/Text.hpp"

using namespace std;

namespace communication::domain {

class ConversationId {
  public:
    static constexpr size_t kMaxLength = 64;

    [[nodiscard]] static std::optional<ConversationId> From(std::string_view raw) {
        auto trimmed = text::Trim(raw);

        if (trimmed.empty() || trimmed.size() > kMaxLength || !text::IsPrintable(trimmed)) {
            return std::nullopt;
        }

        return ConversationId(std::move(trimmed));
    }

    [[nodiscard]] const std::string& Value() const noexcept {
        return value_;
    }

    [[nodiscard]] bool operator==(const ConversationId& other) const noexcept = default;

  private:
    explicit ConversationId(std::string value) : value_(std::move(value)) {}

    std::string value_;
};

}  // namespace communication::domain
