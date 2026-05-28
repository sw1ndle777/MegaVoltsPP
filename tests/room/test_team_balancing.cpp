#include <gtest/gtest.h>
#include <cstdint>

namespace
{
    enum class TeamId { Blue, Red, Observer, None };

    TeamId AssignTeam(size_t blue_size, size_t red_size, size_t max_players, bool allow_observers, size_t observer_count)
    {
        auto blue_not_full = blue_size < max_players / 2;
        auto red_not_full = red_size < max_players / 2;

        if (blue_size <= red_size && blue_not_full)
            return TeamId::Blue;
        else if ((red_size < blue_size && red_not_full) || (red_size <= blue_size && !blue_not_full && red_not_full))
            return TeamId::Red;
        else if (!blue_not_full && !red_not_full)
        {
            if (allow_observers && observer_count < 10)
                return TeamId::Observer;
            return TeamId::None;
        }
        return TeamId::None;
    }
}

TEST(TeamBalancing, EqualTeamsJoinBlue)
{
    EXPECT_EQ(AssignTeam(2, 2, 8, false, 0), TeamId::Blue);
}

TEST(TeamBalancing, EmptyTeamsJoinBlue)
{
    EXPECT_EQ(AssignTeam(0, 0, 8, false, 0), TeamId::Blue);
}

TEST(TeamBalancing, BlueSmaller)
{
    EXPECT_EQ(AssignTeam(1, 3, 8, false, 0), TeamId::Blue);
}

TEST(TeamBalancing, RedSmaller)
{
    EXPECT_EQ(AssignTeam(3, 1, 8, false, 0), TeamId::Red);
}

TEST(TeamBalancing, BothFullNoObservers)
{
    EXPECT_EQ(AssignTeam(4, 4, 8, false, 0), TeamId::None);
}

TEST(TeamBalancing, BothFullWithObservers)
{
    EXPECT_EQ(AssignTeam(4, 4, 8, true, 0), TeamId::Observer);
}

TEST(TeamBalancing, BothFullObserversFull)
{
    EXPECT_EQ(AssignTeam(4, 4, 8, true, 10), TeamId::None);
}

TEST(TeamBalancing, BlueFullRedNotFull)
{
    EXPECT_EQ(AssignTeam(4, 3, 8, false, 0), TeamId::Red);
}

TEST(TeamBalancing, RedFullBlueNotFull)
{
    EXPECT_EQ(AssignTeam(3, 4, 8, false, 0), TeamId::Blue);
}

TEST(TeamBalancing, SmallRoom)
{
    EXPECT_EQ(AssignTeam(0, 0, 2, false, 0), TeamId::Blue);
    EXPECT_EQ(AssignTeam(1, 0, 2, false, 0), TeamId::Red);
    EXPECT_EQ(AssignTeam(1, 1, 2, false, 0), TeamId::None);
}

TEST(TeamBalancing, LargeRoom)
{
    EXPECT_EQ(AssignTeam(7, 8, 16, false, 0), TeamId::Blue);
    EXPECT_EQ(AssignTeam(8, 7, 16, false, 0), TeamId::Red);
    EXPECT_EQ(AssignTeam(8, 8, 16, false, 0), TeamId::None);
}
