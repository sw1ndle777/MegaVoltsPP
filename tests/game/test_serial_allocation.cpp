#include <gtest/gtest.h>
#include <boost_unordered.hpp>
#include <cstdint>
#include <vector>

namespace
{
    struct MockItem
    {
        struct { uint32_t id; } serial_info;
    };

    std::vector<uint32_t> FindLowestAvailableSerialIds(const std::vector<MockItem>& inventory_items, size_t count)
    {
        boost::unordered_flat_set<uint32_t> used_ids;

        for (const auto& item : inventory_items)
            if (item.serial_info.id >= 0 && item.serial_info.id <= 0x100000)
                used_ids.insert(item.serial_info.id);

        std::vector<uint32_t> reserved;
        reserved.reserve(count);

        for (uint32_t id = 0; id <= 0x100000 && reserved.size() < count; id++)
        {
            if (!used_ids.contains(id))
            {
                reserved.push_back(id);
                used_ids.insert(id);
            }
        }
        return reserved;
    }
}

TEST(SerialAllocation, EmptyInventory)
{
    std::vector<MockItem> empty;
    auto ids = FindLowestAvailableSerialIds(empty, 5);
    ASSERT_EQ(ids.size(), 5u);
    EXPECT_EQ(ids[0], 0u);
    EXPECT_EQ(ids[1], 1u);
    EXPECT_EQ(ids[2], 2u);
    EXPECT_EQ(ids[3], 3u);
    EXPECT_EQ(ids[4], 4u);
}

TEST(SerialAllocation, FillsGaps)
{
    std::vector<MockItem> inventory = {{0}, {1}, {3}, {5}};
    auto ids = FindLowestAvailableSerialIds(inventory, 3);
    ASSERT_EQ(ids.size(), 3u);
    EXPECT_EQ(ids[0], 2u);
    EXPECT_EQ(ids[1], 4u);
    EXPECT_EQ(ids[2], 6u);
}

TEST(SerialAllocation, RequestZero)
{
    std::vector<MockItem> inventory;
    auto ids = FindLowestAvailableSerialIds(inventory, 0);
    EXPECT_TRUE(ids.empty());
}

TEST(SerialAllocation, RequestOne)
{
    std::vector<MockItem> inventory = {{0}, {1}, {2}};
    auto ids = FindLowestAvailableSerialIds(inventory, 1);
    ASSERT_EQ(ids.size(), 1u);
    EXPECT_EQ(ids[0], 3u);
}

TEST(SerialAllocation, ConsecutiveIds)
{
    std::vector<MockItem> inventory;
    for (uint32_t i = 0; i < 100; ++i)
        inventory.push_back(MockItem{i});

    auto ids = FindLowestAvailableSerialIds(inventory, 3);
    ASSERT_EQ(ids.size(), 3u);
    EXPECT_EQ(ids[0], 100u);
    EXPECT_EQ(ids[1], 101u);
    EXPECT_EQ(ids[2], 102u);
}

TEST(SerialAllocation, LargeGapInMiddle)
{
    std::vector<MockItem> inventory = {{0}, {100}};
    auto ids = FindLowestAvailableSerialIds(inventory, 3);
    ASSERT_EQ(ids.size(), 3u);
    EXPECT_EQ(ids[0], 1u);
    EXPECT_EQ(ids[1], 2u);
    EXPECT_EQ(ids[2], 3u);
}

TEST(SerialAllocation, NoDuplicatesInResult)
{
    std::vector<MockItem> inventory;
    auto ids = FindLowestAvailableSerialIds(inventory, 100);
    ASSERT_EQ(ids.size(), 100u);

    std::set<uint32_t> unique_ids(ids.begin(), ids.end());
    EXPECT_EQ(unique_ids.size(), ids.size());
}
