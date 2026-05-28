#include <gtest/gtest.h>
#include <common/BaseLib/CDatabaseFactory.h>
#include <common/BaseLib/CMariaDatabase.h>

TEST(DatabaseFactory, MariadbDriver)
{
    auto db = BaseLib::CreateDatabase("mariadb");
    ASSERT_NE(db, nullptr);
    EXPECT_NE(dynamic_cast<BaseLib::CMariaDatabase*>(db.get()), nullptr);
}

TEST(DatabaseFactory, EmptyDriverDefaultsToMariadb)
{
    auto db = BaseLib::CreateDatabase("");
    ASSERT_NE(db, nullptr);
    EXPECT_NE(dynamic_cast<BaseLib::CMariaDatabase*>(db.get()), nullptr);
}

TEST(DatabaseFactory, UnknownDriverDefaultsToMariadb)
{
    auto db = BaseLib::CreateDatabase("sqlite");
    ASSERT_NE(db, nullptr);
    EXPECT_NE(dynamic_cast<BaseLib::CMariaDatabase*>(db.get()), nullptr);
}

TEST(DatabaseFactory, ReturnsUniqueInstances)
{
    auto db1 = BaseLib::CreateDatabase("mariadb");
    auto db2 = BaseLib::CreateDatabase("mariadb");
    EXPECT_NE(db1.get(), db2.get());
}
