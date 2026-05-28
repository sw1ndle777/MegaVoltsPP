#include <gtest/gtest.h>
#include <cstdint>
#include <expected>
#include <vector>
#include <limits>

namespace
{
    enum class CurrencyType { MP, RT, COUPONS, ENERGY };
    enum class CurrencyError
    {
        InsufficientMP, InsufficientRT, InsufficientCOUPONS, InsufficientENERGY,
        MpFull, RtFull, CouponsFull, EnergyFull
    };

    struct CurrencyDelta { CurrencyType type; uint32_t value; bool is_reward; };
    struct AccountState { uint32_t mp; uint32_t rt; uint32_t coupons; uint32_t energy; uint32_t max_energy; };

    struct CurrencyUpdate { CurrencyType type; uint32_t value; bool is_reward; };

    std::expected<std::vector<CurrencyUpdate>, CurrencyError>
    ValidateCurrency(const AccountState& acc, const std::vector<CurrencyDelta>& ops)
    {
        struct Accum { uint32_t rewards{0}, costs{0}; };
        Accum mp{}, rt{}, cp{}, en{};

        for (const auto& op : ops)
        {
            Accum* bucket = nullptr;
            switch (op.type)
            {
                case CurrencyType::MP: bucket = &mp; break;
                case CurrencyType::RT: bucket = &rt; break;
                case CurrencyType::COUPONS: bucket = &cp; break;
                case CurrencyType::ENERGY: bucket = &en; break;
            }
            if (bucket)
                op.is_reward ? bucket->rewards += op.value : bucket->costs += op.value;
        }

        std::vector<CurrencyUpdate> updates;
        auto finalize = [&](Accum a, CurrencyType type)
        {
            if (a.rewards == 0 && a.costs == 0) return;
            if (a.rewards >= a.costs)
            {
                auto v = a.rewards - a.costs;
                if (v > 0) updates.push_back({type, v, true});
            }
            else
            {
                auto v = a.costs - a.rewards;
                if (v > 0) updates.push_back({type, v, false});
            }
        };

        finalize(mp, CurrencyType::MP);
        finalize(rt, CurrencyType::RT);
        finalize(cp, CurrencyType::COUPONS);
        finalize(en, CurrencyType::ENERGY);

        constexpr uint32_t MAX_COUPONS = 250;
        constexpr uint32_t MAX_UINT32 = std::numeric_limits<uint32_t>::max();

        for (const auto& cu : updates)
        {
            switch (cu.type)
            {
                case CurrencyType::MP:
                    if (!cu.is_reward && acc.mp < cu.value)
                        return std::unexpected(CurrencyError::InsufficientMP);
                    if (cu.is_reward && acc.mp > MAX_UINT32 - cu.value)
                        return std::unexpected(CurrencyError::MpFull);
                    break;
                case CurrencyType::RT:
                    if (!cu.is_reward && acc.rt < cu.value)
                        return std::unexpected(CurrencyError::InsufficientRT);
                    if (cu.is_reward && acc.rt > MAX_UINT32 - cu.value)
                        return std::unexpected(CurrencyError::RtFull);
                    break;
                case CurrencyType::COUPONS:
                    if (!cu.is_reward && acc.coupons < cu.value)
                        return std::unexpected(CurrencyError::InsufficientCOUPONS);
                    if (cu.is_reward && acc.coupons > MAX_COUPONS - cu.value)
                        return std::unexpected(CurrencyError::CouponsFull);
                    break;
                case CurrencyType::ENERGY:
                    if (!cu.is_reward && acc.energy < cu.value)
                        return std::unexpected(CurrencyError::InsufficientENERGY);
                    if (cu.is_reward && acc.energy > acc.max_energy - cu.value)
                        return std::unexpected(CurrencyError::EnergyFull);
                    break;
            }
        }
        return updates;
    }
}

TEST(CurrencyValidation, SimpleMPSpend)
{
    AccountState acc{1000, 0, 0, 0, 100};
    auto result = ValidateCurrency(acc, {{CurrencyType::MP, 500, false}});
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->size(), 1u);
    EXPECT_EQ(result->at(0).type, CurrencyType::MP);
    EXPECT_EQ(result->at(0).value, 500u);
    EXPECT_FALSE(result->at(0).is_reward);
}

