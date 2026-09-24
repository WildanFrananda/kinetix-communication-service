#include "adapters/http/CommunicationController.hpp"

#include <cstddef>
#include <string_view>
#include <utility>

#include <json/json.h>

#include "adapters/http/HttpStatus.hpp"
#include "adapters/wire/Json.hpp"

namespace communication::adapters::http {
namespace {

using domain::CommunicationError;

[[nodiscard]] drogon::HttpResponsePtr Failure(CommunicationError error) {
    Json::Value body;
    body["error"] = std::string(domain::Describe(error));
    auto response = drogon::HttpResponse::newHttpJsonResponse(body);
    response->setStatusCode(HttpStatusFor(error));
    return response;
}

[[nodiscard]] drogon::HttpResponsePtr Success(
    const Json::Value& body,
    drogon::HttpStatusCode code
) {
    auto response = drogon::HttpResponse::newHttpJsonResponse(body);
    response->setStatusCode(code);
    return response;
}

[[nodiscard]] std::string BearerToken(const drogon::HttpRequestPtr& request) {
    const std::string& header = request->getHeader("authorization");
    constexpr std::string_view kScheme = "bearer ";

    if (header.size() <= kScheme.size()) {
        return {};
    }

    for (std::size_t at = 0; at < kScheme.size(); ++at) {
        if (std::tolower(static_cast<unsigned char>(header[at])) != kScheme[at]) {
            return {};
        }
    }

    return header.substr(kScheme.size());
}

[[nodiscard]] std::string Field(const std::shared_ptr<Json::Value>& body, const char* name) {
    if (!body || !body->isMember(name) || !(*body)[name].isString()) {
        return {};
    }

    return (*body)[name].asString();
}

[[nodiscard]] std::size_t Limit(const drogon::HttpRequestPtr& request) {
    const std::string raw = request->getParameter("limit");
    if (raw.empty()) {
        return 0;
    }

    try {
        const long long asked = std::stoll(raw);

        return asked <= 0 ? 0 : static_cast<std::size_t>(asked);
    } catch (const std::exception&) {
        return 0;
    }
}

}  // namespace

CommunicationController::CommunicationController(
    std::shared_ptr<ports::IPrincipalAuthenticator> authenticator,
    std::shared_ptr<app::OpenConversation> open,
    std::shared_ptr<app::SendMessage> send,
    std::shared_ptr<app::ListMessages> messages,
    std::shared_ptr<app::ListConversations> conversations,
    std::shared_ptr<RequestWorkers> workers
) : authenticator_(std::move(authenticator)),
      open_(std::move(open)),
      send_(std::move(send)),
      messages_(std::move(messages)),
      conversations_(std::move(conversations)),
      workers_(std::move(workers)) {}

void CommunicationController::OpenConversation(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& respond
) {
    const std::string token = BearerToken(request);
    const auto body = request->getJsonObject();

    if (!workers_->Submit([this, token, body, respond = std::move(respond)] {
        const auto caller = authenticator_->Authenticate(token);
        if (!caller) {
            respond(Failure(caller.error()));
            return;
        }

        const auto opened = open_->Execute(
            caller->Value(), Field(body, "counterpart"),
            Field(body, "order_number")
        );

        if (!opened) {
            respond(Failure(opened.error()));

            return;
        }

        respond(Success(wire::ToJson(*opened, *caller), drogon::k200OK));
    })) {
        respond(Failure(CommunicationError::kDependencyUnavailable));
    }
}

void CommunicationController::ListConversations(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& respond
) {
    const std::string token = BearerToken(request);
    const std::size_t limit = Limit(request);

    if (!workers_->Submit([this, token, limit, respond = std::move(respond)] {
        const auto caller = authenticator_->Authenticate(token);
        if (!caller) {
            respond(Failure(caller.error()));
            return;
        }

        const auto listed = conversations_->Execute(caller->Value(), limit);
        if (!listed) {
            respond(Failure(listed.error()));
            return;
        }

        Json::Value out(Json::arrayValue);
        for (const auto& conversation : *listed) {
            out.append(wire::ToJson(conversation, *caller));
        }
        Json::Value body;
        body["conversations"] = out;
        respond(Success(body, drogon::k200OK));
    })) {
        respond(Failure(CommunicationError::kDependencyUnavailable));
    }
}

void CommunicationController::SendMessage(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& respond,
    std::string conversation
) {
    const std::string token = BearerToken(request);
    const auto body = request->getJsonObject();

    if (!workers_->Submit([this, token, body, conversation = std::move(conversation), respond = std::move(respond)] {
        const auto caller = authenticator_->Authenticate(token);
        if (!caller) {
            respond(Failure(caller.error()));
            return;
        }

        const auto sent = send_->Execute(caller->Value(), conversation, Field(body, "body"));
        if (!sent) {
            respond(Failure(sent.error()));
            return;
        }

        Json::Value out = wire::ToJson(sent->message);

        out["delivered_live"] = sent->broadcast;
        respond(Success(out, drogon::k201Created));
    })) {
        respond(Failure(CommunicationError::kDependencyUnavailable));
    }
}

void CommunicationController::ListMessages(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& respond,
    std::string conversation
) {
    const std::string token = BearerToken(request);
    const std::string before = request->getParameter("before");
    const std::size_t limit = Limit(request);

    if (!workers_->Submit([this, token, before, limit, conversation = std::move(conversation), respond = std::move(respond)] {
        const auto caller = authenticator_->Authenticate(token);
        if (!caller) {
            respond(Failure(caller.error()));
            return;
        }

        const auto listed = messages_->Execute(caller->Value(), conversation, before, limit);
        if (!listed) {
            respond(Failure(listed.error()));
            return;
        }

        Json::Value out(Json::arrayValue);
        for (const auto& message : *listed) {
            out.append(wire::ToJson(message));
        }
        Json::Value body;
        body["messages"] = out;

        body["next_before"] = listed->empty()
            ? Json::Value::nullSingleton()
            : Json::Value(listed->front().Id().Value());
        respond(Success(body, drogon::k200OK));
    })) {
        respond(Failure(CommunicationError::kDependencyUnavailable));
    }
}

}  // namespace communication::adapters::http
