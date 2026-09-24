#pragma once

#include <memory>
#include <string>

#include <drogon/HttpController.h>

#include "ports/IPrincipalAuthenticator.hpp"

#include "adapters/http/RequestWorkers.hpp"
#include "app/ListConversations.hpp"
#include "app/ListMessages.hpp"
#include "app/OpenConversation.hpp"
#include "app/SendMessage.hpp"

namespace communication::adapters::http {

class CommunicationController : public drogon::HttpController<CommunicationController, false> {
  public:
    CommunicationController(
        std::shared_ptr<ports::IPrincipalAuthenticator> authenticator,
        std::shared_ptr<app::OpenConversation> open,
        std::shared_ptr<app::SendMessage> send,
        std::shared_ptr<app::ListMessages> messages,
        std::shared_ptr<app::ListConversations> conversations,
        std::shared_ptr<RequestWorkers> workers
    );

    METHOD_LIST_BEGIN
    METHOD_ADD(CommunicationController::OpenConversation, "", drogon::Post);
    METHOD_ADD(CommunicationController::ListConversations, "", drogon::Get);
    METHOD_ADD(CommunicationController::SendMessage, "/{id}/messages", drogon::Post);
    METHOD_ADD(CommunicationController::ListMessages, "/{id}/messages", drogon::Get);
    METHOD_LIST_END

    void OpenConversation(
        const drogon::HttpRequestPtr& request,
        std::function<void(const drogon::HttpResponsePtr&)>&& respond
    );

    void ListConversations(
        const drogon::HttpRequestPtr& request,
        std::function<void(const drogon::HttpResponsePtr&)>&& respond
    );

    void SendMessage(
        const drogon::HttpRequestPtr& request,
        std::function<void(const drogon::HttpResponsePtr&)>&& respond,
        std::string conversation
    );

    void ListMessages(
        const drogon::HttpRequestPtr& request,
        std::function<void(const drogon::HttpResponsePtr&)>&& respond,
        std::string conversation
    );

  private:
    std::shared_ptr<ports::IPrincipalAuthenticator> authenticator_;
    std::shared_ptr<app::OpenConversation> open_;
    std::shared_ptr<app::SendMessage> send_;
    std::shared_ptr<app::ListMessages> messages_;
    std::shared_ptr<app::ListConversations> conversations_;
    std::shared_ptr<RequestWorkers> workers_;
};

}  // namespace communication::adapters::http
