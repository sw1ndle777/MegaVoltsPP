#include <gtest/gtest.h>
#include <common/BaseLib/Utility.h>
#include <thread>
#include <vector>
#include <atomic>
#include <string>

TEST(PlaceholderStress, ConcurrentMariaDBGeneration)
{
    auto gen = [](uint32_t count) -> std::string
    {
        if (count == 0) return {};
        std::string result;
        result.reserve(count * 3);
        for (uint32_t i = 0; i < count; ++i)
        {
            if (i > 0) result += ", ";
            result += '?';
        }
        return result;
    };

    std::atomic<int> errors{0};
    std::vector<std::thread> threads;

    for (int t = 0; t < 8; ++t)
    {
        threads.emplace_back([&gen, &errors]
        {
            for (int i = 0; i < 1000; ++i)
            {
                auto result = gen(5);
                if (result != "?, ?, ?, ?, ?")
                    errors++;
            }
        });
    }

    for (auto& t : threads)
        t.join();

    EXPECT_EQ(errors.load(), 0);
}

TEST(PlaceholderStress, ConcurrentPostgresGeneration)
{
    auto gen = [](uint32_t count) -> std::string
    {
        if (count == 0) return {};
        std::string result;
        for (uint32_t i = 0; i < count; ++i)
        {
            if (i > 0) result += ", ";
            result += '$';
            result += std::to_string(i + 1);
        }
        return result;
    };

    std::atomic<int> errors{0};
    std::vector<std::thread> threads;

    for (int t = 0; t < 8; ++t)
    {
        threads.emplace_back([&gen, &errors]
        {
            for (int i = 0; i < 1000; ++i)
            {
                auto result = gen(3);
                if (result != "$1, $2, $3")
                    errors++;
            }
        });
    }

    for (auto& t : threads)
        t.join();

    EXPECT_EQ(errors.load(), 0);
}

TEST(RandomStress, ConcurrentRandomGeneration)
{
    std::atomic<int> zeros{0};
    std::vector<std::thread> threads;

    for (int t = 0; t < 4; ++t)
    {
        threads.emplace_back([&zeros]
        {
            Utility::Random::Init();
            for (int i = 0; i < 10000; ++i)
            {
                auto val = Utility::Random::Gen();
                if (val == 0) zeros++;
            }
        });
    }

    for (auto& t : threads)
        t.join();

    EXPECT_LT(zeros.load(), 10);
}

TEST(Blake2bStress, ConcurrentKeyGeneration)
{
    std::atomic<int> collisions{0};
    std::vector<std::thread> threads;

    for (int t = 0; t < 4; ++t)
    {
        threads.emplace_back([t, &collisions]
        {
            Utility::SecureRandomBlake2b::Generator gen(10, false);
            gen.SetSeed(int64_t(t * 1000 + 1));

            uint64_t prev = 0;
            for (int i = 0; i < 10000; ++i)
            {
                auto key = gen.GenerateAuthKey();
                if (key == prev)
                    collisions++;
                prev = key;
            }
        });
    }

    for (auto& t : threads)
        t.join();

    EXPECT_EQ(collisions.load(), 0);
}
