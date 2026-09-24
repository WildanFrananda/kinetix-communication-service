#include "adapters/grpc/GrpcPrincipalAuthenticator.hpp"

#include <string>
#include <utility>

#include <grpcpp/client_context.h>
#include <trantor/utils/Logger.h>

namespace communication::adapters::grpc {

using domain::CommunicationError;
using domain::PrincipalId;

GrpcPrincipalAuthenticator::GrpcPrincipalAuthenticator(
    std::shared_ptr<::grpc::Channel> channel,
    std::chrono::milliseconds deadline
) : stub_(identity::v1::IdentityService::NewStub(std::move(channel))), deadline_(deadline) {}

std::expected<PrincipalId, CommunicationError> GrpcPrincipalAuthenticator::Authenticate(
    std::string_view credential
) {
    if (credential.empty()) {
        return std::unexpected(CommunicationError::kNotAuthenticated);
    }

    identity::v1::ValidateTokenRequest request;
    request.set_access_token(std::string(credential));

    identity::v1::ValidateTokenResponse response;
    ::grpc::ClientContext context;
    context.set_deadline(std::chrono::system_clock::now() + deadline_);

    const ::grpc::Status status = stub_->ValidateToken(&context, request, &response);

    if (!status.ok()) {
        if (status.error_code() == ::grpc::StatusCode::UNAUTHENTICATED) {
            return std::unexpected(CommunicationError::kNotAuthenticated);
        }
        LOG_ERROR << "identity could not validate a token: " << status.error_code() << " " << status.error_message();
        return std::unexpected(CommunicationError::kDependencyUnavailable);
    }

    if (!response.valid()) {
        return std::unexpected(CommunicationError::kNotAuthenticated);
    }

    auto principal = PrincipalId::From(response.principal_id());
    if (!principal) {
        LOG_ERROR << "identity accepted a token but returned an unusable principal id";
        return std::unexpected(CommunicationError::kNotAuthenticated);
    }
    return *principal;
}

}  // namespace communication::adapters::grpc
