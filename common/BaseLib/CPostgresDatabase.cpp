#include "CPostgresDatabase.h"
#include <fmt/color.h>
#include <boost_unordered.hpp>
#include <algorithm>
#include <cctype>
#include <cstdlib>

namespace BaseLib
{
    using enum fmt::color;

    DbError CPostgresDatabase::FromPqxxException(const std::exception& e)
    {
        DbError err;
        err.message = e.what();

        if (auto* pqe = dynamic_cast<const pqxx::unique_violation*>(&e))
        {
            err.type = DbError::Type::DuplicateNickname;
            err.sql_state = "23505";
        }
        else if (dynamic_cast<const pqxx::broken_connection*>(&e))
        {
            err.type = DbError::Type::ConnectionLost;
        }
        else if (dynamic_cast<const pqxx::integrity_constraint_violation*>(&e))
        {
            err.type = DbError::Type::ConstraintViolation;
        }
        else if (dynamic_cast<const pqxx::sql_error*>(&e))
        {
            err.type = DbError::Type::SqlException;
            if (auto* sqle = dynamic_cast<const pqxx::sql_error*>(&e))
                err.sql_state = sqle->sqlstate();
        }
        else
        {
            err.type = DbError::Type::Unknown;
        }
        return err;
    }

    namespace { thread_local pqxx::work* tl_active_txn = nullptr; }

    pqxx::connection& CPostgresDatabase::GetConnection()
    {
        thread_local std::unique_ptr<pqxx::connection> tl_conn;

        auto create_connection = [&]() -> pqxx::connection&
        {
            DEBUGLOG(yellow, "Creating PostgreSQL connection for thread...");
            tl_conn = std::make_unique<pqxx::connection>(conn_string_);
            DEBUGLOG(dark_cyan, "Thread-local PostgreSQL connection established");
            return *tl_conn;
        };

        if (!tl_conn || !tl_conn->is_open())
            return create_connection();

        if (!tl_active_txn)
        {
            try
            {
                pqxx::nontransaction ntxn(*tl_conn);
                ntxn.exec("SELECT 1");
            }
            catch (...)
            {
                DEBUGLOG(yellow, "PostgreSQL connection stale, reconnecting...");
                return create_connection();
            }
        }
        return *tl_conn;
    }

    std::pair<pqxx::work*, std::unique_ptr<pqxx::work>> CPostgresDatabase::AcquireTxn()
    {
        if (tl_active_txn)
            return { tl_active_txn, nullptr };
        auto owned = std::make_unique<pqxx::work>(GetConnection());
        return { owned.get(), std::move(owned) };
    }

    void CPostgresDatabase::CommitIfOwned(std::unique_ptr<pqxx::work>& owned)
    {
        if (owned)
            owned->commit();
    }

    void CPostgresDatabase::CreateEnumTypes(pqxx::work& txn)
    {
        const char* enum_ddl[] = {
            R"(DO $$ BEGIN CREATE TYPE chat_type AS ENUM ('User','Whisper','Team','Clan','Command','Party'); EXCEPTION WHEN duplicate_object THEN NULL; END $$)",
            R"(DO $$ BEGIN CREATE TYPE chat_location AS ENUM ('Lobby','Room','Plaza'); EXCEPTION WHEN duplicate_object THEN NULL; END $$)",
            R"(DO $$ BEGIN CREATE TYPE item_action_type AS ENUM ('Added','Deleted','Sold','Upgraded','Reset','Repaired','EnergyInjected','Gifted','Received'); EXCEPTION WHEN duplicate_object THEN NULL; END $$)",
            R"(DO $$ BEGIN CREATE TYPE item_type_category AS ENUM ('Hair','Face','Upper','Under','Pants','Hands','Boots','AccHead','AccWaist','AccBack','Melee','Rifle','Shotgun','Sniper','Gatling','Bazooka','Grenade','Set','ShieldEnamel','FlagBlue','Gatcha','Unknown1','Diorama1','Diorama2','Question1','MonsterFace','Unknown3','Unknown4'); EXCEPTION WHEN duplicate_object THEN NULL; END $$)",
            R"(DO $$ BEGIN CREATE TYPE item_origin_type AS ENUM ('Shop','ShopCoupon','Gachapon','Package','BossBattle','Tutorial','Story','LevelUp','DailyMission','MonthlyReward','GiftSent','GiftReceived','GMSpawned','Pickup','Unknown'); EXCEPTION WHEN duplicate_object THEN NULL; END $$)",
            R"(DO $$ BEGIN CREATE TYPE currency_type AS ENUM ('MP','RT','Coupons','Energy'); EXCEPTION WHEN duplicate_object THEN NULL; END $$)",
            R"(DO $$ BEGIN CREATE TYPE currency_source_type AS ENUM ('Shop','ShopCoupon','Gachapon','Package','ItemSell','ItemRepair','ItemUpgrade','BossBattle','Tutorial','LevelUp','Achievement','DailyMission','MonthlyReward','GiftSend','VoteKick','MatchReward','Admin','Unknown'); EXCEPTION WHEN duplicate_object THEN NULL; END $$)",
            R"(DO $$ BEGIN CREATE TYPE room_event_type AS ENUM ('RoomCreated','RoomJoined','RoomLeft','RoomKicked','TeamChanged','VoteKickStarted','VoteKickAgreed','VoteKickSucceeded','VoteKickFailed'); EXCEPTION WHEN duplicate_object THEN NULL; END $$)",
            R"(DO $$ BEGIN CREATE TYPE clan_role_type AS ENUM ('Owner','CaptainA','CaptainB','Member'); EXCEPTION WHEN duplicate_object THEN NULL; END $$)",
            R"(DO $$ BEGIN CREATE TYPE vote_type AS ENUM ('Kick','Surrender','MapChange'); EXCEPTION WHEN duplicate_object THEN NULL; END $$)",
            R"(DO $$ BEGIN CREATE TYPE detection_flag AS ENUM ('None','DebuggerPresent','DebugPort','TimingAnomaly','PebDebugFlag','InlineHook','IatHook','HoneypotTriggered','IntegrityViolation','DllInjection','ManualMap','AnonymousThread','ProxyDll','GlobalHookInjection','MappedImage','HookIntegrity','BlacklistedModule','UnsignedModule','DangerousHandle','VulnerableDriver','BlacklistedString','BlacklistedSignature','UnsignedDriver','DriverBlocklistDisabled','HvciDisabled','FileIntegrityFail','MatchKillMismatch','LoginSpam','HeartbeatTimeout','InvalidResponse','UnknownFlag'); EXCEPTION WHEN duplicate_object THEN NULL; END $$)",
            R"(DO $$ BEGIN CREATE TYPE transaction_status AS ENUM ('Pending','Completed','Failed','Cancelled'); EXCEPTION WHEN duplicate_object THEN NULL; END $$)",
        };

        for (const auto& ddl : enum_ddl)
            txn.exec(ddl);
    }

