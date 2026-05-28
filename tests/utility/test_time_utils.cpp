#include <gtest/gtest.h>
#include <common/BaseLib/Utility.h>

TEST(FormatMilliseconds, Zero)
{
    EXPECT_EQ(Utility::FormatMilliseconds(0), "0 Day(s) 0 Hour(s) 0 Minute(s)");
}

TEST(FormatMilliseconds, SubMinute)
{
    EXPECT_EQ(Utility::FormatMilliseconds(999), "0 Day(s) 0 Hour(s) 0 Minute(s)");
}

TEST(FormatMilliseconds, OneMinute)
{
    EXPECT_EQ(Utility::FormatMilliseconds(60000), "0 Day(s) 0 Hour(s) 1 Minute(s)");
}

TEST(FormatMilliseconds, OneHour)
{
    EXPECT_EQ(Utility::FormatMilliseconds(3600000), "0 Day(s) 1 Hour(s) 0 Minute(s)");
}

TEST(FormatMilliseconds, OneDay)
{
    EXPECT_EQ(Utility::FormatMilliseconds(86400000), "1 Day(s) 0 Hour(s) 0 Minute(s)");
}

TEST(FormatMilliseconds, Complex)
{
    uint64_t ms = 2 * 86400000ull + 3 * 3600000ull + 45 * 60000ull;
    EXPECT_EQ(Utility::FormatMilliseconds(ms), "2 Day(s) 3 Hour(s) 45 Minute(s)");
}

TEST(FormatCompactDurationSeconds, Zero)
{
    EXPECT_EQ(Utility::FormatCompactDurationSeconds(0), "0s");
}

TEST(FormatCompactDurationSeconds, SecondsOnly)
{
    EXPECT_EQ(Utility::FormatCompactDurationSeconds(45), "45s");
}

TEST(FormatCompactDurationSeconds, MinutesAndSeconds)
{
    EXPECT_EQ(Utility::FormatCompactDurationSeconds(65), "1m 5s");
}

TEST(FormatCompactDurationSeconds, HoursMinutesSeconds)
{
    EXPECT_EQ(Utility::FormatCompactDurationSeconds(3661), "1h 1m 1s");
}

TEST(FormatCompactDurationSeconds, DaysHoursMinutesSeconds)
{
    EXPECT_EQ(Utility::FormatCompactDurationSeconds(90061), "1d 1h 1m 1s");
}

TEST(FormatCompactDurationSeconds, ExactDay)
{
    EXPECT_EQ(Utility::FormatCompactDurationSeconds(86400), "1d 0h 0m 0s");
}

TEST(ReadableSize, Bytes)
{
    EXPECT_EQ(Utility::readable_size(0), "0 B ");
    EXPECT_EQ(Utility::readable_size(512), "512 B ");
    EXPECT_EQ(Utility::readable_size(1023), "1023 B ");
}

TEST(ReadableSize, Kilobytes)
{
    EXPECT_EQ(Utility::readable_size(1024), "1.00 KB ");
    EXPECT_EQ(Utility::readable_size(2048), "2.00 KB ");
}

TEST(ReadableSize, Megabytes)
{
    EXPECT_EQ(Utility::readable_size(1048576), "1.00 MB ");
    EXPECT_EQ(Utility::readable_size(5242880), "5.00 MB ");
}

TEST(ReadableSize, Gigabytes)
{
    EXPECT_EQ(Utility::readable_size(1073741824), "1.00 GB ");
    EXPECT_EQ(Utility::readable_size(2147483648), "2.00 GB ");
}

TEST(ReadableTime, Nanoseconds)
{
    EXPECT_EQ(Utility::readable_time(500), "500.00 ns");
}

TEST(ReadableTime, Microseconds)
{
    EXPECT_EQ(Utility::readable_time(5000), "5.00 us");
}

TEST(ReadableTime, Milliseconds)
{
    EXPECT_EQ(Utility::readable_time(5000000), "5.00 ms");
}

TEST(ReadableTime, Seconds)
{
    EXPECT_EQ(Utility::readable_time(5000000000ull), "5.00 s");
}

TEST(ReadableTime, Minutes)
{
    EXPECT_EQ(Utility::readable_time(120000000000ull), "2.00 min");
}

TEST(ReadableTime, Hours)
{
    EXPECT_EQ(Utility::readable_time(7200000000000ull), "2.00 h");
}

TEST(ToVector, RoundtripUint32)
{
    uint32_t original = 0xDEADBEEF;
    auto bytes = Utility::ToVector(original);
    ASSERT_EQ(bytes.size(), sizeof(uint32_t));
    auto recovered = Utility::FromVector<uint32_t>(bytes);
    EXPECT_EQ(recovered, original);
}

TEST(ToVector, RoundtripUint64)
{
    uint64_t original = 0x0123456789ABCDEFull;
    auto bytes = Utility::ToVector(original);
    ASSERT_EQ(bytes.size(), sizeof(uint64_t));
    auto recovered = Utility::FromVector<uint64_t>(bytes);
    EXPECT_EQ(recovered, original);
}
