#include <string>

#include <gtest/gtest.h>

#include "domain/ConversationId.hpp"
#include "domain/MessageId.hpp"
#include "domain/OrderNumber.hpp"
#include "domain/PrincipalId.hpp"

namespace communication::domain {
namespace {

/// The four identifier types answer From() by the same rules, so they are tested by the same
/// tests. Written out four times, three copies would drift and one of them would quietly start
/// accepting the empty string.
template <typename T>
class IdentifierTest : public ::testing::Test {};

using Identifiers = ::testing::Types<PrincipalId, ConversationId, MessageId, OrderNumber>;
TYPED_TEST_SUITE(IdentifierTest, Identifiers);

TYPED_TEST(IdentifierTest, AcceptsAnOrdinaryValue) {
    const auto made = TypeParam::From("018f2c4e-9a1b-7000-8000-a1b2c3d4e5f6");
    ASSERT_TRUE(made.has_value());
    EXPECT_EQ(made->Value(), "018f2c4e-9a1b-7000-8000-a1b2c3d4e5f6");
}

TYPED_TEST(IdentifierTest, RefusesEmptyAndWhitespaceOnly) {
    EXPECT_FALSE(TypeParam::From("").has_value());
    EXPECT_FALSE(TypeParam::From("   ").has_value());
    EXPECT_FALSE(TypeParam::From("\t\n").has_value());
}

TYPED_TEST(IdentifierTest, TrimsRatherThanRefusingSurroundingSpace) {
    const auto made = TypeParam::From("  abc  ");
    ASSERT_TRUE(made.has_value());
    EXPECT_EQ(made->Value(), "abc");
}

TYPED_TEST(IdentifierTest, RefusesAnythingLongerThanTheLimit) {
    const std::string at_limit(TypeParam::kMaxLength, 'x');
    const std::string over_limit(TypeParam::kMaxLength + 1, 'x');
    EXPECT_TRUE(TypeParam::From(at_limit).has_value());
    EXPECT_FALSE(TypeParam::From(over_limit).has_value());
}

TYPED_TEST(IdentifierTest, RefusesControlBytes) {
    EXPECT_FALSE(TypeParam::From("abc\x1b[0m").has_value());
    EXPECT_FALSE(TypeParam::From("two\nlines").has_value());
}

TYPED_TEST(IdentifierTest, ComparesByValue) {
    EXPECT_EQ(*TypeParam::From("same"), *TypeParam::From("same"));
    EXPECT_NE(*TypeParam::From("one"), *TypeParam::From("other"));
}

}  // namespace
}  // namespace communication::domain
