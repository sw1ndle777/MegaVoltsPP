#include <gtest/gtest.h>
#include <common/BaseLib/Utility.h>
#include <cstring>
#include <set>

TEST(GenerateAuthKey, DeterministicSameInputs)
{
    uint8_t salt[16];
    std::memset(salt, 0x55, sizeof(salt));

    auto key1 = Utility::GenerateAuthKey("user", "pass", salt);
    auto key2 = Utility::GenerateAuthKey("user", "pass", salt);

    EXPECT_EQ(key1, key2);
}

TEST(GenerateAuthKey, DifferentUsernameDifferentKey)
{
    uint8_t salt[16];
    std::memset(salt, 0x55, sizeof(salt));

    auto key1 = Utility::GenerateAuthKey("user1", "pass", salt);
    auto key2 = Utility::GenerateAuthKey("user2", "pass", salt);

    EXPECT_NE(key1, key2);
}

TEST(GenerateAuthKey, DifferentPasswordDifferentKey)
{
    uint8_t salt[16];
    std::memset(salt, 0x55, sizeof(salt));

    auto key1 = Utility::GenerateAuthKey("user", "pass1", salt);
    auto key2 = Utility::GenerateAuthKey("user", "pass2", salt);

    EXPECT_NE(key1, key2);
}

TEST(GenerateAuthKey, DifferentSaltDifferentKey)
{
    uint8_t salt1[16], salt2[16];
    std::memset(salt1, 0x11, sizeof(salt1));
    std::memset(salt2, 0x22, sizeof(salt2));

    auto key1 = Utility::GenerateAuthKey("user", "pass", salt1);
    auto key2 = Utility::GenerateAuthKey("user", "pass", salt2);

    EXPECT_NE(key1, key2);
}

TEST(GenerateAuthKey, CollisionResistance)
{
    uint8_t salt[16];
    std::memset(salt, 0x77, sizeof(salt));

    std::set<uint64_t> keys;
    for (int i = 0; i < 100; ++i)
    {
        auto key = Utility::GenerateAuthKey("user" + std::to_string(i), "pass" + std::to_string(i), salt);
        keys.insert(key);
    }
    EXPECT_EQ(keys.size(), 100u);
}
