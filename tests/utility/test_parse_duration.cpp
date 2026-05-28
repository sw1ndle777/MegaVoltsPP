#include <gtest/gtest.h>
#include <common/BaseLib/Utility.h>
#include <chrono>

TEST(ParseHumanDuration, Seconds)
{
    auto result = Utility::ParseHumanDuration("5s");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, std::chrono::seconds(5));
}

TEST(ParseHumanDuration, SecondsAliases)
{
    EXPECT_EQ(*Utility::ParseHumanDuration("10sec"), std::chrono::seconds(10));
    EXPECT_EQ(*Utility::ParseHumanDuration("3secs"), std::chrono::seconds(3));
    EXPECT_EQ(*Utility::ParseHumanDuration("1second"), std::chrono::seconds(1));
    EXPECT_EQ(*Utility::ParseHumanDuration("7seconds"), std::chrono::seconds(7));
}

TEST(ParseHumanDuration, Minutes)
{
    auto result = Utility::ParseHumanDuration("10min");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, std::chrono::seconds(600));
}

TEST(ParseHumanDuration, MinutesAliases)
{
    EXPECT_EQ(*Utility::ParseHumanDuration("2mins"), std::chrono::seconds(120));
    EXPECT_EQ(*Utility::ParseHumanDuration("1minute"), std::chrono::seconds(60));
    EXPECT_EQ(*Utility::ParseHumanDuration("3minutes"), std::chrono::seconds(180));
}

TEST(ParseHumanDuration, Hours)
{
    auto result = Utility::ParseHumanDuration("2h");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, std::chrono::seconds(7200));
}

TEST(ParseHumanDuration, HoursAliases)
{
    EXPECT_EQ(*Utility::ParseHumanDuration("1hr"), std::chrono::seconds(3600));
    EXPECT_EQ(*Utility::ParseHumanDuration("3hrs"), std::chrono::seconds(10800));
    EXPECT_EQ(*Utility::ParseHumanDuration("1hour"), std::chrono::seconds(3600));
    EXPECT_EQ(*Utility::ParseHumanDuration("4hours"), std::chrono::seconds(14400));
}

TEST(ParseHumanDuration, Days)
{
    auto result = Utility::ParseHumanDuration("1d");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, std::chrono::seconds(86400));
}

TEST(ParseHumanDuration, DaysAliases)
{
    EXPECT_EQ(*Utility::ParseHumanDuration("1day"), std::chrono::seconds(86400));
    EXPECT_EQ(*Utility::ParseHumanDuration("7days"), std::chrono::seconds(604800));
}

TEST(ParseHumanDuration, Weeks)
{
    auto result = Utility::ParseHumanDuration("1w");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, std::chrono::seconds(604800));
}

TEST(ParseHumanDuration, WeeksAliases)
{
    EXPECT_EQ(*Utility::ParseHumanDuration("2week"), std::chrono::seconds(1209600));
    EXPECT_EQ(*Utility::ParseHumanDuration("3weeks"), std::chrono::seconds(1814400));
}

TEST(ParseHumanDuration, Months)
{
    auto result = Utility::ParseHumanDuration("1m");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, std::chrono::seconds(2592000));
}

TEST(ParseHumanDuration, MonthsAliases)
{
    EXPECT_EQ(*Utility::ParseHumanDuration("1mo"), std::chrono::seconds(2592000));
    EXPECT_EQ(*Utility::ParseHumanDuration("2mon"), std::chrono::seconds(5184000));
    EXPECT_EQ(*Utility::ParseHumanDuration("1month"), std::chrono::seconds(2592000));
    EXPECT_EQ(*Utility::ParseHumanDuration("6months"), std::chrono::seconds(15552000));
}

TEST(ParseHumanDuration, Years)
{
    auto result = Utility::ParseHumanDuration("1y");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, std::chrono::seconds(31536000));
}

TEST(ParseHumanDuration, YearsAliases)
{
    EXPECT_EQ(*Utility::ParseHumanDuration("1year"), std::chrono::seconds(31536000));
    EXPECT_EQ(*Utility::ParseHumanDuration("2years"), std::chrono::seconds(63072000));
}

TEST(ParseHumanDuration, WhitespaceHandling)
{
    EXPECT_EQ(*Utility::ParseHumanDuration("5 s"), std::chrono::seconds(5));
    EXPECT_EQ(*Utility::ParseHumanDuration(" 10min "), std::chrono::seconds(600));
    EXPECT_EQ(*Utility::ParseHumanDuration("  3  h  "), std::chrono::seconds(10800));
}

TEST(ParseHumanDuration, EmptyInput)
{
    auto result = Utility::ParseHumanDuration("");
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), "empty duration");
}

TEST(ParseHumanDuration, WhitespaceOnly)
{
    auto result = Utility::ParseHumanDuration("   ");
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), "empty duration");
}

TEST(ParseHumanDuration, NoNumberPrefix)
{
    auto result = Utility::ParseHumanDuration("hours");
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), "duration must start with a number");
}

TEST(ParseHumanDuration, UnknownSuffix)
{
    auto result = Utility::ParseHumanDuration("5xyz");
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), "unsupported duration suffix");
}

TEST(ParseHumanDuration, OverflowProtection)
{
    auto result = Utility::ParseHumanDuration("99999999999999999y");
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), "duration is too large");
}

TEST(ParseHumanDuration, CaseInsensitive)
{
    EXPECT_EQ(*Utility::ParseHumanDuration("5S"), std::chrono::seconds(5));
    EXPECT_EQ(*Utility::ParseHumanDuration("10MIN"), std::chrono::seconds(600));
    EXPECT_EQ(*Utility::ParseHumanDuration("2Hours"), std::chrono::seconds(7200));
}
