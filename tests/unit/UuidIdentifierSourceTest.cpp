#include <set>
#include <string>

#include <gtest/gtest.h>

#include "adapters/system/UuidIdentifierSource.hpp"

namespace communication::adapters::system {
namespace {

/// This adapter is tested with the domain rather than with the other adapters because it needs
/// nothing but the standard library. The Drogon, gRPC and MQTT adapters cannot be, and that is
/// the line: an adapter that can be checked cheaply should be.
TEST(UuidIdentifierSourceTest, ProducesTheShapeOfAVersionFourUuid) {
    UuidIdentifierSource source;
    const std::string id = source.NewConversationId().Value();

    ASSERT_EQ(id.size(), 36U);
    EXPECT_EQ(id[8], '-');
    EXPECT_EQ(id[13], '-');
    EXPECT_EQ(id[18], '-');
    EXPECT_EQ(id[23], '-');
    EXPECT_EQ(id[14], '4') << "version nibble";
    EXPECT_TRUE(id[19] == '8' || id[19] == '9' || id[19] == 'a' || id[19] == 'b') << "variant";

    for (std::size_t at = 0; at < id.size(); ++at) {
        if (at == 8 || at == 13 || at == 18 || at == 23) {
            continue;
        }
        EXPECT_NE(std::string("0123456789abcdef").find(id[at]), std::string::npos)
            << "non-hex character at " << at;
    }
}

TEST(UuidIdentifierSourceTest, DoesNotRepeatItself) {
    // Not a test of randomness quality — that is not something a unit test can assert. It catches
    // the failure that actually happens: a generator seeded once per call, or not advanced at all,
    // which hands every conversation the same id.
    UuidIdentifierSource source;
    std::set<std::string> seen;
    for (int i = 0; i < 1000; ++i) {
        seen.insert(source.NewMessageId().Value());
    }
    EXPECT_EQ(seen.size(), 1000U);
}

TEST(UuidIdentifierSourceTest, MintsConversationAndMessageIdsFromTheSameStream) {
    UuidIdentifierSource source;
    EXPECT_NE(source.NewConversationId().Value(), source.NewMessageId().Value());
}

}  // namespace
}  // namespace communication::adapters::system
