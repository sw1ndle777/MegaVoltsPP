#include <gtest/gtest.h>
#include <cstdint>
#include <set>
#include <vector>
#include <algorithm>

namespace
{
    enum class VotekickResult
    {
        Ok,
        NotPlaying,
        AlreadyVoted,
        SelfKick,
        InsufficientMP,
        TargetNotInRoom
    };

    VotekickResult ValidateVotekick(
        uint32_t voter_sid,
        uint32_t target_sid,
        uint32_t voter_mp,
        bool is_playing,
        const std::set<uint32_t>& already_voted,
        const std::vector<uint32_t>& playing_sids)
    {
        if (!is_playing)
            return VotekickResult::NotPlaying;
        if (already_voted.contains(voter_sid))
            return VotekickResult::AlreadyVoted;
        if (target_sid == voter_sid)
            return VotekickResult::SelfKick;
        if (voter_mp < 100)
            return VotekickResult::InsufficientMP;
        if (!std::ranges::contains(playing_sids, target_sid))
            return VotekickResult::TargetNotInRoom;
        return VotekickResult::Ok;
    }
}

TEST(VotekickRules, ValidVotekick)
{
    std::set<uint32_t> voted;
    std::vector<uint32_t> playing = {1, 2, 3};
    EXPECT_EQ(ValidateVotekick(1, 2, 500, true, voted, playing), VotekickResult::Ok);
}

TEST(VotekickRules, NotPlaying)
{
    std::set<uint32_t> voted;
    std::vector<uint32_t> playing = {1, 2};
    EXPECT_EQ(ValidateVotekick(1, 2, 500, false, voted, playing), VotekickResult::NotPlaying);
}

TEST(VotekickRules, AlreadyVoted)
{
    std::set<uint32_t> voted = {1};
    std::vector<uint32_t> playing = {1, 2};
    EXPECT_EQ(ValidateVotekick(1, 2, 500, true, voted, playing), VotekickResult::AlreadyVoted);
}

TEST(VotekickRules, SelfKick)
{
    std::set<uint32_t> voted;
    std::vector<uint32_t> playing = {1, 2};
    EXPECT_EQ(ValidateVotekick(1, 1, 500, true, voted, playing), VotekickResult::SelfKick);
}

TEST(VotekickRules, InsufficientMP)
{
    std::set<uint32_t> voted;
    std::vector<uint32_t> playing = {1, 2};
    EXPECT_EQ(ValidateVotekick(1, 2, 99, true, voted, playing), VotekickResult::InsufficientMP);
}

TEST(VotekickRules, ExactlyEnoughMP)
{
    std::set<uint32_t> voted;
    std::vector<uint32_t> playing = {1, 2};
    EXPECT_EQ(ValidateVotekick(1, 2, 100, true, voted, playing), VotekickResult::Ok);
}

TEST(VotekickRules, ZeroMP)
{
    std::set<uint32_t> voted;
    std::vector<uint32_t> playing = {1, 2};
    EXPECT_EQ(ValidateVotekick(1, 2, 0, true, voted, playing), VotekickResult::InsufficientMP);
}

TEST(VotekickRules, TargetNotInRoom)
{
    std::set<uint32_t> voted;
    std::vector<uint32_t> playing = {1, 3};
    EXPECT_EQ(ValidateVotekick(1, 2, 500, true, voted, playing), VotekickResult::TargetNotInRoom);
}

TEST(VotekickRules, TargetIsObserver)
{
    std::set<uint32_t> voted;
    std::vector<uint32_t> playing = {1, 3};
    EXPECT_EQ(ValidateVotekick(1, 99, 500, true, voted, playing), VotekickResult::TargetNotInRoom);
}