    void CPostgresDatabase::Initialize(const std::string& database, const std::string& host, const uint16_t& port, const std::string& user, const std::string& password)
    {
        try
        {
            database_name_ = database;
            conn_string_ = fmt::format("host={} port={} user={} password={} dbname={} connect_timeout=10",
                host, port, user, password, database);

            auto admin_conn_str = fmt::format("host={} port={} user={} password={} dbname=postgres connect_timeout=10",
                host, port, user, password);

            {
                pqxx::connection admin_conn(admin_conn_str);
                pqxx::nontransaction ntxn(admin_conn);
                auto r = ntxn.exec("SELECT 1 FROM pg_database WHERE datname = $1", pqxx::params{database});
                if (r.empty())
                {
                    ntxn.exec("CREATE DATABASE " + ntxn.quote_name(database));
                    DEBUGLOG(dark_cyan, "created database ({})", database);
                }
            }

            conn_ = std::make_unique<pqxx::connection>(conn_string_);
            DEBUGLOG(dark_cyan, "connected to postgresql ({}:{})", host, port);

            {
                pqxx::work txn(*conn_);
                CreateEnumTypes(txn);
                txn.commit();
            }

            CreateTable("accounts", R"(
                Id INTEGER GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
                Email VARCHAR(255) NOT NULL DEFAULT '',
                IsEmailVerified BOOLEAN DEFAULT FALSE,
                ServerId INTEGER NOT NULL DEFAULT 0,
                Username VARCHAR(16) NOT NULL,
                Password VARCHAR(127) NOT NULL,
                Salt VARCHAR(127) NOT NULL,
                Grade SMALLINT NOT NULL DEFAULT 0,
                PCRoom SMALLINT NOT NULL DEFAULT 0,
                AuthKey BIGINT NOT NULL DEFAULT 0,
                ClanId INTEGER DEFAULT NULL,
                ClanKills INTEGER DEFAULT 0,
                ClanDeaths INTEGER DEFAULT 0,
                ClanAssists INTEGER DEFAULT 0,
                ClanContribution INTEGER DEFAULT 0,
                ClanWins INTEGER DEFAULT 0,
                ClanLoses INTEGER DEFAULT 0,
                ClanDraws INTEGER DEFAULT 0,
                Nickname VARCHAR(16) NOT NULL,
                Level INTEGER NOT NULL DEFAULT 0,
                Experience INTEGER NOT NULL DEFAULT 0,
                Tutorial BOOLEAN NOT NULL DEFAULT FALSE,
                Story INTEGER NOT NULL DEFAULT 0,
                GuideMission SMALLINT NOT NULL DEFAULT 0,
                Achievement BIGINT NOT NULL DEFAULT 0,
                VoiceType BIGINT NOT NULL DEFAULT 0,
                VIPExperience INTEGER NOT NULL DEFAULT 0,
                MaximumItems INTEGER NOT NULL DEFAULT 100,
                MaximumEnergy INTEGER NOT NULL DEFAULT 100,
                SelectedCharacter INTEGER NOT NULL DEFAULT 0,
                PlayTime BIGINT NOT NULL DEFAULT 0,
                MutedUntil BIGINT NOT NULL DEFAULT 0,
                Coins INTEGER NOT NULL DEFAULT 1000,
                Energy INTEGER NOT NULL DEFAULT 100,
                LuckyPoints INTEGER NOT NULL DEFAULT 0,
                MicroPoints BIGINT NOT NULL DEFAULT 0,
                RockTokens BIGINT NOT NULL DEFAULT 0,
                Coupons INTEGER NOT NULL DEFAULT 0,
                Wins INTEGER NOT NULL DEFAULT 0,
                Loses INTEGER NOT NULL DEFAULT 0,
                Draws INTEGER NOT NULL DEFAULT 0,
                Kills INTEGER NOT NULL DEFAULT 0,
                Deaths INTEGER NOT NULL DEFAULT 0,
                Assists INTEGER NOT NULL DEFAULT 0,
                Headshots INTEGER NOT NULL DEFAULT 0,
                HighestKillStreak INTEGER NOT NULL DEFAULT 0,
                MeleeKills INTEGER NOT NULL DEFAULT 0,
                RifleKills INTEGER NOT NULL DEFAULT 0,
                ShotgunKills INTEGER NOT NULL DEFAULT 0,
                SniperKills INTEGER NOT NULL DEFAULT 0,
                GatlingKills INTEGER NOT NULL DEFAULT 0,
                BazookaKills INTEGER NOT NULL DEFAULT 0,
                GrenadeKills INTEGER NOT NULL DEFAULT 0,
                ZombieKills INTEGER NOT NULL DEFAULT 0,
                Infections INTEGER NOT NULL DEFAULT 0,
                SingleWaveDailyAttempts INTEGER NOT NULL DEFAULT 0,
                SingleWaveHighestWave INTEGER NOT NULL DEFAULT 0,
                SingleWaveHighScore INTEGER NOT NULL DEFAULT 0,
                SingleWaveLastUpdate BIGINT NOT NULL DEFAULT 0,
                LawfulPoint INTEGER NOT NULL DEFAULT 0,
                ChaoticPoint INTEGER NOT NULL DEFAULT 0,
                IsAdmin BOOLEAN NOT NULL DEFAULT FALSE,
                TwoFactorSecret VARCHAR(255) DEFAULT NULL,
                TwoFactorEnabled BOOLEAN DEFAULT FALSE)");

            CreateTable("game_titles", R"(
                Id INTEGER GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
                TitleName VARCHAR(50) NOT NULL)");

            CreateTable("account_titles", R"(
                AccountId INTEGER NOT NULL,
                TitleId INTEGER NOT NULL,
                PRIMARY KEY (AccountId, TitleId),
                CONSTRAINT fk_acctitle_account FOREIGN KEY (AccountId) REFERENCES accounts (Id) ON DELETE CASCADE,
                CONSTRAINT fk_acctitle_title FOREIGN KEY (TitleId) REFERENCES game_titles (Id) ON DELETE CASCADE)");

            CreateTable("player_sessions", R"(
                PlayerId INTEGER NOT NULL PRIMARY KEY,
                AuthKey BIGINT NOT NULL UNIQUE,
                IssuedAt TIMESTAMP NOT NULL,
                ExpiresAt TIMESTAMP NOT NULL,
                LastSeenAt TIMESTAMP DEFAULT NULL,
                CONSTRAINT FK_player_sessions_accounts FOREIGN KEY (PlayerId) REFERENCES accounts (Id) ON DELETE CASCADE)");

            CreateTable("player_matchhistory", R"(
                Id BIGINT GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
                MatchUniqueId VARCHAR(128) NOT NULL DEFAULT '',
                AccountId INTEGER NOT NULL,
                IsWin BOOLEAN NOT NULL DEFAULT FALSE,
                IsLose BOOLEAN NOT NULL DEFAULT FALSE,
                IsHost BOOLEAN NOT NULL,
                IsDraw BOOLEAN NOT NULL,
                IsClanMatch BOOLEAN NOT NULL,
                WinRule INTEGER NOT NULL DEFAULT 0,
                TimeRule INTEGER NOT NULL DEFAULT 0,
                WinRuleType VARCHAR(32) NOT NULL DEFAULT 'Unknown',
                PlayTime INTEGER NOT NULL,
                Level INTEGER NOT NULL,
                Experience INTEGER NOT NULL,
                Energy INTEGER NOT NULL,
                MicroPoints INTEGER NOT NULL,
                RoomIndex INTEGER NOT NULL,
                RedScore INTEGER NOT NULL,
                BlueScore INTEGER NOT NULL,
                TeamId INTEGER NOT NULL,
                RoomMode INTEGER NOT NULL,
                RoomMap INTEGER NOT NULL,
                SelectedCharacter INTEGER NOT NULL,
                Kills INTEGER NOT NULL,
                Deaths INTEGER NOT NULL,
                Assists INTEGER NOT NULL,
                Headshots INTEGER NOT NULL,
                HighestKillStreak INTEGER NOT NULL,
                MeleeKills INTEGER NOT NULL,
                RifleKills INTEGER NOT NULL,
                ShotgunKills INTEGER NOT NULL,
                SniperKills INTEGER NOT NULL,
                GatlingKills INTEGER NOT NULL,
                BazookaKills INTEGER NOT NULL,
                GrenadeKills INTEGER NOT NULL,
                ZombieKills INTEGER NOT NULL,
                Infections INTEGER NOT NULL,
                MatchStartTime BIGINT NOT NULL DEFAULT 0,
                MatchStartUtc VARCHAR(32) NOT NULL DEFAULT '',
                MatchEndTime BIGINT NOT NULL,
                MatchEndUtc VARCHAR(32) NOT NULL DEFAULT '',
                Hair INTEGER NOT NULL,
                Face INTEGER NOT NULL,
                Upper_ INTEGER NOT NULL,
                Under_ INTEGER NOT NULL,
                Skirt INTEGER NOT NULL,
                Gloves INTEGER NOT NULL,
                Boots INTEGER NOT NULL,
                HeadAcc INTEGER NOT NULL,
                WaistAcc INTEGER NOT NULL,
                BackAcc INTEGER NOT NULL,
                Melee INTEGER NOT NULL,
                Rifle INTEGER NOT NULL,
                Shotgun INTEGER NOT NULL,
                Sniper INTEGER NOT NULL,
                Gatling INTEGER NOT NULL,
                Bazooka INTEGER NOT NULL,
                Grenade INTEGER NOT NULL,
                RewardItem INTEGER NOT NULL,
                IsMvp BOOLEAN NOT NULL,
                IsEntryFragger BOOLEAN NOT NULL,
                IsBullseye BOOLEAN NOT NULL,
                IsSupport BOOLEAN NOT NULL,
                IsBomba BOOLEAN NOT NULL,
                MvpScore INTEGER NOT NULL DEFAULT 0,
                EntryFraggerScore INTEGER NOT NULL DEFAULT 0,
                BullseyeScore INTEGER NOT NULL DEFAULT 0,
                SupportScore INTEGER NOT NULL DEFAULT 0,
                BombaScore INTEGER NOT NULL DEFAULT 0,
                BestKdScore INTEGER NOT NULL DEFAULT 0,
                CaptureScore INTEGER NOT NULL DEFAULT 0,
                WonRoundScore INTEGER NOT NULL DEFAULT 0,
                ArmsRaceScore INTEGER NOT NULL DEFAULT 0,
                ZombieScore INTEGER NOT NULL DEFAULT 0,
                ADR INTEGER NOT NULL DEFAULT 0,
                CONSTRAINT FK_player_matchhistory_accounts FOREIGN KEY (AccountId) REFERENCES accounts (Id) ON DELETE CASCADE)");

            CreateTable("bans", R"(
                Id INTEGER GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
                AccountId INTEGER NOT NULL,
                UnbanDate TIMESTAMP(6) NOT NULL,
                Reason VARCHAR(127) DEFAULT NULL,
                CONSTRAINT FK_bans_accounts FOREIGN KEY (AccountId) REFERENCES accounts (Id) ON DELETE CASCADE)");

            CreateTable("login_history", R"(
                Id INTEGER GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
                AccountId INTEGER NOT NULL,
                LoginDate TIMESTAMP(6) NOT NULL,
                IP VARCHAR(15) NOT NULL,
                CONSTRAINT FK_login_history_accounts FOREIGN KEY (AccountId) REFERENCES accounts (Id) ON DELETE CASCADE)");

            CreateTable("clans", R"(
                Id INTEGER GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
                OwnerId INTEGER DEFAULT NULL,
                ClanName VARCHAR(16) NOT NULL,
                ClanLogoFront SMALLINT NOT NULL DEFAULT 0,
                ClanLogoBack SMALLINT NOT NULL DEFAULT 0,
                ClanContribution INTEGER NOT NULL DEFAULT 0,
                ClanWin INTEGER NOT NULL DEFAULT 0,
                ClanLose INTEGER NOT NULL DEFAULT 0,
                ClanDraw INTEGER NOT NULL DEFAULT 0,
                Kills INTEGER NOT NULL DEFAULT 0,
                Deaths INTEGER NOT NULL DEFAULT 0,
                Assists INTEGER NOT NULL DEFAULT 0,
                Lawful INTEGER NOT NULL DEFAULT 0,
                Chaotic INTEGER NOT NULL DEFAULT 0,
                Description VARCHAR(255) NOT NULL DEFAULT 'Welcome to our clan page.',
                Title VARCHAR(50) NOT NULL DEFAULT 'Rookie Clan',
                TeamACaptainId INTEGER DEFAULT NULL,
                TeamBCaptainId INTEGER DEFAULT NULL,
                CONSTRAINT FK_Clans_CaptainA FOREIGN KEY (TeamACaptainId) REFERENCES accounts (Id) ON DELETE SET NULL,
                CONSTRAINT FK_Clans_CaptainB FOREIGN KEY (TeamBCaptainId) REFERENCES accounts (Id) ON DELETE SET NULL,
                CONSTRAINT FK_clans_accounts_OwnerId FOREIGN KEY (OwnerId) REFERENCES accounts (Id) ON DELETE CASCADE)");

            CreateTable("clan_member_roles", R"(
                AccountId INTEGER NOT NULL,
                ClanId INTEGER NOT NULL,
                Role clan_role_type NOT NULL DEFAULT 'Member',
                PRIMARY KEY (AccountId, ClanId),
                CONSTRAINT fk_cmr_account FOREIGN KEY (AccountId) REFERENCES accounts (Id) ON DELETE CASCADE,
                CONSTRAINT fk_cmr_clan FOREIGN KEY (ClanId) REFERENCES clans (Id) ON DELETE CASCADE)");

            CreateTable("inventory_items", R"(
                AccountId INTEGER NOT NULL,
                SerialData BIGINT NOT NULL,
                ItemId INTEGER NOT NULL DEFAULT 0,
                ItemType SMALLINT NOT NULL DEFAULT 0,
                CharacterId INTEGER NOT NULL DEFAULT 0,
                IsSealed BOOLEAN NOT NULL DEFAULT FALSE,
                IsEquipped BOOLEAN NOT NULL DEFAULT FALSE,
                Stock INTEGER NOT NULL DEFAULT 0,
                ExpireDate BIGINT NOT NULL DEFAULT 0,
                Repair INTEGER NOT NULL DEFAULT 0,
                Energy INTEGER NOT NULL DEFAULT 0,
                PRIMARY KEY (AccountId, SerialData),
                CONSTRAINT FK_inventory_items_accounts FOREIGN KEY (AccountId) REFERENCES accounts (Id) ON DELETE CASCADE)");

            CreateTable("socials", R"(
                AccountId INTEGER NOT NULL,
                TargetAccountId INTEGER NOT NULL,
                Type SMALLINT NOT NULL DEFAULT 0,
                MutualOnline BOOLEAN NOT NULL DEFAULT FALSE,
                MutualServerId INTEGER NOT NULL DEFAULT 0,
                MutualRoomId INTEGER NOT NULL DEFAULT 0,
                PRIMARY KEY (AccountId, TargetAccountId),
                CONSTRAINT FK_socials_accounts FOREIGN KEY (AccountId) REFERENCES accounts (Id) ON DELETE CASCADE,
                CONSTRAINT FK_socials_target_accounts FOREIGN KEY (TargetAccountId) REFERENCES accounts (Id) ON DELETE CASCADE)");

            CreateTable("mailbox", R"(
                Id BIGINT GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
                AccountId INTEGER NOT NULL,
                SenderAccountId INTEGER NOT NULL DEFAULT 0,
                SenderNickname VARCHAR(16) NOT NULL DEFAULT '',
                IsRead BOOLEAN NOT NULL DEFAULT FALSE,
                ItemId INTEGER NOT NULL DEFAULT 0,
                ItemSerialData BIGINT NOT NULL DEFAULT 0,
                ItemAmount INTEGER NOT NULL DEFAULT 0,
                ExpirationDate BIGINT NOT NULL DEFAULT 0,
                SentDate BIGINT NOT NULL DEFAULT 0,
                Message VARCHAR(255) NOT NULL DEFAULT '',
                CONSTRAINT FK_mailbox_accounts FOREIGN KEY (AccountId) REFERENCES accounts (Id) ON DELETE CASCADE)");

            CreateTable("daily_missions", R"(
                AccountId INTEGER NOT NULL PRIMARY KEY,
                MissionId1 INTEGER NOT NULL DEFAULT 0,
                MissionId2 INTEGER NOT NULL DEFAULT 0,
                MissionId3 INTEGER NOT NULL DEFAULT 0,
                Progress1 INTEGER NOT NULL DEFAULT 0,
                Progress2 INTEGER NOT NULL DEFAULT 0,
                Progress3 INTEGER NOT NULL DEFAULT 0,
                IsClaimed1 BOOLEAN NOT NULL DEFAULT FALSE,
                IsClaimed2 BOOLEAN NOT NULL DEFAULT FALSE,
                IsClaimed3 BOOLEAN NOT NULL DEFAULT FALSE,
                IsAllClaimed BOOLEAN NOT NULL DEFAULT FALSE,
                LastResetTime BIGINT NOT NULL DEFAULT 0,
                CONSTRAINT FK_daily_missions_accounts FOREIGN KEY (AccountId) REFERENCES accounts (Id) ON DELETE CASCADE)");

            CreateTable("daily_mission_random_ids", R"(
                AccountId INTEGER NOT NULL,
                MissionId INTEGER NOT NULL,
                PRIMARY KEY (AccountId, MissionId),
                CONSTRAINT FK_dmri_accounts FOREIGN KEY (AccountId) REFERENCES accounts (Id) ON DELETE CASCADE)");

            CreateTable("gacha_pity", R"(
                AccountId INTEGER NOT NULL,
                GachaId INTEGER NOT NULL,
                LuckyPoints INTEGER NOT NULL DEFAULT 0,
                PRIMARY KEY (AccountId, GachaId),
                CONSTRAINT FK_gacha_pity_accounts FOREIGN KEY (AccountId) REFERENCES accounts (Id) ON DELETE CASCADE)");

            CreateTable("system_monthly_rewards", R"(
                Year INTEGER NOT NULL,
                Month INTEGER NOT NULL,
                Day1 INTEGER NOT NULL DEFAULT 0, Day2 INTEGER NOT NULL DEFAULT 0,
                Day3 INTEGER NOT NULL DEFAULT 0, Day4 INTEGER NOT NULL DEFAULT 0,
                Day5 INTEGER NOT NULL DEFAULT 0, Day6 INTEGER NOT NULL DEFAULT 0,
                Day7 INTEGER NOT NULL DEFAULT 0, Day8 INTEGER NOT NULL DEFAULT 0,
                Day9 INTEGER NOT NULL DEFAULT 0, Day10 INTEGER NOT NULL DEFAULT 0,
                Day11 INTEGER NOT NULL DEFAULT 0, Day12 INTEGER NOT NULL DEFAULT 0,
                Day13 INTEGER NOT NULL DEFAULT 0, Day14 INTEGER NOT NULL DEFAULT 0,
                Day15 INTEGER NOT NULL DEFAULT 0, Day16 INTEGER NOT NULL DEFAULT 0,
                Day17 INTEGER NOT NULL DEFAULT 0, Day18 INTEGER NOT NULL DEFAULT 0,
                Day19 INTEGER NOT NULL DEFAULT 0, Day20 INTEGER NOT NULL DEFAULT 0,
                Day21 INTEGER NOT NULL DEFAULT 0, Day22 INTEGER NOT NULL DEFAULT 0,
                Day23 INTEGER NOT NULL DEFAULT 0, Day24 INTEGER NOT NULL DEFAULT 0,
                Day25 INTEGER NOT NULL DEFAULT 0, Day26 INTEGER NOT NULL DEFAULT 0,
                Day27 INTEGER NOT NULL DEFAULT 0, Day28 INTEGER NOT NULL DEFAULT 0,
                Day29 INTEGER NOT NULL DEFAULT 0, Day30 INTEGER NOT NULL DEFAULT 0,
                Day31 INTEGER NOT NULL DEFAULT 0,
                PRIMARY KEY (Year, Month))");

            CreateTable("player_monthly_rewards", R"(
                AccountId INTEGER NOT NULL PRIMARY KEY,
                DayCount SMALLINT NOT NULL DEFAULT 0,
                LastClaimDate BIGINT NOT NULL DEFAULT 0,
                CONSTRAINT FK_pmr_accounts FOREIGN KEY (AccountId) REFERENCES accounts (Id) ON DELETE CASCADE)");

            CreateTable("gachapon_sales", R"(
                GachaponId INTEGER NOT NULL PRIMARY KEY,
                SalePrice INTEGER NOT NULL DEFAULT 0,
                StartDate BIGINT NOT NULL DEFAULT 0,
                EndDate BIGINT NOT NULL DEFAULT 0)");

            CreateTable("log_chat", R"(
                Id BIGINT GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
                Timestamp BIGINT NOT NULL,
                AccountId INTEGER NOT NULL,
                Nickname VARCHAR(16) NOT NULL DEFAULT '',
                ChatType chat_type NOT NULL DEFAULT 'User',
                Location chat_location NOT NULL DEFAULT 'Channel',
                RoomId INTEGER NOT NULL DEFAULT 0,
                Message TEXT NOT NULL DEFAULT '')");

            CreateTable("log_items", R"(
                Id BIGINT GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
                Timestamp BIGINT NOT NULL,
                AccountId INTEGER NOT NULL,
                ActionType item_action_type NOT NULL DEFAULT 'Add',
                ItemId INTEGER NOT NULL DEFAULT 0,
                ItemSerial BIGINT NOT NULL DEFAULT 0,
                Amount INTEGER NOT NULL DEFAULT 0,
                Source item_origin_type NOT NULL DEFAULT 'Unknown',
                Details TEXT NOT NULL DEFAULT '')");

            CreateTable("log_currency", R"(
                Id BIGINT GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
                Timestamp BIGINT NOT NULL,
                AccountId INTEGER NOT NULL,
                CurrencyType currency_type NOT NULL DEFAULT 'MP',
                Amount INTEGER NOT NULL DEFAULT 0,
                IsReward BOOLEAN NOT NULL DEFAULT TRUE,
                Source currency_source_type NOT NULL DEFAULT 'Unknown',
                BalanceBefore BIGINT NOT NULL DEFAULT 0,
                BalanceAfter BIGINT NOT NULL DEFAULT 0,
                Details TEXT NOT NULL DEFAULT '')");

            CreateTable("log_rooms", R"(
                Id BIGINT GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
                Timestamp BIGINT NOT NULL,
                AccountId INTEGER NOT NULL,
                EventType room_event_type NOT NULL DEFAULT 'Create',
                RoomId INTEGER NOT NULL DEFAULT 0,
                Details TEXT NOT NULL DEFAULT '')");

            CreateTable("log_ac_detection", R"(
                Id BIGINT GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
                Timestamp BIGINT NOT NULL,
                AccountId INTEGER NOT NULL,
                Flag detection_flag NOT NULL DEFAULT 'Unknown',
                Details TEXT NOT NULL DEFAULT '')");

            CreateTable("log_auth_history", R"(
                Id BIGINT GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
                Timestamp BIGINT NOT NULL,
                AccountId INTEGER NOT NULL,
                IP VARCHAR(45) NOT NULL DEFAULT '',
                Success BOOLEAN NOT NULL DEFAULT TRUE,
                Details TEXT NOT NULL DEFAULT '')");

            DEBUGLOG(dark_cyan, "postgresql schema initialized");
        }
        catch (const std::exception& e)
        {
            DEBUGLOG(red, "postgresql initialize failed: {}", e.what());
            throw;
        }
    }

