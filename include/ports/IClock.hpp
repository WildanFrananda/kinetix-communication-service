#pragma once

#include <chrono>

using time_point = std::chrono::system_clock::time_point;

namespace communication::ports {

class IClock {
  public:
    virtual ~IClock() = default;

    IClock(const IClock&) = delete;
    IClock& operator=(const IClock&) = delete;
    IClock(IClock&&) = delete;
    IClock& operator=(IClock&&) = delete;

    [[nodiscard]] virtual time_point Now() const = 0;

  protected:
    IClock() = default;
};

}  // namespace communication::ports
