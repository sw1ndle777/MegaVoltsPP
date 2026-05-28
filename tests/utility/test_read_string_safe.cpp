#include <gtest/gtest.h>
#include <common/BaseLib/Utility.h>
#include <cstring>

TEST(ReadStringSafe, StdStringWithinMax)
{
    std::string input = "hello";
    auto result = Utility::ReadStringSafe(std::move(input), 10);
    EXPECT_EQ(result, "hello");
}

TEST(ReadStringSafe, StdStringExceedsMax)
{
    std::string input = "hello world";
    auto result = Utility::ReadStringSafe(input, 5);
    EXPECT_EQ(result, "hello");
}

TEST(ReadStringSafe, StdStringExactMax)
{
    std::string input = "hello";
    auto result = Utility::ReadStringSafe(input, 5);
    EXPECT_EQ(result, "hello");
}

TEST(ReadStringSafe, StringViewWithinMax)
{
    std::string_view sv = "test123";
    auto result = Utility::ReadStringSafe(sv, 20);
    EXPECT_EQ(result, "test123");
}

TEST(ReadStringSafe, StringViewExceedsMax)
{
    std::string_view sv = "test123";
    auto result = Utility::ReadStringSafe(sv, 4);
    EXPECT_EQ(result, "test");
}

TEST(ReadStringSafe, ConstCharPtrNullTerminated)
{
    const char* p = "hello";
    auto result = Utility::ReadStringSafe(p, 10);
    EXPECT_EQ(result, "hello");
}

TEST(ReadStringSafe, ConstCharPtrTruncated)
{
    const char* p = "hello world";
    auto result = Utility::ReadStringSafe(p, 5);
    EXPECT_EQ(result, "hello");
}

TEST(ReadStringSafe, NullPtr)
{
    const char* p = nullptr;
    auto result = Utility::ReadStringSafe(p, 10);
    EXPECT_TRUE(result.empty());
}

TEST(ReadStringSafe, CharArrayWithinMax)
{
    char arr[6] = "hello";
    auto result = Utility::ReadStringSafe(arr, 10);
    EXPECT_EQ(result, "hello");
}

TEST(ReadStringSafe, CharArrayExceedsMax)
{
    char arr[12] = "hello world";
    auto result = Utility::ReadStringSafe(arr, 5);
    EXPECT_EQ(result, "hello");
}

TEST(ReadStringSafe, EmbeddedNull)
{
    char arr[8] = {'h', 'e', '\0', 'l', 'o', '\0', 'x', '\0'};
    auto result = Utility::ReadStringSafe(arr, 8);
    EXPECT_EQ(result, "he");
}

TEST(ReadStringSafe, MaxSizeZero)
{
    const char* p = "test";
    auto result = Utility::ReadStringSafe(p, 0);
    EXPECT_TRUE(result.empty());
}

TEST(ReadStringSafe, EmptyString)
{
    std::string input;
    auto result = Utility::ReadStringSafe(std::move(input), 10);
    EXPECT_TRUE(result.empty());
}

TEST(ReadStringSafe, EmptyStringView)
{
    std::string_view sv;
    auto result = Utility::ReadStringSafe(sv, 10);
    EXPECT_TRUE(result.empty());
}