    std::expected<void, DbError> CPostgresDatabase::EnsureConnected()
    {
        try
        {
            auto& c = GetConnection();
            if (c.is_open()) return {};
            return std::unexpected(DbError{ DbError::Type::ConnectionLost, 0, {}, "No database connection" });
        }
        catch (const std::exception& e)
        {
            return std::unexpected(FromPqxxException(e));
        }
    }

    bool CPostgresDatabase::CreateTable(const std::string& table_name, const std::string& data_collumns)
    {
        try
        {
            pqxx::work txn(*conn_);
            txn.exec("CREATE TABLE IF NOT EXISTS " + txn.quote_name(table_name) + " (" + data_collumns + ")");
            txn.commit();
            return true;
        }
        catch (const std::exception& e)
        {
            DEBUGLOG(red, "CreateTable({}) failed: {}", table_name, e.what());
            return false;
        }
    }

    bool CPostgresDatabase::CreateDatabase(const std::string& name)
    {
        try
        {
            auto admin_str = conn_string_;
            auto pos = admin_str.find("dbname=");
            if (pos != std::string::npos)
            {
                auto end = admin_str.find(' ', pos);
                admin_str.replace(pos, end - pos, "dbname=postgres");
            }

            pqxx::connection admin_conn(admin_str);
            pqxx::nontransaction ntxn(admin_conn);
            auto r = ntxn.exec("SELECT 1 FROM pg_database WHERE datname = $1", pqxx::params{name});
            if (r.empty())
            {
                ntxn.exec("CREATE DATABASE " + ntxn.quote_name(name));
                return true;
            }
            return false;
        }
        catch (const std::exception& e)
        {
            DEBUGLOG(red, "CreateDatabase({}) failed: {}", name, e.what());
            return false;
        }
    }

    std::string CPostgresDatabase::GenerateQuestionMarks(size_t n)
    {
        if (n == 0) return {};
        std::string result;
        result.reserve(n * 4);
        for (size_t i = 0; i < n; ++i)
        {
            if (i > 0) result += ", ";
            result += '$';
            result += std::to_string(i + 1);
        }
        return result;
    }

    std::string CPostgresDatabase::GenerateQuestionMarks(size_t rows, size_t cols)
    {
        if (rows == 0 || cols == 0) return {};
        std::string result;
        size_t param = 1;
        for (size_t r = 0; r < rows; ++r)
        {
            if (r > 0) result += ", ";
            result += '(';
            for (size_t c = 0; c < cols; ++c)
            {
                if (c > 0) result += ", ";
                result += '$';
                result += std::to_string(param++);
            }
            result += ')';
        }
        return result;
    }

    std::string CPostgresDatabase::GenerateInTuples(size_t rows, size_t cols)
    {
        return GenerateQuestionMarks(rows, cols);
    }

    std::string CPostgresDatabase::GenerateJoinedString(const std::vector<std::string>& vec, const std::string& delim)
    {
        std::string result;
        for (size_t i = 0; i < vec.size(); ++i)
        {
            if (i > 0) result += delim;
            result += vec[i];
        }
        return result;
    }

    std::string CPostgresDatabase::GetDatabaseName()
    {
        return database_name_;
    }

    // ============================================================
    // Account methods — stub implementations that follow the same
    // query pattern as CMariaDatabase but use PostgreSQL syntax.
    // Each method will be fully implemented as the port progresses.
    // ============================================================

    bool CPostgresDatabase::GetFrontAccount(const std::string& ip, const std::string& username, const std::string& password, FrontAccount* outFrontAccount, ClanInfo* outClanInfo)
    {
        try
        {
            auto connected = EnsureConnected();
            if (!connected) return false;

            pqxx::work txn(GetConnection());
            auto r = txn.exec(
                "SELECT Id, ServerId, Username, Password, Salt, Grade, PCRoom, AuthKey, "
                "ClanId, ClanKills, ClanDeaths, ClanAssists, ClanContribution, ClanWins, ClanLoses, ClanDraws, "
                "Nickname, Level, Experience, Tutorial, Story, GuideMission, Achievement, VoiceType, "
                "VIPExperience, MaximumItems, MaximumEnergy, SelectedCharacter, PlayTime, MutedUntil, "
                "Coins, Energy, LuckyPoints, MicroPoints, RockTokens, Coupons, "
                "Wins, Loses, Draws, Kills, Deaths, Assists, Headshots, HighestKillStreak, "
                "MeleeKills, RifleKills, ShotgunKills, SniperKills, GatlingKills, BazookaKills, GrenadeKills, "
                "ZombieKills, Infections, IsAdmin, TwoFactorSecret, TwoFactorEnabled "
                "FROM accounts WHERE Username = $1",
                pqxx::params{username});

            if (r.empty()) return false;

            auto row = r[0];
            outFrontAccount->Index = row["Id"].as<int32_t>();
            outFrontAccount->ServerId = row["ServerId"].as<uint32_t>();
            outFrontAccount->Username = row["Username"].as<std::string>();
            outFrontAccount->Password = row["Password"].as<std::string>();
            outFrontAccount->Salt = row["Salt"].as<std::string>();
            outFrontAccount->Grade = static_cast<uint8_t>(row["Grade"].as<int>());
            outFrontAccount->PCRoom = static_cast<uint8_t>(row["PCRoom"].as<int>());
            outFrontAccount->AuthKey = row["AuthKey"].as<uint64_t>();
            outFrontAccount->Nickname = row["Nickname"].as<std::string>();
            outFrontAccount->Level = row["Level"].as<uint32_t>();
            outFrontAccount->Experience = row["Experience"].as<uint32_t>();
            outFrontAccount->Tutorial = row["Tutorial"].as<bool>();

            if (!row["ClanId"].is_null())
            {
                outFrontAccount->ClanId = row["ClanId"].as<uint32_t>();
                auto clan_r = txn.exec(
                    "SELECT Id, OwnerId, ClanName, ClanLogoFront, ClanLogoBack FROM clans WHERE Id = $1",
                    pqxx::params{outFrontAccount->ClanId});
                if (!clan_r.empty())
                {
                    auto cr = clan_r[0];
                    outClanInfo->id = cr["Id"].as<uint32_t>();
                    outClanInfo->owner_id = cr["OwnerId"].is_null() ? 0 : cr["OwnerId"].as<uint32_t>();
                    outClanInfo->name = cr["ClanName"].as<std::string>();
                    outClanInfo->logo_front = cr["ClanLogoFront"].as<uint32_t>();
                    outClanInfo->logo_back = cr["ClanLogoBack"].as<uint32_t>();
                }
            }

            txn.exec(
                "INSERT INTO login_history (AccountId, LoginDate, IP) VALUES ($1, CURRENT_TIMESTAMP, $2)",
                pqxx::params{outFrontAccount->Index, ip});

            txn.commit();
            return true;
        }
        catch (const std::exception& e)
        {
            DEBUGLOG(red, "GetFrontAccount failed: {}", e.what());
            return false;
        }
    }

    bool CPostgresDatabase::GetFrontAccount(const uint64_t authKey, FrontAccount* outFrontAccount, ClanInfo* outClanInfo)
    {
        try
        {
            auto connected = EnsureConnected();
            if (!connected) return false;

            pqxx::work txn(GetConnection());
            auto r = txn.exec(
                "SELECT a.Id, a.ServerId, a.Username, a.Password, a.Salt, a.Grade, a.PCRoom, a.AuthKey, "
                "a.Nickname, a.Level, a.Experience, a.Tutorial, a.ClanId "
                "FROM accounts a INNER JOIN player_sessions ps ON a.Id = ps.PlayerId "
                "WHERE ps.AuthKey = $1 AND ps.ExpiresAt > CURRENT_TIMESTAMP",
                pqxx::params{static_cast<int64_t>(authKey)});

            if (r.empty()) return false;

            auto row = r[0];
            outFrontAccount->Index = row["Id"].as<int32_t>();
            outFrontAccount->ServerId = row["ServerId"].as<uint32_t>();
            outFrontAccount->Username = row["Username"].as<std::string>();
            outFrontAccount->Password = row["Password"].as<std::string>();
            outFrontAccount->Salt = row["Salt"].as<std::string>();
            outFrontAccount->Grade = static_cast<uint8_t>(row["Grade"].as<int>());
            outFrontAccount->Nickname = row["Nickname"].as<std::string>();
            outFrontAccount->Level = row["Level"].as<uint32_t>();
            outFrontAccount->AuthKey = static_cast<uint64_t>(authKey);

            txn.commit();
            return true;
        }
        catch (const std::exception& e)
        {
            DEBUGLOG(red, "GetFrontAccount(authKey) failed: {}", e.what());
            return false;
        }
    }

    std::expected<int32_t, DbError> CPostgresDatabase::GetAccountIdByNickname(std::string_view nickname)
    {
        try
        {
            auto connected = EnsureConnected();
            if (!connected) return std::unexpected(connected.error());

            pqxx::work txn(GetConnection());
            auto r = txn.exec("SELECT Id FROM accounts WHERE Nickname = $1", pqxx::params{std::string(nickname)});
            txn.commit();

            if (r.empty())
            {
                DbError err;
                err.type = DbError::Type::NicknameNotFound;
                err.message = "nickname not found";
                return std::unexpected(err);
            }
            return r[0]["Id"].as<int32_t>();
        }
        catch (const std::exception& e)
        {
            return std::unexpected(FromPqxxException(e));
        }
    }

    std::expected<bool, DbError> CPostgresDatabase::AccountExists(int32_t aid)
    {
        try
        {
            auto connected = EnsureConnected();
            if (!connected) return std::unexpected(connected.error());

            pqxx::work txn(GetConnection());
            auto r = txn.exec("SELECT 1 FROM accounts WHERE Id = $1", pqxx::params{aid});
            txn.commit();
            return !r.empty();
        }
        catch (const std::exception& e)
        {
            return std::unexpected(FromPqxxException(e));
        }
    }

    std::expected<void, DbError> CPostgresDatabase::SetAccountMutedUntil(int32_t aid, uint64_t muted_until)
    {
        try
        {
            auto connected = EnsureConnected();
            if (!connected) return std::unexpected(connected.error());

            pqxx::work txn(GetConnection());
            txn.exec("UPDATE accounts SET MutedUntil = $1 WHERE Id = $2",
                pqxx::params{static_cast<int64_t>(muted_until), aid});
            txn.commit();
            return {};
        }
        catch (const std::exception& e)
        {
            return std::unexpected(FromPqxxException(e));
        }
    }

    std::expected<void, DbError> CPostgresDatabase::UpsertAccountBan(int32_t aid, uint64_t unban_unix, std::string_view reason)
    {
        try
        {
            auto connected = EnsureConnected();
            if (!connected) return std::unexpected(connected.error());

            pqxx::work txn(GetConnection());
            txn.exec(
                "INSERT INTO bans (AccountId, UnbanDate, Reason) VALUES ($1, to_timestamp($2), $3) "
                "ON CONFLICT (AccountId) DO UPDATE SET UnbanDate = EXCLUDED.UnbanDate, Reason = EXCLUDED.Reason",
                pqxx::params{aid, static_cast<int64_t>(unban_unix), std::string(reason)});
            txn.commit();
            return {};
        }
        catch (const std::exception& e)
        {
            return std::unexpected(FromPqxxException(e));
        }
    }

    std::expected<void, DbError> CPostgresDatabase::RemoveAccountBan(int32_t aid)
    {
        try
        {
            auto connected = EnsureConnected();
            if (!connected) return std::unexpected(connected.error());

            pqxx::work txn(GetConnection());
            txn.exec("DELETE FROM bans WHERE AccountId = $1", pqxx::params{aid});
            txn.commit();
            return {};
        }
        catch (const std::exception& e)
        {
            return std::unexpected(FromPqxxException(e));
        }
    }

    std::expected<std::optional<AccountPenaltyInfo>, DbError> CPostgresDatabase::GetActiveBan(int32_t aid)
    {
        try
        {
            auto connected = EnsureConnected();
            if (!connected) return std::unexpected(connected.error());

            pqxx::work txn(GetConnection());
            auto r = txn.exec(
                "SELECT AccountId, EXTRACT(EPOCH FROM UnbanDate)::bigint AS UnbanUnix, Reason "
                "FROM bans WHERE AccountId = $1 AND UnbanDate > CURRENT_TIMESTAMP",
                pqxx::params{aid});
            txn.commit();

            if (r.empty()) return std::nullopt;

            AccountPenaltyInfo info;
            info.account_id = r[0]["AccountId"].as<int32_t>();
            info.until_unix = r[0]["UnbanUnix"].as<uint64_t>();
            info.reason = r[0]["Reason"].is_null() ? "" : r[0]["Reason"].as<std::string>();
            return info;
        }
        catch (const std::exception& e)
        {
            return std::unexpected(FromPqxxException(e));
        }
    }

    std::expected<void, DbError> CPostgresDatabase::InsertAccount(const std::string& username, const std::string& password_hash, const std::string& salt, const std::string& nickname)
    {
        try
        {
            auto connected = EnsureConnected();
            if (!connected) return std::unexpected(connected.error());

            pqxx::work txn(GetConnection());
            txn.exec(
                "INSERT INTO accounts (Username, Password, Salt, Nickname) VALUES ($1, $2, $3, $4)",
                pqxx::params{username, password_hash, salt, nickname});
            txn.commit();
            return {};
        }
        catch (const std::exception& e)
        {
            return std::unexpected(FromPqxxException(e));
        }
    }

