#include <gtest/gtest.h>
#include <cstdint>

namespace NetEngine::Items
{
    namespace OtherItems { enum Type : uint32_t { Question = 17, ShieldEnamel = 18, FlagBlue = 19, BombDrop = 20, GatchaItem = 21, Question1 = 24, MonsterFace = 25, Undefined1 = 26, Undefined2 = 27 }; }
    namespace DioramaItems { enum Type : uint32_t { Footing = 22, Object = 23 }; }
}

namespace
{
    uint32_t AdjustItemType(const uint32_t& item_type)
    {
        using namespace NetEngine::Items;
        if (item_type == static_cast<uint32_t>(OtherItems::Question) ||
            item_type == static_cast<uint32_t>(OtherItems::ShieldEnamel) ||
            item_type == static_cast<uint32_t>(OtherItems::FlagBlue) ||
            item_type == static_cast<uint32_t>(OtherItems::BombDrop) ||
            item_type == static_cast<uint32_t>(OtherItems::GatchaItem) ||
            item_type == static_cast<uint32_t>(OtherItems::Question1) ||
            item_type == static_cast<uint32_t>(OtherItems::MonsterFace) ||
            item_type == static_cast<uint32_t>(OtherItems::Undefined1) ||
            item_type == static_cast<uint32_t>(OtherItems::Undefined2))
        {
            return 17;
        }
        if (item_type == static_cast<uint32_t>(DioramaItems::Footing)) return 19;
        if (item_type == static_cast<uint32_t>(DioramaItems::Object)) return 20;
        return item_type;
    }
}

TEST(AdjustItemType, OtherItemsMapTo17)
{
    EXPECT_EQ(AdjustItemType(17), 17u);  // Question
    EXPECT_EQ(AdjustItemType(18), 17u);  // ShieldEnamel
    EXPECT_EQ(AdjustItemType(19), 17u);  // FlagBlue
    EXPECT_EQ(AdjustItemType(20), 17u);  // BombDrop
    EXPECT_EQ(AdjustItemType(21), 17u);  // GatchaItem
    EXPECT_EQ(AdjustItemType(24), 17u);  // Question1
    EXPECT_EQ(AdjustItemType(25), 17u);  // MonsterFace
    EXPECT_EQ(AdjustItemType(26), 17u);  // Undefined1
    EXPECT_EQ(AdjustItemType(27), 17u);  // Undefined2
}

TEST(AdjustItemType, FootingMapsTo19)
{
    EXPECT_EQ(AdjustItemType(22), 19u);
}

TEST(AdjustItemType, ObjectMapsTo20)
{
    EXPECT_EQ(AdjustItemType(23), 20u);
}

TEST(AdjustItemType, NormalTypesPassthrough)
{
    EXPECT_EQ(AdjustItemType(0), 0u);
    EXPECT_EQ(AdjustItemType(1), 1u);
    EXPECT_EQ(AdjustItemType(5), 5u);
    EXPECT_EQ(AdjustItemType(10), 10u);
    EXPECT_EQ(AdjustItemType(16), 16u);
    EXPECT_EQ(AdjustItemType(100), 100u);
}

TEST(AdjustItemType, GapValues)
{
    // 22 and 23 are DioramaItems, but values in between OtherItems and gap should passthrough
    // Note: value 22 is Footing → 19, value 23 is Object → 20
    // Values not in OtherItems/DioramaItems lists
    EXPECT_EQ(AdjustItemType(28), 28u);
    EXPECT_EQ(AdjustItemType(50), 50u);
}
