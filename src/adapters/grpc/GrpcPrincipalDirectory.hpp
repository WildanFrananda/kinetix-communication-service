#pragma once

#include <chrono>
#include <expected>
#include <memory>

#include <grpcpp/channel.h>

#include "ports/IPrincipalDirectory.hpp"

#include "identity/v1/identity.grpc.pb.h"

namespace communication::adapters::grpc {

class GrpcPrincipalDirectory final : public ports::IPrincipalDirectory {
  public:
    GrpcPrincipalDirectory(
        std::shared_ptr<::grpc::Channel> channel,
        std::chrono::milliseconds deadline
    );

    [[nodiscard]] std::expected<bool, domain::CommunicationError> Exists(
        const domain::PrincipalId& principal
    ) override;

  private:
    std::unique_ptr<identity::v1::IdentityService::Stub> stub_;
    std::chrono::milliseconds deadline_;
};

}  // namespace communication::adapters::grpc