    std::expected<void, DbError> CPostgresDatabase::PersistCurrenciesPatches(ValidatedDbUpdates& v)
    {
        try
        {
            auto connected = EnsureConnected();
            if (!connected) return std::unexpected(connected.error());

            auto [txn, owned] = AcquireTxn();
            for (const auto& cu : v.currency_updates)
            {
                std::string col;
                using enum CurrencyType;
                switch (cu.type)
                {
                    case MP: col = "MicroPoints"; break;
                    case RT: col = "RockTokens"; break;
                    case COUPONS: col = "Coupons"; break;
                    case ENERGY: col = "Energy"; break;
                }
                if (cu.is_reward)
                    txn->exec("UPDATE accounts SET " + col + " = " + col + " + $1 WHERE Id = $2",
                        pqxx::params{static_cast<int64_t>(cu.value), v.aid});
                else
                    txn->exec("UPDATE accounts SET " + col + " = " + col + " - $1 WHERE Id = $2 AND " + col + " >= $1",
                        pqxx::params{static_cast<int64_t>(cu.value), v.aid});
            }
            CommitIfOwned(owned);
            return {};
        }
        catch (const std::exception& e)
        {
            return std::unexpected(FromPqxxException(e));
        }
    }

    bool CPostgresDatabase::GetMainFrontAccount(const uint64_t authKey, uint32_t server_id, FrontAccount* outFrontAccount, ClanInfo* outClanInfo, PlayerDailyMission* outDailyMission, std::vector<Item>& inv_items, std::vector<SocialInfo>& socials, std::vector<BlockedInfo>& blockeds, std::vector<FriendInfo>& friends, std::vector<MailboxInfo>& mailbox_list, std::vector<uint32_t>& daily_mission_random_ids, std::vector<GachaPityEntry>& gacha_pity, SystemMonthlyRewards* outMonthlyRewards, PlayerMonthlyReward* outPlayerMonthlyReward)
    {
        try
        {
            auto connected = EnsureConnected();
            if (!connected) return false;

            pqxx::work txn(GetConnection());

            auto r = txn.exec(R"(
                SELECT
                    a.Id, a.ServerId, a.Username, a.Password, a.Salt, a.Grade, a.PCRoom, a.ClanId,
                    a.ClanKills, a.ClanDeaths, a.ClanAssists, a.ClanContribution, a.ClanWins, a.ClanLoses, a.ClanDraws,
                    a.Nickname, a.Level, a.Experience, a.Tutorial, a.Story, a.GuideMission, a.Achievement, a.VoiceType, a.VIPExperience,
                    a.MaximumItems, a.MaximumEnergy, a.SelectedCharacter, a.PlayTime, a.MutedUntil, a.Coins, a.Energy, a.LuckyPoints,
                    a.MicroPoints, a.RockTokens, a.Coupons, a.Wins, a.Loses, a.Draws, a.Kills, a.Deaths, a.Assists, a.Headshots,
                    a.HighestKillStreak, a.MeleeKills, a.RifleKills, a.ShotgunKills, a.SniperKills, a.GatlingKills, a.BazookaKills,
                    a.GrenadeKills, a.ZombieKills, a.Infections, a.SingleWaveDailyAttempts, a.SingleWaveHighestWave, a.SingleWaveHighScore,
                    a.SingleWaveLastUpdate,
                    c.OwnerId, c.ClanName, c.ClanLogoFront, c.ClanLogoBack,
                    d.LastResetTime, d.MissionId1, d.MissionId2, d.MissionId3, d.Progress1, d.Progress2, d.Progress3
                FROM player_sessions s
                JOIN accounts a ON s.PlayerId = a.Id
                LEFT JOIN clans c ON a.ClanId = c.Id
                LEFT JOIN daily_missions d ON a.Id = d.AccountId
                WHERE s.AuthKey = $1
            )", pqxx::params{static_cast<int64_t>(authKey)});

            if (r.empty()) return false;

            auto row = r[0];
            auto accId = row["Id"].as<int32_t>();

            *outFrontAccount = FrontAccount(
                accId,
                row["ServerId"].as<uint32_t>(),
                row["Username"].as<std::string>(),
                row["Password"].as<std::string>(),
                row["Salt"].as<std::string>(),
                static_cast<uint8_t>(row["Grade"].as<int>()),
                static_cast<uint8_t>(row["PCRoom"].as<int>()),
                authKey,
                row["ClanId"].is_null() ? 0u : row["ClanId"].as<uint32_t>(),
                row["ClanKills"].as<uint32_t>(),
                row["ClanDeaths"].as<uint32_t>(),
                row["ClanAssists"].as<uint32_t>(),
                row["ClanContribution"].as<uint32_t>(),
                row["ClanWins"].as<uint32_t>(),
                row["ClanLoses"].as<uint32_t>(),
                row["ClanDraws"].as<uint32_t>(),
                row["Nickname"].as<std::string>(),
                row["Level"].as<uint32_t>(),
                row["Experience"].as<uint32_t>(),
                row["Tutorial"].as<bool>(),
                row["Story"].as<uint32_t>(),
                static_cast<uint8_t>(row["GuideMission"].as<int>()),
                row["Achievement"].as<uint64_t>(),
                row["VoiceType"].as<uint64_t>(),
                row["VIPExperience"].as<uint32_t>(),
                row["MaximumItems"].as<uint32_t>(),
                row["MaximumEnergy"].as<uint32_t>(),
                row["SelectedCharacter"].as<uint32_t>(),
                row["PlayTime"].as<uint64_t>(),
                row["MutedUntil"].as<uint64_t>(),
                row["Coins"].as<uint32_t>(),
                row["Energy"].as<uint32_t>(),
                row["LuckyPoints"].as<uint32_t>(),
                row["MicroPoints"].as<uint32_t>(),
                row["RockTokens"].as<uint32_t>(),
                row["Coupons"].as<uint32_t>(),
                row["Wins"].as<uint32_t>(),
                row["Loses"].as<uint32_t>(),
                row["Draws"].as<uint32_t>(),
                row["Kills"].as<uint32_t>(),
                row["Deaths"].as<uint32_t>(),
                row["Assists"].as<uint32_t>(),
                row["Headshots"].as<uint32_t>(),
                row["HighestKillStreak"].as<uint32_t>(),
                row["MeleeKills"].as<uint32_t>(),
                row["RifleKills"].as<uint32_t>(),
                row["ShotgunKills"].as<uint32_t>(),
                row["SniperKills"].as<uint32_t>(),
                row["GatlingKills"].as<uint32_t>(),
                row["BazookaKills"].as<uint32_t>(),
                row["GrenadeKills"].as<uint32_t>(),
                row["ZombieKills"].as<uint32_t>(),
                row["Infections"].as<uint32_t>(),
                row["SingleWaveDailyAttempts"].as<uint32_t>(),
                row["SingleWaveHighestWave"].as<uint32_t>(),
                row["SingleWaveHighScore"].as<uint32_t>(),
                row["SingleWaveLastUpdate"].as<uint64_t>()
            );

            if (outClanInfo && !row["ClanId"].is_null() && row["ClanId"].as<uint32_t>() > 0)
            {
                *outClanInfo = ClanInfo(
                    row["ClanId"].as<uint32_t>(),
                    row["OwnerId"].is_null() ? 0u : row["OwnerId"].as<uint32_t>(),
                    row["ClanName"].as<std::string>(),
                    row["ClanLogoFront"].as<uint32_t>(),
                    row["ClanLogoBack"].as<uint32_t>()
                );
            }

            if (outDailyMission)
            {
                uint64_t now_time = Utility::GetUtcTimeNow64();
                uint64_t last_6am = Utility::GetLast6AMUtc();
                bool existed = !row["LastResetTime"].is_null();
                uint64_t last_update = existed ? row["LastResetTime"].as<uint64_t>() : 0;

                PlayerDailyMission dm = {
                    accId,
                    last_update,
                    row["MissionId1"].is_null() ? 0u : row["MissionId1"].as<uint32_t>(),
                    row["MissionId2"].is_null() ? 0u : row["MissionId2"].as<uint32_t>(),
                    row["MissionId3"].is_null() ? 0u : row["MissionId3"].as<uint32_t>(),
                    row["Progress1"].is_null() ? 0u : row["Progress1"].as<uint32_t>(),
                    row["Progress2"].is_null() ? 0u : row["Progress2"].as<uint32_t>(),
                    row["Progress3"].is_null() ? 0u : row["Progress3"].as<uint32_t>()
                };

                if (last_update < last_6am)
                {
                    dm.mission1 = daily_mission_random_ids[0]; dm.goal_mission1 = 0;
                    dm.mission2 = daily_mission_random_ids[1]; dm.goal_mission2 = 0;
                    dm.mission3 = daily_mission_random_ids[2]; dm.goal_mission3 = 0;
                    dm.update_time = now_time;

                    if (existed)
                    {
                        txn.exec(
                            "UPDATE daily_missions SET LastResetTime = $1, MissionId1 = $2, MissionId2 = $3, MissionId3 = $4, "
                            "Progress1 = $5, Progress2 = $6, Progress3 = $7 WHERE AccountId = $8",
                            pqxx::params{static_cast<int64_t>(dm.update_time),
                            dm.mission1, dm.mission2, dm.mission3,
                            dm.goal_mission1, dm.goal_mission2, dm.goal_mission3,
                            accId});
                    }
                    else
                    {
                        txn.exec(
                            "INSERT INTO daily_missions (AccountId, LastResetTime, MissionId1, MissionId2, MissionId3, Progress1, Progress2, Progress3) "
                            "VALUES ($1, $2, $3, $4, $5, $6, $7, $8)",
                            pqxx::params{accId, static_cast<int64_t>(dm.update_time),
                            dm.mission1, dm.mission2, dm.mission3,
                            dm.goal_mission1, dm.goal_mission2, dm.goal_mission3});
                    }
                }
                *outDailyMission = dm;
            }

            auto invRes = txn.exec(
                "SELECT SerialData, ItemId, ItemType, ExpireDate, Repair, Energy, IsSealed, Stock, IsEquipped, CharacterId "
                "FROM inventory_items WHERE AccountId = $1", pqxx::params{accId});
            for (const auto& ir : invRes)
            {
                Item newItem;
                NetEngine::Packets::Main::InventoryItemInfo info;
                info.serial_info.data = ir["SerialData"].as<uint64_t>();
                info.item_number.item_id = ir["ItemId"].as<uint32_t>();
                info.expire_date = ir["ExpireDate"].as<uint32_t>();
                info.repair = ir["Repair"].as<uint32_t>();
                info.energy = ir["Energy"].as<uint32_t>();
                newItem.stock = ir["Stock"].as<uint32_t>();
                info.item_number.stock = newItem.stock;
                newItem.is_equipped = ir["IsEquipped"].as<bool>() ? 1 : 0;
                newItem.character_id = static_cast<uint8_t>(ir["CharacterId"].as<int>());
                newItem.in_database = 1;
                newItem.item_info = info;
                inv_items.push_back(newItem);
            }

            auto socialRes = txn.exec(
                "SELECT s.TargetAccountId, s.Type, a.Nickname AS TargetNickname "
                "FROM socials s JOIN accounts a ON a.Id = s.TargetAccountId "
                "WHERE s.AccountId = $1", pqxx::params{accId});
            for (const auto& sr : socialRes)
                socials.push_back({ accId, sr["TargetAccountId"].as<int32_t>(), static_cast<uint8_t>(sr["Type"].as<int>()), sr["TargetNickname"].as<std::string>() });

            auto mailRes = txn.exec(
                "SELECT Id, SenderAccountId, SenderNickname, AccountId, '' AS ReceiverNickname, SentDate, ItemId, Message, IsRead, FALSE AS DeletedFromSender, FALSE AS DeletedFromReceiver "
                "FROM mailbox WHERE AccountId = $1", pqxx::params{accId});
            for (const auto& mr : mailRes)
            {
                mailbox_list.push_back({
                    mr["Id"].as<uint32_t>(),
                    mr["SenderAccountId"].as<uint32_t>(),
                    mr["SenderNickname"].as<std::string>(),
                    mr["AccountId"].as<uint32_t>(),
                    mr["ReceiverNickname"].as<std::string>(),
                    mr["SentDate"].as<uint32_t>(),
                    mr["ItemId"].as<uint32_t>(),
                    mr["Message"].as<std::string>(),
                    !mr["IsRead"].as<bool>(),
                    mr["DeletedFromSender"].as<bool>(),
                    mr["DeletedFromReceiver"].as<bool>()
                });
            }

            auto pityRes = txn.exec("SELECT GachaId, LuckyPoints FROM gacha_pity WHERE AccountId = $1", pqxx::params{accId});
            for (const auto& pr : pityRes)
            {
                GachaPityEntry entry;
                entry.gacha_id = pr["GachaId"].as<uint32_t>();
                entry.lucky_points = pr["LuckyPoints"].as<uint32_t>();
                gacha_pity.push_back(entry);
            }

            if (outMonthlyRewards)
            {
                uint32_t curYear = Utility::GetCurrentYear();
                uint32_t curMonth = Utility::GetCurrentMonth();
                auto mRes = txn.exec("SELECT * FROM system_monthly_rewards WHERE Year = $1 AND Month = $2", pqxx::params{curYear, curMonth});
                if (!mRes.empty())
                {
                    auto mr = mRes[0];
                    std::array<uint32_t, 31> rewards{};
                    for (int i = 0; i < 31; ++i)
                        rewards[i] = mr["Day" + std::to_string(i + 1)].as<uint32_t>();
                    *outMonthlyRewards = SystemMonthlyRewards(curYear, curMonth, rewards);
                }
            }

            if (outPlayerMonthlyReward)
            {
                auto pmRes = txn.exec(
                    "SELECT DayCount, LastClaimDate FROM player_monthly_rewards WHERE AccountId = $1", pqxx::params{accId});
                if (!pmRes.empty())
                {
                    outPlayerMonthlyReward->player_account_id = accId;
                    outPlayerMonthlyReward->day_count = static_cast<uint8_t>(pmRes[0]["DayCount"].as<int>());
                    outPlayerMonthlyReward->last_time_update = pmRes[0]["LastClaimDate"].as<uint64_t>();
                }
            }

            txn.exec("UPDATE accounts SET ServerId = $1 WHERE Id = $2", pqxx::params{server_id, accId});
            txn.commit();
            return true;
        }
        catch (const std::exception& e)
        {
            DEBUGLOG(red, "GetMainFrontAccount failed: {}", e.what());
            return false;
        }
    }

