#include <gtest/gtest.h>
#include <cstdint>
#include <vector>

namespace
{
    struct PityAccumulator
    {
        uint32_t lucky_points = 0;
        uint32_t pity_per_spin;
        uint32_t threshold;

        PityAccumulator(uint32_t per_spin, uint32_t thresh)
            : pity_per_spin(per_spin), threshold(thresh) {}

        bool spin()
        {
            lucky_points += pity_per_spin;
            if (lucky_points >= threshold)
            {
                lucky_points = 0;
                return true;
            }
            return false;
        }
    };
}

TEST(GachaPity, RTGachaTriggersAt50Spins)
{
    PityAccumulator pity(20, 1000);
    int trigger_spin = -1;
    for (int i = 1; i <= 100; ++i)
    {
        if (pity.spin())
        {
            trigger_spin = i;
            break;
        }
    }
    EXPECT_EQ(trigger_spin, 50);
}

TEST(GachaPity, MPGachaTriggersAt100Spins)
{
    PityAccumulator pity(10, 1000);
    int trigger_spin = -1;
    for (int i = 1; i <= 200; ++i)
    {
        if (pity.spin())
        {
            trigger_spin = i;
            break;
        }
    }
    EXPECT_EQ(trigger_spin, 100);
}

TEST(GachaPity, ResetsAfterTrigger)
{
    PityAccumulator pity(20, 1000);
    for (int i = 0; i < 50; ++i)
        pity.spin();
    EXPECT_EQ(pity.lucky_points, 0u);

    pity.spin();
    EXPECT_EQ(pity.lucky_points, 20u);
}

TEST(GachaPity, MultiSpinPityCanTriggerMidBatch)
{
    PityAccumulator pity(20, 1000);
    pity.lucky_points = 980;

    std::vector<bool> results;
    for (int i = 0; i < 5; ++i)
        results.push_back(pity.spin());

    EXPECT_TRUE(results[0]);
    EXPECT_EQ(pity.lucky_points, 80u);
}

TEST(GachaPity, SingleSpinAtExactThreshold)
{
    PityAccumulator pity(20, 1000);
    pity.lucky_points = 980;
    EXPECT_TRUE(pity.spin());
    EXPECT_EQ(pity.lucky_points, 0u);
}

TEST(GachaPity, ZeroPerSpinNeverTriggers)
{
    PityAccumulator pity(0, 1000);
    for (int i = 0; i < 1000; ++i)
        EXPECT_FALSE(pity.spin());
}

TEST(GachaPity, AccumulatesCorrectly)
{
    PityAccumulator pity(20, 1000);
    for (int i = 0; i < 10; ++i)
        pity.spin();
    EXPECT_EQ(pity.lucky_points, 200u);
}
