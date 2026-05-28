#include <gtest/gtest.h>
#include <common/BaseLib/Utility.h>
#include <cstdint>

namespace
{
    struct UpgradeResult
    {
        bool success;
        bool held;
        uint32_t energy_refund;
    };

    UpgradeResult SimulateUpgrade(uint32_t probability, bool hold, uint32_t use_exp, uint32_t refund_percent)
    {
        Utility::Random::Init();
        uint32_t roll = Utility::Random::CustomGen(0, 100);
        bool success = roll < probability;

        uint32_t energy_refund = 0;
        if (!success && !hold)
            energy_refund = use_exp * refund_percent / 100;

        return {success, hold && !success, energy_refund};
    }
}

TEST(UpgradeProbability, Guaranteed100Percent)
{
    Utility::Random::Init();
    for (int i = 0; i < 100; ++i)
    {
        uint32_t roll = Utility::Random::CustomGen(0, 100);
        EXPECT_TRUE(roll < 101);
    }
}

TEST(UpgradeProbability, ZeroPercentAlwaysFails)
{
    for (int i = 0; i < 100; ++i)
    {
        uint32_t roll = Utility::Random::CustomGen(0, 100);
        EXPECT_FALSE(roll < 0);
    }
}

TEST(UpgradeProbability, EnergyRefund30Percent)
{
    uint32_t use_exp = 1000;
    uint32_t refund = use_exp * 30 / 100;
    EXPECT_EQ(refund, 300u);
}

TEST(UpgradeProbability, EnergyRefund50Percent)
{
    uint32_t use_exp = 1000;
    uint32_t refund = use_exp * 50 / 100;
    EXPECT_EQ(refund, 500u);
}

TEST(UpgradeProbability, EnergyRefund100Percent)
{
    uint32_t use_exp = 1000;
    uint32_t refund = use_exp * 100 / 100;
    EXPECT_EQ(refund, 1000u);
}

TEST(UpgradeProbability, EnergyRefundIntegerDivision)
{
    EXPECT_EQ(1u * 30 / 100, 0u);
    EXPECT_EQ(3u * 30 / 100, 0u);
    EXPECT_EQ(10u * 30 / 100, 3u);
    EXPECT_EQ(33u * 30 / 100, 9u);
}

TEST(UpgradeProbability, HoldPreventsDowngrade)
{
    auto result = SimulateUpgrade(0, true, 1000, 30);
    EXPECT_FALSE(result.success);
    EXPECT_TRUE(result.held);
    EXPECT_EQ(result.energy_refund, 0u);
}

TEST(UpgradeProbability, NoHoldGivesRefund)
{
    auto result = SimulateUpgrade(0, false, 1000, 50);
    EXPECT_FALSE(result.success);
    EXPECT_FALSE(result.held);
    EXPECT_EQ(result.energy_refund, 500u);
}

TEST(UpgradeProbability, StatisticalDistribution)
{
    Utility::Random::Init();
    int successes = 0;
    constexpr int trials = 10000;
    constexpr uint32_t probability = 50;

    for (int i = 0; i < trials; ++i)
    {
        uint32_t roll = Utility::Random::CustomGen(0, 100);
        if (roll < probability) ++successes;
    }

    double pct = successes * 100.0 / trials;
    EXPECT_NEAR(pct, 49.5, 3.0);
}