    bool CPostgresDatabase::GetSystemMonthlyRewards(uint32_t year, uint32_t month, SystemMonthlyRewards* out)
    {
        try
        {
            auto connected = EnsureConnected();
            if (!connected) return false;

            pqxx::work txn(GetConnection());
            auto r = txn.exec("SELECT * FROM system_monthly_rewards WHERE Year = $1 AND Month = $2", pqxx::params{year, month});
            txn.commit();
            if (r.empty()) return false;

            std::array<uint32_t, 31> rewards{};
            for (int i = 0; i < 31; ++i)
                rewards[i] = r[0]["Day" + std::to_string(i + 1)].as<uint32_t>();
            *out = SystemMonthlyRewards(year, month, rewards);
            return true;
        }
        catch (const std::exception& e)
        {
            DEBUGLOG(red, "GetSystemMonthlyRewards failed: {}", e.what());
            return false;
        }
    }

    bool CPostgresDatabase::GetPlayerMonthlyReward(uint32_t player_id, PlayerMonthlyReward* out)
    {
        try
        {
            auto connected = EnsureConnected();
            if (!connected) return false;

            pqxx::work txn(GetConnection());
            auto r = txn.exec(
                "SELECT DayCount, LastClaimDate FROM player_monthly_rewards WHERE AccountId = $1", pqxx::params{player_id});
            txn.commit();
            if (r.empty()) return false;

            out->player_account_id = player_id;
            out->day_count = static_cast<uint8_t>(r[0]["DayCount"].as<int>());
            out->last_time_update = r[0]["LastClaimDate"].as<uint64_t>();
            return true;
        }
        catch (const std::exception& e)
        {
            DEBUGLOG(red, "GetPlayerMonthlyReward failed: {}", e.what());
            return false;
        }
    }

    bool CPostgresDatabase::ClaimMonthlyReward(uint32_t player_id, uint8_t new_day_count, uint64_t now, const Item& reward_item)
    {
        try
        {
            auto connected = EnsureConnected();
            if (!connected) return false;

            pqxx::work txn(GetConnection());
            txn.exec(
                "INSERT INTO player_monthly_rewards (AccountId, DayCount, LastClaimDate) VALUES ($1, $2, $3) "
                "ON CONFLICT (AccountId) DO UPDATE SET DayCount = EXCLUDED.DayCount, LastClaimDate = EXCLUDED.LastClaimDate",
                pqxx::params{player_id, static_cast<int>(new_day_count), static_cast<int64_t>(now)});

            if (reward_item.item_info.item_number.item_id > 0)
            {
                txn.exec(
                    "INSERT INTO inventory_items (AccountId, SerialData, ItemId, ItemType, IsSealed, IsEquipped, Stock, ExpireDate, Repair, Energy, CharacterId) "
                    "VALUES ($1, $2, $3, $4, $5, $6, $7, $8, $9, $10, $11)",
                    pqxx::params{player_id,
                    static_cast<int64_t>(reward_item.item_info.serial_info.data),
                    reward_item.item_info.item_number.item_id,
                    static_cast<int>(reward_item.item_type),
                    false,
                    static_cast<bool>(reward_item.is_equipped),
                    reward_item.stock,
                    reward_item.item_info.expire_date,
                    reward_item.item_info.repair,
                    reward_item.item_info.energy,
                    static_cast<int>(reward_item.character_id)});
            }
            txn.commit();
            return true;
        }
        catch (const std::exception& e)
        {
            DEBUGLOG(red, "ClaimMonthlyReward failed: {}", e.what());
            return false;
        }
    }

    bool CPostgresDatabase::GetPlazaAuthKey(const std::string& ip, const std::string& username, const std::string& password, PlazaAuth* outPlazaAuth)
    {
        try
        {
            auto connected = EnsureConnected();
            if (!connected) return false;

            constexpr uint32_t kSessionTTLSeconds = 5u * 60u;
            const uint64_t nowUnix = Utility::GetUtcTimeNow64();
            const uint64_t newExp = nowUnix + kSessionTTLSeconds;

            pqxx::work txn(GetConnection());

            auto r = txn.exec(
                "SELECT Id, ServerId, Password, Salt, Grade, IsEmailVerified, TwoFactorSecret, TwoFactorEnabled "
                "FROM accounts WHERE Username = $1", pqxx::params{username});
            if (r.empty()) return false;

            auto row = r[0];
            auto password_str = row["Password"].as<std::string>();
            auto salt_str = row["Salt"].as<std::string>();
            std::string stored_hash_str = Utility::Base64::from_base64(password_str);
            std::string stored_salt_str = Utility::Base64::from_base64(salt_str);
            if (stored_hash_str.size() != 32 || stored_salt_str.size() != 16) return false;

            uint8_t stored_hash[32], stored_salt[16], computed_hash[32];
            std::memcpy(stored_hash, stored_hash_str.data(), 32);
            std::memcpy(stored_salt, stored_salt_str.data(), 16);
            if (!Utility::HashPassword(password, stored_salt, computed_hash)) return false;
            if (crypto_verify32(computed_hash, stored_hash) != 0) return false;

            outPlazaAuth->Index = row["Id"].as<int32_t>();
            outPlazaAuth->ServerId = row["ServerId"].as<uint32_t>();
            outPlazaAuth->Grade = static_cast<uint8_t>(row["Grade"].as<int>());
            outPlazaAuth->emailVerified = row["IsEmailVerified"].as<bool>();
            outPlazaAuth->has2fa = row["TwoFactorEnabled"].as<bool>();
            outPlazaAuth->secret2fa = row["TwoFactorSecret"].is_null() ? "" : row["TwoFactorSecret"].as<std::string>();

            txn.exec("DELETE FROM player_sessions WHERE ExpiresAt <= to_timestamp($1)", pqxx::params{static_cast<int64_t>(nowUnix)});

            auto sessR = txn.exec(
                "SELECT AuthKey, EXTRACT(EPOCH FROM ExpiresAt)::bigint AS ExpiresAt "
                "FROM player_sessions WHERE PlayerId = $1", pqxx::params{outPlazaAuth->Index});
            if (!sessR.empty())
            {
                auto curKey = sessR[0]["AuthKey"].as<uint64_t>();
                auto curExpiry = sessR[0]["ExpiresAt"].as<uint64_t>();
                if (curExpiry > nowUnix)
                {
                    txn.exec("UPDATE player_sessions SET LastSeenAt = to_timestamp($1) WHERE PlayerId = $2",
                        pqxx::params{static_cast<int64_t>(nowUnix), outPlazaAuth->Index});
                    outPlazaAuth->AuthKey = curKey;
                    txn.exec("INSERT INTO login_history (AccountId, LoginDate, IP) VALUES ($1, to_timestamp($2), $3)",
                        pqxx::params{outPlazaAuth->Index, static_cast<int64_t>(nowUnix), ip});
                    txn.commit();
                    return true;
                }
            }

            thread_local Utility::SecureRandomBlake2b::Generator rng;
            uint64_t newAuthKey = rng.GenerateAuthKey();
            txn.exec(
                "INSERT INTO player_sessions (PlayerId, AuthKey, IssuedAt, ExpiresAt, LastSeenAt) "
                "VALUES ($1, $2, to_timestamp($3), to_timestamp($4), to_timestamp($5)) "
                "ON CONFLICT (PlayerId) DO UPDATE SET AuthKey = EXCLUDED.AuthKey, IssuedAt = EXCLUDED.IssuedAt, ExpiresAt = EXCLUDED.ExpiresAt, LastSeenAt = EXCLUDED.LastSeenAt",
                pqxx::params{outPlazaAuth->Index, static_cast<int64_t>(newAuthKey),
                static_cast<int64_t>(nowUnix), static_cast<int64_t>(newExp), static_cast<int64_t>(nowUnix)});
            outPlazaAuth->AuthKey = newAuthKey;

            txn.exec("INSERT INTO login_history (AccountId, LoginDate, IP) VALUES ($1, to_timestamp($2), $3)",
                pqxx::params{outPlazaAuth->Index, static_cast<int64_t>(nowUnix), ip});
            txn.commit();
            return true;
        }
        catch (const std::exception& e)
        {
            DEBUGLOG(red, "GetPlazaAuthKey failed: {}", e.what());
            return false;
        }
    }

    bool CPostgresDatabase::GetPlazaAuthKey(const std::string& ip, const uint64_t authKey, PlazaAuth* outPlazaAuth)
    {
        try
        {
            auto connected = EnsureConnected();
            if (!connected) return false;

            const uint64_t nowUnix = Utility::GetUtcTimeNow64();

            pqxx::work txn(GetConnection());
            txn.exec("DELETE FROM player_sessions WHERE ExpiresAt <= to_timestamp($1)", pqxx::params{static_cast<int64_t>(nowUnix)});

            auto r = txn.exec(
                "SELECT a.Id, a.ServerId, a.Grade, a.IsEmailVerified, a.TwoFactorEnabled, a.TwoFactorSecret "
                "FROM player_sessions s JOIN accounts a ON s.PlayerId = a.Id "
                "WHERE s.AuthKey = $1", pqxx::params{static_cast<int64_t>(authKey)});
            if (r.empty()) return false;

            auto row = r[0];
            outPlazaAuth->Index = row["Id"].as<int32_t>();
            outPlazaAuth->ServerId = row["ServerId"].as<uint32_t>();
            outPlazaAuth->Grade = static_cast<uint8_t>(row["Grade"].as<int>());
            outPlazaAuth->emailVerified = row["IsEmailVerified"].as<bool>();
            outPlazaAuth->has2fa = row["TwoFactorEnabled"].as<bool>();
            outPlazaAuth->secret2fa = row["TwoFactorSecret"].is_null() ? "" : row["TwoFactorSecret"].as<std::string>();
            outPlazaAuth->AuthKey = authKey;

            txn.exec("UPDATE player_sessions SET LastSeenAt = to_timestamp($1) WHERE AuthKey = $2",
                pqxx::params{static_cast<int64_t>(nowUnix), static_cast<int64_t>(authKey)});
            txn.exec("INSERT INTO login_history (AccountId, LoginDate, IP) VALUES ($1, to_timestamp($2), $3)",
                pqxx::params{outPlazaAuth->Index, static_cast<int64_t>(nowUnix), ip});
            txn.commit();
            return true;
        }
        catch (const std::exception& e)
        {
            DEBUGLOG(red, "GetPlazaAuthKey(authKey) failed: {}", e.what());
            return false;
        }
    }

