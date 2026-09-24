#include "adapters/grpc/GrpcPrincipalDirectory.hpp"

#include <utility>

#include <grpcpp/client_context.h>
#include <trantor/utils/Logger.h>

#include "domain/CommunicationError.hpp"

namespace communication::adapters::grpc {

using domain::CommunicationError;

GrpcPrincipalDirectory::GrpcPrincipalDirectory(
    std::shared_ptr<::grpc::Channel> channel,
    std::chrono::milliseconds deadline
) : stub_(identity::v1::IdentityService::NewStub(std::move(channel))), deadline_(deadline) {}

std::expected<bool, CommunicationError> GrpcPrincipalDirectory::Exists(
    const domain::PrincipalId& principal) {
    identity::v1::GetPrincipalRequest request;
    request.set_principal_id(principal.Value());

    identity::v1::GetPrincipalResponse response;
    ::grpc::ClientContext context;
    context.set_deadline(std::chrono::system_clock::now() + deadline_);

    const ::grpc::Status status = stub_->GetPrincipal(&context, request, &response);

    if (status.ok()) {
        return response.found();
    }

    if (status.error_code() == ::grpc::StatusCode::NOT_FOUND) {
        return false;
    }

    LOG_ERROR << "identity could not answer whether a principal exists: " << status.error_code() << " " << status.error_message();

    return std::unexpected(CommunicationError::kDependencyUnavailable);
}

}  // namespace communication::adapters::grpc
