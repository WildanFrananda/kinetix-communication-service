#pragma once

#include <chrono>

#include "ports/IClock.hpp"

namespace communication::testing {

/// A clock the test decides.
///
/// A real clock would make every assertion about ordering depend on how fast the machine happened
/// to be, which is the shape of a test that passes for a year and then fails once in CI.
class FixedClock final : public ports::IClock {
  public:
    explicit FixedClock(std::chrono::system_clock::time_point now) : now_(now) {}

    [[nodiscard]] std::chrono::system_clock::time_point Now() const override { return now_; }

    void Advance(std::chrono::seconds by) { now_ += by; }

  private:
    std::chrono::system_clock::time_point now_;
};

}  // namespace communication::testing
