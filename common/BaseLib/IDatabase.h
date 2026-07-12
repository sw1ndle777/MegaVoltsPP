#pragma once
#include "DbError.h"

#include <string>
#include <vector>
#include <memory>
#include <expected>

namespace BaseLib
{
    class IDatabase
    {
    public:
        virtual ~IDatabase() = default;

        /// @brief create the database connection, optionally create the database, and initialize all tables.
        /// @param database name of the database to connect to or create.
        /// @param host hostname or ip address of the database server.
        /// @param port tcp port number.
        /// @param user database authentication username.
        /// @param password database authentication password.
        virtual void Initialize(const std::string& database, const std::string& host, const uint16_t& port, const std::string& user, const std::string& password) = 0;

        /// @brief verify the connection is alive and reconnect if it dropped.
        /// @return void on success, or a DbError describing the failure.
        [[nodiscard]] virtual std::expected<void, DbError> EnsureConnected() = 0;

        /// @brief create a table if it does not already exist.
        /// @param table_name the sql table name.
        /// @param data_collumns column definitions as raw sql.
        /// @return true if the table was created or already exists.
        virtual bool CreateTable(const std::string& table_name, const std::string& data_collumns) = 0;

        /// @brief create the database if it does not already exist.
        /// @param name the database name to create.
        /// @return true on success.
        virtual bool CreateDatabase(const std::string& name) = 0;

        /// @brief generate a comma-separated list of parameter placeholders.
        /// @param n number of placeholders.
        /// @return placeholder string (e.g. "?, ?, ?" for mariadb or "$1, $2, $3" for postgresql).
        [[nodiscard]] virtual std::string GenerateQuestionMarks(size_t n) = 0;

        /// @brief generate a comma-separated list of grouped parameter placeholders for multi-row inserts.
        /// @param rows number of rows.
        /// @param cols number of columns per row.
        /// @return grouped placeholder string.
        [[nodiscard]] virtual std::string GenerateQuestionMarks(size_t rows, size_t cols) = 0;

        /// @brief generate comma-separated tuples for use in IN clauses.
        /// @param rows number of tuples.
        /// @param cols number of elements per tuple.
        /// @return tuple string.
        [[nodiscard]] virtual std::string GenerateInTuples(size_t rows, size_t cols) = 0;

        /// @brief join a vector of strings with a delimiter.
        /// @param vec strings to join.
        /// @param delim separator between elements.
        /// @return the joined string.
        [[nodiscard]] virtual std::string GenerateJoinedString(const std::vector<std::string>& vec, const std::string& delim) = 0;

        /// @brief authenticate a front-server login by username and password.
        /// @param ip client ip address for logging.
        /// @param username the account username.
        /// @param password the account password (plaintext, hashed server-side).
        /// @param outFrontAccount receives the account data on success.
        /// @param outClanInfo receives the clan data on success.
        /// @return true if authentication succeeded.
        virtual bool GetFrontAccount(const std::string& ip, const std::string& username, const std::string& password, FrontAccount* outFrontAccount, ClanInfo* outClanInfo) = 0;

        /// @brief authenticate a front-server login by auth key.
        /// @param authKey the session authentication key.
        /// @param outFrontAccount receives the account data on success.
        /// @param outClanInfo receives the clan data on success.
        /// @return true if the auth key matched an account.
        virtual bool GetFrontAccount(const uint64_t authKey, FrontAccount* outFrontAccount, ClanInfo* outClanInfo) = 0;

        /// @brief load the full account state for the main server (account, clan, items, socials, mailbox, pity, monthly rewards).
        /// @param authKey the session authentication key.
        /// @param server_id the server id to stamp on the account.
        /// @param outFrontAccount receives account data.
        /// @param outClanInfo receives clan data.
        /// @param outDailyMission receives daily mission progress.
        /// @param inv_items receives the player inventory.
        /// @param socials receives social entries.
        /// @param blockeds receives blocked-player entries.
        /// @param friends receives friend entries.
        /// @param mailbox_list receives mailbox messages.
        /// @param daily_mission_random_ids receives the random mission ids pool.
        /// @param gacha_pity receives gacha pity state per gacha id.
        /// @param outMonthlyRewards receives the system monthly reward schedule.
        /// @param outPlayerMonthlyReward receives the player monthly reward progress.
        /// @return true on success.
        virtual bool GetMainFrontAccount(const uint64_t authKey, uint32_t server_id, FrontAccount* outFrontAccount, ClanInfo* outClanInfo, PlayerDailyMission* outDailyMission, std::vector<Item>& inv_items, std::vector<SocialInfo>& socials, std::vector<BlockedInfo>& blockeds, std::vector<FriendInfo>& friends, std::vector<MailboxInfo>& mailbox_list, std::vector<std::uint32_t>& daily_mission_random_ids, std::vector<GachaPityEntry>& gacha_pity, SystemMonthlyRewards* outMonthlyRewards, PlayerMonthlyReward* outPlayerMonthlyReward, SystemWeeklyRewards* outWeeklyRewards, PlayerWeeklyReward* outPlayerWeeklyReward) = 0;

