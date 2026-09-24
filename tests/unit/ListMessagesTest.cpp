#include <memory>
#include <optional>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "app/ListMessages.hpp"
#include "support/MockConversationRepository.hpp"
#include "support/MockMessageRepository.hpp"
#include "support/Sample.hpp"

namespace communication::app {
namespace {

using communication::testing::kNoon;
using ::testing::_;
using ::testing::Eq;
using ::testing::NiceMock;
using ::testing::Return;

class ListMessagesTest : public ::testing::Test {
  protected:
    std::shared_ptr<NiceMock<communication::testing::MockConversationRepository>> conversations_ =
        std::make_shared<NiceMock<communication::testing::MockConversationRepository>>();
    std::shared_ptr<NiceMock<communication::testing::MockMessageRepository>> messages_ =
        std::make_shared<NiceMock<communication::testing::MockMessageRepository>>();

    ListMessages Subject() { return ListMessages(conversations_, messages_); }

    void ConversationExists() {
        ON_CALL(*conversations_, FindById(_))
            .WillByDefault(Return(std::optional<domain::Conversation>{
                communication::testing::Conversation("conv-1", "buyer", "merchant")}));
    }

    void PageIsEmpty() {
        ON_CALL(*messages_, ListIn(_, _, _)).WillByDefault(Return(std::vector<domain::Message>{}));
    }
};

TEST_F(ListMessagesTest, ReturnsThePageForAParticipant) {
    ConversationExists();
    const std::vector<domain::Message> page{
        domain::Message(communication::testing::MessageId("msg-1"), communication::testing::ConversationId("conv-1"),
                        communication::testing::Principal("buyer"), communication::testing::Body("hello"), kNoon)};
    EXPECT_CALL(*messages_, ListIn(communication::testing::ConversationId("conv-1"), Eq(std::nullopt),
                                   ListMessages::kDefaultPage))
        .WillOnce(Return(page));

    const auto listed = Subject().Execute("buyer", "conv-1", "", 0);

    ASSERT_TRUE(listed.has_value());
    ASSERT_EQ(listed->size(), 1U);
    EXPECT_EQ(listed->front().Id(), communication::testing::MessageId("msg-1"));
}

TEST_F(ListMessagesTest, PassesTheCursorThrough) {
    ConversationExists();
    EXPECT_CALL(*messages_, ListIn(_, Eq(std::optional{communication::testing::MessageId("msg-9")}), _))
        .WillOnce(Return(std::vector<domain::Message>{}));

    EXPECT_TRUE(Subject().Execute("buyer", "conv-1", "msg-9", 0).has_value());
}

TEST_F(ListMessagesTest, CapsAPageNobodyShouldBeAbleToAskFor) {
    // The cap lives in the use case so that every edge inherits it, rather than the HTTP adapter
    // having one and the MQTT and gRPC edges having none.
    ConversationExists();
    EXPECT_CALL(*messages_, ListIn(_, _, ListMessages::kMaxPage))
        .WillOnce(Return(std::vector<domain::Message>{}));

    EXPECT_TRUE(Subject().Execute("buyer", "conv-1", "", 1'000'000).has_value());
}

TEST_F(ListMessagesTest, HonoursASmallerPageThanTheCap) {
    ConversationExists();
    EXPECT_CALL(*messages_, ListIn(_, _, 5U)).WillOnce(Return(std::vector<domain::Message>{}));

    EXPECT_TRUE(Subject().Execute("buyer", "conv-1", "", 5).has_value());
}

TEST_F(ListMessagesTest, RefusesToShowAConversationToSomebodyOutsideIt) {
    ConversationExists();
    EXPECT_CALL(*messages_, ListIn(_, _, _)).Times(0);

    const auto refused = Subject().Execute("stranger", "conv-1", "", 0);

    ASSERT_FALSE(refused.has_value());
    EXPECT_EQ(refused.error(), domain::CommunicationError::kNotAParticipant);
}

TEST_F(ListMessagesTest, RefusesAConversationThatDoesNotExist) {
    ON_CALL(*conversations_, FindById(_))
        .WillByDefault(Return(std::optional<domain::Conversation>{}));

    const auto refused = Subject().Execute("buyer", "conv-nowhere", "", 0);

    ASSERT_FALSE(refused.has_value());
    EXPECT_EQ(refused.error(), domain::CommunicationError::kConversationNotFound);
}

TEST_F(ListMessagesTest, RefusesACursorItCannotParse) {
    ConversationExists();
    PageIsEmpty();

    const auto refused = Subject().Execute("buyer", "conv-1", "\x1b[0m", 0);

    ASSERT_FALSE(refused.has_value());
    EXPECT_EQ(refused.error(), domain::CommunicationError::kMalformedInput);
}

TEST_F(ListMessagesTest, ReportsAFailureRatherThanAnEmptyConversation) {
    // An empty list and a database that will not answer look identical to a reader. They must not
    // look identical to the caller.
    ConversationExists();
    ON_CALL(*messages_, ListIn(_, _, _))
        .WillByDefault(Return(std::unexpected(domain::CommunicationError::kDependencyUnavailable)));

    const auto refused = Subject().Execute("buyer", "conv-1", "", 0);

    ASSERT_FALSE(refused.has_value());
    EXPECT_EQ(refused.error(), domain::CommunicationError::kDependencyUnavailable);
}

}  // namespace
}  // namespace communication::app
