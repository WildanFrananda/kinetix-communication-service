#include <memory>
#include <optional>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "app/OpenConversation.hpp"
#include "support/CountingIdentifierSource.hpp"
#include "support/FixedClock.hpp"
#include "support/MockConversationRepository.hpp"
#include "support/MockPrincipalDirectory.hpp"
#include "support/Sample.hpp"

namespace communication::app {
namespace {

using communication::testing::kNoon;
using ::testing::_;
using ::testing::NiceMock;
using ::testing::Return;

class OpenConversationTest : public ::testing::Test {
  protected:
    std::shared_ptr<NiceMock<communication::testing::MockConversationRepository>> conversations_ =
        std::make_shared<NiceMock<communication::testing::MockConversationRepository>>();
    std::shared_ptr<NiceMock<communication::testing::MockPrincipalDirectory>> directory_ =
        std::make_shared<NiceMock<communication::testing::MockPrincipalDirectory>>();
    std::shared_ptr<communication::testing::CountingIdentifierSource> identifiers_ =
        std::make_shared<communication::testing::CountingIdentifierSource>();
    std::shared_ptr<communication::testing::FixedClock> clock_ =
        std::make_shared<communication::testing::FixedClock>(kNoon);

    OpenConversation Subject() {
        return OpenConversation(conversations_, directory_, identifiers_, clock_);
    }

    void NoExistingConversation() {
        ON_CALL(*conversations_, FindBetween(_, _, _))
            .WillByDefault(Return(std::optional<domain::Conversation>{}));
    }