TEST(CurrencyValidation, InsufficientMP)
{
    AccountState acc{100, 0, 0, 0, 100};
    auto result = ValidateCurrency(acc, {{CurrencyType::MP, 200, false}});
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), CurrencyError::InsufficientMP);
}

TEST(CurrencyValidation, InsufficientRT)
{
    AccountState acc{0, 50, 0, 0, 100};
    auto result = ValidateCurrency(acc, {{CurrencyType::RT, 100, false}});
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), CurrencyError::InsufficientRT);
}

TEST(CurrencyValidation, MPRewardOverflow)
{
    AccountState acc{UINT32_MAX, 0, 0, 0, 100};
    auto result = ValidateCurrency(acc, {{CurrencyType::MP, 1, true}});
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), CurrencyError::MpFull);
}

TEST(CurrencyValidation, RTRewardOverflow)
{
    AccountState acc{0, UINT32_MAX, 0, 0, 100};
    auto result = ValidateCurrency(acc, {{CurrencyType::RT, 1, true}});
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), CurrencyError::RtFull);
}

TEST(CurrencyValidation, CouponsCapAt250)
{
    AccountState acc{0, 0, 250, 0, 100};
    auto result = ValidateCurrency(acc, {{CurrencyType::COUPONS, 1, true}});
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), CurrencyError::CouponsFull);
}

TEST(CurrencyValidation, CouponsUnderCap)
{
    AccountState acc{0, 0, 200, 0, 100};
    auto result = ValidateCurrency(acc, {{CurrencyType::COUPONS, 10, true}});
    ASSERT_TRUE(result.has_value());
}

TEST(CurrencyValidation, EnergyCapAtMax)
{
    AccountState acc{0, 0, 0, 100, 100};
    auto result = ValidateCurrency(acc, {{CurrencyType::ENERGY, 1, true}});
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), CurrencyError::EnergyFull);
}

TEST(CurrencyValidation, MixedRewardsAndCostsNet)
{
    AccountState acc{1000, 0, 0, 0, 100};
    std::vector<CurrencyDelta> ops = {
        {CurrencyType::MP, 300, false},
        {CurrencyType::MP, 200, true}
    };
    auto result = ValidateCurrency(acc, ops);
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->size(), 1u);
    EXPECT_EQ(result->at(0).value, 100u);
    EXPECT_FALSE(result->at(0).is_reward);
}

TEST(CurrencyValidation, RewardsExceedCosts)
{
    AccountState acc{1000, 0, 0, 0, 100};
    std::vector<CurrencyDelta> ops = {
        {CurrencyType::MP, 100, false},
        {CurrencyType::MP, 500, true}
    };
    auto result = ValidateCurrency(acc, ops);
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->size(), 1u);
    EXPECT_EQ(result->at(0).value, 400u);
    EXPECT_TRUE(result->at(0).is_reward);
}

TEST(CurrencyValidation, EqualRewardsAndCosts)
{
    AccountState acc{500, 0, 0, 0, 100};
    std::vector<CurrencyDelta> ops = {
        {CurrencyType::MP, 200, false},
        {CurrencyType::MP, 200, true}
    };
    auto result = ValidateCurrency(acc, ops);
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(result->empty());
}

TEST(CurrencyValidation, MultipleCurrencyTypes)
{
    AccountState acc{1000, 500, 100, 50, 200};
    std::vector<CurrencyDelta> ops = {
        {CurrencyType::MP, 100, false},
        {CurrencyType::RT, 50, true},
        {CurrencyType::COUPONS, 10, true},
        {CurrencyType::ENERGY, 20, false}
    };
    auto result = ValidateCurrency(acc, ops);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->size(), 4u);
}

TEST(CurrencyValidation, NoOpsEmptyResult)
{
    AccountState acc{0, 0, 0, 0, 100};
    auto result = ValidateCurrency(acc, {});
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(result->empty());
}

TEST(CurrencyValidation, ExactSpendAllMP)
{
    AccountState acc{500, 0, 0, 0, 100};
    auto result = ValidateCurrency(acc, {{CurrencyType::MP, 500, false}});
    ASSERT_TRUE(result.has_value());
}
