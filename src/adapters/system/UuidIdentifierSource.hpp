#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <random>
#include <stdexcept>
#include <string>

#include "ports/IIdentifierSource.hpp"

namespace communication::adapters::system {

class UuidIdentifierSource final : public ports::IIdentifierSource {
  public:
    UuidIdentifierSource() : generator_(std::random_device{}()) {}

    [[nodiscard]] domain::ConversationId NewConversationId() override {
        return Make<domain::ConversationId>();
    }

    [[nodiscard]] domain::MessageId NewMessageId() override {
        return Make<domain::MessageId>();
    }

  private:
    static constexpr std::array<char, 16> kHex{
        '0', '1', '2', '3', '4', '5', '6', '7',
        '8', '9', 'a', 'b', 'c', 'd', 'e', 'f',
    };

    static constexpr std::size_t kHexDigits = 32;
    static constexpr std::size_t kVersionAt = 12;
    static constexpr std::size_t kVariantAt = 16;

    static constexpr std::array<std::size_t, 4> kDashBefore{8, 12, 16, 20};

    template <typename Id>
    [[nodiscard]] Id Make() {
        auto made = Id::From(Next());
        if (!made) {
            throw std::logic_error("UuidIdentifierSource produced a value the identifier refused");
        }
        return *made;
    }

    [[nodiscard]] std::string Next() {
        std::uniform_int_distribution<std::size_t> nibble(0, kHex.size() - 1);

        std::string out;
        out.reserve(36);
        for (std::size_t position = 0; position < kHexDigits; ++position) {
            if (std::ranges::find(kDashBefore, position) != kDashBefore.end()) {
                out.push_back('-');
            }
            if (position == kVersionAt) {
                out.push_back('4');
                continue;
            }
            if (position == kVariantAt) {
                out.push_back(kHex.at((nibble(generator_) & 0x3U) | 0x8U));
                continue;
            }
            out.push_back(kHex.at(nibble(generator_)));
        }
        return out;
    }

    std::mt19937 generator_;
};

}  // namespace communication::adapters::system