    void EverybodyExists() { ON_CALL(*directory_, Exists(_)).WillByDefault(Return(true)); }
};

TEST_F(OpenConversationTest, MintsAConversationWhenTheTwoHaveNoneYet) {
    NoExistingConversation();
    EverybodyExists();
    EXPECT_CALL(*conversations_, Save(_))
        .WillOnce(Return(std::expected<void, domain::CommunicationError>{}));

    const auto opened = Subject().Execute("buyer", "merchant", "");

    ASSERT_TRUE(opened.has_value());
    EXPECT_EQ(opened->Id(), communication::testing::ConversationId("conv-1"));
    EXPECT_EQ(opened->First(), communication::testing::Principal("buyer"));
    EXPECT_EQ(opened->Second(), communication::testing::Principal("merchant"));
    EXPECT_EQ(opened->StartedAt(), kNoon);
}

TEST_F(OpenConversationTest, HandsBackTheExistingConversationInsteadOfASecondOne) {
    const auto already = communication::testing::Conversation("conv-existing", "buyer", "merchant");
    EXPECT_CALL(*conversations_, FindBetween(_, _, _))
        .WillOnce(Return(std::optional<domain::Conversation>{already}));
    EXPECT_CALL(*conversations_, Save(_)).Times(0);

    const auto opened = Subject().Execute("buyer", "merchant", "");

    ASSERT_TRUE(opened.has_value());
    EXPECT_EQ(opened->Id(), communication::testing::ConversationId("conv-existing"));
}

TEST_F(OpenConversationTest, DoesNotAskIdentityWhenTheConversationAlreadyExists) {
    const auto already = communication::testing::Conversation("conv-existing", "buyer", "merchant");
    ON_CALL(*conversations_, FindBetween(_, _, _))
        .WillByDefault(Return(std::optional<domain::Conversation>{already}));
    EXPECT_CALL(*directory_, Exists(_)).Times(0);

    EXPECT_TRUE(Subject().Execute("buyer", "merchant", "").has_value());
}

TEST_F(OpenConversationTest, RemembersTheOrderWhenOneIsGiven) {
    NoExistingConversation();
    EverybodyExists();
    ON_CALL(*conversations_, Save(_))
        .WillByDefault(Return(std::expected<void, domain::CommunicationError>{}));

    const auto opened = Subject().Execute("buyer", "merchant", "KNX-20260921-7QF3");

    ASSERT_TRUE(opened.has_value());
    ASSERT_TRUE(opened->AboutOrder().has_value());
    EXPECT_EQ(opened->AboutOrder()->Value(), "KNX-20260921-7QF3");
}

TEST_F(OpenConversationTest, RefusesAnOrderThatIsPresentButUnusable) {
    // Silently dropping it would file the conversation under "no order" and lose it there.
    const auto refused = Subject().Execute("buyer", "merchant", "\x1b[31m");
    ASSERT_FALSE(refused.has_value());
    EXPECT_EQ(refused.error(), domain::CommunicationError::kMalformedInput);
}

TEST_F(OpenConversationTest, RefusesAPrincipalItCannotParse) {
    EXPECT_EQ(Subject().Execute("", "merchant", "").error(), domain::CommunicationError::kMalformedInput);
    EXPECT_EQ(Subject().Execute("buyer", "   ", "").error(), domain::CommunicationError::kMalformedInput);
}

TEST_F(OpenConversationTest, RefusesAConversationWithOneself) {
    NoExistingConversation();
    EverybodyExists();
    const auto refused = Subject().Execute("buyer", "buyer", "");
    ASSERT_FALSE(refused.has_value());
    EXPECT_EQ(refused.error(), domain::CommunicationError::kConversationWithSelf);
}

TEST_F(OpenConversationTest, RefusesWhenIdentityDoesNotKnowTheCounterpart) {
    NoExistingConversation();
    EXPECT_CALL(*directory_, Exists(communication::testing::Principal("buyer"))).WillOnce(Return(true));
    EXPECT_CALL(*directory_, Exists(communication::testing::Principal("ghost"))).WillOnce(Return(false));
    EXPECT_CALL(*conversations_, Save(_)).Times(0);

    const auto refused = Subject().Execute("buyer", "ghost", "");

    ASSERT_FALSE(refused.has_value());
    EXPECT_EQ(refused.error(), domain::CommunicationError::kUnknownPrincipal);
}

TEST_F(OpenConversationTest, ChecksTheRequesterToo) {
    NoExistingConversation();
    EXPECT_CALL(*directory_, Exists(communication::testing::Principal("ghost"))).WillOnce(Return(false));

    const auto refused = Subject().Execute("ghost", "merchant", "");

    ASSERT_FALSE(refused.has_value());
    EXPECT_EQ(refused.error(), domain::CommunicationError::kUnknownPrincipal);
}

TEST_F(OpenConversationTest, FailsRatherThanGuessingWhenIdentityIsUnreachable) {
    // The distinction this whole port exists for: an outage must not be reported as "no such
    // customer", which is what a bool-returning directory would have made it.
    NoExistingConversation();
    ON_CALL(*directory_, Exists(_))
        .WillByDefault(Return(std::unexpected(domain::CommunicationError::kDependencyUnavailable)));
    EXPECT_CALL(*conversations_, Save(_)).Times(0);

    const auto refused = Subject().Execute("buyer", "merchant", "");

    ASSERT_FALSE(refused.has_value());
    EXPECT_EQ(refused.error(), domain::CommunicationError::kDependencyUnavailable);
}

TEST_F(OpenConversationTest, ReportsAFailureToSave) {
    NoExistingConversation();
    EverybodyExists();
    ON_CALL(*conversations_, Save(_))
        .WillByDefault(Return(std::unexpected(domain::CommunicationError::kDependencyUnavailable)));

    const auto refused = Subject().Execute("buyer", "merchant", "");

    ASSERT_FALSE(refused.has_value());
    EXPECT_EQ(refused.error(), domain::CommunicationError::kDependencyUnavailable);
}

TEST_F(OpenConversationTest, UsesTheWinnersConversationWhenTwoDevicesOpenAtTheSameInstant) {
    const auto theirs = communication::testing::Conversation("conv-theirs", "buyer", "merchant");
    EverybodyExists();
    // Nothing on the first look; the row appears between the look and the write.
    EXPECT_CALL(*conversations_, FindBetween(_, _, _))
        .WillOnce(Return(std::optional<domain::Conversation>{}))
        .WillOnce(Return(std::optional<domain::Conversation>{theirs}));
    EXPECT_CALL(*conversations_, Save(_))
        .WillOnce(Return(std::unexpected(domain::CommunicationError::kConversationAlreadyExists)));

    const auto opened = Subject().Execute("buyer", "merchant", "");

    ASSERT_TRUE(opened.has_value())
        << "opening stays idempotent under a race, not only in sequence";
    EXPECT_EQ(opened->Id(), communication::testing::ConversationId("conv-theirs"));
}

TEST_F(OpenConversationTest, RefusesToInventAnAnswerWhenTheWriteAndTheReadDisagree) {
    // The write says the row exists, the read says it does not. Returning the conversation it
    // failed to store would hand the caller an id that is in nobody's database.
    EverybodyExists();
    EXPECT_CALL(*conversations_, FindBetween(_, _, _))
        .WillOnce(Return(std::optional<domain::Conversation>{}))
        .WillOnce(Return(std::optional<domain::Conversation>{}));
    ON_CALL(*conversations_, Save(_))
        .WillByDefault(Return(std::unexpected(domain::CommunicationError::kConversationAlreadyExists)));

    const auto refused = Subject().Execute("buyer", "merchant", "");

    ASSERT_FALSE(refused.has_value());
    EXPECT_EQ(refused.error(), domain::CommunicationError::kDependencyUnavailable);
}

TEST_F(OpenConversationTest, ReportsAFailureToLookForAnExistingConversation) {
    ON_CALL(*conversations_, FindBetween(_, _, _))
        .WillByDefault(Return(std::unexpected(domain::CommunicationError::kDependencyUnavailable)));

    const auto refused = Subject().Execute("buyer", "merchant", "");

    ASSERT_FALSE(refused.has_value());
    EXPECT_EQ(refused.error(), domain::CommunicationError::kDependencyUnavailable);
}

}  // namespace
}  // namespace communication::app
