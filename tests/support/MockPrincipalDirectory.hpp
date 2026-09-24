#pragma once

#include <expected>

#include <gmock/gmock.h>

#include "ports/IPrincipalDirectory.hpp"

namespace communication::testing {

// Not `final`: NiceMock<T> derives from T, and a final mock cannot be quietened.
class MockPrincipalDirectory : public ports::IPrincipalDirectory {
  public:
    MOCK_METHOD((std::expected<bool, domain::CommunicationError>), Exists, (const domain::PrincipalId&),
                (override));
};

}  // namespace communication::testing
