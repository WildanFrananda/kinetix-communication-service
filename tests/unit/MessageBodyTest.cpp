#include <string>

#include <gtest/gtest.h>

#include "domain/MessageBody.hpp"

namespace communication::domain {
namespace {

TEST(MessageBodyTest, RefusesAnEmptyMessage) {
    EXPECT_FALSE(MessageBody::From("").has_value());
    EXPECT_FALSE(MessageBody::From("    ").has_value());
}

TEST(MessageBodyTest, KeepsTheLineBreaksPeopleActuallyType) {
    const auto body = MessageBody::From("Jalan Merdeka 12\nBlok B, no 4\nJakarta");
    ASSERT_TRUE(body.has_value());
    EXPECT_EQ(body->Value(), "Jalan Merdeka 12\nBlok B, no 4\nJakarta");
}

TEST(MessageBodyTest, RefusesControlBytesThatAreNotLayout) {
    EXPECT_FALSE(MessageBody::From("hello\x1b[31mred").has_value());
}

TEST(MessageBodyTest, RefusesAMessageOverTheLimitButAcceptsOneAtIt) {
    const std::string at_limit(MessageBody::kMaxLength, 'a');
    const std::string over_limit(MessageBody::kMaxLength + 1, 'a');
    EXPECT_TRUE(MessageBody::From(at_limit).has_value());
    EXPECT_FALSE(MessageBody::From(over_limit).has_value());
}

TEST(MessageBodyTest, LengthIsMeasuredAfterTrimming) {
    // A body that is only over the limit because of trailing spaces is not over the limit.
    const std::string padded = std::string(MessageBody::kMaxLength, 'a') + "   ";
    EXPECT_TRUE(MessageBody::From(padded).has_value());
}

}  // namespace
}  // namespace communication::domain
