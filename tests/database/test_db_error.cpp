#include <gtest/gtest.h>
#include <common/BaseLib/DbError.h>

using namespace BaseLib;

TEST(DbError, DefaultTypeIsNone)
{
    DbError err;
    EXPECT_EQ(err.type, DbError::Type::None);
    EXPECT_TRUE(err.message.empty());
    EXPECT_EQ(err.error_code, 0);
    EXPECT_TRUE(err.sql_state.empty());
}

TEST(DbError, AllTypesExist)
{
    EXPECT_EQ(static_cast<int>(DbError::Type::None), 0);
    EXPECT_NE(static_cast<int>(DbError::Type::ConnectionLost), static_cast<int>(DbError::Type::None));
    EXPECT_NE(static_cast<int>(DbError::Type::DuplicateNickname), static_cast<int>(DbError::Type::None));
    EXPECT_NE(static_cast<int>(DbError::Type::ConstraintViolation), static_cast<int>(DbError::Type::None));
    EXPECT_NE(static_cast<int>(DbError::Type::NoRowsAffected), static_cast<int>(DbError::Type::None));
    EXPECT_NE(static_cast<int>(DbError::Type::NicknameNotFound), static_cast<int>(DbError::Type::None));
    EXPECT_NE(static_cast<int>(DbError::Type::MailboxFull), static_cast<int>(DbError::Type::None));
    EXPECT_NE(static_cast<int>(DbError::Type::BlockedByReceiver), static_cast<int>(DbError::Type::None));
    EXPECT_NE(static_cast<int>(DbError::Type::SqlException), static_cast<int>(DbError::Type::None));
    EXPECT_NE(static_cast<int>(DbError::Type::Unknown), static_cast<int>(DbError::Type::None));
}

TEST(DbUpdateError, AllErrorsDistinct)
{
    EXPECT_NE(DbUpdateError::ItemNotFound, DbUpdateError::InsufficientMP);
    EXPECT_NE(DbUpdateError::InsufficientMP, DbUpdateError::InsufficientRT);
    EXPECT_NE(DbUpdateError::InsufficientRT, DbUpdateError::InventoryFull);
    EXPECT_NE(DbUpdateError::MpFull, DbUpdateError::RtFull);
}

TEST(ItemUpdateInfo, DefaultZero)
{
    ItemUpdateInfo info;
    EXPECT_EQ(info.data, 0u);
    EXPECT_EQ(info.id, 0u);
    EXPECT_EQ(info.expire_date, 0u);
    EXPECT_EQ(info.energy, 0u);
}

TEST(ItemUpdateInfo, SetBits)
{
    ItemUpdateInfo info;
    info.id = 1;
    info.energy = 1;
    EXPECT_EQ(info.id, 1u);
    EXPECT_EQ(info.energy, 1u);
    EXPECT_EQ(info.expire_date, 0u);
    EXPECT_NE(info.data, 0u);
}

TEST(ItemUpdateCtx, ChangeType)
{
    ItemUpdateCtx ctx;
    ctx.change_type = ItemUpdateCtxType::Equip;
    EXPECT_EQ(ctx.change_type, ItemUpdateCtxType::Equip);

    ctx.change_type = ItemUpdateCtxType::Upgrade;
    EXPECT_EQ(ctx.change_type, ItemUpdateCtxType::Upgrade);
}
