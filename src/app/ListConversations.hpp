#pragma once

#include <cstddef>
#include <expected>
#include <memory>
#include <string_view>
#include <vector>

#include "domain/CommunicationError.hpp"
#include "domain/Conversation.hpp"
#include "ports/IConversationRepository.hpp"

namespace communication::app {

class ListConversations {
  public:
    static constexpr std::size_t kMaxPage = 100;
    static constexpr std::size_t kDefaultPage = 50;

    explicit ListConversations(std::shared_ptr<ports::IConversationRepository> conversations);

    [[nodiscard]] std::expected<std::vector<domain::Conversation>, domain::CommunicationError> Execute(
        std::string_view requester,
        std::size_t limit
    );

  private:
    std::shared_ptr<ports::IConversationRepository> conversations_;
};

}  // namespace communication::app
