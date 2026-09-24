#include <memory>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "app/ListConversations.hpp"
#include "support/MockConversationRepository.hpp"
#include "support/Sample.hpp"

namespace communication::app {
namespace {

using ::testing::_;
using ::testing::NiceMock;
using ::testing::Return;

class ListConversationsTest : public ::testing::Test {
  protected:
    std::shared_ptr<NiceMock<communication::testing::MockConversationRepository>> conversations_ =
        std::make_shared<NiceMock<communication::testing::MockConversationRepository>>();

    ListConversations Subject() { return ListConversations(conversations_); }
};

TEST_F(ListConversationsTest, AsksOnlyForTheRequestersOwnConversations) {
    // The requester is the filter. There is no argument for whose list to fetch, because such an
    // argument is an endpoint that reads somebody else's inbox.
    EXPECT_CALL(*conversations_,
                ListFor(communication::testing::Principal("buyer"), ListConversations::kDefaultPage))
        .WillOnce(Return(std::vector<domain::Conversation>{
            communication::testing::Conversation("conv-1", "buyer", "merchant")}));

    const auto listed = Subject().Execute("buyer", 0);

    ASSERT_TRUE(listed.has_value());
    ASSERT_EQ(listed->size(), 1U);
    EXPECT_EQ(listed->front().Id(), communication::testing::ConversationId("conv-1"));
}

TEST_F(ListConversationsTest, CapsThePage) {
    EXPECT_CALL(*conversations_, ListFor(_, ListConversations::kMaxPage))
        .WillOnce(Return(std::vector<domain::Conversation>{}));

    EXPECT_TRUE(Subject().Execute("buyer", 1'000'000).has_value());
}

TEST_F(ListConversationsTest, RefusesAPrincipalItCannotParse) {
    EXPECT_CALL(*conversations_, ListFor(_, _)).Times(0);

    const auto refused = Subject().Execute("  ", 0);

    ASSERT_FALSE(refused.has_value());
    EXPECT_EQ(refused.error(), domain::CommunicationError::kMalformedInput);
}

TEST_F(ListConversationsTest, ReportsAFailureRatherThanAnEmptyInbox) {
    ON_CALL(*conversations_, ListFor(_, _))
        .WillByDefault(Return(std::unexpected(domain::CommunicationError::kDependencyUnavailable)));

    const auto refused = Subject().Execute("buyer", 0);

    ASSERT_FALSE(refused.has_value());
    EXPECT_EQ(refused.error(), domain::CommunicationError::kDependencyUnavailable);
}

}  // namespace
}  // namespace communication::app