    std::expected<void, DbError> CPostgresDatabase::PersistAccountInfoPatches(ValidatedDbUpdates& v)
    {
        try
        {
            if (v.acc_info_patches.empty()) return {};

            std::optional<uint32_t> server_id, sw_daily_attempts, sw_high_score, sw_highest_wave;
            std::optional<uint64_t> sw_last_update;
            std::optional<uint32_t> clan_kills, clan_deaths, clan_assists, clan_contribution, clan_wins, clan_loses, clan_draws;
            std::optional<uint32_t> selected_character;
            std::optional<uint64_t> play_time;
            std::optional<uint8_t> story;
            std::optional<uint64_t> achievement, voice;
            std::optional<bool> tutorial;
            std::optional<uint8_t> guide;
            std::optional<uint32_t> exp, max_items, max_energy, lvl, lucky;
            std::optional<uint32_t> wins, loses, draws, kills, deaths, assists, headshots, highest_kill_streak;
            std::optional<uint32_t> melee_kills, rifle_kills, shotgun_kills, sniper_kills, gatling_kills, bazooka_kills, grenade_kills;
            std::optional<uint32_t> zombie_kills, infections;
            std::optional<std::string> nickname;

            for (const auto& p : v.acc_info_patches)
            {
                if (p.server_id) server_id = *p.server_id;
                if (p.sw_daily_attempts) sw_daily_attempts = *p.sw_daily_attempts;
                if (p.sw_high_score) sw_high_score = *p.sw_high_score;
                if (p.sw_highest_wave) sw_highest_wave = *p.sw_highest_wave;
                if (p.sw_last_update) sw_last_update = *p.sw_last_update;
                if (p.clan_kills) clan_kills = *p.clan_kills;
                if (p.clan_deaths) clan_deaths = *p.clan_deaths;
                if (p.clan_assists) clan_assists = *p.clan_assists;
                if (p.clan_contribution) clan_contribution = *p.clan_contribution;
                if (p.clan_wins) clan_wins = *p.clan_wins;
                if (p.clan_loses) clan_loses = *p.clan_loses;
                if (p.clan_draws) clan_draws = *p.clan_draws;
                if (p.selected_character) selected_character = *p.selected_character;
                if (p.play_time) play_time = *p.play_time;
                if (p.story) story = *p.story;
                if (p.achievement_tier1) achievement = *p.achievement_tier1;
                if (p.voice_type) voice = *p.voice_type;
                if (p.bTutorial) tutorial = *p.bTutorial;
                if (p.guide_mission) guide = *p.guide_mission;
                if (p.experience) exp = *p.experience;
                if (p.maximum_items) max_items = *p.maximum_items;
                if (p.maximum_energy) max_energy = *p.maximum_energy;
                if (p.level) lvl = *p.level;
                if (p.lucky_points) lucky = *p.lucky_points;
                if (p.wins) wins = *p.wins;
                if (p.loses) loses = *p.loses;
                if (p.draws) draws = *p.draws;
                if (p.kills) kills = *p.kills;
                if (p.deaths) deaths = *p.deaths;
                if (p.assists) assists = *p.assists;
                if (p.headshots) headshots = *p.headshots;
                if (p.highest_kill_streak) highest_kill_streak = *p.highest_kill_streak;
                if (p.melee_kills) melee_kills = *p.melee_kills;
                if (p.rifle_kills) rifle_kills = *p.rifle_kills;
                if (p.shotgun_kills) shotgun_kills = *p.shotgun_kills;
                if (p.sniper_kills) sniper_kills = *p.sniper_kills;
                if (p.gatling_kills) gatling_kills = *p.gatling_kills;
                if (p.bazooka_kills) bazooka_kills = *p.bazooka_kills;
                if (p.grenade_kills) grenade_kills = *p.grenade_kills;
                if (p.zombie_kills) zombie_kills = *p.zombie_kills;
                if (p.infections) infections = *p.infections;
                if (p.nickname) nickname = *p.nickname;
            }

            std::string sql = "UPDATE accounts SET ";
            std::vector<std::string> params_str;
            size_t param_idx = 1;
            bool first = true;
            auto add_set = [&](const char* col) {
                if (!first) sql += ", ";
                sql += col;
                sql += " = $" + std::to_string(param_idx++);
                first = false;
            };

            pqxx::params params;
            if (server_id)           { add_set("ServerId"); params.append(*server_id); }
            if (sw_daily_attempts)   { add_set("SingleWaveDailyAttempts"); params.append(*sw_daily_attempts); }
            if (sw_high_score)       { add_set("SingleWaveHighScore"); params.append(*sw_high_score); }
            if (sw_highest_wave)     { add_set("SingleWaveHighestWave"); params.append(*sw_highest_wave); }
            if (sw_last_update)      { add_set("SingleWaveLastUpdate"); params.append(static_cast<int64_t>(*sw_last_update)); }
            if (clan_kills)          { add_set("ClanKills"); params.append(*clan_kills); }
            if (clan_deaths)         { add_set("ClanDeaths"); params.append(*clan_deaths); }
            if (clan_assists)        { add_set("ClanAssists"); params.append(*clan_assists); }
            if (clan_contribution)   { add_set("ClanContribution"); params.append(*clan_contribution); }
            if (clan_wins)           { add_set("ClanWins"); params.append(*clan_wins); }
            if (clan_loses)          { add_set("ClanLoses"); params.append(*clan_loses); }
            if (clan_draws)          { add_set("ClanDraws"); params.append(*clan_draws); }
            if (selected_character)  { add_set("SelectedCharacter"); params.append(*selected_character); }
            if (play_time)           { add_set("PlayTime"); params.append(static_cast<int64_t>(*play_time)); }
            if (story)               { add_set("Story"); params.append(static_cast<int>(*story)); }
            if (achievement)         { add_set("Achievement"); params.append(static_cast<int64_t>(*achievement)); }
            if (voice)               { add_set("VoiceType"); params.append(static_cast<int64_t>(*voice)); }
            if (tutorial)            { add_set("Tutorial"); params.append(*tutorial); }
            if (guide)               { add_set("GuideMission"); params.append(static_cast<int>(*guide)); }
            if (exp)                 { add_set("Experience"); params.append(*exp); }
            if (max_items)           { add_set("MaximumItems"); params.append(*max_items); }
            if (max_energy)          { add_set("MaximumEnergy"); params.append(*max_energy); }
            if (lvl)                 { add_set("Level"); params.append(*lvl); }
            if (lucky)               { add_set("LuckyPoints"); params.append(*lucky); }
            if (wins)                { add_set("Wins"); params.append(*wins); }
            if (loses)               { add_set("Loses"); params.append(*loses); }
            if (draws)               { add_set("Draws"); params.append(*draws); }
            if (kills)               { add_set("Kills"); params.append(*kills); }
            if (deaths)              { add_set("Deaths"); params.append(*deaths); }
            if (assists)             { add_set("Assists"); params.append(*assists); }
            if (headshots)           { add_set("Headshots"); params.append(*headshots); }
            if (highest_kill_streak) { add_set("HighestKillStreak"); params.append(*highest_kill_streak); }
            if (melee_kills)         { add_set("MeleeKills"); params.append(*melee_kills); }
            if (rifle_kills)         { add_set("RifleKills"); params.append(*rifle_kills); }
            if (shotgun_kills)       { add_set("ShotgunKills"); params.append(*shotgun_kills); }
            if (sniper_kills)        { add_set("SniperKills"); params.append(*sniper_kills); }
            if (gatling_kills)       { add_set("GatlingKills"); params.append(*gatling_kills); }
            if (bazooka_kills)       { add_set("BazookaKills"); params.append(*bazooka_kills); }
            if (grenade_kills)       { add_set("GrenadeKills"); params.append(*grenade_kills); }
            if (zombie_kills)        { add_set("ZombieKills"); params.append(*zombie_kills); }
            if (infections)          { add_set("Infections"); params.append(*infections); }
            if (nickname)            { add_set("Nickname"); params.append(*nickname); }

            if (first) return {};

            sql += " WHERE Id = $" + std::to_string(param_idx);
            params.append(v.aid);

            auto [txn, owned] = AcquireTxn();
            txn->exec(sql, params);
            CommitIfOwned(owned);
            DEBUGLOG(green, "PersistAccountInfoPatches updated account {}", v.aid);
            return {};
        }
        catch (const std::exception& e)
        {
            DEBUGLOG(red, "PersistAccountInfoPatches failed: {}", e.what());
            return std::unexpected(FromPqxxException(e));
        }
    }

    std::expected<void, DbError> CPostgresDatabase::PersistItemDeletes(ValidatedDbUpdates& v, ResultDbUpdateInfo& out)
    {
        try
        {
            if (v.items_deleted.empty()) return {};

            std::string sql = "DELETE FROM inventory_items WHERE AccountId = $1 AND SerialData IN (";
            pqxx::params params;
            params.append(v.aid);
            for (size_t i = 0; i < v.items_deleted.size(); ++i)
            {
                if (i > 0) sql += ", ";
                sql += "$" + std::to_string(i + 2);
                params.append(static_cast<int64_t>(v.items_deleted[i].data));
            }
            sql += ")";

            auto [txn, owned] = AcquireTxn();
            auto result = txn->exec(sql, params);
            CommitIfOwned(owned);

            auto deleted = result.affected_rows();
            if (!deleted)
                return std::unexpected(DbError{ DbError::Type::NoRowsAffected, 0, {}, fmt::format("PersistItemDeletes: expected {} deletes, got 0", v.items_deleted.size()) });

            out.deleted_rows_count += static_cast<int32_t>(deleted);
            for (const auto& s : v.items_deleted) out.deleted_serials.push_back(s.data);
            DEBUGLOG(green, "Deleted {} items for account {}", deleted, v.aid);
            return {};
        }
        catch (const std::exception& e)
        {
            DEBUGLOG(red, "PersistItemDeletes failed: {}", e.what());
            return std::unexpected(FromPqxxException(e));
        }
    }

    std::expected<void, DbError> CPostgresDatabase::PersistItemPatches(ValidatedDbUpdates& v, ResultDbUpdateInfo& out)
    {
        try
        {
            if (v.items_patches.empty()) return {};

            boost::unordered_flat_map<uint64_t, ItemPatchCtx> merged;
            for (const auto& rp : v.items_patches)
            {
                auto& existing = merged[rp.serial];
                if (rp.patch.new_item_id) existing.new_item_id = rp.patch.new_item_id;
                if (rp.patch.expire_date) existing.expire_date = rp.patch.expire_date;
                if (rp.patch.repair) existing.repair = rp.patch.repair;
                if (rp.patch.energy) existing.energy = rp.patch.energy;
                if (rp.patch.is_equipped) existing.is_equipped = rp.patch.is_equipped;
                if (rp.patch.character_id) existing.character_id = rp.patch.character_id;
            }
            if (merged.empty()) return {};

            {
                std::string checkSql = "SELECT SerialData FROM inventory_items WHERE AccountId = $1 AND SerialData IN (";
                pqxx::params checkParams;
                checkParams.append(v.aid);
                size_t idx = 2;
                for (const auto& [serial, _] : merged)
                {
                    if (idx > 2) checkSql += ", ";
                    checkSql += "$" + std::to_string(idx++);
                    checkParams.append(static_cast<int64_t>(serial));
                }
                checkSql += ")";
                auto [checkTxn, checkOwned] = AcquireTxn();
                auto checkR = checkTxn->exec(checkSql, checkParams);
                CommitIfOwned(checkOwned);
                DEBUGLOG(yellow, "PersistItemPatches: Found {} of {} items in DB for account {}", checkR.size(), merged.size(), v.aid);
            }

            std::string c_itemId, c_exp, c_rep, c_eng, c_equip, c_char;
            std::vector<uint64_t> serials;
            serials.reserve(merged.size());

            for (const auto& [serial, patch] : merged)
            {
                std::string s = std::to_string(serial);
                serials.push_back(serial);
                if (patch.new_item_id) c_itemId += "WHEN " + s + " THEN " + std::to_string(*patch.new_item_id) + " ";
                if (patch.expire_date) c_exp += "WHEN " + s + " THEN " + std::to_string(*patch.expire_date) + " ";
                if (patch.repair) c_rep += "WHEN " + s + " THEN " + std::to_string(*patch.repair) + " ";
                if (patch.energy) c_eng += "WHEN " + s + " THEN " + std::to_string(*patch.energy) + " ";
                if (patch.is_equipped) c_equip += "WHEN " + s + " THEN " + std::to_string(*patch.is_equipped ? 1 : 0) + " ";
                if (patch.character_id) c_char += "WHEN " + s + " THEN " + std::to_string(*patch.character_id) + " ";
            }

            if (serials.empty()) return {};

            std::vector<std::string> sets;
            if (!c_itemId.empty()) sets.push_back("ItemId = CASE SerialData " + c_itemId + "ELSE ItemId END");
            if (!c_exp.empty()) sets.push_back("ExpireDate = CASE SerialData " + c_exp + "ELSE ExpireDate END");
            if (!c_rep.empty()) sets.push_back("Repair = CASE SerialData " + c_rep + "ELSE Repair END");
            if (!c_eng.empty()) sets.push_back("Energy = CASE SerialData " + c_eng + "ELSE Energy END");
            if (!c_equip.empty()) sets.push_back("IsEquipped = CASE SerialData " + c_equip + "ELSE IsEquipped END");
            if (!c_char.empty()) sets.push_back("CharacterId = CASE SerialData " + c_char + "ELSE CharacterId END");

            if (sets.empty()) return {};

            std::string psql = "UPDATE inventory_items SET " + GenerateJoinedString(sets, ", ") + " WHERE AccountId = $1 AND SerialData IN (";
            pqxx::params params;
            params.append(v.aid);
            for (size_t i = 0; i < serials.size(); ++i)
            {
                if (i > 0) psql += ", ";
                psql += "$" + std::to_string(i + 2);
                params.append(static_cast<int64_t>(serials[i]));
            }
            psql += ")";

            auto [txn, owned] = AcquireTxn();
            auto result = txn->exec(psql, params);
            CommitIfOwned(owned);

            auto patched = result.affected_rows();
            if (!patched)
                return std::unexpected(DbError{ DbError::Type::NoRowsAffected, 0, {}, fmt::format("PersistItemPatches: expected {} patches, got 0", serials.size()) });

            out.patched_rows_count += static_cast<int32_t>(patched);
            out.patched_serials.insert(out.patched_serials.end(), serials.begin(), serials.end());
            DEBUGLOG(green, "Updated {} items for account {}", patched, v.aid);
            return {};
        }
        catch (const std::exception& e)
        {
            DEBUGLOG(red, "PersistItemPatches failed: {}", e.what());
            return std::unexpected(FromPqxxException(e));
        }
    }

    std::expected<void, DbError> CPostgresDatabase::PersistItemAdds(ValidatedDbUpdates& v, ResultDbUpdateInfo& out)
    {
        try
        {
            if (v.items_added.empty()) return {};

            std::string sql =
                "INSERT INTO inventory_items (AccountId, SerialData, ItemId, ItemType, ExpireDate, Repair, Energy, IsSealed, Stock, IsEquipped, CharacterId) VALUES ";
            pqxx::params params;
            for (size_t i = 0; i < v.items_added.size(); ++i)
            {
                if (i > 0) sql += ", ";
                size_t base = i * 11 + 1;
                sql += "(";
                for (size_t j = 0; j < 11; ++j)
                {
                    if (j > 0) sql += ", ";
                    sql += "$" + std::to_string(base + j);
                }
                sql += ")";

                const auto& item = v.items_added[i];
                params.append(v.aid);
                params.append(static_cast<int64_t>(item.item_info.serial_info.data));
                params.append(item.item_info.item_number.item_id);
                params.append(static_cast<int>(item.item_type));
                params.append(item.item_info.expire_date);
                params.append(item.item_info.repair);
                params.append(item.item_info.energy);
                params.append(false);
                params.append(item.stock);
                params.append(static_cast<bool>(item.is_equipped));
                params.append(static_cast<int>(item.character_id));
            }

            auto [txn, owned] = AcquireTxn();
            auto result = txn->exec(sql, params);
            CommitIfOwned(owned);

            auto inserted = result.affected_rows();
            if (!inserted)
                return std::unexpected(DbError{ DbError::Type::NoRowsAffected, 0, {}, fmt::format("PersistItemAdds: expected {} adds, got 0", v.items_added.size()) });

            out.added_rows_count += static_cast<int32_t>(inserted);
            for (const auto& it : v.items_added) out.added_serials.push_back(it.item_info.serial_info.data);
            DEBUGLOG(green, "Added {} items for account {}", inserted, v.aid);
            return {};
        }
        catch (const std::exception& e)
        {
            DEBUGLOG(red, "PersistItemAdds failed: {}", e.what());
            return std::unexpected(FromPqxxException(e));
        }
    }

    std::expected<void, DbError> CPostgresDatabase::PersistMissionsPatches(ValidatedDbUpdates& v)
    {
        try
        {
            if (v.player_missions_patches.empty()) return {};

            PlayerMissionsPatch m;
            for (const auto& p : v.player_missions_patches)
            {
                if (p.update_time) m.update_time = *p.update_time;
                if (p.mission1) m.mission1 = *p.mission1;
                if (p.mission2) m.mission2 = *p.mission2;
                if (p.mission3) m.mission3 = *p.mission3;
                if (p.goal1) m.goal1 = *p.goal1;
                if (p.goal2) m.goal2 = *p.goal2;
                if (p.goal3) m.goal3 = *p.goal3;
            }
            if (!m.update_time && !m.mission1 && !m.mission2 && !m.mission3 &&
                !m.goal1 && !m.goal2 && !m.goal3) return {};

            std::string cols = "AccountId";
            std::string vals = "$1";
            std::string updates;
            pqxx::params params;
            params.append(v.aid);
            size_t idx = 2;
            bool first_update = true;

            auto add_col = [&](const char* col, auto val) {
                cols += std::string(", ") + col;
                vals += ", $" + std::to_string(idx++);
                if (!first_update) updates += ", ";
                updates += std::string(col) + " = EXCLUDED." + col;
                first_update = false;
                params.append(val);
            };

            if (m.update_time) add_col("LastResetTime", static_cast<int64_t>(*m.update_time));
            if (m.mission1)    add_col("MissionId1", *m.mission1);
            if (m.mission2)    add_col("MissionId2", *m.mission2);
            if (m.mission3)    add_col("MissionId3", *m.mission3);
            if (m.goal1)       add_col("Progress1", *m.goal1);
            if (m.goal2)       add_col("Progress2", *m.goal2);
            if (m.goal3)       add_col("Progress3", *m.goal3);

            std::string sql = "INSERT INTO daily_missions (" + cols + ") VALUES (" + vals + ") "
                "ON CONFLICT (AccountId) DO UPDATE SET " + (updates.empty() ? "AccountId = EXCLUDED.AccountId" : updates);

            auto [txn, owned] = AcquireTxn();
            txn->exec(sql, params);
            CommitIfOwned(owned);
            DEBUGLOG(green, "PersistMissionPatches upserted daily mission for account {}", v.aid);
            return {};
        }
        catch (const std::exception& e)
        {
            DEBUGLOG(red, "PersistMissionPatches failed: {}", e.what());
            return std::unexpected(FromPqxxException(e));
        }
    }

