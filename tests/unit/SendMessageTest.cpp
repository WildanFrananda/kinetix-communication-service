#include <memory>
#include <optional>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "app/SendMessage.hpp"
#include "support/CountingIdentifierSource.hpp"
#include "support/FixedClock.hpp"
#include "support/MockConversationRepository.hpp"
#include "support/MockMessageBroadcaster.hpp"
#include "support/MockMessageRepository.hpp"
#include "support/Sample.hpp"

namespace communication::app {
namespace {

using communication::testing::kNoon;
using ::testing::_;
using ::testing::InSequence;
using ::testing::NiceMock;
using ::testing::Return;

class SendMessageTest : public ::testing::Test {
  protected:
    std::shared_ptr<NiceMock<communication::testing::MockConversationRepository>> conversations_ =
        std::make_shared<NiceMock<communication::testing::MockConversationRepository>>();
    std::shared_ptr<NiceMock<communication::testing::MockMessageRepository>> messages_ =
        std::make_shared<NiceMock<communication::testing::MockMessageRepository>>();
    std::shared_ptr<NiceMock<communication::testing::MockMessageBroadcaster>> broadcaster_ =
        std::make_shared<NiceMock<communication::testing::MockMessageBroadcaster>>();
    std::shared_ptr<communication::testing::CountingIdentifierSource> identifiers_ =
        std::make_shared<communication::testing::CountingIdentifierSource>();
    std::shared_ptr<communication::testing::FixedClock> clock_ =
        std::make_shared<communication::testing::FixedClock>(kNoon);

    SendMessage Subject() {
        return SendMessage(conversations_, messages_, broadcaster_, identifiers_, clock_);
    }

    void ConversationExists() {
        ON_CALL(*conversations_, FindById(_))
            .WillByDefault(Return(std::optional<domain::Conversation>{
                communication::testing::Conversation("conv-1", "buyer", "merchant")}));
    }

    void StorageWorks() {
        ON_CALL(*messages_, Append(_))
            .WillByDefault(Return(std::expected<void, domain::CommunicationError>{}));
    }

    void BroadcastWorks() {
        ON_CALL(*broadcaster_, Publish(_, _))
            .WillByDefault(Return(std::expected<void, domain::CommunicationError>{}));
    }
};

TEST_F(SendMessageTest, StoresTheMessageAndSaysItWentOut) {
    ConversationExists();
    StorageWorks();
    BroadcastWorks();

    const auto sent = Subject().Execute("buyer", "conv-1", "Is this still available?");

    ASSERT_TRUE(sent.has_value());
    EXPECT_TRUE(sent->broadcast);
    EXPECT_EQ(sent->message.Id(), communication::testing::MessageId("msg-1"));
    EXPECT_EQ(sent->message.BelongsTo(), communication::testing::ConversationId("conv-1"));
    EXPECT_EQ(sent->message.Sender(), communication::testing::Principal("buyer"));
    EXPECT_EQ(sent->message.Body().Value(), "Is this still available?");
    EXPECT_EQ(sent->message.SentAt(), kNoon);
}

TEST_F(SendMessageTest, StoresBeforeItBroadcasts) {
    // The ordering is the point. A message broadcast first and then lost is one the other party has
    // read and will never see again, and no later fetch can repair that.
    ConversationExists();
    StorageWorks();
    BroadcastWorks();

    InSequence ordered;
    EXPECT_CALL(*messages_, Append(_)).WillOnce(Return(std::expected<void, domain::CommunicationError>{}));
    EXPECT_CALL(*broadcaster_, Publish(_, _))
        .WillOnce(Return(std::expected<void, domain::CommunicationError>{}));

    EXPECT_TRUE(Subject().Execute("buyer", "conv-1", "hello").has_value());
}

TEST_F(SendMessageTest, SucceedsButReportsWhenTheLiveBroadcastFails) {
    ConversationExists();
    StorageWorks();
    ON_CALL(*broadcaster_, Publish(_, _))
        .WillByDefault(Return(std::unexpected(domain::CommunicationError::kDependencyUnavailable)));

    const auto sent = Subject().Execute("buyer", "conv-1", "hello");

    ASSERT_TRUE(sent.has_value()) << "the message is in the record; the send did not fail";
    EXPECT_FALSE(sent->broadcast);
}

TEST_F(SendMessageTest, DoesNotBroadcastWhatItCouldNotStore) {
    ConversationExists();
    ON_CALL(*messages_, Append(_))
        .WillByDefault(Return(std::unexpected(domain::CommunicationError::kDependencyUnavailable)));
    EXPECT_CALL(*broadcaster_, Publish(_, _)).Times(0);

    const auto refused = Subject().Execute("buyer", "conv-1", "hello");

    ASSERT_FALSE(refused.has_value());
    EXPECT_EQ(refused.error(), domain::CommunicationError::kDependencyUnavailable);
}

TEST_F(SendMessageTest, RefusesAPrincipalWhoIsNotAParty) {
    ConversationExists();
    EXPECT_CALL(*messages_, Append(_)).Times(0);

    const auto refused = Subject().Execute("stranger", "conv-1", "hello");

    ASSERT_FALSE(refused.has_value());
    EXPECT_EQ(refused.error(), domain::CommunicationError::kNotAParticipant);
}

TEST_F(SendMessageTest, RefusesAConversationThatDoesNotExist) {
    ON_CALL(*conversations_, FindById(_))
        .WillByDefault(Return(std::optional<domain::Conversation>{}));

    const auto refused = Subject().Execute("buyer", "conv-nowhere", "hello");

    ASSERT_FALSE(refused.has_value());
    EXPECT_EQ(refused.error(), domain::CommunicationError::kConversationNotFound);
}

TEST_F(SendMessageTest, RefusesAnEmptyMessage) {
    ConversationExists();
    EXPECT_CALL(*messages_, Append(_)).Times(0);

    const auto refused = Subject().Execute("buyer", "conv-1", "   ");

    ASSERT_FALSE(refused.has_value());
    EXPECT_EQ(refused.error(), domain::CommunicationError::kMalformedInput);
}

TEST_F(SendMessageTest, RefusesBeforeTouchingStorageWhenTheInputIsMalformed) {
    EXPECT_CALL(*conversations_, FindById(_)).Times(0);

    EXPECT_EQ(Subject().Execute("", "conv-1", "hello").error(), domain::CommunicationError::kMalformedInput);
}

TEST_F(SendMessageTest, ReportsAFailureToLoadTheConversation) {
    ON_CALL(*conversations_, FindById(_))
        .WillByDefault(Return(std::unexpected(domain::CommunicationError::kDependencyUnavailable)));

    const auto refused = Subject().Execute("buyer", "conv-1", "hello");

    ASSERT_FALSE(refused.has_value());
    EXPECT_EQ(refused.error(), domain::CommunicationError::kDependencyUnavailable);
}

}  // namespace
}  // namespace communication::app
