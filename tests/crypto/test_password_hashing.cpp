#include <gtest/gtest.h>
#include <common/BaseLib/Utility.h>
#include <cstring>

TEST(HashPassword, DeterministicSameInputs)
{
    uint8_t salt[16];
    std::memset(salt, 0x42, sizeof(salt));

    uint8_t hash1[32], hash2[32];
    ASSERT_TRUE(Utility::HashPassword("testpassword", salt, hash1));
    ASSERT_TRUE(Utility::HashPassword("testpassword", salt, hash2));

    EXPECT_EQ(std::memcmp(hash1, hash2, 32), 0);
}

TEST(HashPassword, DifferentPasswordDifferentHash)
{
    uint8_t salt[16];
    std::memset(salt, 0x42, sizeof(salt));

    uint8_t hash1[32], hash2[32];
    ASSERT_TRUE(Utility::HashPassword("password1", salt, hash1));
    ASSERT_TRUE(Utility::HashPassword("password2", salt, hash2));

    EXPECT_NE(std::memcmp(hash1, hash2, 32), 0);
}

TEST(HashPassword, DifferentSaltDifferentHash)
{
    uint8_t salt1[16], salt2[16];
    std::memset(salt1, 0x11, sizeof(salt1));
    std::memset(salt2, 0x22, sizeof(salt2));

    uint8_t hash1[32], hash2[32];
    ASSERT_TRUE(Utility::HashPassword("samepassword", salt1, hash1));
    ASSERT_TRUE(Utility::HashPassword("samepassword", salt2, hash2));

    EXPECT_NE(std::memcmp(hash1, hash2, 32), 0);
}

TEST(HashPassword, EmptyPasswordSucceeds)
{
    uint8_t salt[16];
    std::memset(salt, 0xAA, sizeof(salt));

    uint8_t hash[32];
    EXPECT_TRUE(Utility::HashPassword("", salt, hash));
}