    std::expected<void, DbError> CPostgresDatabase::PersistMonthlyRewardsPatches(ValidatedDbUpdates& v)
    {
        try
        {
            if (v.player_monthly_reward_patches.empty()) return {};

            PlayerMonthlyRewardPatch m;
            for (const auto& p : v.player_monthly_reward_patches)
            {
                if (p.day_count) m.day_count = *p.day_count;
                if (p.last_time_update) m.last_time_update = *p.last_time_update;
            }
            if (!m.day_count && !m.last_time_update) return {};

            std::string cols = "AccountId";
            std::string vals = "$1";
            std::string updates;
            pqxx::params params;
            params.append(v.aid);
            size_t idx = 2;
            bool first_update = true;

            auto add_col = [&](const char* col, auto val) {
                cols += std::string(", ") + col;
                vals += ", $" + std::to_string(idx++);
                if (!first_update) updates += ", ";
                updates += std::string(col) + " = EXCLUDED." + col;
                first_update = false;
                params.append(val);
            };

            if (m.day_count)         add_col("DayCount", static_cast<int>(*m.day_count));
            if (m.last_time_update)  add_col("LastClaimDate", static_cast<int64_t>(*m.last_time_update));

            std::string sql = "INSERT INTO player_monthly_rewards (" + cols + ") VALUES (" + vals + ") "
                "ON CONFLICT (AccountId) DO UPDATE SET " + (updates.empty() ? "AccountId = EXCLUDED.AccountId" : updates);

            auto [txn, owned] = AcquireTxn();
            txn->exec(sql, params);
            CommitIfOwned(owned);
            DEBUGLOG(green, "PersistMonthlyRewardPatches upserted monthly reward for account {}", v.aid);
            return {};
        }
        catch (const std::exception& e)
        {
            DEBUGLOG(red, "PersistMonthlyRewardPatches failed: {}", e.what());
            return std::unexpected(FromPqxxException(e));
        }
    }

    std::expected<void, DbError> CPostgresDatabase::PersistMailboxPatches(ValidatedDbUpdates& v)
    {
        try
        {
            if (v.mailbox_patches.empty()) return {};

            std::vector<uint32_t> mark_read_ids;
            std::vector<uint32_t> del_ids;
            std::vector<size_t> insert_idx;
            for (size_t i = 0; i < v.mailbox_patches.size(); ++i)
            {
                const auto& p = v.mailbox_patches[i];
                switch (p.op)
                {
                    case MailboxPatch::Op::MarkRead:
                        if (p.mail_id && p.read && *p.read) mark_read_ids.push_back(p.mail_id);
                        break;
                    case MailboxPatch::Op::Delete:
                        if (p.mail_id) del_ids.push_back(p.mail_id);
                        break;
                    case MailboxPatch::Op::Insert:
                        if (p.insert && p.insert->receiver_nickname && p.insert->sender_nickname)
                            insert_idx.push_back(i);
                        break;
                }
            }
            if (mark_read_ids.empty() && del_ids.empty() && insert_idx.empty()) return {};

            auto [txn, owned] = AcquireTxn();

            if (!mark_read_ids.empty())
            {
                std::string sql = "UPDATE mailbox SET IsRead = TRUE WHERE Id IN (";
                pqxx::params params;
                for (size_t i = 0; i < mark_read_ids.size(); ++i)
                {
                    if (i > 0) sql += ", ";
                    sql += "$" + std::to_string(i + 1);
                    params.append(mark_read_ids[i]);
                }
                sql += ")";
                txn->exec(sql, params);
            }

            if (!del_ids.empty())
            {
                std::string sql = "DELETE FROM mailbox WHERE Id IN (";
                pqxx::params params;
                for (size_t i = 0; i < del_ids.size(); ++i)
                {
                    if (i > 0) sql += ", ";
                    sql += "$" + std::to_string(i + 1);
                    params.append(del_ids[i]);
                }
                sql += ")";
                txn->exec(sql, params);
            }

            if (!insert_idx.empty())
            {
                static constexpr uint32_t kMailboxLimit = 100;
                for (const size_t j : insert_idx)
                {
                    auto& patch = v.mailbox_patches[j];
                    auto& in = *patch.insert;

                    auto nickR = txn->exec("SELECT Id FROM accounts WHERE Nickname = $1 LIMIT 1", pqxx::params{*in.receiver_nickname});
                    if (nickR.empty())
                        return std::unexpected(DbError{ DbError::Type::NicknameNotFound, 0, {}, fmt::format("Receiver nickname '{}' not found", *in.receiver_nickname) });

                    uint32_t rid = nickR[0]["Id"].as<uint32_t>();
                    in.receiver_id = rid;

                    auto blockedR = txn->exec(
                        "SELECT EXISTS (SELECT 1 FROM socials WHERE AccountId = $1 AND TargetAccountId = $2 AND Type = 2) AS IsBlocked",
                        pqxx::params{rid, v.aid});
                    if (!blockedR.empty() && blockedR[0]["IsBlocked"].as<bool>())
                        return std::unexpected(DbError{ DbError::Type::BlockedByReceiver, 0, {}, fmt::format("Receiver {} has blocked sender {}", rid, v.aid) });

                    auto countR = txn->exec("SELECT COUNT(*) AS cnt FROM mailbox WHERE AccountId = $1", pqxx::params{rid});
                    if (!countR.empty() && countR[0]["cnt"].as<uint32_t>() >= kMailboxLimit)
                        return std::unexpected(DbError{ DbError::Type::MailboxFull, 0, {}, fmt::format("Receiver {} mailbox full", rid) });

                    auto insR = txn->exec(
                        "INSERT INTO mailbox (AccountId, SenderAccountId, SenderNickname, IsRead, ItemId, SentDate, Message) "
                        "VALUES ($1, $2, $3, $4, $5, $6, $7) RETURNING Id",
                        pqxx::params{rid, v.aid, *in.sender_nickname, !in.is_new,
                        in.gift_item_id, Utility::GetUtcTimeNow(), in.message.substr(0, 255)});
                    if (!insR.empty()) patch.mail_id = insR[0]["Id"].as<uint32_t>();
                    DEBUGLOG(green, "PersistMailboxPatches inserted mail id {} from aid {} to {}", patch.mail_id, v.aid, rid);
                }
            }

            CommitIfOwned(owned);
            return {};
        }
        catch (const std::exception& e)
        {
            DEBUGLOG(red, "PersistMailboxPatches failed: {}", e.what());
            return std::unexpected(FromPqxxException(e));
        }
    }

    std::expected<void, DbError> CPostgresDatabase::PersistMatchHistoryAdds(ValidatedDbUpdates& v)
    {
        try
        {
            if (v.match_history_adds.empty()) return {};

            auto [txn, owned] = AcquireTxn();
            for (const auto& match : v.match_history_adds)
            {
                txn->exec(
                    "INSERT INTO player_matchhistory (MatchUniqueId, AccountId, IsWin, IsLose, IsHost, IsDraw, IsClanMatch, "
                    "WinRule, TimeRule, WinRuleType, PlayTime, Level, Experience, Energy, MicroPoints, RoomIndex, RedScore, BlueScore, "
                    "TeamId, RoomMode, RoomMap, SelectedCharacter, Kills, Deaths, Assists, Headshots, HighestKillStreak, "
                    "MeleeKills, RifleKills, ShotgunKills, SniperKills, GatlingKills, BazookaKills, GrenadeKills, ZombieKills, Infections, "
                    "MatchStartTime, MatchStartUtc, MatchEndTime, MatchEndUtc, Hair, Face, Upper_, Under_, Skirt, Gloves, Boots, "
                    "HeadAcc, WaistAcc, BackAcc, Melee, Rifle, Shotgun, Sniper, Gatling, Bazooka, Grenade, RewardItem, "
                    "IsMvp, IsEntryFragger, IsBullseye, IsSupport, IsBomba, MvpScore, EntryFraggerScore, BullseyeScore, "
                    "SupportScore, BombaScore, BestKdScore, CaptureScore, WonRoundScore, ArmsRaceScore, ZombieScore, ADR) "
                    "VALUES ($1,$2,$3,$4,$5,$6,$7,$8,$9,$10,$11,$12,$13,$14,$15,$16,$17,$18,$19,$20,$21,$22,$23,$24,$25,$26,$27,"
                    "$28,$29,$30,$31,$32,$33,$34,$35,$36,$37,$38,$39,$40,$41,$42,$43,$44,$45,$46,$47,$48,$49,$50,$51,$52,$53,$54,"
                    "$55,$56,$57,$58,$59,$60,$61,$62,$63,$64,$65,$66,$67,$68,$69,$70,$71,$72,$73,$74)",
                    pqxx::params{match.MatchUniqueId, match.Aid, match.IsWin, match.IsLose, match.IsHost, match.IsDraw, match.IsClanMatch,
                    match.WinRule, match.TimeRule, match.WinRuleType, match.PlayTime, match.Level, match.Experience,
                    match.Energy, match.MicroPoints, match.room_index, match.redscore, match.bluescore,
                    match.team_id, match.room_mode, match.room_map, match.SelectedCharacter,
                    match.Kills, match.Deaths, match.Assists, match.Headshots, match.HighestKillStreak,
                    match.MeleeKills, match.RifleKills, match.ShotgunKills, match.SniperKills, match.GatlingKills,
                    match.BazookaKills, match.GrenadeKills, match.ZombieKills, match.Infections,
                    static_cast<int64_t>(match.MatchStartTime), match.MatchStartUtc,
                    static_cast<int64_t>(match.MatchEndTime), match.MatchEndUtc,
                    match.Hair, match.Face, match.Upper, match.Under, match.Skirt, match.Gloves, match.Boots,
                    match.HeadAcc, match.WaistAcc, match.BackAcc,
                    match.Melee, match.Rifle, match.Shotgun, match.Sniper, match.Gatling, match.Bazooka, match.Grenade,
                    match.IsItemReward ? match.reward_item : 0u,
                    match.IsMvp, match.IsEntryFragger, match.IsBullseye, match.IsSupport, match.IsBomba,
                    match.MvpScore, match.EntryFraggerScore, match.BullseyeScore, match.SupportScore, match.BombaScore,
                    match.BestKdScore, match.CaptureScore, match.WonRoundScore, match.ArmsRaceScore, match.ZombieScore, match.ADR});
            }
            CommitIfOwned(owned);
            DEBUGLOG(green, "Added {} matches to history for account {}", v.match_history_adds.size(), v.aid);
            return {};
        }
        catch (const std::exception& e)
        {
            DEBUGLOG(red, "PersistMatchHistoryAdds failed: {}", e.what());
            return std::unexpected(FromPqxxException(e));
        }
    }

    std::expected<void, DbError> CPostgresDatabase::PersistPlayerSessionsPatches(ValidatedDbUpdates& v)
    {
        try
        {
            if (v.player_sessions_patches.empty()) return {};

            std::vector<std::pair<int32_t, uint64_t>> to_insert, to_delete;
            for (const auto& p : v.player_sessions_patches)
            {
                if (p.aid == 0 || p.key == 0) continue;
                (p.op == PlayerSessionsPatch::Op::Insert ? to_insert : to_delete).emplace_back(p.aid, p.key);
            }
            if (to_insert.empty() && to_delete.empty()) return {};

            auto [txn, owned] = AcquireTxn();

            for (const auto& [aid, key] : to_delete)
                txn->exec("DELETE FROM player_sessions WHERE PlayerId = $1 AND AuthKey = $2", pqxx::params{aid, static_cast<int64_t>(key)});

            for (const auto& [aid, key] : to_insert)
                txn->exec(
                    "INSERT INTO player_sessions (PlayerId, AuthKey, IssuedAt, ExpiresAt) "
                    "VALUES ($1, $2, CURRENT_TIMESTAMP, CURRENT_TIMESTAMP + INTERVAL '5 minutes') "
                    "ON CONFLICT (PlayerId) DO UPDATE SET AuthKey = EXCLUDED.AuthKey, IssuedAt = EXCLUDED.IssuedAt, ExpiresAt = EXCLUDED.ExpiresAt",
                    pqxx::params{aid, static_cast<int64_t>(key)});

            CommitIfOwned(owned);
            DEBUGLOG(green, "PersistPlayerSessionsPatches: deleted {}, inserted {}", to_delete.size(), to_insert.size());
            return {};
        }
        catch (const std::exception& e)
        {
            DEBUGLOG(red, "PersistPlayerSessionsPatches failed: {}", e.what());
            return std::unexpected(FromPqxxException(e));
        }
    }

