#include <gtest/gtest.h>
#include <common/BaseLib/Utility.h>
#include <map>
#include <vector>
#include <cstdint>

namespace
{
    struct BossReward { uint32_t item_id; uint32_t weight; };

    const std::vector<BossReward> boss_rewards =
    {
        {4801002, 50},
        {4801001, 35},
        {4801000, 10},
        {4801003, 5}
    };

    uint32_t get_random_boss_reward()
    {
        uint32_t total_weight = 0;
        for (const auto& reward : boss_rewards)
            total_weight += reward.weight;

        auto random_value = Utility::Random::CustomGen(1, total_weight);

        uint32_t cumulative_weight = 0;
        for (const auto& reward : boss_rewards)
        {
            cumulative_weight += reward.weight;
            if (random_value <= cumulative_weight)
                return reward.item_id;
        }
        return boss_rewards[0].item_id;
    }
}

TEST(BossRewards, TotalWeightIs100)
{
    uint32_t total = 0;
    for (const auto& r : boss_rewards)
        total += r.weight;
    EXPECT_EQ(total, 100u);
}

TEST(BossRewards, AllTiersCanBeSelected)
{
    Utility::Random::Init();
    std::map<uint32_t, int> counts;
    constexpr int trials = 100000;

    for (int i = 0; i < trials; ++i)
        counts[get_random_boss_reward()]++;

    EXPECT_GT(counts[4801002], 0);
    EXPECT_GT(counts[4801001], 0);
    EXPECT_GT(counts[4801000], 0);
    EXPECT_GT(counts[4801003], 0);
}

TEST(BossRewards, DistributionMatchesWeights)
{
    Utility::Random::Init();
    std::map<uint32_t, int> counts;
    constexpr int trials = 100000;

    for (int i = 0; i < trials; ++i)
        counts[get_random_boss_reward()]++;

    double bronze_pct = counts[4801002] * 100.0 / trials;
    double silver_pct = counts[4801001] * 100.0 / trials;
    double gold_pct   = counts[4801000] * 100.0 / trials;
    double diamond_pct = counts[4801003] * 100.0 / trials;

    EXPECT_NEAR(bronze_pct, 50.0, 2.0);
    EXPECT_NEAR(silver_pct, 35.0, 2.0);
    EXPECT_NEAR(gold_pct, 10.0, 2.0);
    EXPECT_NEAR(diamond_pct, 5.0, 2.0);
}

TEST(BossRewards, AlwaysReturnsValidItem)
{
    Utility::Random::Init();
    std::set<uint32_t> valid_ids = {4801002, 4801001, 4801000, 4801003};

    for (int i = 0; i < 1000; ++i)
        EXPECT_TRUE(valid_ids.contains(get_random_boss_reward()));
}
