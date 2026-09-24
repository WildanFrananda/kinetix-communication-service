#pragma once

#include <chrono>
#include <expected>
#include <memory>
#include <string_view>

#include <grpcpp/channel.h>

#include "ports/IPrincipalAuthenticator.hpp"

#include "identity/v1/identity.grpc.pb.h"

namespace communication::adapters::grpc {

class GrpcPrincipalAuthenticator final : public ports::IPrincipalAuthenticator {
  public:
    GrpcPrincipalAuthenticator(
        std::shared_ptr<::grpc::Channel> channel,
        std::chrono::milliseconds deadline
    );

    [[nodiscard]] std::expected<domain::PrincipalId, domain::CommunicationError> Authenticate(
        std::string_view credential
    ) override;

  private:
    std::unique_ptr<identity::v1::IdentityService::Stub> stub_;
    std::chrono::milliseconds deadline_;
};

}  // namespace communication::adapters::grpc