    std::expected<void, DbError> CPostgresDatabase::PersistPlayerSocialsPatches(ValidatedDbUpdates& v, ResultDbUpdateInfo& out)
    {
        try
        {
            if (v.player_social_patches.empty()) return {};

            constexpr uint8_t kBlockedState = NetEngine::Socials::State::Blocked;
            constexpr uint8_t kAcceptedState = NetEngine::Socials::State::Accepted;

            auto [txn, owned] = AcquireTxn();

            std::vector<PlayerSocialPatch> to_upsert, to_delete;

            for (const auto& p : v.player_social_patches)
            {
                if (!p.aid) continue;
                PlayerSocialPatch tmp = p;

                if (!tmp.targetAid && tmp.TargetNickname && !tmp.TargetNickname->empty())
                {
                    auto r = txn->exec("SELECT Id FROM accounts WHERE Nickname = $1 LIMIT 1", pqxx::params{*tmp.TargetNickname});
                    if (r.empty()) { out.target_not_found = true; continue; }
                    tmp.targetAid = r[0]["Id"].as<int32_t>();
                }
                if (!tmp.targetAid) continue;

                if (tmp.op == PlayerSocialPatch::Op::Delete)
                {
                    PlayerSocialPatch mir = tmp;
                    std::swap(mir.aid, mir.targetAid);
                    to_delete.push_back(std::move(mir));
                    to_delete.push_back(std::move(tmp));
                }
                else
                {
                    auto checkBlock = [&](int32_t a, int32_t b) {
                        auto r = txn->exec("SELECT Type FROM socials WHERE AccountId = $1 AND TargetAccountId = $2 LIMIT 1", pqxx::params{a, b});
                        return !r.empty() && r[0]["Type"].as<int>() == kBlockedState;
                    };
                    if (checkBlock(tmp.aid, tmp.targetAid)) out.target_blocked = true;
                    if (checkBlock(tmp.targetAid, tmp.aid)) out.player_blocked = true;

                    if (tmp.State && *tmp.State == kAcceptedState)
                    {
                        PlayerSocialPatch mir = tmp;
                        std::swap(mir.aid, mir.targetAid);
                        to_upsert.push_back(std::move(mir));
                    }
                    if (tmp.State && *tmp.State == kBlockedState)
                    {
                        PlayerSocialPatch mir = tmp;
                        std::swap(mir.aid, mir.targetAid);
                        to_delete.push_back(std::move(mir));
                    }
                    to_upsert.push_back(std::move(tmp));
                }
            }

            for (const auto& d : to_delete)
                txn->exec("DELETE FROM socials WHERE AccountId = $1 AND TargetAccountId = $2", pqxx::params{d.aid, d.targetAid});

            for (const auto& u : to_upsert)
            {
                txn->exec(
                    "INSERT INTO socials (AccountId, TargetAccountId, Type) VALUES ($1, $2, $3) "
                    "ON CONFLICT (AccountId, TargetAccountId) DO UPDATE SET Type = EXCLUDED.Type",
                    pqxx::params{u.aid, u.targetAid, static_cast<int>(u.State.value_or(0))});
            }

            CommitIfOwned(owned);
            return {};
        }
        catch (const std::exception& e)
        {
            DEBUGLOG(red, "PersistPlayerSocialsPatches failed: {}", e.what());
            return std::unexpected(FromPqxxException(e));
        }
    }

    std::expected<void, DbError> CPostgresDatabase::PersistGachaPityPatches(ValidatedDbUpdates& v)
    {
        try
        {
            if (v.gacha_pity_patches.empty()) return {};

            auto [txn, owned] = AcquireTxn();
            for (const auto& patch : v.gacha_pity_patches)
            {
                txn->exec(
                    "INSERT INTO gacha_pity (AccountId, GachaId, LuckyPoints) VALUES ($1, $2, $3) "
                    "ON CONFLICT (AccountId, GachaId) DO UPDATE SET LuckyPoints = EXCLUDED.LuckyPoints",
                    pqxx::params{v.aid, patch.gacha_id, patch.lucky_points});
            }
            CommitIfOwned(owned);
            return {};
        }
        catch (const std::exception& e)
        {
            DEBUGLOG(red, "PersistGachaPityPatches failed: {}", e.what());
            return std::unexpected(FromPqxxException(e));
        }
    }

    std::expected<void, DbError> CPostgresDatabase::UpdateAccount(ValidatedDbUpdates& v, ResultDbUpdateInfo& out)
    {
        try
        {
            auto connected = EnsureConnected();
            if (!connected) return std::unexpected(DbError{ DbError::Type::ConnectionLost, 0, {}, "Not connected" });

            pqxx::work txn(GetConnection());
            tl_active_txn = &txn;

            auto aid_str = std::to_string(v.aid);
            auto fail = [&](DbError err, std::string_view reason) {
                tl_active_txn = nullptr;
                DEBUGLOG(red, "UpdateAccount failed: {}", reason);
                return std::unexpected(err);
            };

            if (auto r = PersistCurrenciesPatches(v); !r) return fail(r.error(), "PersistCurrenciesPatches aid " + aid_str);
            if (auto r = PersistAccountInfoPatches(v); !r) return fail(r.error(), "PersistAccountInfoPatches aid " + aid_str);
            if (auto r = PersistItemDeletes(v, out); !r) return fail(r.error(), "PersistItemDeletes aid " + aid_str);
            if (auto r = PersistItemPatches(v, out); !r) return fail(r.error(), "PersistItemPatches aid " + aid_str);
            if (auto r = PersistItemAdds(v, out); !r) return fail(r.error(), "PersistItemAdds aid " + aid_str);
            if (auto r = PersistMissionsPatches(v); !r) return fail(r.error(), "PersistMissionsPatches aid " + aid_str);
            if (auto r = PersistMailboxPatches(v); !r) return fail(r.error(), "PersistMailboxPatches aid " + aid_str);
            if (auto r = PersistMonthlyRewardsPatches(v); !r) return fail(r.error(), "PersistMonthlyRewardsPatches aid " + aid_str);
            if (auto r = PersistMatchHistoryAdds(v); !r) return fail(r.error(), "PersistMatchHistoryAdds aid " + aid_str);
            if (auto r = PersistPlayerSessionsPatches(v); !r) return fail(r.error(), "PersistPlayerSessionsPatches aid " + aid_str);
            if (auto r = PersistPlayerSocialsPatches(v, out); !r) return fail(r.error(), "PersistPlayerSocialsPatches aid " + aid_str);
            if (auto r = PersistGachaPityPatches(v); !r) return fail(r.error(), "PersistGachaPityPatches aid " + aid_str);

            txn.commit();
            tl_active_txn = nullptr;
            return {};
        }
        catch (const std::exception& e)
        {
            tl_active_txn = nullptr;
            DEBUGLOG(red, "UpdateAccount exception: {}", e.what());
            return std::unexpected(FromPqxxException(e));
        }
    }

    std::expected<void, DbError> CPostgresDatabase::UpdateAccounts(std::vector<ValidatedDbUpdates>& batch, std::vector<ResultDbUpdateInfo>& results)
    {
        try
        {
            auto connected = EnsureConnected();
            if (!connected) return std::unexpected(DbError{ DbError::Type::ConnectionLost, 0, {}, "Not connected" });

            results.resize(batch.size());
            for (size_t i = 0; i < batch.size(); ++i)
            {
                auto r = UpdateAccount(batch[i], results[i]);
                if (!r) return r;
            }
            return {};
        }
        catch (const std::exception& e)
        {
            DEBUGLOG(red, "UpdateAccounts exception: {}", e.what());
            return std::unexpected(FromPqxxException(e));
        }
    }

    std::expected<void, DbError> CPostgresDatabase::PersistChatLogs(const std::vector<ChatLogEntry>& logs)
    {
        try
        {
            if (logs.empty()) return {};

            pqxx::work txn(GetConnection());
            for (const auto& log : logs)
            {
                txn.exec(
                    "INSERT INTO log_chat (Timestamp, AccountId, Nickname, ChatType, Location, RoomId, Message) "
                    "VALUES ($1, $2, '', $3::chat_type, $4::chat_location, $5, $6)",
                    pqxx::params{Utility::GetUtcTimeNow(), log.aid,
                    ChatLog::TypeToString(log.chat_type),
                    log.location ? ChatLog::LocationToString(*log.location) : "Channel",
                    log.room_id.value_or(0),
                    log.message.substr(0, 256)});
            }
            txn.commit();
            DEBUGLOG(green, "Persisted {} chat logs", logs.size());
            return {};
        }
        catch (const std::exception& e)
        {
            DEBUGLOG(red, "PersistChatLogs failed: {}", e.what());
            return std::unexpected(FromPqxxException(e));
        }
    }

    std::expected<void, DbError> CPostgresDatabase::PersistItemLogs(const std::vector<ItemLogEntry>& logs)
    {
        try
        {
            if (logs.empty()) return {};

            pqxx::work txn(GetConnection());
            for (const auto& log : logs)
            {
                txn.exec(
                    "INSERT INTO log_items (Timestamp, AccountId, ActionType, ItemId, ItemSerial, Amount, Source, Details) "
                    "VALUES ($1, $2, $3::item_action_type, $4, $5, $6, $7::item_origin_type, $8)",
                    pqxx::params{Utility::GetUtcTimeNow(), log.aid,
                    ItemLog::ActionTypeToString(log.action_type),
                    log.item_id,
                    static_cast<int64_t>(log.serial_info.value_or(0)),
                    log.mp_delta,
                    ItemLog::OriginTypeToString(log.origin_type),
                    ""});
            }
            txn.commit();
            DEBUGLOG(green, "Persisted {} item logs", logs.size());
            return {};
        }
        catch (const std::exception& e)
        {
            DEBUGLOG(red, "PersistItemLogs failed: {}", e.what());
            return std::unexpected(FromPqxxException(e));
        }
    }

    std::expected<void, DbError> CPostgresDatabase::PersistCurrencyLogs(const std::vector<CurrencyLogEntry>& logs)
    {
        try
        {
            if (logs.empty()) return {};

            pqxx::work txn(GetConnection());
            for (const auto& log : logs)
            {
                txn.exec(
                    "INSERT INTO log_currency (Timestamp, AccountId, CurrencyType, Amount, IsReward, Source, BalanceBefore, BalanceAfter, Details) "
                    "VALUES ($1, $2, $3::currency_type, $4, $5, $6::currency_source_type, $7, $8, $9)",
                    pqxx::params{Utility::GetUtcTimeNow(), log.aid,
                    CurrencyLog::TypeToString(log.currency_type),
                    log.amount,
                    log.amount >= 0,
                    CurrencyLog::SourceTypeToString(log.source_type),
                    static_cast<int64_t>(log.before_value),
                    static_cast<int64_t>(log.after_value),
                    ""});
            }
            txn.commit();
            DEBUGLOG(green, "Persisted {} currency logs", logs.size());
            return {};
        }
        catch (const std::exception& e)
        {
            DEBUGLOG(red, "PersistCurrencyLogs failed: {}", e.what());
            return std::unexpected(FromPqxxException(e));
        }
    }

    std::expected<void, DbError> CPostgresDatabase::PersistRoomLogs(const std::vector<RoomLogEntry>& logs)
    {
        try
        {
            if (logs.empty()) return {};

            pqxx::work txn(GetConnection());
            for (const auto& log : logs)
            {
                txn.exec(
                    "INSERT INTO log_rooms (Timestamp, AccountId, EventType, RoomId, Details) "
                    "VALUES ($1, $2, $3::room_event_type, $4, $5)",
                    pqxx::params{Utility::GetUtcTimeNow(), log.aid,
                    RoomLog::EventTypeToString(log.event_type),
                    log.room_id, ""});
            }
            txn.commit();
            DEBUGLOG(green, "Persisted {} room logs", logs.size());
            return {};
        }
        catch (const std::exception& e)
        {
            DEBUGLOG(red, "PersistRoomLogs failed: {}", e.what());
            return std::unexpected(FromPqxxException(e));
        }
    }

    std::expected<void, DbError> CPostgresDatabase::PersistAcDetectionLogs(const std::vector<AcDetectionLogEntry>& logs)
    {
        try
        {
            if (logs.empty()) return {};

            pqxx::work txn(GetConnection());
            for (const auto& log : logs)
            {
                txn.exec(
                    "INSERT INTO log_ac_detection (Timestamp, AccountId, Flag, Details) "
                    "VALUES ($1, $2, $3::detection_flag, $4)",
                    pqxx::params{Utility::GetUtcTimeNow(), log.aid,
                    AcDetection::FlagToString(log.detection_flag),
                    log.details.substr(0, 64)});
            }
            txn.commit();
            DEBUGLOG(green, "Persisted {} ac detection logs", logs.size());
            return {};
        }
        catch (const std::exception& e)
        {
            DEBUGLOG(red, "PersistAcDetectionLogs failed: {}", e.what());
            return std::unexpected(FromPqxxException(e));
        }
    }

    std::expected<void, DbError> CPostgresDatabase::PersistAuthHistory(const AuthHistoryLogEntry& entry)
    {
        try
        {
            pqxx::work txn(GetConnection());
            txn.exec(
                "INSERT INTO log_auth_history (Timestamp, AccountId, IP, Details) VALUES ($1, $2, $3, $4)",
                pqxx::params{Utility::GetUtcTimeNow(), entry.aid, entry.ip, ""});
            txn.commit();
            return {};
        }
        catch (const std::exception& e)
        {
            DEBUGLOG(red, "PersistAuthHistory failed: {}", e.what());
            return std::unexpected(FromPqxxException(e));
        }
    }

    std::expected<void, DbError> CPostgresDatabase::PersistLogs(const LogContext& ctx)
    {
        try
        {
            if (ctx.empty()) return {};
            auto connected = EnsureConnected();
            if (!connected) return std::unexpected(DbError{ DbError::Type::ConnectionLost, 0, {}, "Not connected" });

            if (auto r = PersistChatLogs(ctx.chat_logs); !r) return r;
            if (auto r = PersistItemLogs(ctx.item_logs); !r) return r;
            if (auto r = PersistCurrencyLogs(ctx.currency_logs); !r) return r;
            if (auto r = PersistRoomLogs(ctx.room_logs); !r) return r;
            if (auto r = PersistAcDetectionLogs(ctx.ac_detection_logs); !r) return r;
            return {};
        }
        catch (const std::exception& e)
        {
            DEBUGLOG(red, "PersistLogs failed: {}", e.what());
            return std::unexpected(FromPqxxException(e));
        }
    }

    std::vector<GachaponSaleInfo> CPostgresDatabase::GetGachaponSalesInfo()
    {
        std::vector<GachaponSaleInfo> sales;
        try
        {
            auto connected = EnsureConnected();
            if (!connected) return sales;

            pqxx::work txn(GetConnection());
            for (const auto& row : txn.exec("SELECT GachaponId, SalePrice, StartDate, EndDate FROM gachapon_sales"))
            {
                sales.emplace_back(
                    row["GachaponId"].as<uint32_t>(),
                    row["SalePrice"].as<uint32_t>(),
                    row["StartDate"].as<uint32_t>(),
                    row["EndDate"].as<uint32_t>());
            }
            txn.commit();
        }
        catch (const std::exception& e)
        {
            DEBUGLOG(red, "GetGachaponSalesInfo failed: {}", e.what());
        }
        return sales;
    }

    bool CPostgresDatabase::DeleteGachaponSaleInfo(const uint32_t& gachapon_id)
    {
        try
        {
            auto connected = EnsureConnected();
            if (!connected) return false;

            pqxx::work txn(GetConnection());
            auto r = txn.exec("DELETE FROM gachapon_sales WHERE GachaponId = $1", pqxx::params{gachapon_id});
            txn.commit();
            return r.affected_rows() > 0;
        }
        catch (const std::exception& e)
        {
            DEBUGLOG(red, "DeleteGachaponSaleInfo failed: {}", e.what());
            return false;
        }
    }
}
