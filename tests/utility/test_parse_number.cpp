#include <gtest/gtest.h>
#include <common/BaseLib/Utility.h>
#include <cstdint>
#include <limits>

TEST(ParseNumber, ValidUint32)
{
    auto result = Utility::ParseNumber<uint32_t>("12345");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, 12345u);
}

TEST(ParseNumber, ValidInt32)
{
    auto result = Utility::ParseNumber<int32_t>("-42");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, -42);
}

TEST(ParseNumber, ValidUint64)
{
    auto result = Utility::ParseNumber<uint64_t>("18446744073709551615");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, UINT64_MAX);
}

TEST(ParseNumber, Zero)
{
    auto result = Utility::ParseNumber<uint32_t>("0");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, 0u);
}

TEST(ParseNumber, LeadingZeros)
{
    auto result = Utility::ParseNumber<uint32_t>("007");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, 7u);
}

TEST(ParseNumber, EmptyString)
{
    auto result = Utility::ParseNumber<uint32_t>("");
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), "empty input");
}

TEST(ParseNumber, NonNumeric)
{
    auto result = Utility::ParseNumber<uint32_t>("abc");
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), "not a number");
}

TEST(ParseNumber, TrailingCharacters)
{
    auto result = Utility::ParseNumber<uint32_t>("123abc");
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), "contains non-digit characters");
}

TEST(ParseNumber, OverflowUint32)
{
    auto result = Utility::ParseNumber<uint32_t>("4294967296");
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), "number out of range");
}

TEST(ParseNumber, OverflowInt32)
{
    auto result = Utility::ParseNumber<int32_t>("2147483648");
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), "number out of range");
}

TEST(ParseNumber, BoundaryUint32Max)
{
    auto result = Utility::ParseNumber<uint32_t>("4294967295");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, UINT32_MAX);
}

TEST(ParseNumber, BoundaryInt32Max)
{
    auto result = Utility::ParseNumber<int32_t>("2147483647");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, INT32_MAX);
}

TEST(ParseNumber, BoundaryInt32Min)
{
    auto result = Utility::ParseNumber<int32_t>("-2147483648");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, INT32_MIN);
}

TEST(ParseNumber, NegativeForUnsigned)
{
    auto result = Utility::ParseNumber<uint32_t>("-1");
    ASSERT_FALSE(result.has_value());
}

TEST(ParseNumber, StringViewInput)
{
    std::string_view sv = "99999";
    auto result = Utility::ParseNumber<uint32_t>(sv);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, 99999u);
}

TEST(ParseNumber, StdStringInput)
{
    std::string s = "65535";
    auto result = Utility::ParseNumber<uint16_t>(s);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, 65535u);
}
