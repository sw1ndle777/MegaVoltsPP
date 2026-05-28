#pragma once
#include "IDatabase.h"
#include "CLog.h"

#include <mariadb/conncpp.hpp>

namespace BaseLib
{
    class CMariaDatabase : public IDatabase
    {
    public:
        CMariaDatabase() = default;
        ~CMariaDatabase() override = default;

        void Initialize(const std::string& database, const std::string& host, const uint16_t& port, const std::string& user, const std::string& password) override;
        bool CreateTable(const std::string& table_name, const std::string& data_collumns) override;
        bool CreateDatabase(const std::string& name) override;
        [[nodiscard]] std::string GenerateQuestionMarks(size_t n) override;
        [[nodiscard]] std::string GenerateQuestionMarks(size_t rows, size_t cols) override;
        [[nodiscard]] std::string GenerateInTuples(size_t rows, size_t cols) override;
        [[nodiscard]] std::string GenerateJoinedString(const std::vector<std::string>& vec, const std::string& delim) override;
        bool GetFrontAccount(const std::string& ip, const std::string& username, const std::string& password, FrontAccount* outFrontAccount, ClanInfo* outClanInfo) override;
        bool GetFrontAccount(const uint64_t authKey, FrontAccount* outFrontAccount, ClanInfo* outClanInfo) override;
        bool GetMainFrontAccount(const uint64_t authKey, uint32_t server_id, FrontAccount* outFrontAccount, ClanInfo* outClanInfo, PlayerDailyMission* outDailyMission, std::vector<Item>& inv_items, std::vector<SocialInfo>& socials, std::vector<BlockedInfo>& blockeds, std::vector<FriendInfo>& friends, std::vector<MailboxInfo>& mailbox_list, std::vector<std::uint32_t>& daily_mission_random_ids, std::vector<GachaPityEntry>& gacha_pity, SystemMonthlyRewards* outMonthlyRewards, PlayerMonthlyReward* outPlayerMonthlyReward) override;
        bool GetSystemMonthlyRewards(uint32_t year, uint32_t month, SystemMonthlyRewards* out) override;
        bool GetPlayerMonthlyReward(uint32_t player_id, PlayerMonthlyReward* out) override;
        bool ClaimMonthlyReward(uint32_t player_id, uint8_t new_day_count, uint64_t now, const Item& reward_item) override;
        bool GetPlazaAuthKey(const std::string& ip, const std::string& username, const std::string& password, PlazaAuth* outPlazaAuth) override;
        bool GetPlazaAuthKey(const std::string& ip, const uint64_t authKey, PlazaAuth* outPlazaAuth) override;
        [[nodiscard]] std::expected<int32_t, DbError> GetAccountIdByNickname(std::string_view nickname) override;
        [[nodiscard]] std::expected<bool, DbError> AccountExists(int32_t aid) override;
        [[nodiscard]] std::expected<void, DbError> SetAccountMutedUntil(int32_t aid, uint64_t muted_until) override;
        [[nodiscard]] std::expected<void, DbError> UpsertAccountBan(int32_t aid, uint64_t unban_unix, std::string_view reason) override;
        [[nodiscard]] std::expected<void, DbError> RemoveAccountBan(int32_t aid) override;
        [[nodiscard]] std::expected<std::optional<AccountPenaltyInfo>, DbError> GetActiveBan(int32_t aid) override;

        [[nodiscard]] std::expected<void, DbError> EnsureConnected() override;
        [[nodiscard]] std::expected<void, DbError> PersistCurrenciesPatches(ValidatedDbUpdates& v) override;
        [[nodiscard]] std::expected<void, DbError> PersistAccountInfoPatches(ValidatedDbUpdates& v) override;
        [[nodiscard]] std::expected<void, DbError> PersistItemDeletes(ValidatedDbUpdates& v, ResultDbUpdateInfo& out) override;
        [[nodiscard]] std::expected<void, DbError> PersistItemPatches(ValidatedDbUpdates& v, ResultDbUpdateInfo& out) override;
        [[nodiscard]] std::expected<void, DbError> PersistItemAdds(ValidatedDbUpdates& v, ResultDbUpdateInfo& out) override;
        [[nodiscard]] std::expected<void, DbError> PersistMissionsPatches(ValidatedDbUpdates& v) override;
        [[nodiscard]] std::expected<void, DbError> PersistMonthlyRewardsPatches(ValidatedDbUpdates& v) override;
        [[nodiscard]] std::expected<void, DbError> PersistMailboxPatches(ValidatedDbUpdates& v) override;
        [[nodiscard]] std::expected<void, DbError> PersistMatchHistoryAdds(ValidatedDbUpdates& v) override;
        [[nodiscard]] std::expected<void, DbError> PersistPlayerSessionsPatches(ValidatedDbUpdates& v) override;
        [[nodiscard]] std::expected<void, DbError> PersistPlayerSocialsPatches(ValidatedDbUpdates& v, ResultDbUpdateInfo& out) override;
        [[nodiscard]] std::expected<void, DbError> PersistGachaPityPatches(ValidatedDbUpdates& v) override;
        [[nodiscard]] std::expected<void, DbError> UpdateAccount(ValidatedDbUpdates& v, ResultDbUpdateInfo& out) override;
        [[nodiscard]] std::expected<void, DbError> UpdateAccounts(std::vector<ValidatedDbUpdates>& batch, std::vector<ResultDbUpdateInfo>& results) override;
        [[nodiscard]] std::expected<void, DbError> InsertAccount(const std::string& username, const std::string& password_hash, const std::string& salt, const std::string& nickname) override;

        [[nodiscard]] std::expected<void, DbError> PersistChatLogs(const std::vector<ChatLogEntry>& logs) override;
        [[nodiscard]] std::expected<void, DbError> PersistItemLogs(const std::vector<ItemLogEntry>& logs) override;
        [[nodiscard]] std::expected<void, DbError> PersistCurrencyLogs(const std::vector<CurrencyLogEntry>& logs) override;
        [[nodiscard]] std::expected<void, DbError> PersistRoomLogs(const std::vector<RoomLogEntry>& logs) override;
        [[nodiscard]] std::expected<void, DbError> PersistAcDetectionLogs(const std::vector<AcDetectionLogEntry>& logs) override;
        [[nodiscard]] std::expected<void, DbError> PersistAuthHistory(const AuthHistoryLogEntry& entry) override;
        [[nodiscard]] std::expected<void, DbError> PersistLogs(const LogContext& ctx) override;

        std::vector<GachaponSaleInfo> GetGachaponSalesInfo() override;
        bool DeleteGachaponSaleInfo(const uint32_t& gachapon_id) override;
        std::string GetDatabaseName() override;

        static DbError FromSQLException(sql::SQLException& e)
        {
            DbError err;
            err.type = (e.getErrorCode() == 1062) ? DbError::Type::DuplicateNickname : DbError::Type::SqlException;
            err.error_code = e.getErrorCode();
            err.sql_state = e.getSQLState();
            err.message = e.what();
            return err;
        }

        sql::Connection* GetConnection();

    private:
        std::string database_name;
        sql::Driver* driver = nullptr;
        sql::Connection* conn = nullptr; // used during Initialize() only
        sql::Properties properties;
    };
}
