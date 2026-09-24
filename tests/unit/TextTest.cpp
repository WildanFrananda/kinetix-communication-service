#include <string>

#include <gtest/gtest.h>

#include "domain/Text.hpp"

namespace communication::domain::text {
namespace {

TEST(TrimTest, RemovesWhitespaceFromBothEnds) {
    EXPECT_EQ(Trim("  hello  "), "hello");
    EXPECT_EQ(Trim("\t\nhello\r\n"), "hello");
}

TEST(TrimTest, LeavesInnerWhitespaceAlone) { EXPECT_EQ(Trim("  two  words  "), "two  words"); }

TEST(TrimTest, WhitespaceOnlyBecomesEmpty) {
    EXPECT_EQ(Trim(" \t\n "), "");
    EXPECT_EQ(Trim(""), "");
}

TEST(IsPrintableTest, AcceptsOrdinaryText) { EXPECT_TRUE(IsPrintable("Jalan Merdeka 12")); }

TEST(IsPrintableTest, RefusesTheControlBytesThatColourSomebodyElsesTerminal) {
    EXPECT_FALSE(IsPrintable("before\x1b[31mred"));
    EXPECT_FALSE(IsPrintable(std::string("null\0byte", 9)));
    EXPECT_FALSE(IsPrintable("\x7f"));
}

TEST(IsPrintableTest, RefusesLayoutCharactersToo) {
    // The difference from IsPrintableWithLayout, and the reason both exist: a newline inside an
    // identifier turns one log line into two, and the attacker wrote the second one.
    EXPECT_FALSE(IsPrintable("line one\nline two"));
    EXPECT_FALSE(IsPrintable("col\tcol"));
}

TEST(IsPrintableWithLayoutTest, AcceptsOrdinaryTextAndTheTwoLayoutCharacters) {
    EXPECT_TRUE(IsPrintableWithLayout("Jalan Merdeka 12"));
    EXPECT_TRUE(IsPrintableWithLayout("line one\nline two"));
    EXPECT_TRUE(IsPrintableWithLayout("col\tcol"));
}

TEST(IsPrintableWithLayoutTest, StillRefusesEveryOtherControlByte) {
    EXPECT_FALSE(IsPrintableWithLayout("before\x1b[31mred"));
    EXPECT_FALSE(IsPrintableWithLayout(std::string("null\0byte", 9)));
    EXPECT_FALSE(IsPrintableWithLayout("\x7f"));
}

}  // namespace
}  // namespace communication::domain::text