        /// @brief load the system monthly reward schedule for a given month.
        /// @param year calendar year.
        /// @param month calendar month (1-12).
        /// @param out receives the reward data.
        /// @return true if found.
        virtual bool GetSystemMonthlyRewards(uint32_t year, uint32_t month, SystemMonthlyRewards* out) = 0;

        /// @brief load a player monthly reward progress.
        /// @param player_id the account id.
        /// @param out receives the reward progress.
        /// @return true if found.
        virtual bool GetPlayerMonthlyReward(uint32_t player_id, PlayerMonthlyReward* out) = 0;
        virtual bool GetSystemPlaytimeRewards(uint32_t year, uint32_t month, SystemPlaytimeRewards* out) = 0;
        virtual bool GetPlayerPlaytime(uint32_t player_id, PlayerPlaytime* out) = 0;
        // Battle Pass (MICROPASS)
        virtual bool GetActiveBattlePassSeason(SystemBattlePassSeason* out) = 0;
        virtual bool GetSystemBattlePassLevels(uint32_t season, std::vector<SystemBattlePassLevel>* out) = 0;
        virtual bool GetSystemBattlePassMissions(std::vector<SystemBattlePassMission>* out) = 0;
        virtual bool GetPlayerBattlePass(uint32_t player_id, PlayerBattlePass* out) = 0;

        /// @brief claim a daily monthly reward and insert the reward item.
        /// @param player_id the account id.
        /// @param new_day_count the updated day count after claiming.
        /// @param now current unix timestamp.
        /// @param reward_item the item to grant.
        /// @return true on success.
        virtual bool ClaimMonthlyReward(uint32_t player_id, uint8_t new_day_count, uint64_t now, const Item& reward_item) = 0;

        /// @brief authenticate a plaza login by username and password.
        /// @param ip client ip address for logging.
        /// @param username the account username.
        /// @param password the account password.
        /// @param outPlazaAuth receives the plaza auth data.
        /// @return true if authentication succeeded.
        virtual bool GetPlazaAuthKey(const std::string& ip, const std::string& username, const std::string& password, PlazaAuth* outPlazaAuth) = 0;

        /// @brief authenticate a plaza login by auth key.
        /// @param ip client ip address for logging.
        /// @param authKey the session authentication key.
        /// @param outPlazaAuth receives the plaza auth data.
        /// @return true if the auth key matched.
        virtual bool GetPlazaAuthKey(const std::string& ip, const uint64_t authKey, PlazaAuth* outPlazaAuth) = 0;

        /// @brief look up an account id by nickname.
        /// @param nickname the player nickname to search for.
        /// @return the account id, or a DbError if not found or on failure.
        [[nodiscard]] virtual std::expected<int32_t, DbError> GetAccountIdByNickname(std::string_view nickname) = 0;

        /// @brief check whether an account exists by id.
        /// @param aid the account id.
        /// @return true if the account exists, or a DbError on failure.
        [[nodiscard]] virtual std::expected<bool, DbError> AccountExists(int32_t aid) = 0;

        /// @brief set the muted-until timestamp for an account.
        /// @param aid the account id.
        /// @param muted_until unix timestamp when the mute expires.
        [[nodiscard]] virtual std::expected<void, DbError> SetAccountMutedUntil(int32_t aid, uint64_t muted_until) = 0;

        /// @brief insert or update an account ban.
        /// @param aid the account id.
        /// @param unban_unix unix timestamp when the ban expires.
        /// @param reason human-readable ban reason.
        [[nodiscard]] virtual std::expected<void, DbError> UpsertAccountBan(int32_t aid, uint64_t unban_unix, std::string_view reason) = 0;

