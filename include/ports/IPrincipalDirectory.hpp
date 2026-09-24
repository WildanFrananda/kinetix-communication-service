#pragma once

#include <expected>

#include "domain/CommunicationError.hpp"
#include "domain/PrincipalId.hpp"

namespace communication::ports {

class IPrincipalDirectory {
  public:
    virtual ~IPrincipalDirectory() = default;

    IPrincipalDirectory(const IPrincipalDirectory&) = delete;
    IPrincipalDirectory& operator=(const IPrincipalDirectory&) = delete;
    IPrincipalDirectory(IPrincipalDirectory&&) = delete;
    IPrincipalDirectory& operator=(IPrincipalDirectory&&) = delete;

    [[nodiscard]] virtual std::expected<bool, domain::CommunicationError> Exists(
        const domain::PrincipalId& principal
    ) = 0;

  protected:
    IPrincipalDirectory() = default;
};

}  // namespace communication::ports
