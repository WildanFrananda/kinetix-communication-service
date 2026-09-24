#pragma once

#include <expected>
#include <string_view>

#include "domain/CommunicationError.hpp"
#include "domain/PrincipalId.hpp"

namespace communication::ports {

class IPrincipalAuthenticator {
  public:
    virtual ~IPrincipalAuthenticator() = default;

    IPrincipalAuthenticator(const IPrincipalAuthenticator&) = delete;
    IPrincipalAuthenticator& operator=(const IPrincipalAuthenticator&) = delete;
    IPrincipalAuthenticator(IPrincipalAuthenticator&&) = delete;
    IPrincipalAuthenticator& operator=(IPrincipalAuthenticator&&) = delete;

    [[nodiscard]] virtual std::expected<domain::PrincipalId, domain::CommunicationError> Authenticate(
        std::string_view credential
    ) = 0;

  protected:
    IPrincipalAuthenticator() = default;
};

}  // namespace communication::ports