        /// @brief remove an active ban for an account.
        /// @param aid the account id.
        [[nodiscard]] virtual std::expected<void, DbError> RemoveAccountBan(int32_t aid) = 0;

        /// @brief retrieve the active ban for an account, if any.
        /// @param aid the account id.
        /// @return the ban info if active, nullopt if not banned, or a DbError on failure.
        [[nodiscard]] virtual std::expected<std::optional<AccountPenaltyInfo>, DbError> GetActiveBan(int32_t aid) = 0;

        /// @brief persist currency delta patches inside a transaction.
        /// @param v the validated updates containing currency deltas.
        [[nodiscard]] virtual std::expected<void, DbError> PersistCurrenciesPatches(ValidatedDbUpdates& v) = 0;

        /// @brief persist account info field patches inside a transaction.
        /// @param v the validated updates containing account info patches.
        [[nodiscard]] virtual std::expected<void, DbError> PersistAccountInfoPatches(ValidatedDbUpdates& v) = 0;

        /// @brief persist item deletions inside a transaction.
        /// @param v the validated updates containing item deletions.
        /// @param out receives the serials and row counts of deleted items.
        [[nodiscard]] virtual std::expected<void, DbError> PersistItemDeletes(ValidatedDbUpdates& v, ResultDbUpdateInfo& out) = 0;

        /// @brief persist item field patches inside a transaction.
        /// @param v the validated updates containing item patches.
        /// @param out receives the serials and row counts of patched items.
        [[nodiscard]] virtual std::expected<void, DbError> PersistItemPatches(ValidatedDbUpdates& v, ResultDbUpdateInfo& out) = 0;

        /// @brief persist new item inserts inside a transaction.
        /// @param v the validated updates containing items to add.
        /// @param out receives the serials and row counts of added items.
        [[nodiscard]] virtual std::expected<void, DbError> PersistItemAdds(ValidatedDbUpdates& v, ResultDbUpdateInfo& out) = 0;

        /// @brief persist daily mission progress patches inside a transaction.
        /// @param v the validated updates containing mission patches.
        [[nodiscard]] virtual std::expected<void, DbError> PersistMissionsPatches(ValidatedDbUpdates& v) = 0;

        /// @brief persist monthly reward progress patches inside a transaction.
        /// @param v the validated updates containing monthly reward patches.
        [[nodiscard]] virtual std::expected<void, DbError> PersistMonthlyRewardsPatches(ValidatedDbUpdates& v) = 0;
        [[nodiscard]] virtual std::expected<void, DbError> PersistWeeklyRewardsPatches(ValidatedDbUpdates& v) = 0;
        [[nodiscard]] virtual std::expected<void, DbError> PersistPlaytimePatches(ValidatedDbUpdates& v) = 0;
        [[nodiscard]] virtual std::expected<void, DbError> PersistBattlePassPatches(ValidatedDbUpdates& v) = 0;

        /// @brief persist mailbox operations (read, delete, insert) inside a transaction.
        /// @param v the validated updates containing mailbox patches.
        [[nodiscard]] virtual std::expected<void, DbError> PersistMailboxPatches(ValidatedDbUpdates& v) = 0;

        /// @brief persist match history inserts inside a transaction.
        /// @param v the validated updates containing match history entries.
        [[nodiscard]] virtual std::expected<void, DbError> PersistMatchHistoryAdds(ValidatedDbUpdates& v) = 0;

        /// @brief persist player session inserts and deletions inside a transaction.
        /// @param v the validated updates containing session patches.
        [[nodiscard]] virtual std::expected<void, DbError> PersistPlayerSessionsPatches(ValidatedDbUpdates& v) = 0;

        /// @brief persist social relationship changes (friend/block add, remove, update) inside a transaction.
        /// @param v the validated updates containing social patches.
        /// @param out receives target_not_found / target_blocked / player_blocked flags.
        [[nodiscard]] virtual std::expected<void, DbError> PersistPlayerSocialsPatches(ValidatedDbUpdates& v, ResultDbUpdateInfo& out) = 0;

        /// @brief persist gacha pity point updates inside a transaction.
        /// @param v the validated updates containing gacha pity patches.
        [[nodiscard]] virtual std::expected<void, DbError> PersistGachaPityPatches(ValidatedDbUpdates& v) = 0;

        /// @brief execute a full account update transaction (currencies, info, items, socials, etc.).
        /// @param v the validated updates for one player.
        /// @param out receives per-category result info.
        [[nodiscard]] virtual std::expected<void, DbError> UpdateAccount(ValidatedDbUpdates& v, ResultDbUpdateInfo& out) = 0;

