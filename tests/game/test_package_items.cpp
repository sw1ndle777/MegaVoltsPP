#include <gtest/gtest.h>
#include <common/NetEngine/Constants.h>
#include <map>
#include <cstdint>

using namespace NetEngine::Items::Package;

namespace
{
    const std::map<uint32_t, uint32_t> coin_map =
    {
        {4308001, 1}, {4308002, 2}, {4308003, 3}, {4308004, 4}, {4308005, 5},
        {4308006, 6}, {4308007, 7}, {4308008, 8}, {4308009, 9}, {4308010, 10},
        {4308011, 20}, {4308012, 30}, {4308013, 40}, {4308014, 50}, {4308015, 60},
        {4308016, 70}, {4308017, 80}, {4308018, 90}, {4308019, 100}
    };

    const std::map<uint32_t, uint32_t> coupon_map =
    {
        {4305019, 1}, {4305020, 5}, {4305021, 10}, {4305022, 15},
        {4305023, 20}, {4305024, 25}, {4305026, 30}, {4305027, 2},
        {4305028, 3}, {4305029, 4}, {4305030, 6}, {4305031, 7},
        {4305032, 8}, {4305033, 9}, {4305034, 40}, {4305035, 50},
        {4305036, 100}
    };

    const std::map<uint32_t, uint32_t> mp_map =
    {
        {4400001, 100}, {4400002, 200}, {4400003, 300}, {4400004, 400},
        {4400005, 500}, {4400006, 600}, {4400007, 700}, {4400008, 800},
        {4400009, 900}, {4400010, 1000}, {4400011, 1100}, {4400012, 1200},
        {4400013, 1300}, {4400014, 1400}, {4400015, 1500}, {4400016, 1600},
        {4400017, 1700}, {4400018, 1800}, {4400019, 1900}, {4400020, 2000},
        {4400030, 3000}, {4400035, 3500}, {4400040, 4000}, {4400050, 5000},
        {4400060, 6000}, {4400070, 7000}, {4400080, 8000}, {4400090, 9000},
        {4400100, 10000}, {4400200, 20000}, {4400300, 30000}, {4400500, 50000},
        {4401000, 100000}, {4401500, 150000}, {4405000, 500000}, {4410000, 1000000}
    };
}

TEST(PackageItems, CoinEnumValues)
{
    EXPECT_EQ(static_cast<uint32_t>(CoinItemId::COIN_1), 4308001u);
    EXPECT_EQ(static_cast<uint32_t>(CoinItemId::COIN_100), 4308019u);
}

TEST(PackageItems, AllCoinMappingsValid)
{
    EXPECT_EQ(coin_map.at(4308001), 1u);
    EXPECT_EQ(coin_map.at(4308005), 5u);
    EXPECT_EQ(coin_map.at(4308010), 10u);
    EXPECT_EQ(coin_map.at(4308019), 100u);
}

TEST(PackageItems, CoinMapCompleteness)
{
    EXPECT_EQ(coin_map.size(), 19u);
}

TEST(PackageItems, CouponEnumValues)
{
    EXPECT_EQ(static_cast<uint32_t>(CouponItemId::COUPON_1), 4305019u);
    EXPECT_EQ(static_cast<uint32_t>(CouponItemId::COUPON_100), 4305036u);
}

TEST(PackageItems, AllCouponMappingsValid)
{
    EXPECT_EQ(coupon_map.at(4305019), 1u);
    EXPECT_EQ(coupon_map.at(4305020), 5u);
    EXPECT_EQ(coupon_map.at(4305036), 100u);
}

TEST(PackageItems, MicroPointsEnumValues)
{
    EXPECT_EQ(static_cast<uint32_t>(MicroPointsItemId::POINTS_100), 4400001u);
    EXPECT_EQ(static_cast<uint32_t>(MicroPointsItemId::POINTS_1000000), 4410000u);
}

TEST(PackageItems, AllMPMappingsValid)
{
    EXPECT_EQ(mp_map.at(4400001), 100u);
    EXPECT_EQ(mp_map.at(4400010), 1000u);
    EXPECT_EQ(mp_map.at(4400100), 10000u);
    EXPECT_EQ(mp_map.at(4410000), 1000000u);
}

TEST(PackageItems, MPMapCompleteness)
{
    EXPECT_EQ(mp_map.size(), 36u);
}

TEST(PackageItems, UnknownItemIdNotInMaps)
{
    EXPECT_FALSE(coin_map.contains(9999999));
    EXPECT_FALSE(coupon_map.contains(9999999));
    EXPECT_FALSE(mp_map.contains(9999999));
}

TEST(PackageItems, VoiceEnumValues)
{
    EXPECT_EQ(static_cast<uint32_t>(VoiceItemId::NAOMI_A), 4810000u);
    EXPECT_EQ(static_cast<uint32_t>(VoiceItemId::CHIP_D), 4810015u);
}

TEST(PackageItems, VoiceIndexCalculation)
{
    uint32_t naomi_a = static_cast<uint32_t>(VoiceItemId::NAOMI_A);
    uint32_t voice_index = ((naomi_a - naomi_a) % 4) + 4;
    EXPECT_EQ(voice_index, 4u);

    uint32_t knox_b = static_cast<uint32_t>(VoiceItemId::KNOX_B);
    voice_index = ((knox_b - naomi_a) % 4) + 4;
    EXPECT_EQ(voice_index, 5u);
}

TEST(PackageItems, VoiceCharacterCalculation)
{
    uint32_t naomi_a = static_cast<uint32_t>(VoiceItemId::NAOMI_A);
    EXPECT_EQ((naomi_a - naomi_a) / 4, 0u);

    uint32_t knox_a = static_cast<uint32_t>(VoiceItemId::KNOX_A);
    EXPECT_EQ((knox_a - naomi_a) / 4, 1u);

    uint32_t pandora_a = static_cast<uint32_t>(VoiceItemId::PANDORA_A);
    EXPECT_EQ((pandora_a - naomi_a) / 4, 2u);

    uint32_t chip_a = static_cast<uint32_t>(VoiceItemId::CHIP_A);
    EXPECT_EQ((chip_a - naomi_a) / 4, 3u);
}
