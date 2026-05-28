#include <gtest/gtest.h>
#include <common/BaseLib/Utility.h>

TEST(IsValidNickname, ValidSimple)
{
    EXPECT_TRUE(Utility::IsValidNickname(std::string("player123")));
}

TEST(IsValidNickname, ValidSpecialChars)
{
    EXPECT_TRUE(Utility::IsValidNickname(std::string("cool~name")));
    EXPECT_TRUE(Utility::IsValidNickname(std::string("a!b@c")));
    EXPECT_TRUE(Utility::IsValidNickname(std::string("test#$%")));
    EXPECT_TRUE(Utility::IsValidNickname(std::string("[hello]")));
}

TEST(IsValidNickname, ValidAllAllowedChars)
{
    EXPECT_TRUE(Utility::IsValidNickname(std::string("abcdefghijklmnopqrstuvwxyz")));
    EXPECT_TRUE(Utility::IsValidNickname(std::string("0123456789")));
    EXPECT_TRUE(Utility::IsValidNickname(std::string("~!@#$%^&*()-_=+[{]}<.>/?")));
}

TEST(IsValidNickname, ForbiddenGM)
{
    EXPECT_FALSE(Utility::IsValidNickname(std::string("[gm]player")));
    EXPECT_FALSE(Utility::IsValidNickname(std::string("[GM]Player")));
    EXPECT_FALSE(Utility::IsValidNickname(std::string("x[gm]x")));
}

TEST(IsValidNickname, ForbiddenMod)
{
    EXPECT_FALSE(Utility::IsValidNickname(std::string("[mod]test")));
    EXPECT_FALSE(Utility::IsValidNickname(std::string("{mod}test")));
    EXPECT_FALSE(Utility::IsValidNickname(std::string("(mod)test")));
}

TEST(IsValidNickname, ForbiddenAdmin)
{
    EXPECT_FALSE(Utility::IsValidNickname(std::string("admin")));
    EXPECT_FALSE(Utility::IsValidNickname(std::string("ADMIN")));
    EXPECT_FALSE(Utility::IsValidNickname(std::string("Admin")));
    EXPECT_FALSE(Utility::IsValidNickname(std::string("administrator")));
}

TEST(IsValidNickname, ForbiddenGamemaster)
{
    EXPECT_FALSE(Utility::IsValidNickname(std::string("gamemaster")));
    EXPECT_FALSE(Utility::IsValidNickname(std::string("moderator")));
}

TEST(IsValidNickname, ForbiddenSlurs)
{
    EXPECT_FALSE(Utility::IsValidNickname(std::string("nigger")));
    EXPECT_FALSE(Utility::IsValidNickname(std::string("NIGGA")));
    EXPECT_FALSE(Utility::IsValidNickname(std::string("faggot")));
    EXPECT_FALSE(Utility::IsValidNickname(std::string("tranny")));
    EXPECT_FALSE(Utility::IsValidNickname(std::string("retard")));
}

TEST(IsValidNickname, ForbiddenCaseInsensitive)
{
    EXPECT_FALSE(Utility::IsValidNickname(std::string("AdMiN")));
    EXPECT_FALSE(Utility::IsValidNickname(std::string("GAMEMASTER")));
    EXPECT_FALSE(Utility::IsValidNickname(std::string("{GM}cool")));
    EXPECT_FALSE(Utility::IsValidNickname(std::string("(GM)cool")));
}

TEST(IsValidNickname, DisallowedSpace)
{
    EXPECT_FALSE(Utility::IsValidNickname(std::string("hello world")));
}

TEST(IsValidNickname, DisallowedBackslash)
{
    EXPECT_FALSE(Utility::IsValidNickname(std::string("test\\name")));
}

TEST(IsValidNickname, DisallowedUnicode)
{
    EXPECT_FALSE(Utility::IsValidNickname(std::string("\xc3\xa9")));
}

TEST(IsValidNickname, EmptyString)
{
    EXPECT_TRUE(Utility::IsValidNickname(std::string("")));
}

TEST(IsValidNickname, SingleChar)
{
    EXPECT_TRUE(Utility::IsValidNickname(std::string("a")));
    EXPECT_TRUE(Utility::IsValidNickname(std::string("1")));
}

TEST(IsValidNickname, ForbiddenAsSubstring)
{
    EXPECT_FALSE(Utility::IsValidNickname(std::string("xadminx")));
    EXPECT_FALSE(Utility::IsValidNickname(std::string("theretardx")));
}