        /// @brief execute a batch account update transaction for multiple players (match end).
        /// @param batch the validated updates for all players.
        /// @param results receives per-player result info, one per entry in batch.
        [[nodiscard]] virtual std::expected<void, DbError> UpdateAccounts(std::vector<ValidatedDbUpdates>& batch, std::vector<ResultDbUpdateInfo>& results) = 0;

        /// @brief register a new account.
        /// @param username the account username.
        /// @param password_hash the bcrypt/argon2 password hash.
        /// @param salt the password salt.
        /// @param nickname the initial player nickname.
        [[nodiscard]] virtual std::expected<void, DbError> InsertAccount(const std::string& username, const std::string& password_hash, const std::string& salt, const std::string& nickname) = 0;

        /// @brief persist chat log entries.
        /// @param logs the chat log entries to insert.
        [[nodiscard]] virtual std::expected<void, DbError> PersistChatLogs(const std::vector<ChatLogEntry>& logs) = 0;

        /// @brief persist item action log entries.
        /// @param logs the item log entries to insert.
        [[nodiscard]] virtual std::expected<void, DbError> PersistItemLogs(const std::vector<ItemLogEntry>& logs) = 0;

        /// @brief persist currency transaction log entries.
        /// @param logs the currency log entries to insert.
        [[nodiscard]] virtual std::expected<void, DbError> PersistCurrencyLogs(const std::vector<CurrencyLogEntry>& logs) = 0;

        /// @brief persist room event log entries.
        /// @param logs the room log entries to insert.
        [[nodiscard]] virtual std::expected<void, DbError> PersistRoomLogs(const std::vector<RoomLogEntry>& logs) = 0;

        /// @brief persist per-match player session spans (join/leave). Default no-op
        ///        so non-MariaDB backends don't have to implement it.
        [[nodiscard]] virtual std::expected<void, DbError> PersistPlayerMatchSessionsAdds(const std::vector<PlayerMatchSessionAdd>& adds) { (void)adds; return {}; }
        [[nodiscard]] virtual std::expected<void, DbError> PersistPlayerMatchCombatAdds(const std::vector<PlayerMatchCombatAdd>& adds) { (void)adds; return {}; }
        [[nodiscard]] virtual std::expected<void, DbError> PersistPlayerMatchEventAdds(const std::vector<PlayerMatchEventAdd>& adds) { (void)adds; return {}; }

        /// @brief persist anticheat detection log entries.
        /// @param logs the detection log entries to insert.
        [[nodiscard]] virtual std::expected<void, DbError> PersistAcDetectionLogs(const std::vector<AcDetectionLogEntry>& logs) = 0;

        /// @brief persist a single auth history log entry.
        /// @param entry the auth history entry to insert.
        [[nodiscard]] virtual std::expected<void, DbError> PersistAuthHistory(const AuthHistoryLogEntry& entry) = 0;

        /// @brief persist all log categories from an aggregated log context.
        /// @param ctx the log context containing all pending log entries.
        [[nodiscard]] virtual std::expected<void, DbError> PersistLogs(const LogContext& ctx) = 0;

        /// @brief load all active gachapon sale overrides.
        /// @return vector of sale info entries.
        virtual std::vector<GachaponSaleInfo> GetGachaponSalesInfo() = 0;

        /// @brief delete a gachapon sale override.
        /// @param gachapon_id the gachapon id to remove the sale for.
        /// @return true on success.
        virtual bool DeleteGachaponSaleInfo(const uint32_t& gachapon_id) = 0;

        /// @brief load the invisible flag from the player_misc table.
        /// @param aid the account id.
        /// @return true if the player is invisible, false if not or row doesn't exist.
        virtual bool GetPlayerMiscInvisible(int32_t aid) = 0;

        /// @brief set the invisible flag in the player_misc table (upserts).
        /// @param aid the account id.
        /// @param val the invisible state.
        [[nodiscard]] virtual std::expected<void, DbError> SetPlayerMiscInvisible(int32_t aid, bool val) = 0;

        /// @brief get the name of the connected database.
        /// @return the database name string.
        virtual std::string GetDatabaseName() = 0;
    };

    /// @brief global database instance, created by the factory at startup.
    extern std::unique_ptr<IDatabase> Database;
}
