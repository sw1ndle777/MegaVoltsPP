#include <gtest/gtest.h>
#include <common/BaseLib/Utility.h>

TEST(ToLowercase, AllUppercase)
{
    EXPECT_EQ(Utility::ToLowercase(std::string("HELLO")), "hello");
}

TEST(ToLowercase, MixedCase)
{
    EXPECT_EQ(Utility::ToLowercase(std::string("HeLLo WoRLd")), "hello world");
}

TEST(ToLowercase, AlreadyLowercase)
{
    EXPECT_EQ(Utility::ToLowercase(std::string("test")), "test");
}

TEST(ToLowercase, EmptyString)
{
    EXPECT_EQ(Utility::ToLowercase(std::string("")), "");
}

TEST(ToLowercase, WithDigitsAndSpecial)
{
    EXPECT_EQ(Utility::ToLowercase(std::string("Test123!@#")), "test123!@#");
}

TEST(SplitStrings, BasicSplit)
{
    auto result = Utility::SplitStrings("a,b,c", ',');
    ASSERT_EQ(result.size(), 3u);
    EXPECT_EQ(result[0], "a");
    EXPECT_EQ(result[1], "b");
    EXPECT_EQ(result[2], "c");
}

TEST(SplitStrings, NoDelimiter)
{
    auto result = Utility::SplitStrings("hello", ',');
    ASSERT_EQ(result.size(), 1u);
    EXPECT_EQ(result[0], "hello");
}

TEST(SplitStrings, EmptyString)
{
    auto result = Utility::SplitStrings("", ',');
    EXPECT_TRUE(result.empty());
}

TEST(SplitStrings, ConsecutiveDelimiters)
{
    auto result = Utility::SplitStrings("a,,b", ',');
    ASSERT_EQ(result.size(), 2u);
    EXPECT_EQ(result[0], "a");
    EXPECT_EQ(result[1], "b");
}

TEST(SplitStrings, TrailingDelimiter)
{
    auto result = Utility::SplitStrings("a,b,", ',');
    ASSERT_EQ(result.size(), 2u);
    EXPECT_EQ(result[0], "a");
    EXPECT_EQ(result[1], "b");
}

TEST(SplitStrings, LeadingDelimiter)
{
    auto result = Utility::SplitStrings(",a,b", ',');
    ASSERT_EQ(result.size(), 2u);
    EXPECT_EQ(result[0], "a");
    EXPECT_EQ(result[1], "b");
}

TEST(IsDigitsOnly, AllDigits)
{
    EXPECT_TRUE(Utility::IsDigitsOnly("12345"));
}

TEST(IsDigitsOnly, ContainsLetters)
{
    EXPECT_FALSE(Utility::IsDigitsOnly("123abc"));
}

TEST(IsDigitsOnly, EmptyString)
{
    EXPECT_TRUE(Utility::IsDigitsOnly(""));
}

TEST(IsDigitsOnly, SingleDigit)
{
    EXPECT_TRUE(Utility::IsDigitsOnly("0"));
}

TEST(IsDigitsOnly, NegativeSign)
{
    EXPECT_FALSE(Utility::IsDigitsOnly("-1"));
}

TEST(ReadMVString, NullTerminated)
{
    std::string_view sv("hello\0world", 11);
    auto result = Utility::ReadMVString(sv);
    EXPECT_EQ(result, "hello");
}

TEST(ReadMVString, NoNullTerminator)
{
    std::string_view sv("hello");
    auto result = Utility::ReadMVString(sv);
    EXPECT_EQ(result, "");
}

TEST(ReadMVString, EmptyInput)
{
    std::string_view sv;
    auto result = Utility::ReadMVString(sv);
    EXPECT_EQ(result, "");
}

TEST(ReadMVString, ImmediateNull)
{
    std::string_view sv("\0hello", 6);
    auto result = Utility::ReadMVString(sv);
    EXPECT_EQ(result, "");
}

TEST(ReadMicrovoltsString, BasicUsage)
{
    const char data[] = "test\0extra";
    auto result = Utility::ReadMicrovoltsString(data, 10);
    EXPECT_EQ(result, "test");
}

TEST(ReadMicrovoltsString, NullData)
{
    auto result = Utility::ReadMicrovoltsString(nullptr, 10);
    EXPECT_EQ(result, "");
}

TEST(ReadMicrovoltsString, ZeroSize)
{
    auto result = Utility::ReadMicrovoltsString("hello", 0);
    EXPECT_EQ(result, "");
}

TEST(ReadMicrovoltsString, NoTerminator)
{
    const char data[] = {'h', 'e', 'l', 'l', 'o'};
    auto result = Utility::ReadMicrovoltsString(data, 5);
    EXPECT_EQ(result, "hello");
}
