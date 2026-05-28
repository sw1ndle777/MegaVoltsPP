#include <gtest/gtest.h>
#include <common/BaseLib/Utility.h>
#include <vector>
#include <optional>
#include <map>
#include <numeric>
#include <cmath>

namespace
{
    template <class It, class Proj>
    [[nodiscard]] std::optional<std::size_t> ExtractIndex(It first, It last, Proj proj) noexcept
    {
        if (first == last) return std::nullopt;
        uint32_t total = 0;
        for (auto it = first; it != last; it++)
            total += static_cast<uint32_t>(proj(*it));
        if (total == 0) return std::nullopt;
        const uint32_t r = Utility::Random::CustomGen(0, total - 1);
        uint32_t acc = 0;
        std::size_t idx = 0;
        for (auto it = first; it != last; it++, idx++)
        {
            acc += static_cast<uint32_t>(proj(*it));
            if (r < acc) return idx;
        }
        return std::nullopt;
    }

    template <class Container, class Proj>
    [[nodiscard]] std::optional<std::size_t> ExtractIndex(const Container& c, Proj proj) noexcept
    {
        return ExtractIndex(std::begin(c), std::end(c), proj);
    }
}

struct WeightedItem { uint32_t weight; };

TEST(ExtractIndex, SingleItem)
{
    Utility::Random::Init();
    std::vector<WeightedItem> items = {{100}};
    for (int i = 0; i < 100; ++i)
    {
        auto idx = ExtractIndex(items, [](const WeightedItem& e) { return e.weight; });
        ASSERT_TRUE(idx.has_value());
        EXPECT_EQ(*idx, 0u);
    }
}

TEST(ExtractIndex, EmptyContainer)
{
    std::vector<WeightedItem> items;
    auto idx = ExtractIndex(items, [](const WeightedItem& e) { return e.weight; });
    EXPECT_FALSE(idx.has_value());
}

TEST(ExtractIndex, AllZeroWeights)
{
    std::vector<WeightedItem> items = {{0}, {0}, {0}};
    auto idx = ExtractIndex(items, [](const WeightedItem& e) { return e.weight; });
    EXPECT_FALSE(idx.has_value());
}

TEST(ExtractIndex, OnlyOneNonZero)
{
    Utility::Random::Init();
    std::vector<WeightedItem> items = {{0}, {0}, {100}};
    for (int i = 0; i < 100; ++i)
    {
        auto idx = ExtractIndex(items, [](const WeightedItem& e) { return e.weight; });
        ASSERT_TRUE(idx.has_value());
        EXPECT_EQ(*idx, 2u);
    }
}

TEST(ExtractIndex, FirstOnlyNonZero)
{
    Utility::Random::Init();
    std::vector<WeightedItem> items = {{1}, {0}, {0}};
    for (int i = 0; i < 100; ++i)
    {
        auto idx = ExtractIndex(items, [](const WeightedItem& e) { return e.weight; });
        ASSERT_TRUE(idx.has_value());
        EXPECT_EQ(*idx, 0u);
    }
}

TEST(ExtractIndex, EqualWeightsDistribution)
{
    Utility::Random::Init();
    std::vector<WeightedItem> items = {{100}, {100}};
    std::map<size_t, int> counts;

    constexpr int trials = 10000;
    for (int i = 0; i < trials; ++i)
    {
        auto idx = ExtractIndex(items, [](const WeightedItem& e) { return e.weight; });
        ASSERT_TRUE(idx.has_value());
        counts[*idx]++;
    }

    double expected = trials / 2.0;
    double chi_sq = 0;
    for (auto& [k, v] : counts)
        chi_sq += (v - expected) * (v - expected) / expected;

    EXPECT_LT(chi_sq, 6.635);
}

TEST(ExtractIndex, UnequalWeightsDistribution)
{
    Utility::Random::Init();
    std::vector<WeightedItem> items = {{75}, {25}};
    std::map<size_t, int> counts;

    constexpr int trials = 10000;
    for (int i = 0; i < trials; ++i)
    {
        auto idx = ExtractIndex(items, [](const WeightedItem& e) { return e.weight; });
        ASSERT_TRUE(idx.has_value());
        counts[*idx]++;
    }

    double exp0 = trials * 0.75;
    double exp1 = trials * 0.25;
    double chi_sq = (counts[0] - exp0) * (counts[0] - exp0) / exp0
                  + (counts[1] - exp1) * (counts[1] - exp1) / exp1;

    EXPECT_LT(chi_sq, 6.635);
}

TEST(ExtractIndex, AlwaysReturnsValidIndex)
{
    Utility::Random::Init();
    std::vector<WeightedItem> items = {{10}, {20}, {30}, {40}};
    for (int i = 0; i < 1000; ++i)
    {
        auto idx = ExtractIndex(items, [](const WeightedItem& e) { return e.weight; });
        ASSERT_TRUE(idx.has_value());
        EXPECT_LT(*idx, items.size());
    }
}
