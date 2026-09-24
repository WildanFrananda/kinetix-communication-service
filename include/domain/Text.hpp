#pragma once

#include <algorithm>
#include <string>
#include <string_view>

using namespace std;

namespace ranges = std::ranges;

namespace communication::domain::text {

[[nodiscard]] inline string Trim(string_view raw) {
    constexpr string_view kSpace = " \t\n\v\f\r";

    const auto first = raw.find_first_not_of(kSpace);

    if (first == string_view::npos) {
        return {};
    }

    const auto last = raw.find_last_not_of(kSpace);

    return string(raw.substr(first, last - first + 1));
}

[[nodiscard]] inline bool IsPrintable(string_view value) {
    return ranges::none_of(value, [](char c) {
        const auto byte = static_cast<unsigned char>(c);

        return byte < 0x20 || byte == 0x7F;
    });
}

[[nodiscard]] inline bool IsPrintableWithLayout(string_view value) {
    return ranges::none_of(value, [](char c) {
        const auto byte = static_cast<unsigned char>(c);

        if (byte == '\t' || byte == '\n') {
            return false;
        }

        return byte < 0x20 || byte == 0x7F;
    });
}

}  // namespace communication::domain::text
