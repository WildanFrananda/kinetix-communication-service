#pragma once

#include <chrono>

#include "ports/IClock.hpp"

namespace communication::adapters::system {

class SystemClock final : public ports::IClock {
  public:
    [[nodiscard]] std::chrono::system_clock::time_point Now() const override {
        return std::chrono::system_clock::now();
    }
};

}  // namespace communication::adapters::system
