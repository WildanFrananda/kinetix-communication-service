#include <optional>

#include <gtest/gtest.h>

#include "domain/Conversation.hpp"

#include "support/Sample.hpp"

namespace communication::domain {
namespace {

using testing::kNoon;

TEST(ConversationTest, HoldsTwoPrincipalsAndOptionallyAnOrder) {
    const auto thread = testing::Conversation("c1", "buyer", "merchant");
    EXPECT_EQ(thread.Id(), testing::ConversationId("c1"));
    EXPECT_EQ(thread.First(), testing::Principal("buyer"));
    EXPECT_EQ(thread.Second(), testing::Principal("merchant"));
    EXPECT_FALSE(thread.AboutOrder().has_value());
    EXPECT_EQ(thread.StartedAt(), kNoon);
}

TEST(ConversationTest, RemembersTheOrderWhenThereIsOne) {
    const auto thread = testing::Conversation("c1", "buyer", "merchant", testing::Order("KNX-1"));
    ASSERT_TRUE(thread.AboutOrder().has_value());
    EXPECT_EQ(thread.AboutOrder()->Value(), "KNX-1");
}

TEST(ConversationTest, RefusesAConversationWithOneself) {
    const auto refused =
        Conversation::Between(testing::ConversationId("c1"), testing::Principal("buyer"),
                              testing::Principal("buyer"), std::nullopt, kNoon);
    ASSERT_FALSE(refused.has_value());
    EXPECT_EQ(refused.error(), CommunicationError::kConversationWithSelf);
}

TEST(ConversationTest, IncludesOnlyItsTwoParties) {
    const auto thread = testing::Conversation("c1", "buyer", "merchant");
    EXPECT_TRUE(thread.Includes(testing::Principal("buyer")));
    EXPECT_TRUE(thread.Includes(testing::Principal("merchant")));
    EXPECT_FALSE(thread.Includes(testing::Principal("stranger")));
}

TEST(ConversationTest, NamesTheCounterpartFromEitherSide) {
    const auto thread = testing::Conversation("c1", "buyer", "merchant");
    EXPECT_EQ(thread.CounterpartOf(testing::Principal("buyer")), testing::Principal("merchant"));
    EXPECT_EQ(thread.CounterpartOf(testing::Principal("merchant")), testing::Principal("buyer"));
}

TEST(ConversationTest, NamesNobodyToAStranger) {
    // The point of the empty answer: a stranger asking who the other party is must not be told.
    const auto thread = testing::Conversation("c1", "buyer", "merchant");
    EXPECT_FALSE(thread.CounterpartOf(testing::Principal("stranger")).has_value());
}

TEST(ConversationTest, OrdersAPairTheSameWayRoundWhicheverWayItIsGiven) {
    const auto buyer = testing::Principal("aaa");
    const auto merchant = testing::Principal("bbb");
    const auto one_way = Conversation::InCanonicalOrder(buyer, merchant);
    const auto other_way = Conversation::InCanonicalOrder(merchant, buyer);
    EXPECT_EQ(one_way, other_way);
    EXPECT_EQ(one_way[0], buyer);
    EXPECT_EQ(one_way[1], merchant);
}

}  // namespace
}  // namespace communication::domain
