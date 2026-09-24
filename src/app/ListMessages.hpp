#pragma once

#include <cstddef>
#include <expected>
#include <memory>
#include <string_view>
#include <vector>

#include "domain/CommunicationError.hpp"
#include "domain/Message.hpp"
#include "ports/IConversationRepository.hpp"
#include "ports/IMessageRepository.hpp"

namespace communication::app {

class ListMessages {
  public:
    static constexpr std::size_t kMaxPage = 100;
    static constexpr std::size_t kDefaultPage = 50;

    ListMessages(
        std::shared_ptr<ports::IConversationRepository> conversations,
        std::shared_ptr<ports::IMessageRepository> messages
    );

    [[nodiscard]] std::expected<std::vector<domain::Message>, domain::CommunicationError> Execute(
        std::string_view requester, std::string_view conversation, std::string_view before,
        std::size_t limit
    );

  private:
    std::shared_ptr<ports::IConversationRepository> conversations_;
    std::shared_ptr<ports::IMessageRepository> messages_;
};

}  // namespace communication::app
