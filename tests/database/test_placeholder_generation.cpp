#include <gtest/gtest.h>
#include <string>
#include <sstream>
#include <cstdint>

namespace
{
    std::string GenerateMariaDBQuestionMarks(uint32_t count)
    {
        if (count == 0) return {};
        std::string result;
        result.reserve(count * 3);
        for (uint32_t i = 0; i < count; ++i)
        {
            if (i > 0) result += ", ";
            result += '?';
        }
        return result;
    }

    std::string GeneratePostgresQuestionMarks(uint32_t count)
    {
        if (count == 0) return {};
        std::string result;
        result.reserve(count * 4);
        for (uint32_t i = 0; i < count; ++i)
        {
            if (i > 0) result += ", ";
            result += '$';
            result += std::to_string(i + 1);
        }
        return result;
    }

    std::string GenerateMariaDBMultiRow(uint32_t cols, uint32_t rows)
    {
        if (cols == 0 || rows == 0) return {};
        std::string row = "(" + GenerateMariaDBQuestionMarks(cols) + ")";
        std::string result;
        for (uint32_t i = 0; i < rows; ++i)
        {
            if (i > 0) result += ", ";
            result += row;
        }
        return result;
    }

    std::string GeneratePostgresMultiRow(uint32_t cols, uint32_t rows)
    {
        if (cols == 0 || rows == 0) return {};
        std::string result;
        uint32_t param = 1;
        for (uint32_t r = 0; r < rows; ++r)
        {
            if (r > 0) result += ", ";
            result += '(';
            for (uint32_t c = 0; c < cols; ++c)
            {
                if (c > 0) result += ", ";
                result += '$';
                result += std::to_string(param++);
            }
            result += ')';
        }
        return result;
    }
}

TEST(PlaceholderGeneration, MariaDBSingle)
{
    EXPECT_EQ(GenerateMariaDBQuestionMarks(1), "?");
}

TEST(PlaceholderGeneration, MariaDBMultiple)
{
    EXPECT_EQ(GenerateMariaDBQuestionMarks(3), "?, ?, ?");
}

TEST(PlaceholderGeneration, MariaDBZero)
{
    EXPECT_EQ(GenerateMariaDBQuestionMarks(0), "");
}

TEST(PlaceholderGeneration, PostgresSingle)
{
    EXPECT_EQ(GeneratePostgresQuestionMarks(1), "$1");
}

TEST(PlaceholderGeneration, PostgresMultiple)
{
    EXPECT_EQ(GeneratePostgresQuestionMarks(3), "$1, $2, $3");
}

TEST(PlaceholderGeneration, PostgresZero)
{
    EXPECT_EQ(GeneratePostgresQuestionMarks(0), "");
}

TEST(PlaceholderGeneration, PostgresLargeCount)
{
    auto result = GeneratePostgresQuestionMarks(10);
    EXPECT_EQ(result, "$1, $2, $3, $4, $5, $6, $7, $8, $9, $10");
}

TEST(PlaceholderGeneration, MariaDBMultiRow)
{
    EXPECT_EQ(GenerateMariaDBMultiRow(2, 3), "(?, ?), (?, ?), (?, ?)");
}

TEST(PlaceholderGeneration, PostgresMultiRow)
{
    EXPECT_EQ(GeneratePostgresMultiRow(2, 3), "($1, $2), ($3, $4), ($5, $6)");
}

TEST(PlaceholderGeneration, MultiRowSingleCol)
{
    EXPECT_EQ(GenerateMariaDBMultiRow(1, 3), "(?), (?), (?)");
    EXPECT_EQ(GeneratePostgresMultiRow(1, 3), "($1), ($2), ($3)");
}

TEST(PlaceholderGeneration, MultiRowSingleRow)
{
    EXPECT_EQ(GenerateMariaDBMultiRow(3, 1), "(?, ?, ?)");
    EXPECT_EQ(GeneratePostgresMultiRow(3, 1), "($1, $2, $3)");
}

TEST(PlaceholderGeneration, MultiRowZeroCols)
{
    EXPECT_EQ(GenerateMariaDBMultiRow(0, 3), "");
    EXPECT_EQ(GeneratePostgresMultiRow(0, 3), "");
}

TEST(PlaceholderGeneration, MultiRowZeroRows)
{
    EXPECT_EQ(GenerateMariaDBMultiRow(3, 0), "");
    EXPECT_EQ(GeneratePostgresMultiRow(3, 0), "");
}
