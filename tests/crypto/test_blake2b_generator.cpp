#include <gtest/gtest.h>
#include <common/BaseLib/Utility.h>
#include <set>
#include <cstring>

TEST(Blake2bGenerator, DeterministicWithFixedSeed)
{
    Utility::SecureRandomBlake2b::Generator gen1(10, false);
    gen1.SetSeed(int64_t(12345));

    Utility::SecureRandomBlake2b::Generator gen2(10, false);
    gen2.SetSeed(int64_t(12345));

    for (int i = 0; i < 100; ++i)
        EXPECT_EQ(gen1.NextUInt64(), gen2.NextUInt64());
}

TEST(Blake2bGenerator, DifferentSeedsDifferentOutput)
{
    Utility::SecureRandomBlake2b::Generator gen1(10, false);
    gen1.SetSeed(int64_t(111));

    Utility::SecureRandomBlake2b::Generator gen2(10, false);
    gen2.SetSeed(int64_t(222));

    bool any_different = false;
    for (int i = 0; i < 10; ++i)
    {
        if (gen1.NextUInt64() != gen2.NextUInt64())
        {
            any_different = true;
            break;
        }
    }
    EXPECT_TRUE(any_different);
}

TEST(Blake2bGenerator, NextBytesLength)
{
    Utility::SecureRandomBlake2b::Generator gen(10, false);
    gen.SetSeed(int64_t(42));

    uint8_t buffer[128] = {};
    gen.NextBytes(buffer, 128);

    bool all_zero = true;
    for (auto b : buffer)
    {
        if (b != 0) { all_zero = false; break; }
    }
    EXPECT_FALSE(all_zero);
}

TEST(Blake2bGenerator, NextUInt32Deterministic)
{
    Utility::SecureRandomBlake2b::Generator gen1(10, false);
    gen1.SetSeed(int64_t(999));

    Utility::SecureRandomBlake2b::Generator gen2(10, false);
    gen2.SetSeed(int64_t(999));

    for (int i = 0; i < 50; ++i)
        EXPECT_EQ(gen1.NextUInt32(), gen2.NextUInt32());
}

TEST(Blake2bGenerator, GenerateAuthKeyNonZero)
{
    Utility::SecureRandomBlake2b::Generator gen(10, false);
    gen.SetSeed(int64_t(42));

    for (int i = 0; i < 100; ++i)
        EXPECT_NE(gen.GenerateAuthKey(), 0u);
}

TEST(Blake2bGenerator, SeedFromByteArray)
{
    uint8_t seed_bytes[16];
    std::memset(seed_bytes, 0xAB, sizeof(seed_bytes));

    Utility::SecureRandomBlake2b::Generator gen1(10, false);
    gen1.SetSeed(seed_bytes, sizeof(seed_bytes));

    Utility::SecureRandomBlake2b::Generator gen2(10, false);
    gen2.SetSeed(seed_bytes, sizeof(seed_bytes));

    for (int i = 0; i < 50; ++i)
        EXPECT_EQ(gen1.NextUInt64(), gen2.NextUInt64());
}

TEST(Blake2bGenerator, UniqueOutputs)
{
    Utility::SecureRandomBlake2b::Generator gen(10, false);
    gen.SetSeed(int64_t(7777));

    std::set<uint64_t> values;
    for (int i = 0; i < 1000; ++i)
        values.insert(gen.NextUInt64());

    EXPECT_EQ(values.size(), 1000u);
}
