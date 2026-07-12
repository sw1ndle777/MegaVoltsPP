#include "CMariaDatabase.h"
#include <fmt/color.h>
#include <boost_unordered.hpp>
#include <algorithm>
#include <cctype>
#include <cstdlib>
namespace BaseLib
{
	using enum fmt::color;
    void CMariaDatabase::Initialize(const std::string& database, const std::string& host, const uint16_t& port, const std::string& user, const std::string& password)
    {
        try
        {
            this->database_name = database;
            this->driver = sql::mariadb::get_driver_instance();
            this->properties["hostName"] = std::string(host + ":" + std::to_string(port)).c_str();
            this->properties["userName"] = user.c_str();
            this->properties["password"] = password.c_str();
            this->properties["autoReconnect"] = "true";

            // Aliases used by different connector builds.
            this->properties["host"] = host.c_str();
            this->properties["port"] = std::to_string(port).c_str();
            this->properties["user"] = user.c_str();

            auto getenv_string = [](const char* name) -> std::string
            {
                const char* raw = std::getenv(name);
                return raw ? std::string(raw) : std::string();
            };
            auto to_lower = [](std::string s) -> std::string
            {
                std::transform(s.begin(), s.end(), s.begin(), [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
                return s;
            };
            auto apply_property_if_set = [&](const char* key, const std::string& value)
            {
                if (!value.empty())
                    this->properties[key] = value.c_str();
            };

            // Optional TLS overrides from environment (used by Docker setup).
            const std::string env_ssl_mode = getenv_string("DB_SSL_MODE");
            const std::string env_use_tls = getenv_string("DB_USE_TLS");
            const std::string env_tls_ca = getenv_string("DB_TLS_CA");
            const std::string env_server_ssl_cert = getenv_string("DB_SERVER_SSL_CERT");
            const std::string env_tls_cert = getenv_string("DB_TLS_CERT");
            const std::string env_tls_key = getenv_string("DB_TLS_KEY");
            const std::string env_tls_version = getenv_string("DB_TLS_VERSION");
            const std::string env_trust_server_certificate = getenv_string("DB_TRUST_SERVER_CERTIFICATE");
            const std::string env_disable_ssl_hostname_verification = getenv_string("DB_DISABLE_SSL_HOSTNAME_VERIFICATION");

            apply_property_if_set("sslMode", env_ssl_mode);
            apply_property_if_set("tlsCA", env_tls_ca);
            apply_property_if_set("serverSslCert", env_server_ssl_cert);
            apply_property_if_set("tlsCert", env_tls_cert);
            apply_property_if_set("tlsKey", env_tls_key);
            apply_property_if_set("tlsVersion", env_tls_version);
            apply_property_if_set("trustServerCertificate", to_lower(env_trust_server_certificate));
            apply_property_if_set("disableSslHostnameVerification", to_lower(env_disable_ssl_hostname_verification));

            if (!env_use_tls.empty())
            {
                const auto normalized = to_lower(env_use_tls);
                this->properties["useTls"] = normalized.c_str();
                this->properties["useSsl"] = normalized.c_str();
                this->properties["useSSL"] = normalized.c_str();
            }

            const bool has_tls_ca = !env_tls_ca.empty();
            const bool has_server_ssl_cert = !env_server_ssl_cert.empty();

            auto connect_with_current_properties = [&]()
            {
                conn = driver->connect(this->properties);
            };

            const bool is_host_gateway = (host == "host.docker.internal" || host == "host.containers.internal");

            try
            {
                connect_with_current_properties();
            }
            catch (sql::SQLException& e)
            {
                const std::string msg = e.what();
                const bool is_tls_self_signed =
                    msg.contains("TLS/SSL error") ||
                    msg.contains("self-signed certificate") ||
                    msg.contains("certificate verify failed");

                if (!is_host_gateway || !is_tls_self_signed)
                    throw;

                DEBUGLOG(yellow, "DB TLS self-signed on host gateway detected, retrying with connector fallbacks...");

                // If a CA/self-signed PEM is provided, try verify-ca first.
                if (has_tls_ca || has_server_ssl_cert)
                {
                    try
                    {
                        this->properties["sslMode"] = "verify-ca";
                        this->properties["useTls"] = "true";
                        this->properties["useSsl"] = "true";
                        this->properties["useSSL"] = "true";
                        if (has_tls_ca)
                            this->properties["tlsCA"] = env_tls_ca.c_str();
                        if (has_server_ssl_cert)
                            this->properties["serverSslCert"] = env_server_ssl_cert.c_str();
                        connect_with_current_properties();
                    }
                    catch (const sql::SQLException& cert_retry_error)
                    {
                        DEBUGLOG(yellow, "DB verify-ca retry failed: {}", cert_retry_error.what());
                    }
                }

                // Retry strategy for Docker->Windows host local DB:
                // 1) trust TLS (skip hostname/cert-chain verification)
                // 2) disable TLS entirely
                if (!conn || !conn->isValid())
                {
                    try
                    {
                        this->properties["sslMode"] = "trust";
                        this->properties["useTls"] = "true";
                        this->properties["useSsl"] = "true";
                        this->properties["useSSL"] = "true";
                        this->properties["trustServerCertificate"] = "true";
                        this->properties["disableSslHostnameVerification"] = "true";
                        connect_with_current_properties();
                    }
                    catch (const sql::SQLException& e2)
                    {
                        const std::string msg2 = e2.what();

                        // Some server/plugin combinations continue to fail in trust mode.
                        // Last fallback is to explicitly disable TLS.
                        if (msg2.contains("TLS/SSL error") ||
                            msg2.contains("self-signed certificate") ||
                            msg2.contains("certificate verify failed") ||
                            msg2.contains("GSSAPI") ||
                            msg2.contains("auth_gssapi"))
                        {
                            this->properties["sslMode"] = "disable";
                            this->properties["useTls"] = "false";
                            this->properties["useSsl"] = "false";
                            this->properties["useSSL"] = "false";
                            connect_with_current_properties();
                        }
                        else
                        {
                            throw;
                        }
                    }
                }
            }
            if (conn)
            {
				DEBUGLOG(dark_cyan, "connected to ({}:{})", host, port);
                if (CreateDatabase(database))
                    DEBUGLOG(dark_cyan, "created database ({})", database);

                conn->setSchema(database);

                CreateTable("accounts", R"(
                    Id int unsigned NOT NULL AUTO_INCREMENT,
                    Email varchar(255) NOT NULL DEFAULT '',
                    IsEmailVerified tinyint(1) DEFAULT 0,
                    ServerId int unsigned NOT NULL DEFAULT 0,
                    Username varchar(16) NOT NULL, 
                    Password varchar(127) NOT NULL, 
                    Salt varchar(127) NOT NULL, 
                    Grade tinyint unsigned NOT NULL DEFAULT 0,
                    PCRoom tinyint unsigned NOT NULL DEFAULT 0,
                    AuthKey bigint unsigned NOT NULL DEFAULT 0,
                    ClanId int unsigned DEFAULT NULL,
                    ClanKills int unsigned DEFAULT 0,
                    ClanDeaths int unsigned DEFAULT 0,
                    ClanAssists int unsigned DEFAULT 0,
                    ClanContribution int unsigned DEFAULT 0,
                    ClanWins int unsigned DEFAULT 0,
                    ClanLoses int unsigned DEFAULT 0,
                    ClanDraws int unsigned DEFAULT 0,
                    Nickname varchar(16) NOT NULL, 
                    Level int unsigned NOT NULL DEFAULT 0, 
                    Experience int unsigned NOT NULL DEFAULT 0, 
                    Tutorial bit(1) NOT NULL DEFAULT b'0', 
                    Story int unsigned NOT NULL DEFAULT 0,
                    GuideMission tinyint unsigned NOT NULL DEFAULT 0,
                    Achievement bigint unsigned NOT NULL DEFAULT 0,
                    VoiceType bigint unsigned NOT NULL DEFAULT 0,
                    VIPExperience int unsigned NOT NULL DEFAULT 0, 
                    MaximumItems int unsigned NOT NULL DEFAULT 100, 
                    MaximumEnergy int unsigned NOT NULL DEFAULT 100, 
                    SelectedCharacter int unsigned NOT NULL DEFAULT 0, 
                    PlayTime bigint unsigned NOT NULL DEFAULT 0, 
                    MutedUntil bigint unsigned NOT NULL DEFAULT 0, 
                    Coins int unsigned NOT NULL DEFAULT 1000,
                    Energy int unsigned NOT NULL DEFAULT 100,
                    LuckyPoints int unsigned NOT NULL DEFAULT 0,
                    MicroPoints bigint unsigned NOT NULL DEFAULT 0,
                    RockTokens bigint unsigned NOT NULL DEFAULT 0,
                    Coupons int unsigned NOT NULL DEFAULT 0,
                    Wins int unsigned NOT NULL DEFAULT 0,
                    Loses int unsigned NOT NULL DEFAULT 0,
                    Draws int unsigned NOT NULL DEFAULT 0,
                    Kills int unsigned NOT NULL DEFAULT 0,
                    Deaths int unsigned NOT NULL DEFAULT 0,
                    Assists int unsigned NOT NULL DEFAULT 0,
                    Headshots int unsigned NOT NULL DEFAULT 0,
                    HighestKillStreak int unsigned NOT NULL DEFAULT 0,
                    MeleeKills int unsigned NOT NULL DEFAULT 0,
                    RifleKills int unsigned NOT NULL DEFAULT 0,
                    ShotgunKills int unsigned NOT NULL DEFAULT 0,
                    SniperKills int unsigned NOT NULL DEFAULT 0,
                    GatlingKills int unsigned NOT NULL DEFAULT 0,
                    BazookaKills int unsigned NOT NULL DEFAULT 0,
                    GrenadeKills int unsigned NOT NULL DEFAULT 0,
                    ZombieKills int unsigned NOT NULL DEFAULT 0,
                    Infections int unsigned NOT NULL DEFAULT 0,
                    SingleWaveDailyAttempts int unsigned NOT NULL DEFAULT 0,
                    SingleWaveHighestWave int unsigned NOT NULL DEFAULT 0,
                    SingleWaveHighScore int unsigned NOT NULL DEFAULT 0,
                    SingleWaveLastUpdate bigint unsigned NOT NULL DEFAULT 0,
                    LawfulPoint int unsigned NOT NULL DEFAULT 0,
                    ChaoticPoint int unsigned NOT NULL DEFAULT 0,
                    IsAdmin bit(1) NOT NULL DEFAULT b'0',
                    TwoFactorSecret varchar(255) DEFAULT NULL,
                    TwoFactorEnabled tinyint(1) DEFAULT 0,
                    PRIMARY KEY(Id))");

                CreateTable("game_titles", R"(
                    Id int unsigned NOT NULL AUTO_INCREMENT,
                    TitleName varchar(50) NOT NULL,
                    PRIMARY KEY (Id))");

                CreateTable("account_titles", R"(
                    AccountId int unsigned NOT NULL,
                    TitleId int unsigned NOT NULL,
                    PRIMARY KEY (AccountId, TitleId),
                    KEY fk_acctitle_title (TitleId),
                    CONSTRAINT fk_acctitle_account FOREIGN KEY (AccountId) REFERENCES accounts (Id) ON DELETE CASCADE,
                    CONSTRAINT fk_acctitle_title FOREIGN KEY (TitleId) REFERENCES game_titles (Id) ON DELETE CASCADE)");

                CreateTable("player_sessions", R"(
                    PlayerId int unsigned NOT NULL,
                    AuthKey bigint unsigned NOT NULL,
                    IssuedAt DATETIME NOT NULL,
                    ExpiresAt DATETIME NOT NULL,
                    LastSeenAt DATETIME NULL DEFAULT NULL,
                    PRIMARY KEY (PlayerId),
                    UNIQUE KEY uq_authkey (AuthKey),
                    KEY ix_expires (ExpiresAt),
                    CONSTRAINT FK_player_sessions_accounts_PlayerId FOREIGN KEY (PlayerId) REFERENCES accounts (Id) ON DELETE CASCADE)");

                CreateTable("player_matchhistory", R"(
                    Id bigint unsigned NOT NULL AUTO_INCREMENT,
                    MatchUniqueId varchar(128) NOT NULL DEFAULT '',
                    AccountId int unsigned NOT NULL,
                    IsWin bool NOT NULL DEFAULT 0,
                    IsLose bool NOT NULL DEFAULT 0,
                    IsHost bool NOT NULL,
                    IsDraw bool NOT NULL,
                    IsClanMatch bool NOT NULL,
                    WinRule int unsigned NOT NULL DEFAULT 0,
                    TimeRule int unsigned NOT NULL DEFAULT 0,
                    WinRuleType varchar(32) NOT NULL DEFAULT 'Unknown',
                    PlayTime int unsigned NOT NULL,
                    Level int unsigned NOT NULL,
                    Experience int unsigned NOT NULL,
                    Energy int unsigned NOT NULL,
                    MicroPoints int unsigned NOT NULL,
                    RoomIndex int unsigned NOT NULL,
                    RedScore int unsigned NOT NULL,
                    BlueScore int unsigned NOT NULL,
                    TeamId int unsigned NOT NULL,
                    RoomMode int unsigned NOT NULL,
                    RoomMap int unsigned NOT NULL,
                    SelectedCharacter int unsigned NOT NULL,
                    Kills int unsigned NOT NULL,
                    Deaths int unsigned NOT NULL,
                    Assists int unsigned NOT NULL,
                    Headshots int unsigned NOT NULL,
                    HighestKillStreak int unsigned NOT NULL,
                    MeleeKills int unsigned NOT NULL,
                    RifleKills int unsigned NOT NULL,
                    ShotgunKills int unsigned NOT NULL,
                    SniperKills int unsigned NOT NULL,
                    GatlingKills int unsigned NOT NULL,
                    BazookaKills int unsigned NOT NULL,
                    GrenadeKills int unsigned NOT NULL,
                    ZombieKills int unsigned NOT NULL,
                    Infections int unsigned NOT NULL,
                    MatchStartTime bigint unsigned NOT NULL DEFAULT 0,
                    MatchStartUtc varchar(32) NOT NULL DEFAULT '',
                    MatchEndTime bigint unsigned NOT NULL,
                    MatchEndUtc varchar(32) NOT NULL DEFAULT '',
                    Hair int unsigned NOT NULL,
                    Face int unsigned NOT NULL,
                    Upper int unsigned NOT NULL,
                    Under int unsigned NOT NULL,
                    Skirt int unsigned NOT NULL,
                    Gloves int unsigned NOT NULL,
                    Boots int unsigned NOT NULL,
                    HeadAcc int unsigned NOT NULL,
                    WaistAcc int unsigned NOT NULL,
                    BackAcc int unsigned NOT NULL,
                    Melee int unsigned NOT NULL,
                    Rifle int unsigned NOT NULL,
                    Shotgun int unsigned NOT NULL,
                    Sniper int unsigned NOT NULL,
                    Gatling int unsigned NOT NULL,
                    Bazooka int unsigned NOT NULL,
                    Grenade int unsigned NOT NULL,
                    RewardItem int unsigned NOT NULL,
                    IsMvp bool NOT NULL,
                    IsEntryFragger bool NOT NULL,
                    IsBullseye bool NOT NULL,
                    IsSupport bool NOT NULL,
                    IsBomba bool NOT NULL,
                    MvpScore int unsigned NOT NULL DEFAULT 0,
                    EntryFraggerScore int unsigned NOT NULL DEFAULT 0,
                    BullseyeScore int unsigned NOT NULL DEFAULT 0,
                    SupportScore int unsigned NOT NULL DEFAULT 0,
                    BombaScore int unsigned NOT NULL DEFAULT 0,
                    BestKdScore int unsigned NOT NULL DEFAULT 0,
                    CaptureScore int unsigned NOT NULL DEFAULT 0,
                    WonRoundScore int unsigned NOT NULL DEFAULT 0,
                    ArmsRaceScore int unsigned NOT NULL DEFAULT 0,
                    ZombieScore int unsigned NOT NULL DEFAULT 0,
                    ADR int unsigned NOT NULL DEFAULT 0,
                    IsParty bool NOT NULL DEFAULT 0,
                    Restriction int unsigned NOT NULL DEFAULT 0,
                    MaxPlayers int unsigned NOT NULL DEFAULT 0,
                    PRIMARY KEY(Id),
                    KEY IX_player_matchhistory_MatchUniqueId (MatchUniqueId),
                    KEY IX_player_matchhistory_AccountId (AccountId),
                    CONSTRAINT FK_player_matchhistory_accounts_AccountId FOREIGN KEY (AccountId) REFERENCES accounts (Id) ON DELETE CASCADE)");

                try
                {
                    std::unique_ptr<sql::Statement> match_history_migration_stmt(conn->createStatement());
                    match_history_migration_stmt->execute("ALTER TABLE `player_matchhistory` MODIFY COLUMN `MatchUniqueId` varchar(128) NOT NULL DEFAULT ''");
                    match_history_migration_stmt->execute("ALTER TABLE `player_matchhistory` ADD COLUMN IF NOT EXISTS `WinRuleType` varchar(32) NOT NULL DEFAULT 'Unknown' AFTER `TimeRule`");
                    match_history_migration_stmt->execute("ALTER TABLE `player_matchhistory` ADD COLUMN IF NOT EXISTS `MatchStartUtc` varchar(32) NOT NULL DEFAULT '' AFTER `MatchStartTime`");
                    match_history_migration_stmt->execute("ALTER TABLE `player_matchhistory` ADD COLUMN IF NOT EXISTS `MatchEndUtc` varchar(32) NOT NULL DEFAULT '' AFTER `MatchEndTime`");
                    // Match-type metadata for website filtering (party / weapon restriction / room size).
                    match_history_migration_stmt->execute("ALTER TABLE `player_matchhistory` ADD COLUMN IF NOT EXISTS `IsParty` bool NOT NULL DEFAULT 0 AFTER `ADR`");
                    match_history_migration_stmt->execute("ALTER TABLE `player_matchhistory` ADD COLUMN IF NOT EXISTS `Restriction` int unsigned NOT NULL DEFAULT 0 AFTER `IsParty`");
                    match_history_migration_stmt->execute("ALTER TABLE `player_matchhistory` ADD COLUMN IF NOT EXISTS `MaxPlayers` int unsigned NOT NULL DEFAULT 0 AFTER `Restriction`");
                }
                catch (const sql::SQLException& e)
                {
                    DEBUGLOG(yellow, "player_matchhistory migration failed: {}", e.what());
                }


                CreateTable("bans", R"(
                    Id int unsigned NOT NULL AUTO_INCREMENT, 
                    AccountId int unsigned NOT NULL, 
                    UnbanDate datetime(6) NOT NULL, 
                    Reason varchar(127) DEFAULT NULL, 
                    PRIMARY KEY(Id), 
                    KEY IX_bans_AccountId (AccountId), 
                    CONSTRAINT FK_bans_accounts_AccountId FOREIGN KEY (AccountId) REFERENCES accounts (Id) ON DELETE CASCADE)");

                CreateTable("login_history", R"(
                    Id int unsigned NOT NULL AUTO_INCREMENT, 
                    AccountId int unsigned NOT NULL, 
                    LoginDate datetime(6) NOT NULL,
                    IP varchar(15) NOT NULL, 
                    PRIMARY KEY(Id), 
                    KEY IX_login_history_AccountId (AccountId), 
                    CONSTRAINT FK_login_history_accounts_AccountId FOREIGN KEY (AccountId) REFERENCES accounts (Id) ON DELETE CASCADE)");

                CreateTable("clans", R"(
                    Id int unsigned NOT NULL AUTO_INCREMENT, 
                    OwnerId int unsigned DEFAULT NULL, 
                    ClanName varchar(16) NOT NULL, 
                    ClanLogoFront smallint unsigned NOT NULL DEFAULT 0, 
                    ClanLogoBack smallint unsigned NOT NULL DEFAULT 0, 
                    ClanContribution int unsigned NOT NULL DEFAULT 0, 
                    ClanWin int unsigned NOT NULL DEFAULT 0, 
                    ClanLose int unsigned NOT NULL DEFAULT 0, 
                    ClanDraw int unsigned NOT NULL DEFAULT 0, 
                    Kills int unsigned NOT NULL DEFAULT 0, 
                    Deaths int unsigned NOT NULL DEFAULT 0, 
                    Assists int unsigned NOT NULL DEFAULT 0,
                    Lawful int unsigned NOT NULL DEFAULT 0,
                    Chaotic int unsigned NOT NULL DEFAULT 0,
                    Description varchar(255) NOT NULL DEFAULT 'Welcome to our clan page.',
                    Title varchar(50) NOT NULL DEFAULT 'Rookie Clan',
                    TeamACaptainId int unsigned DEFAULT NULL,
                    TeamBCaptainId int unsigned DEFAULT NULL,
                    PRIMARY KEY(Id), 
                    KEY IX_clans_OwnerId (OwnerId),
                    KEY FK_Clans_CaptainA (TeamACaptainId),
                    KEY FK_Clans_CaptainB (TeamBCaptainId),
                    CONSTRAINT FK_Clans_CaptainA FOREIGN KEY (TeamACaptainId) REFERENCES accounts (Id) ON DELETE SET NULL,
                    CONSTRAINT FK_Clans_CaptainB FOREIGN KEY (TeamBCaptainId) REFERENCES accounts (Id) ON DELETE SET NULL,
                    CONSTRAINT FK_clans_accounts_OwnerId FOREIGN KEY (OwnerId) REFERENCES accounts (Id) ON DELETE CASCADE)");

                CreateTable("clan_member_roles", R"(
                    AccountId int unsigned NOT NULL,
                    ClanId int unsigned NOT NULL,
                    Role enum('TeamA','TeamB','Reserves') NOT NULL DEFAULT 'Reserves',
                    PRIMARY KEY (AccountId),
                    KEY FK_roles_clans (ClanId),
                    CONSTRAINT FK_roles_accounts FOREIGN KEY (AccountId) REFERENCES accounts (Id) ON DELETE CASCADE,
                    CONSTRAINT FK_roles_clans FOREIGN KEY (ClanId) REFERENCES clans (Id) ON DELETE CASCADE)");

                CreateTable("clan_messages", R"(
                    Id int unsigned NOT NULL AUTO_INCREMENT,
                    ClanId int unsigned NOT NULL,
                    SenderId int unsigned NOT NULL,
                    Message varchar(255) NOT NULL,
                    SentDate datetime NOT NULL DEFAULT current_timestamp(),
                    PRIMARY KEY (Id),
                    KEY FK_clan_messages_clans (ClanId),
                    KEY FK_clan_messages_accounts (SenderId),
                    CONSTRAINT FK_clan_messages_accounts FOREIGN KEY (SenderId) REFERENCES accounts (Id) ON DELETE CASCADE,
                    CONSTRAINT FK_clan_messages_clans FOREIGN KEY (ClanId) REFERENCES clans (Id) ON DELETE CASCADE)");

                CreateTable("clan_requests", R"(
                    Id int unsigned NOT NULL AUTO_INCREMENT,
                    ClanId int unsigned NOT NULL,
                    AccountId int unsigned NOT NULL,
                    RequestDate datetime NOT NULL DEFAULT current_timestamp(),
                    PRIMARY KEY (Id),
                    UNIQUE KEY IX_Request (ClanId, AccountId),
                    KEY FK_requests_accounts (AccountId),
                    CONSTRAINT FK_requests_accounts FOREIGN KEY (AccountId) REFERENCES accounts (Id) ON DELETE CASCADE,
                    CONSTRAINT FK_requests_clans FOREIGN KEY (ClanId) REFERENCES clans (Id) ON DELETE CASCADE)");

                CreateTable("clan_votes", R"(
                    ClanId int unsigned NOT NULL,
                    AccountId int unsigned NOT NULL,
                    VoteType enum('lawful','chaotic') NOT NULL,
                    PRIMARY KEY (ClanId, AccountId),
                    KEY FK_clan_votes_accounts (AccountId),
                    CONSTRAINT FK_clan_votes_accounts FOREIGN KEY (AccountId) REFERENCES accounts (Id) ON DELETE CASCADE,
                    CONSTRAINT FK_clan_votes_clans FOREIGN KEY (ClanId) REFERENCES clans (Id) ON DELETE CASCADE)");

                CreateTable("player_socials", R"(
                    Aid int unsigned NOT NULL,
                    TargetAid int unsigned NOT NULL,
                    State tinyint unsigned NOT NULL,
                    KEY IX_player_socials_TargetAid (TargetAid),
                    KEY IX_player_socials_Aid (Aid),
                    CONSTRAINT uq_player_socials_aid_target UNIQUE (Aid, TargetAid),
                    CONSTRAINT FK_player_socials_accounts_Aid FOREIGN KEY (Aid) REFERENCES accounts (Id) ON DELETE CASCADE,
                    CONSTRAINT FK_player_socials_accounts_TargetAid FOREIGN KEY (TargetAid) REFERENCES accounts (Id) ON DELETE CASCADE)");

                CreateTable("player_items", R"(
                    SerialInfo bigint unsigned NOT NULL,
                    PlayerId int unsigned NOT NULL,
                    ItemId int unsigned NOT NULL,
                    ItemType int unsigned NOT NULL,
                    ExpirationDate int unsigned NOT NULL,
                    Repair smallint unsigned NOT NULL,
                    Energy smallint unsigned NOT NULL,
                    IsSealed int unsigned NOT NULL,
                    SealLevel int unsigned NOT NULL,
                    EnhanceExp int unsigned NOT NULL,
                    EnhanceLevel int unsigned NOT NULL,
                    Stock int unsigned NOT NULL,
                    IsEquipped tinyint unsigned NOT NULL,
                    CharacterId tinyint unsigned NOT NULL,
                    PRIMARY KEY (PlayerId, SerialInfo),
                    KEY IX_player_items_SerialInfo (SerialInfo),
                    KEY IX_player_items_PlayerId (PlayerId),
                    CONSTRAINT FK_player_items_accounts_PlayerId FOREIGN KEY (PlayerId) REFERENCES accounts (Id) ON DELETE CASCADE)");

                CreateTable("player_mailbox", R"(
                    Id int unsigned NOT NULL AUTO_INCREMENT,
                    SenderId int unsigned NOT NULL,
                    SenderNickname varchar(16) NOT NULL,
                    ReceiverId int unsigned NOT NULL,
                    ReceiverNickname varchar(16) NOT NULL,  
                    Date int unsigned NOT NULL,
                    GiftItemId int unsigned DEFAULT NULL,
                    Message varchar(256) NOT NULL,
                    IsNew bit(1) NOT NULL DEFAULT 0,
                    DeletedFromSender bit(1) NOT NULL,
                    DeletedFromReceiver bit(1) NOT NULL,
                    PRIMARY KEY (Id),
                    KEY IX_player_mailbox_ReceiverId (ReceiverId),
                    KEY IX_player_mailbox_SenderId (SenderId),
                    CONSTRAINT FK_player_mailbox_accounts_ReceiverId FOREIGN KEY (ReceiverId) REFERENCES accounts (Id) ON DELETE CASCADE,
                    CONSTRAINT FK_player_mailbox_accounts_SenderId FOREIGN KEY (SenderId) REFERENCES accounts (Id))");

                CreateTable("player_monthly_rewards", R"(
                    ID int unsigned NOT NULL AUTO_INCREMENT,
                    PlayerId int unsigned NOT NULL,
                    RewardCount tinyint unsigned NOT NULL,
                    LastUpdate datetime(6) NOT NULL,
                    PRIMARY KEY (ID),
                    UNIQUE KEY UX_player_monthly_rewards_PlayerId (PlayerId),
                    CONSTRAINT FK_player_monthly_rewards_accounts_PlayerId FOREIGN KEY (PlayerId) REFERENCES accounts (Id) ON DELETE CASCADE)");

                try
                {
                    std::unique_ptr<sql::Statement> dedupe_monthly_rewards_stmt(conn->createStatement());
                    dedupe_monthly_rewards_stmt->execute(R"(
                        DELETE pmr1
                        FROM player_monthly_rewards pmr1
                        JOIN player_monthly_rewards pmr2
                          ON pmr1.PlayerId = pmr2.PlayerId
                         AND (pmr1.LastUpdate < pmr2.LastUpdate
                              OR (pmr1.LastUpdate = pmr2.LastUpdate AND pmr1.ID < pmr2.ID))
                    )");

                    std::unique_ptr<sql::PreparedStatement> monthly_rewards_idx_stmt(conn->prepareStatement(
                        "SELECT COUNT(*) AS cnt FROM INFORMATION_SCHEMA.STATISTICS "
                        "WHERE TABLE_SCHEMA = ? AND TABLE_NAME = 'player_monthly_rewards' "
                        "AND INDEX_NAME = 'UX_player_monthly_rewards_PlayerId'"
                    ));
                    monthly_rewards_idx_stmt->setString(1, database_name);
                    std::unique_ptr<sql::ResultSet> monthly_rewards_idx_res(monthly_rewards_idx_stmt->executeQuery());
                    bool has_unique_monthly_reward_playerid = monthly_rewards_idx_res->next() && monthly_rewards_idx_res->getUInt("cnt") > 0;
                    if (!has_unique_monthly_reward_playerid)
                    {
                        std::unique_ptr<sql::Statement> add_monthly_rewards_idx_stmt(conn->createStatement());
                        add_monthly_rewards_idx_stmt->execute("ALTER TABLE `player_monthly_rewards` ADD UNIQUE KEY `UX_player_monthly_rewards_PlayerId` (`PlayerId`)");
                    }
                }
                catch (const sql::SQLException& e)
                {
                    DEBUGLOG(red, "player_monthly_rewards migration failed: {}", e.what());
                }

                CreateTable("player_weekly_rewards", R"(
                    ID int unsigned NOT NULL AUTO_INCREMENT,
                    PlayerId int unsigned NOT NULL,
                    RewardCount tinyint unsigned NOT NULL,
                    LastUpdate datetime(6) NOT NULL,
                    PRIMARY KEY (ID),
                    UNIQUE KEY UX_player_weekly_rewards_PlayerId (PlayerId),
                    CONSTRAINT FK_player_weekly_rewards_accounts_PlayerId FOREIGN KEY (PlayerId) REFERENCES accounts (Id) ON DELETE CASCADE)");

                try
                {
                    std::unique_ptr<sql::Statement> dedupe_weekly_rewards_stmt(conn->createStatement());
                    dedupe_weekly_rewards_stmt->execute(R"(
                        DELETE pwr1
                        FROM player_weekly_rewards pwr1
                        JOIN player_weekly_rewards pwr2
                          ON pwr1.PlayerId = pwr2.PlayerId
                         AND (pwr1.LastUpdate < pwr2.LastUpdate
                              OR (pwr1.LastUpdate = pwr2.LastUpdate AND pwr1.ID < pwr2.ID))
                    )");

                    std::unique_ptr<sql::PreparedStatement> weekly_rewards_idx_stmt(conn->prepareStatement(
                        "SELECT COUNT(*) AS cnt FROM INFORMATION_SCHEMA.STATISTICS "
                        "WHERE TABLE_SCHEMA = ? AND TABLE_NAME = 'player_weekly_rewards' "
                        "AND INDEX_NAME = 'UX_player_weekly_rewards_PlayerId'"
                    ));
                    weekly_rewards_idx_stmt->setString(1, database_name);
                    std::unique_ptr<sql::ResultSet> weekly_rewards_idx_res(weekly_rewards_idx_stmt->executeQuery());
                    bool has_unique_weekly_reward_playerid = weekly_rewards_idx_res->next() && weekly_rewards_idx_res->getUInt("cnt") > 0;
                    if (!has_unique_weekly_reward_playerid)
                    {
                        std::unique_ptr<sql::Statement> add_weekly_rewards_idx_stmt(conn->createStatement());
                        add_weekly_rewards_idx_stmt->execute("ALTER TABLE `player_weekly_rewards` ADD UNIQUE KEY `UX_player_weekly_rewards_PlayerId` (`PlayerId`)");
                    }
                }
                catch (const sql::SQLException& e)
                {
                    DEBUGLOG(red, "player_weekly_rewards migration failed: {}", e.what());
                }

                CreateTable("player_profiles", R"(
                    AccountId int unsigned NOT NULL,
                    Description varchar(140) NOT NULL DEFAULT '',
                    SelectedTitleId int unsigned NOT NULL DEFAULT 1,
                    AvatarUrl varchar(255) DEFAULT NULL,
                    PRIMARY KEY (AccountId),
                    CONSTRAINT fk_profile_account FOREIGN KEY (AccountId) REFERENCES accounts (Id) ON DELETE CASCADE)");

                CreateTable("player_messages", R"(
                    Id int unsigned NOT NULL AUTO_INCREMENT,
                    TargetId int unsigned NOT NULL,
                    SenderId int unsigned NOT NULL,
                    Message varchar(255) NOT NULL,
                    SentDate datetime DEFAULT current_timestamp(),
                    PRIMARY KEY (Id),
                    KEY fk_player_msg_target (TargetId),
                    KEY fk_player_msg_sender (SenderId),
                    CONSTRAINT fk_player_msg_sender FOREIGN KEY (SenderId) REFERENCES accounts (Id) ON DELETE CASCADE,
                    CONSTRAINT fk_player_msg_target FOREIGN KEY (TargetId) REFERENCES accounts (Id) ON DELETE CASCADE)");

                CreateTable("player_votes", R"(
                    Id int unsigned NOT NULL AUTO_INCREMENT,
                    TargetId int unsigned NOT NULL,
                    VoterId int unsigned NOT NULL,
                    VoteType enum('lawful','chaotic') NOT NULL,
                    PRIMARY KEY (Id),
                    UNIQUE KEY unique_player_vote (TargetId, VoterId),
                    KEY fk_player_vote_voter (VoterId),
                    CONSTRAINT fk_player_vote_target FOREIGN KEY (TargetId) REFERENCES accounts (Id) ON DELETE CASCADE,
                    CONSTRAINT fk_player_vote_voter FOREIGN KEY (VoterId) REFERENCES accounts (Id) ON DELETE CASCADE)");

                CreateTable("system_gachapon_machine", R"(
                    GachaponId int unsigned DEFAULT NULL,
                    SalePrice int unsigned NOT NULL,
                    EventStartDate DATETIME NOT NULL,
                    EventEndDate DATETIME NOT NULL,
                    PRIMARY KEY (GachaponId))");

                CreateTable("system_event_mod", R"(
                    ModId int unsigned DEFAULT NULL,
                    EventStartDate DATETIME NOT NULL,
                    EventEndDate DATETIME NOT NULL,
                    PRIMARY KEY (ModId))");

                CreateTable("system_event_map", R"(
                    MapId int unsigned DEFAULT NULL,
                    EventStartDate DATETIME NOT NULL,
                    EventEndDate DATETIME NOT NULL,
                    PRIMARY KEY (MapId))");

                CreateTable("system_monthly_rewards", R"(
                    Year smallint unsigned NOT NULL,
                    Month tinyint unsigned NOT NULL,
                    Day1 int unsigned NOT NULL DEFAULT 0,
                    Day2 int unsigned NOT NULL DEFAULT 0,
                    Day3 int unsigned NOT NULL DEFAULT 0,
                    Day4 int unsigned NOT NULL DEFAULT 0,
                    Day5 int unsigned NOT NULL DEFAULT 0,
                    Day6 int unsigned NOT NULL DEFAULT 0,
                    Day7 int unsigned NOT NULL DEFAULT 0,
                    Day8 int unsigned NOT NULL DEFAULT 0,
                    Day9 int unsigned NOT NULL DEFAULT 0,
                    Day10 int unsigned NOT NULL DEFAULT 0,
                    Day11 int unsigned NOT NULL DEFAULT 0,
                    Day12 int unsigned NOT NULL DEFAULT 0,
                    Day13 int unsigned NOT NULL DEFAULT 0,
                    Day14 int unsigned NOT NULL DEFAULT 0,
                    Day15 int unsigned NOT NULL DEFAULT 0,
                    Day16 int unsigned NOT NULL DEFAULT 0,
                    Day17 int unsigned NOT NULL DEFAULT 0,
                    Day18 int unsigned NOT NULL DEFAULT 0,
                    Day19 int unsigned NOT NULL DEFAULT 0,
                    Day20 int unsigned NOT NULL DEFAULT 0,
                    Day21 int unsigned NOT NULL DEFAULT 0,
                    Day22 int unsigned NOT NULL DEFAULT 0,
                    Day23 int unsigned NOT NULL DEFAULT 0,
                    Day24 int unsigned NOT NULL DEFAULT 0,
                    Day25 int unsigned NOT NULL DEFAULT 0,
                    Day26 int unsigned NOT NULL DEFAULT 0,
                    Day27 int unsigned NOT NULL DEFAULT 0,
                    Day28 int unsigned NOT NULL DEFAULT 0,
                    Day29 int unsigned NOT NULL DEFAULT 0,
                    Day30 int unsigned NOT NULL DEFAULT 0,
                    Day31 int unsigned NOT NULL DEFAULT 0,
                    PRIMARY KEY (Year, Month))");

                CreateTable("system_weekly_rewards", R"(
                    Year smallint unsigned NOT NULL,
                    Week tinyint unsigned NOT NULL,
                    Day1 int unsigned NOT NULL DEFAULT 0,
                    Day2 int unsigned NOT NULL DEFAULT 0,
                    Day3 int unsigned NOT NULL DEFAULT 0,
                    Day4 int unsigned NOT NULL DEFAULT 0,
                    Day5 int unsigned NOT NULL DEFAULT 0,
                    Day6 int unsigned NOT NULL DEFAULT 0,
                    Day7 int unsigned NOT NULL DEFAULT 0,
                    PRIMARY KEY (Year, Week))");

                CreateTable("system_playtime_rewards", R"(
                    Year smallint unsigned NOT NULL,
                    Month tinyint unsigned NOT NULL,
                    Reward1 int unsigned NOT NULL DEFAULT 0,
                    Reward2 int unsigned NOT NULL DEFAULT 0,
                    Reward3 int unsigned NOT NULL DEFAULT 0,
                    PRIMARY KEY (Year, Month))");

                CreateTable("player_playtime", R"(
                    ID int unsigned NOT NULL AUTO_INCREMENT,
                    PlayerId int unsigned NOT NULL,
                    DailySeconds int unsigned NOT NULL DEFAULT 0,
                    ClaimedStage tinyint unsigned NOT NULL DEFAULT 0,
                    LastUpdate datetime(6) NOT NULL,
                    PRIMARY KEY (ID),
                    UNIQUE KEY UX_player_playtime_PlayerId (PlayerId),
                    CONSTRAINT FK_player_playtime_accounts_PlayerId FOREIGN KEY (PlayerId) REFERENCES accounts (Id) ON DELETE CASCADE)");

                // ── Battle Pass (MICROPASS) ──────────────────────────────────
                CreateTable("system_battlepass_season", R"(
                    Season int unsigned NOT NULL,
                    StartDate datetime(6) NOT NULL,
                    EndDate datetime(6) NOT NULL,
                    ResetBaseCost int unsigned NOT NULL DEFAULT 0,
                    PRIMARY KEY (Season))");

                CreateTable("system_battlepass_rewards", R"(
                    Season int unsigned NOT NULL,
                    Level int unsigned NOT NULL,
                    XpRequired int unsigned NOT NULL DEFAULT 0,
                    FreeItem int unsigned NOT NULL DEFAULT 0,
                    PremiumItem int unsigned NOT NULL DEFAULT 0,
                    PRIMARY KEY (Season, Level))");

                CreateTable("system_battlepass_missions", R"(
                    MissionId int unsigned NOT NULL,
                    Description varchar(255) NOT NULL DEFAULT '',
                    CriteriaType int unsigned NOT NULL DEFAULT 0,
                    CriteriaTarget int unsigned NOT NULL DEFAULT 0,
                    XpReward int unsigned NOT NULL DEFAULT 0,
                    PRIMARY KEY (MissionId))");

                CreateTable("player_battlepass", R"(
                    PlayerId int unsigned NOT NULL,
                    Season int unsigned NOT NULL DEFAULT 0,
                    Level int unsigned NOT NULL DEFAULT 1,
                    Xp int unsigned NOT NULL DEFAULT 0,
                    HasPremium tinyint unsigned NOT NULL DEFAULT 0,
                    ClaimedFree varchar(32) NOT NULL DEFAULT '00000000000000000000000000000000',
                    ClaimedPremium varchar(32) NOT NULL DEFAULT '00000000000000000000000000000000',
                    CurrentMissionId int unsigned NOT NULL DEFAULT 0,
                    MissionProgress int unsigned NOT NULL DEFAULT 0,
                    ResetCount int unsigned NOT NULL DEFAULT 0,
                    UNIQUE KEY UX_player_battlepass_PlayerId (PlayerId),
                    CONSTRAINT FK_player_battlepass_accounts_PlayerId FOREIGN KEY (PlayerId) REFERENCES accounts (Id) ON DELETE CASCADE)");

                // Default seed (idempotent): season 1 + 100 levels with every item slot
                // = 3010050, and a few starter missions. INSERT IGNORE = only fills when
                // rows are missing (effectively "only if table was just created").
                {
                    std::unique_ptr<sql::Statement> st(GetConnection()->createStatement());
                    st->execute("INSERT IGNORE INTO system_battlepass_season (Season, StartDate, EndDate, ResetBaseCost) "
                                "VALUES (1, UTC_TIMESTAMP(), DATE_ADD(UTC_TIMESTAMP(), INTERVAL 30 DAY), 100)");
                    std::string rows;
                    for (int lv = 1; lv <= 100; ++lv)
                    {
                        if (!rows.empty()) rows += ",";
                        rows += "(1," + std::to_string(lv) + ",5000,3010050,3010050)";
                    }
                    st->execute("INSERT IGNORE INTO system_battlepass_rewards (Season, Level, XpRequired, FreeItem, PremiumItem) VALUES " + rows);
                    st->execute("INSERT IGNORE INTO system_battlepass_missions (MissionId, Description, CriteriaType, CriteriaTarget, XpReward) VALUES "
                                "(1,'Get 30 Kills in FFA',0,30,2500),"
                                "(2,'Collect 5000 EXP',1,5000,2500),"
                                "(3,'Win 3 Matches',2,3,2500),"
                                "(4,'Play 5 Matches',3,5,1500)");
                }

                CreateTable("player_daily_mission", R"(
                    PlayerId int unsigned NOT NULL,
                    UpdateTime BIGINT UNSIGNED NOT NULL DEFAULT 0,
                    Mission1 int unsigned NOT NULL DEFAULT 0,
                    Mission2 int unsigned NOT NULL DEFAULT 0,
                    Mission3 int unsigned NOT NULL DEFAULT 0,
                    GoalMission1 int unsigned NOT NULL DEFAULT 0,
                    GoalMission2 int unsigned NOT NULL DEFAULT 0,
                    GoalMission3 int unsigned NOT NULL DEFAULT 0,
                    UNIQUE KEY IX_player_daily_mission_PlayerId (PlayerId),
                    CONSTRAINT FK_player_daily_mission_accounts_PlayerId FOREIGN KEY (PlayerId) REFERENCES accounts (Id) ON DELETE CASCADE)");

                CreateTable("news_posts", R"(
                    Id int unsigned NOT NULL AUTO_INCREMENT,
                    Title varchar(255) NOT NULL,
                    Category enum('announcements','events','patchnotes','promotions') NOT NULL,
                    Content text NOT NULL,
                    BannerUrl varchar(512) DEFAULT NULL,
                    Author varchar(50) NOT NULL,
                    CreatedAt datetime DEFAULT current_timestamp(),
                    PRIMARY KEY (Id),
                    KEY idx_category (Category),
                    KEY idx_date (CreatedAt))");

                CreateTable("otp_codes", R"(
                    Id int unsigned NOT NULL AUTO_INCREMENT,
                    AccountId int unsigned NOT NULL,
                    Code varchar(6) NOT NULL,
                    ExpiresAt datetime NOT NULL,
                    CreatedAt datetime DEFAULT current_timestamp(),
                    PRIMARY KEY (Id),
                    KEY idx_account (AccountId),
                    CONSTRAINT otp_codes_ibfk_1 FOREIGN KEY (AccountId) REFERENCES accounts (Id) ON DELETE CASCADE)");

                CreateTable("support_tickets", R"(
                    Id int unsigned NOT NULL AUTO_INCREMENT,
                    AccountId int unsigned NOT NULL,
                    Category enum('Account','Billing','Bugs','Cheater Report','Other') NOT NULL,
                    Subject varchar(100) NOT NULL,
                    Status enum('Open','Waiting for Admin','Waiting for User','Closed') NOT NULL DEFAULT 'Open',
                    CreatedAt datetime DEFAULT current_timestamp(),
                    PRIMARY KEY (Id),
                    KEY fk_ticket_owner (AccountId),
                    CONSTRAINT fk_ticket_owner FOREIGN KEY (AccountId) REFERENCES accounts (Id) ON DELETE CASCADE)");

                CreateTable("support_replies", R"(
                    Id int unsigned NOT NULL AUTO_INCREMENT,
                    TicketId int unsigned NOT NULL,
                    SenderId int unsigned NOT NULL,
                    Message text NOT NULL,
                    SentAt datetime DEFAULT current_timestamp(),
                    PRIMARY KEY (Id),
                    KEY fk_reply_ticket (TicketId),
                    KEY fk_reply_sender (SenderId),
                    CONSTRAINT fk_reply_sender FOREIGN KEY (SenderId) REFERENCES accounts (Id) ON DELETE CASCADE,
                    CONSTRAINT fk_reply_ticket FOREIGN KEY (TicketId) REFERENCES support_tickets (Id) ON DELETE CASCADE)");

                CreateTable("transactions", R"(
                    Id int unsigned NOT NULL AUTO_INCREMENT,
                    AccountId int unsigned NOT NULL,
                    TransactionNo varchar(100) DEFAULT NULL,
                    OrderNumber varchar(100) NOT NULL,
                    Amount decimal(10,2) NOT NULL,
                    Currency varchar(10) NOT NULL DEFAULT 'EUR',
                    TokensAmount int unsigned NOT NULL,
                    Status varchar(20) NOT NULL DEFAULT 'Pending',
                    PaymentUrl text DEFAULT NULL,
                    CreatedAt timestamp NULL DEFAULT current_timestamp(),
                    UpdatedAt timestamp NULL DEFAULT current_timestamp() ON UPDATE current_timestamp(),
                    PRIMARY KEY (Id),
                    UNIQUE KEY IX_OrderNumber (OrderNumber),
                    UNIQUE KEY IX_TransactionNo (TransactionNo),
                    KEY FK_Transactions_Account (AccountId),
                    CONSTRAINT FK_Transactions_Account FOREIGN KEY (AccountId) REFERENCES accounts (Id) ON DELETE CASCADE)");

                CreateTable("player_chatlogs", R"(
                    Id bigint unsigned NOT NULL AUTO_INCREMENT,
                    Aid int unsigned NOT NULL,
                    TargetAid int unsigned DEFAULT NULL,
                    ChatType enum('User','Whisper','Team','Clan','Command','Party') NOT NULL,
                    ChatLocation enum('Lobby','Room','Plaza') DEFAULT NULL,
                    ServerId int unsigned NOT NULL DEFAULT 0,
                    RoomId int unsigned DEFAULT NULL,
                    PlazaId int unsigned DEFAULT NULL,
                    ClanId int unsigned DEFAULT NULL,
                    Message varchar(256) NOT NULL,
                    CreatedAt datetime NOT NULL DEFAULT current_timestamp(),
                    PRIMARY KEY (Id),
                    KEY IX_chatlogs_Aid (Aid),
                    KEY IX_chatlogs_TargetAid (TargetAid),
                    KEY IX_chatlogs_ClanId (ClanId),
                    KEY IX_chatlogs_CreatedAt (CreatedAt),
                    CONSTRAINT FK_chatlogs_Aid FOREIGN KEY (Aid) REFERENCES accounts (Id) ON DELETE CASCADE,
                    CONSTRAINT FK_chatlogs_TargetAid FOREIGN KEY (TargetAid) REFERENCES accounts (Id) ON DELETE SET NULL,
                    CONSTRAINT FK_chatlogs_ClanId FOREIGN KEY (ClanId) REFERENCES clans (Id) ON DELETE SET NULL)");

                
                CreateTable("player_itemlogs", R"(
                    Id bigint unsigned NOT NULL AUTO_INCREMENT,
                    Aid int unsigned NOT NULL,
                    RelatedAid int unsigned DEFAULT NULL,
                    ActionType enum('Added','Deleted','Sold','Upgraded','Reset','Repaired','EnergyInjected','Gifted','Received') NOT NULL,
                    ItemId int unsigned NOT NULL,
                    ItemType enum('Hair','Face','Upper','Under','Pants','Hands','Boots','AccHead','AccWaist','AccBack','Melee','Rifle','Shotgun','Sniper','Gatling','Bazooka','Grenade','Set','ShieldEnamel','FlagBlue','Gatcha','Unknown1','Diorama1','Diorama2','Question1','MonsterFace','Unknown3','Unknown4') DEFAULT NULL,
                    SerialInfo bigint unsigned DEFAULT NULL,
                    OriginType enum('Shop','ShopCoupon','Gachapon','Package','BossBattle','Tutorial','Story','LevelUp','DailyMission','MonthlyReward','GiftSent','GiftReceived','GMSpawned','Pickup','Unknown') NOT NULL,
                    MpDelta int DEFAULT 0,
                    RtDelta int DEFAULT 0,
                    CouponDelta int DEFAULT 0,
                    EnergyDelta int DEFAULT 0,
                    NewItemId int unsigned DEFAULT NULL,
                    NewRepair smallint unsigned DEFAULT NULL,
                    CreatedAt datetime NOT NULL DEFAULT current_timestamp(),
                    PRIMARY KEY (Id),
                    KEY IX_itemlogs_Aid (Aid),
                    KEY IX_itemlogs_RelatedAid (RelatedAid),
                    KEY IX_itemlogs_ActionType (ActionType),
                    KEY IX_itemlogs_OriginType (OriginType),
                    KEY IX_itemlogs_CreatedAt (CreatedAt),
                    CONSTRAINT FK_itemlogs_Aid FOREIGN KEY (Aid) REFERENCES accounts (Id) ON DELETE CASCADE,
                    CONSTRAINT FK_itemlogs_RelatedAid FOREIGN KEY (RelatedAid) REFERENCES accounts (Id) ON DELETE SET NULL)");

                CreateTable("player_currencylogs", R"(
                    Id bigint unsigned NOT NULL AUTO_INCREMENT,
                    Aid int unsigned NOT NULL,
                    CurrencyType enum('MP','RT','Coupons','Energy') NOT NULL,
                    Amount int NOT NULL,
                    BeforeValue bigint unsigned NOT NULL,
                    AfterValue bigint unsigned NOT NULL,
                    SourceType enum('Shop','ShopCoupon','Gachapon','Package','ItemSell','ItemRepair','ItemUpgrade','BossBattle','Tutorial','LevelUp','Achievement','DailyMission','MonthlyReward','GiftSend','VoteKick','MatchReward','Admin','Unknown') NOT NULL,
                    RelatedItemId int unsigned DEFAULT NULL,
                    CreatedAt datetime NOT NULL DEFAULT current_timestamp(),
                    PRIMARY KEY (Id),
                    KEY IX_currencylogs_Aid (Aid),
                    KEY IX_currencylogs_CurrencyType (CurrencyType),
                    KEY IX_currencylogs_SourceType (SourceType),
                    KEY IX_currencylogs_CreatedAt (CreatedAt),
                    CONSTRAINT FK_currencylogs_Aid FOREIGN KEY (Aid) REFERENCES accounts (Id) ON DELETE CASCADE)");

                CreateTable("player_roomlogs", R"(
                    Id bigint unsigned NOT NULL AUTO_INCREMENT,
                    Aid int unsigned NOT NULL,
                    TargetAid int unsigned DEFAULT NULL,
                    EventType enum('RoomCreated','RoomJoined','RoomLeft','RoomKicked','TeamChanged','VoteKickStarted','VoteKickAgreed','VoteKickSucceeded','VoteKickFailed','MatchStarted','MatchEntered','MatchLeft','MapChanged','ModeChanged','ScoreRuleChanged','TimeRuleChanged','MaxPlayersChanged') NOT NULL,
                    ServerId int unsigned NOT NULL DEFAULT 0,
                    RoomId int unsigned NOT NULL,
                    HostAid int unsigned DEFAULT NULL,
                    TeamId tinyint unsigned DEFAULT NULL,
                    NewTeamId tinyint unsigned DEFAULT NULL,
                    VoteKickReason tinyint unsigned DEFAULT NULL,
                    OldValue int DEFAULT NULL,
                    NewValue int DEFAULT NULL,
                    CreatedAt datetime NOT NULL DEFAULT current_timestamp(),
                    PRIMARY KEY (Id),
                    KEY IX_roomlogs_Aid (Aid),
                    KEY IX_roomlogs_TargetAid (TargetAid),
                    KEY IX_roomlogs_EventType (EventType),
                    KEY IX_roomlogs_RoomId (RoomId),
                    KEY IX_roomlogs_CreatedAt (CreatedAt),
                    CONSTRAINT FK_roomlogs_Aid FOREIGN KEY (Aid) REFERENCES accounts (Id) ON DELETE CASCADE,
                    CONSTRAINT FK_roomlogs_TargetAid FOREIGN KEY (TargetAid) REFERENCES accounts (Id) ON DELETE SET NULL,
                    CONSTRAINT FK_roomlogs_HostAid FOREIGN KEY (HostAid) REFERENCES accounts (Id) ON DELETE SET NULL)");

                // Existing DBs: extend the EventType enum with the match-lifecycle + setting-change values.
                try
                {
                    std::unique_ptr<sql::Statement> alter_roomlogs_stmt(conn->createStatement());
                    alter_roomlogs_stmt->execute(
                        "ALTER TABLE `player_roomlogs` MODIFY COLUMN `EventType` "
                        "enum('RoomCreated','RoomJoined','RoomLeft','RoomKicked','TeamChanged','VoteKickStarted','VoteKickAgreed','VoteKickSucceeded','VoteKickFailed','MatchStarted','MatchEntered','MatchLeft','MapChanged','ModeChanged','ScoreRuleChanged','TimeRuleChanged','MaxPlayersChanged') NOT NULL"
                    );
                }
                catch (const sql::SQLException& e)
                {
                    DEBUGLOG(yellow, "player_roomlogs EventType migration failed: {}", e.what());
                }

                // Existing DBs: add the generic old/new value columns for setting-change rows.
                for (const char* col : { "OldValue", "NewValue" })
                {
                    try
                    {
                        std::unique_ptr<sql::Statement> add_col_stmt(conn->createStatement());
                        add_col_stmt->execute(std::string("ALTER TABLE `player_roomlogs` ADD COLUMN `") + col + "` int DEFAULT NULL");
                    }
                    catch (const sql::SQLException&) { /* column already exists */ }
                }

                // Per-match session spans (join/leave) keyed by the match unique id.
                CreateTable("player_match_sessions", R"(
                    Id bigint unsigned NOT NULL AUTO_INCREMENT,
                    MatchUniqueId varchar(128) NOT NULL,
                    Aid int unsigned NOT NULL,
                    TeamId tinyint unsigned NOT NULL DEFAULT 0,
                    JoinedMs bigint unsigned NOT NULL,
                    LeftMs bigint unsigned NOT NULL,
                    Reason enum('Finished','Leave','Kicked','Disconnect') NOT NULL,
                    CreatedAt datetime NOT NULL DEFAULT current_timestamp(),
                    PRIMARY KEY (Id),
                    KEY IX_match_sessions_MatchUniqueId (MatchUniqueId),
                    KEY IX_match_sessions_Aid (Aid),
                    CONSTRAINT FK_match_sessions_Aid FOREIGN KEY (Aid) REFERENCES accounts (Id) ON DELETE CASCADE)");

                // Per-hit combat log keyed by the match unique id: powers the website's
                // accuracy breakdown (head/body/arms/legs) and the kill/death timeline.
                CreateTable("player_match_combat", R"(
                    Id bigint unsigned NOT NULL AUTO_INCREMENT,
                    MatchUniqueId varchar(128) NOT NULL,
                    AttackerAid int unsigned NOT NULL,
                    VictimAid int unsigned NOT NULL,
                    Weapon tinyint unsigned NOT NULL DEFAULT 0,
                    BodyPart tinyint unsigned NOT NULL DEFAULT 0,
                    HitVariant tinyint unsigned NOT NULL DEFAULT 255,
                    Damage int unsigned NOT NULL DEFAULT 0,
                    VictimHpAfter int unsigned NOT NULL DEFAULT 0,
                    IsKill tinyint unsigned NOT NULL DEFAULT 0,
                    EventMs bigint unsigned NOT NULL DEFAULT 0,
                    CreatedAt datetime NOT NULL DEFAULT current_timestamp(),
                    PRIMARY KEY (Id),
                    KEY IX_match_combat_MatchUniqueId (MatchUniqueId),
                    KEY IX_match_combat_AttackerAid (AttackerAid),
                    KEY IX_match_combat_VictimAid (VictimAid),
                    CONSTRAINT FK_match_combat_Attacker FOREIGN KEY (AttackerAid) REFERENCES accounts (Id) ON DELETE CASCADE,
                    CONSTRAINT FK_match_combat_Victim FOREIGN KEY (VictimAid) REFERENCES accounts (Id) ON DELETE CASCADE)");

                // Per-match non-combat timeline events: respawns, bomb plant/defuse
                // progress, and item pickups. EventType: 1 Respawn, 2 Bomb, 3 ItemPickup.
                // For Bomb: SubA = role (0 defuser, 1 planter), SubB = phase (0 start, 1 stop, 2 finish).
                // For ItemPickup: Value = item id.
                CreateTable("player_match_events", R"(
                    Id bigint unsigned NOT NULL AUTO_INCREMENT,
                    MatchUniqueId varchar(128) NOT NULL,
                    Aid int unsigned NOT NULL,
                    EventType tinyint unsigned NOT NULL DEFAULT 0,
                    SubA tinyint unsigned NOT NULL DEFAULT 0,
                    SubB tinyint unsigned NOT NULL DEFAULT 0,
                    Value int unsigned NOT NULL DEFAULT 0,
                    EventMs bigint unsigned NOT NULL DEFAULT 0,
                    CreatedAt datetime NOT NULL DEFAULT current_timestamp(),
                    PRIMARY KEY (Id),
                    KEY IX_match_events_MatchUniqueId (MatchUniqueId),
                    KEY IX_match_events_Aid (Aid),
                    KEY IX_match_events_EventType (EventType),
                    CONSTRAINT FK_match_events_Aid FOREIGN KEY (Aid) REFERENCES accounts (Id) ON DELETE CASCADE)");

                {
                    auto checkStmt = conn->prepareStatement(
                        "SELECT 1 FROM information_schema.COLUMNS WHERE TABLE_SCHEMA = DATABASE() "
                        "AND TABLE_NAME = 'player_gacha_pity' AND COLUMN_NAME = 'GachaId' LIMIT 1");
                    auto checkRes = checkStmt->executeQuery();
                    if (checkRes->next())
                    {
                        conn->createStatement()->executeUpdate("DROP TABLE player_gacha_pity");
                        DEBUGLOG(dark_cyan, "dropped legacy player_gacha_pity table (per-id -> per-type migration)");
                    }
                }
                CreateTable("player_gacha_pity", R"(
                    PlayerId int unsigned NOT NULL,
                    GachaType int unsigned NOT NULL,
                    LuckyPoints int unsigned NOT NULL DEFAULT 0,
                    PRIMARY KEY (PlayerId, GachaType),
                    CONSTRAINT FK_gacha_pity_accounts FOREIGN KEY (PlayerId) REFERENCES accounts (Id) ON DELETE CASCADE
                )");

                CreateTable("ac_detections", R"(
                    Id bigint unsigned NOT NULL AUTO_INCREMENT,
                    Aid int unsigned NOT NULL,
                    Ip varchar(45) NOT NULL,
                    Hwid varchar(64) NOT NULL,
                    DetectionFlag enum('None','DebuggerPresent','DebugPort','TimingAnomaly','PebDebugFlag','InlineHook','IatHook','HoneypotTriggered','IntegrityViolation','DllInjection','ManualMap','AnonymousThread','ProxyDll','GlobalHookInjection','MappedImage','HookIntegrity','BlacklistedModule','UnsignedModule','DangerousHandle','VulnerableDriver','BlacklistedString','BlacklistedSignature','UnsignedDriver','DriverBlocklistDisabled','HvciDisabled','FileIntegrityFail','MatchKillMismatch','LoginSpam','HeartbeatTimeout','InvalidResponse','UnknownFlag') NOT NULL,
                    Extra int unsigned NOT NULL DEFAULT 0,
                    Details varchar(64) NOT NULL DEFAULT '',
                    ServerId int unsigned NOT NULL DEFAULT 0,
                    CreatedAt datetime NOT NULL DEFAULT current_timestamp(),
                    PRIMARY KEY (Id),
                    KEY IX_ac_detections_Aid (Aid),
                    KEY IX_ac_detections_Flag (DetectionFlag),
                    KEY IX_ac_detections_CreatedAt (CreatedAt),
                    CONSTRAINT FK_ac_detections_Aid FOREIGN KEY (Aid) REFERENCES accounts (Id) ON DELETE CASCADE)");

                try
                {
                    std::unique_ptr<sql::Statement> alter_ac_detections_stmt(conn->createStatement());
                    alter_ac_detections_stmt->execute(
                        "ALTER TABLE `ac_detections` MODIFY COLUMN `DetectionFlag` "
                        "enum('None','DebuggerPresent','DebugPort','TimingAnomaly','PebDebugFlag','InlineHook','IatHook','HoneypotTriggered','IntegrityViolation','DllInjection','ManualMap','AnonymousThread','ProxyDll','GlobalHookInjection','MappedImage','HookIntegrity','BlacklistedModule','UnsignedModule','DangerousHandle','VulnerableDriver','BlacklistedString','BlacklistedSignature','UnsignedDriver','DriverBlocklistDisabled','HvciDisabled','FileIntegrityFail','MatchKillMismatch','LoginSpam','HeartbeatTimeout','InvalidResponse','UnknownFlag') NOT NULL"
                    );
                }
                catch (const sql::SQLException& e)
                {
                    DEBUGLOG(yellow, "ac_detections DetectionFlag migration failed: {}", e.what());
                }

                CreateTable("ac_auth_history", R"(
                    Id bigint unsigned NOT NULL AUTO_INCREMENT,
                    Aid int unsigned NOT NULL,
                    Ip varchar(45) NOT NULL,
                    Hwid varchar(128) NOT NULL,
                    ServerId int unsigned NOT NULL DEFAULT 0,
                    CreatedAt datetime NOT NULL DEFAULT current_timestamp(),
                    PRIMARY KEY (Id),
                    KEY IX_ac_auth_history_Aid (Aid),
                    KEY IX_ac_auth_history_CreatedAt (CreatedAt),
                    CONSTRAINT FK_ac_auth_history_Aid FOREIGN KEY (Aid) REFERENCES accounts (Id) ON DELETE CASCADE)");

                CreateTable("player_misc", R"(
                    AccountId int unsigned NOT NULL,
                    IsInvisible tinyint(1) NOT NULL DEFAULT 0,
                    PRIMARY KEY (AccountId),
                    CONSTRAINT FK_player_misc_account FOREIGN KEY (AccountId) REFERENCES accounts (Id) ON DELETE CASCADE)");

                // Create default admin account if it doesn't exist
                try
                {
                    std::unique_ptr<sql::PreparedStatement> checkAdmin(conn->prepareStatement(
                        "SELECT Id FROM accounts WHERE Username = 'admin' LIMIT 1"
                    ));
                    std::unique_ptr<sql::ResultSet> adminResult(checkAdmin->executeQuery());

                    if (!adminResult->next())
                    {
                        const std::string adminUsername = "admin";
                        const std::string adminPassword = "admin123";
                        const std::string adminNickname = "admin";
                        const uint8_t adminGrade = 4;

                        thread_local Utility::SecureRandomBlake2b::Generator rng;
                        uint64_t salt_part1 = rng.GenerateAuthKey();
                        uint64_t salt_part2 = rng.GenerateAuthKey();
                        uint8_t salt[16];
                        std::memcpy(salt, &salt_part1, 8);
                        std::memcpy(salt + 8, &salt_part2, 8);

                        uint8_t hash[32];
                        if (Utility::HashPassword(adminPassword, salt, hash))
                        {
                            std::string salt_b64 = Utility::Base64::to_base64(std::string_view(reinterpret_cast<char*>(salt), 16));
                            std::string hash_b64 = Utility::Base64::to_base64(std::string_view(reinterpret_cast<char*>(hash), 32));

                            std::unique_ptr<sql::PreparedStatement> insertAdmin(conn->prepareStatement(
                                "INSERT INTO accounts (Username, Password, Salt, Nickname, Grade, Level, MaximumEnergy) "
                                "VALUES (?, ?, ?, ?, ?, 0, 1000)"
                            ));
                            insertAdmin->setString(1, adminUsername);
                            insertAdmin->setString(2, hash_b64);
                            insertAdmin->setString(3, salt_b64);
                            insertAdmin->setString(4, adminNickname);
                            insertAdmin->setUInt(5, adminGrade);

                            if (insertAdmin->executeUpdate())
                                DEBUGLOG(green, "Created default admin account (username: admin, password: admin123, grade: 4)");
                            else
                                DEBUGLOG(red, "Failed to create default admin account");
                        }
                        else
                        {
                            DEBUGLOG(red, "Failed to hash password for default admin account");
                        }
                    }
                    else
                    {
                        DEBUGLOG(dark_cyan, "Default admin account already exists");
                    }
                }
                catch (sql::SQLException& e)
                {
                    DEBUGLOG(red, "Error checking/creating default admin account: {}", e.what());
                }
            }
        }
        catch (sql::SQLException& e)
        {
			DEBUGLOG(red, "exception: ({})", e.what());
        }
    }

    bool CMariaDatabase::CreateTable(const std::string& table_name, const std::string& data_columns)
    {
        try
        {
            if (!conn || !conn->isValid())
            {
                DEBUGLOG(yellow, "Reconnecting to the database...");
                conn = driver->connect(this->properties);
                if (!conn || !conn->isValid())
                {
                    DEBUGLOG(red, "Failed to reconnect to database for table ({})", table_name);
                    return false;
                }
                conn->setSchema(database_name);
                DEBUGLOG(dark_cyan, "Successfully reconnected to database");
            }

            if (database_name.empty())
            {
                DEBUGLOG(red, "Database name is empty, cannot create table ({})", table_name);
                return false;
            }

            std::unique_ptr<sql::PreparedStatement> check_stmt(conn->prepareStatement("SHOW TABLES LIKE ?"));
            check_stmt->setString(1, table_name);
            std::unique_ptr<sql::ResultSet> res(check_stmt->executeQuery());

            bool table_exists = res->next();

            if (!table_exists)
            {
                std::string create_query = "CREATE TABLE `" + table_name + "` (" + data_columns + ")";
                std::unique_ptr<sql::Statement> create_stmt(conn->createStatement());
                create_stmt->execute(create_query);
                DEBUGLOG(green, "Created table: ({})", table_name);
                return true;
            }

            // Table exists - check for missing columns
            std::unique_ptr<sql::PreparedStatement> col_stmt(conn->prepareStatement(
                "SELECT COLUMN_NAME FROM INFORMATION_SCHEMA.COLUMNS WHERE TABLE_SCHEMA = ? AND TABLE_NAME = ?"));
            col_stmt->setString(1, database_name);
            col_stmt->setString(2, table_name);
            std::unique_ptr<sql::ResultSet> col_res(col_stmt->executeQuery());

            boost::unordered_flat_set<std::string> existing_columns;
            while (col_res->next())
            {
                std::string col_name = col_res->getString("COLUMN_NAME").c_str();
                std::transform(col_name.begin(), col_name.end(), col_name.begin(), ::tolower);
                existing_columns.insert(col_name);
            }

            std::vector<std::pair<std::string, std::string>> desired_columns;
            int paren_depth = 0;
            std::string current_def;

            for (char c : data_columns)
            {
                if (c == '(') paren_depth++;
                else if (c == ')') paren_depth--;

                if (c == ',' && paren_depth == 0)
                {
                    if (!current_def.empty())
                    {
                        size_t start = current_def.find_first_not_of(" \t\n\r");
                        size_t end = current_def.find_last_not_of(" \t\n\r");
                        if (start != std::string::npos)
                        {
                            std::string trimmed = current_def.substr(start, end - start + 1);
                            std::string upper = trimmed;
                            std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);
                            if (upper.find("PRIMARY KEY") != 0 &&
                                upper.find("KEY ") != 0 &&
                                upper.find("CONSTRAINT") != 0 &&
                                upper.find("UNIQUE KEY") != 0 &&
                                upper.find("INDEX") != 0 &&
                                upper.find("FOREIGN KEY") != 0)
                            {
                                size_t space_pos = trimmed.find_first_of(" \t");
                                if (space_pos != std::string::npos)
                                {
                                    std::string col_name = trimmed.substr(0, space_pos);
                                    std::transform(col_name.begin(), col_name.end(), col_name.begin(), ::tolower);
                                    desired_columns.emplace_back(col_name, trimmed);
                                }
                            }
                        }
                    }
                    current_def.clear();
                }
                else
                {
                    current_def += c;
                }
            }

            // Handle last column definition
            if (!current_def.empty())
            {
                size_t start = current_def.find_first_not_of(" \t\n\r");
                size_t end = current_def.find_last_not_of(" \t\n\r");
                if (start != std::string::npos)
                {
                    std::string trimmed = current_def.substr(start, end - start + 1);
                    std::string upper = trimmed;
                    std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);
                    if (upper.find("PRIMARY KEY") != 0 &&
                        upper.find("KEY ") != 0 &&
                        upper.find("CONSTRAINT") != 0 &&
                        upper.find("UNIQUE KEY") != 0 &&
                        upper.find("INDEX") != 0 &&
                        upper.find("FOREIGN KEY") != 0)
                    {
                        size_t space_pos = trimmed.find_first_of(" \t");
                        if (space_pos != std::string::npos)
                        {
                            std::string col_name = trimmed.substr(0, space_pos);
                            std::transform(col_name.begin(), col_name.end(), col_name.begin(), ::tolower);
                            desired_columns.emplace_back(col_name, trimmed);
                        }
                    }
                }
            }

            // Add missing columns
            for (const auto& [col_name, col_def] : desired_columns)
            {
                if (existing_columns.find(col_name) == existing_columns.end())
                {
                    try
                    {
                        std::string alter_query = "ALTER TABLE `" + table_name + "` ADD COLUMN " + col_def;
                        std::unique_ptr<sql::Statement> alter_stmt(conn->createStatement());
                        alter_stmt->execute(alter_query);
                        DEBUGLOG(green, "Added column '{}' to table ({})", col_name, table_name);
                    }
                    catch (sql::SQLException& e)
                    {
                        DEBUGLOG(red, "Failed to add column '{}' to table ({}): {}", col_name, table_name, e.what());
                    }
                }
            }

            return true;
        }
        catch (sql::SQLException& e)
        {
            DEBUGLOG(red, "CreateTable '{}' exception: ({}) - SQLState: {}", table_name, e.what(), e.getSQLState().c_str());
            return false;
        }
    }

    bool CMariaDatabase::CreateDatabase(const std::string& name)
    {
        try
        {
            if (!conn || !conn->isValid())
            {
                DEBUGLOG(yellow, "Reconnecting to the database...");
                conn = driver->connect(this->properties);
                if (conn)
                    DEBUGLOG(dark_cyan, "Successfully reconnected to database");
            }

            std::unique_ptr<sql::Statement> stmt(conn->createStatement());
            return !stmt->execute(std::format("CREATE DATABASE {0}", name.c_str()));
        }
        catch (sql::SQLException& e)
        {
            if (!std::string_view(e.what()).contains("database exists"))
                DEBUGLOG(red, "exception=({})", e.what());

            return false;
        }
    }

    std::string CMariaDatabase::GenerateQuestionMarks(size_t n)
    {
        if (n == 0) return {};
        std::string result;
        result.reserve(n * 3);
        for (size_t i = 0; i < n; i++)
            result += (i > 0) ? ", ?" : "?";
        return result;
    }
    std::string CMariaDatabase::GenerateQuestionMarks(size_t rows, size_t cols)
    {
        if (rows == 0 || cols == 0) return {};
        auto one_row = "(" + GenerateQuestionMarks(cols) + ")";
        std::string result;
        result.reserve(rows * one_row.size());
        for (size_t i = 0; i < rows; i++)
            result += (i > 0 ? ", " : "") + one_row;
        return result;
    }
    std::string CMariaDatabase::GenerateInTuples(size_t rows, size_t cols)
    {
        if (rows == 0 || cols == 0) return {};
        auto one_row = "(" + GenerateQuestionMarks(cols) + ")";
        std::string result;
        result.reserve(rows * one_row.size() + 2); // +2 for outer ()
        result += "("; 
        for (size_t i = 0; i < rows; i++)
            result += (i > 0 ? ", " : "") + one_row;
        result += ")";
        return result;
    }
    std::string CMariaDatabase::GenerateJoinedString(const std::vector<std::string>& vec, const std::string& delim)
    {
        if (vec.empty()) return {};
        size_t total = (vec.size() - 1) * delim.size();
        for (const auto& s : vec) total += s.size();
        std::string result;
        result.reserve(total);
        result += vec[0];
        for (size_t i = 1; i < vec.size(); i++) 
        {
            result += delim;
            result += vec[i];
        }
        return result;
    }

    // Bumped whenever the calling thread's connection is replaced or reconnected,
    // so Prep() knows its cached server-side statements are no longer valid.
    static thread_local uint64_t tl_conn_generation = 0;

    // Set while a transaction is open on the calling thread's connection. While set,
    // GetConnection() must NOT ping/reconnect/replace the connection: swapping it would
    // orphan the open transaction with its row/metadata locks still held server-side,
    // freezing every other DB worker and the other servers that share this database
    // (which presents as "all servers frozen forever" until a manual restart).
    static thread_local bool tl_in_transaction = false;

    namespace
    {
        // RAII: marks the calling thread as inside a transaction for the lifetime of the
        // object. Place one immediately after "START TRANSACTION" so it stays active across
        // every statement up to COMMIT/ROLLBACK and is cleared on any exit path (return,
        // exception unwinding).
        struct TransactionGuard
        {
            TransactionGuard() noexcept { tl_in_transaction = true; }
            ~TransactionGuard() { tl_in_transaction = false; }
            TransactionGuard(const TransactionGuard&) = delete;
            TransactionGuard& operator=(const TransactionGuard&) = delete;
        };
    }

    sql::Connection* CMariaDatabase::GetConnection()
    {
        thread_local sql::Connection* tl_conn = nullptr;

        auto create_connection = [&]() -> bool
        {
            DEBUGLOG(yellow, "Creating database connection for thread...");
            delete tl_conn;
            tl_conn = nullptr;
            tl_conn = driver->connect(this->properties);
            tl_conn_generation++;
            if (tl_conn && tl_conn->isValid())
            {
                tl_conn->setSchema(database_name);
                DEBUGLOG(dark_cyan, "Thread-local DB connection established");
                return true;
            }
            DEBUGLOG(red, "Failed to create thread-local DB connection");
            return false;
        };

        if (!tl_conn)
        {
            create_connection();
            return tl_conn;
        }

        // Never revalidate or reconnect while a transaction is open on this connection:
        // replacing it would abandon the in-flight transaction with its locks held until
        // the DB's wait_timeout reaps the orphaned connection. A genuinely dead connection
        // here instead surfaces as a failing statement -> exception -> rollback, which is
        // the correct outcome (a transaction cannot survive a reconnect anyway).
        if (tl_in_transaction)
            return tl_conn;

        // isValid(1) sends an actual ping to the server with 1 second timeout,
        // detecting connections killed by server-side wait_timeout.
        if (!tl_conn->isValid(1))
        {
            DEBUGLOG(yellow, "DB connection stale, attempting reconnect...");
            try
            {
                if (tl_conn->reconnect())
                {
                    DEBUGLOG(dark_cyan, "DB reconnect succeeded");
                    tl_conn_generation++;
                    return tl_conn;
                }
            }
            catch (...) {}
            create_connection();
        }
        return tl_conn;
    }

    sql::PreparedStatement* CMariaDatabase::Prep(const std::string& sql_text)
    {
        thread_local boost::unordered_flat_map<std::string, std::unique_ptr<sql::PreparedStatement>> tl_stmt_cache;
        thread_local uint64_t tl_cache_generation = ~0ull;

        auto* connection = GetConnection();
        if (!connection)
            throw sql::SQLException("No database connection");

        if (tl_cache_generation != tl_conn_generation)
        {
            tl_stmt_cache.clear();
            tl_cache_generation = tl_conn_generation;
        }

        if (auto it = tl_stmt_cache.find(sql_text); it != tl_stmt_cache.end())
            return it->second.get();

        // Bound growth from dynamically generated SQL (multi-row inserts etc.).
        if (tl_stmt_cache.size() >= 512)
            tl_stmt_cache.clear();

        auto stmt = std::unique_ptr<sql::PreparedStatement>(connection->prepareStatement(sql_text));
        auto* raw = stmt.get();
        tl_stmt_cache.emplace(sql_text, std::move(stmt));
        return raw;
    }

    std::expected<void, DbError> CMariaDatabase::EnsureConnected()
    {
        auto* c = GetConnection();
        if (c && c->isValid()) return {};
        return std::unexpected(DbError{ DbError::Type::ConnectionLost, 0, {}, "No database connection" });
    }
    std::expected<void, DbError> CMariaDatabase::PersistCurrenciesPatches(ValidatedDbUpdates& v)
    {
        try
        {
            if (v.currency_updates.empty()) return {};
            struct Accum { uint32_t rewards{0}, costs{0}; };
            Accum mp{}, rt{}, cp{}, en{};
            using enum CurrencyType;
            std::vector<AccountCurrencyDelta> nets;
            for (const auto& cur : v.currency_updates) 
            {
                auto& bucket = (cur.type==MP) ? mp
                    : (cur.type==RT) ? rt
                    : (cur.type==COUPONS) ? cp
                    : en;
                (cur.is_reward) ? bucket.rewards += cur.value : bucket.costs += cur.value;
            }
            auto finalize = [&](Accum a, CurrencyType type) 
            {
                if (a.rewards == 0 && a.costs == 0) return;
                if (a.rewards >= a.costs) 
                {
                    auto v = a.rewards - a.costs;
                    if (v > 0) nets.push_back(AccountCurrencyDelta{type, v, true});
                } else 
                {
                    auto v = a.costs - a.rewards;
                    if (v > 0) nets.push_back(AccountCurrencyDelta{type, v, false});
                }
            };
            finalize(mp, MP);
            finalize(rt, RT);
            finalize(cp, COUPONS);
            finalize(en, ENERGY);
            if (nets.empty()) return {};
            std::vector<std::string> set_parts;
            std::vector<std::string> guards;
            set_parts.reserve(nets.size());
            auto column_for = [](CurrencyType t) -> const char* 
            {
                using enum CurrencyType;
                switch (t) 
                {
                    case MP:      return "MicroPoints";
                    case RT:      return "RockTokens";
                    case COUPONS: return "Coupons";
                    case ENERGY:  return "Energy";
                }
                return "MicroPoints"; 
            };
            for (const auto& n : nets) 
            {
                const char* col = column_for(n.type);
                set_parts.push_back(std::string(col) + " = " + col + (n.is_reward ? " + ?" : " - ?"));
                if (!n.is_reward) guards.push_back(std::string(col) + " >= ?");
            }
            std::string sql = "UPDATE accounts SET " + GenerateJoinedString(set_parts, ", ") + " WHERE Id = ?" + (guards.empty() ? "" : " AND " + GenerateJoinedString(guards, " AND "));
            auto* ps = Prep(sql);
            unsigned idx = 1;
            for (const auto& n : nets) ps->setUInt(idx++, n.value);
            ps->setUInt(idx++, v.aid);
            for (const auto& n : nets) if (!n.is_reward) ps->setUInt(idx++, n.value);
            if (!ps->executeUpdate())  
                return std::unexpected(DbError{ DbError::Type::NoRowsAffected, 0, {}, "No rows affected" });
            DEBUGLOG(green, "Updated account {} currency", v.aid);
            return {};
		}
		catch (sql::SQLException& e)
		{
			DEBUGLOG(red, "PersistCurrenciesPatches sql exception: ({})", e.what());
            return std::unexpected(CMariaDatabase::FromSQLException(e));
		}
    }
    std::expected<void, DbError> CMariaDatabase::PersistAccountInfoPatches(ValidatedDbUpdates& v)
    {
        try 
        {
            if (v.acc_info_patches.empty()) return {};
            std::optional<uint32_t> server_id;
			std::optional<uint32_t> sw_daily_attempts;
            std::optional<uint32_t> sw_high_score;
            std::optional<uint32_t> sw_highest_wave;
            std::optional<uint64_t> sw_last_update;
            std::optional<uint32_t> clan_kills;
            std::optional<uint32_t> clan_deaths;
            std::optional<uint32_t> clan_assists;
            std::optional<uint32_t> clan_contribution;
            std::optional<uint32_t> clan_wins;
            std::optional<uint32_t> clan_loses;
            std::optional<uint32_t> clan_draws;
            std::optional<uint32_t> selected_character;
            std::optional<uint64_t> play_time;
            std::optional<uint8_t> story;
            std::optional<uint64_t> achievement;
            std::optional<uint64_t> voice;
            std::optional<bool> tutorial;
            std::optional<uint8_t> guide;
            std::optional<uint32_t> exp;
			std::optional<uint32_t> max_items;
			std::optional<uint32_t> max_energy;
            std::optional<uint32_t> lvl;
            std::optional<uint32_t> lucky;
            std::optional<uint32_t> wins;
            std::optional<uint32_t> loses;
            std::optional<uint32_t> draws;
            std::optional<uint32_t> kills;
            std::optional<uint32_t> deaths;
            std::optional<uint32_t> assists;
            std::optional<uint32_t> headshots;
            std::optional<uint32_t> highest_kill_streak;
            std::optional<uint32_t> melee_kills;
            std::optional<uint32_t> rifle_kills;
            std::optional<uint32_t> shotgun_kills;
            std::optional<uint32_t> sniper_kills;
            std::optional<uint32_t> gatling_kills;
            std::optional<uint32_t> bazooka_kills;
            std::optional<uint32_t> grenade_kills;
            std::optional<uint32_t> zombie_kills;
            std::optional<uint32_t> infections;
            std::optional<std::string> nickname;
            //merge all patches into a single update
            for (const auto& p : v.acc_info_patches)
            {
				if (p.server_id.has_value()) server_id = p.server_id.value();
                if (p.sw_daily_attempts.has_value()) sw_daily_attempts = p.sw_daily_attempts.value();
				if (p.sw_high_score.has_value()) sw_high_score = p.sw_high_score.value();
				if (p.sw_highest_wave.has_value()) sw_highest_wave = p.sw_highest_wave.value();
				if (p.sw_last_update.has_value()) sw_last_update = p.sw_last_update.value();
				if (p.clan_kills.has_value()) clan_kills = p.clan_kills.value();
				if (p.clan_deaths.has_value()) clan_deaths = p.clan_deaths.value();
				if (p.clan_assists.has_value()) clan_assists = p.clan_assists.value();
				if (p.clan_contribution.has_value()) clan_contribution = p.clan_contribution.value();
				if (p.clan_wins.has_value()) clan_wins = p.clan_wins.value();
				if (p.clan_loses.has_value()) clan_loses = p.clan_loses.value();
				if (p.clan_draws.has_value()) clan_draws = p.clan_draws.value();
				if (p.selected_character.has_value()) selected_character = p.selected_character.value();
				if (p.play_time.has_value()) play_time = p.play_time.value();
				if (p.story.has_value()) story = p.story.value();
                if (p.achievement_tier1.has_value()) achievement = p.achievement_tier1.value();
                if (p.voice_type.has_value()) voice = p.voice_type.value();
                if (p.bTutorial.has_value()) tutorial = p.bTutorial.value();
                if (p.guide_mission.has_value()) guide = p.guide_mission.value();
				if (p.experience.has_value()) exp = p.experience.value();
				if (p.maximum_items.has_value()) max_items = p.maximum_items.value();
				if (p.maximum_energy.has_value()) max_energy = p.maximum_energy.value();
				if (p.level.has_value()) lvl = p.level.value();
				if (p.lucky_points.has_value()) lucky = p.lucky_points.value();
				if (p.wins.has_value()) wins = p.wins.value();
				if (p.loses.has_value()) loses = p.loses.value();
				if (p.draws.has_value()) draws = p.draws.value();
				if (p.kills.has_value()) kills = p.kills.value();
				if (p.deaths.has_value()) deaths = p.deaths.value();
				if (p.assists.has_value()) assists = p.assists.value();
				if (p.headshots.has_value()) headshots = p.headshots.value();
				if (p.highest_kill_streak.has_value()) highest_kill_streak = p.highest_kill_streak.value();
				if (p.melee_kills.has_value()) melee_kills = p.melee_kills.value();
				if (p.rifle_kills.has_value()) rifle_kills = p.rifle_kills.value();
				if (p.shotgun_kills.has_value()) shotgun_kills = p.shotgun_kills.value();
				if (p.sniper_kills.has_value()) sniper_kills = p.sniper_kills.value();
				if (p.gatling_kills.has_value()) gatling_kills = p.gatling_kills.value();
				if (p.bazooka_kills.has_value()) bazooka_kills = p.bazooka_kills.value();
				if (p.grenade_kills.has_value()) grenade_kills = p.grenade_kills.value();
				if (p.zombie_kills.has_value()) zombie_kills = p.zombie_kills.value();
				if (p.infections.has_value()) infections = p.infections.value();
				if (p.nickname.has_value()) nickname = p.nickname.value();
            }
			if (!server_id.has_value()&&
                !sw_daily_attempts.has_value()&&
                !sw_high_score.has_value()&&
                !sw_highest_wave.has_value()&&
                !sw_last_update.has_value()&&
                !clan_kills.has_value()&&
                !clan_deaths.has_value()&&
                !clan_assists.has_value()&&
                !clan_contribution.has_value()&&
                !clan_wins.has_value()&&
                !clan_loses.has_value()&&
                !clan_draws.has_value()&&
                !selected_character.has_value()&&
                !play_time.has_value()&&
                !story.has_value()&&
                !achievement.has_value()&&
                !voice.has_value()&& 
                !tutorial.has_value()&& 
                !guide.has_value()&& 
                !exp.has_value()&& 
				!max_items.has_value() &&
				!max_energy.has_value() &&
                !lvl.has_value()&&
                !lucky.has_value()&&
				!wins.has_value()&&
				!loses.has_value()&&
				!draws.has_value()&&
				!kills.has_value()&&
				!deaths.has_value()&&
				!assists.has_value()&&
				!headshots.has_value()&&
				!highest_kill_streak.has_value()&&
				!melee_kills.has_value()&&
				!rifle_kills.has_value()&&
				!shotgun_kills.has_value()&&
				!sniper_kills.has_value()&&
				!gatling_kills.has_value()&&
				!bazooka_kills.has_value()&&
				!grenade_kills.has_value()&&
				!zombie_kills.has_value()&&
				!infections.has_value()&&
				!nickname.has_value()
                )  return {};

            std::string sql = "UPDATE accounts SET ";
            bool first = true;
            auto add_set = [&](const char* col) 
            {
                if (!first) sql += ", ";
                sql += col;
                sql += " = ?";
                first = false;
            };
            if (server_id.has_value()) add_set("ServerId");
            if (sw_daily_attempts.has_value()) add_set("SingleWaveDailyAttempts");
			if (sw_high_score.has_value()) add_set("SingleWaveHighScore");
			if (sw_highest_wave.has_value()) add_set("SingleWaveHighestWave");
			if (sw_last_update.has_value()) add_set("SingleWaveLastUpdate");
			if (clan_kills.has_value()) add_set("ClanKills");
			if (clan_deaths.has_value()) add_set("ClanDeaths");
			if (clan_assists.has_value()) add_set("ClanAssists");
			if (clan_contribution.has_value()) add_set("ClanContribution");
			if (clan_wins.has_value()) add_set("ClanWins");
			if (clan_loses.has_value()) add_set("ClanLoses");
			if (clan_draws.has_value()) add_set("ClanDraws");
			if (selected_character.has_value()) add_set("SelectedCharacter");
			if (play_time.has_value()) add_set("PlayTime");
			if (story.has_value()) add_set("Story");
            if (achievement.has_value()) add_set("Achievement");
            if (voice.has_value()) add_set("VoiceType");
            if (tutorial.has_value()) add_set("Tutorial");
            if (guide.has_value()) add_set("GuideMission");
			if (exp.has_value()) add_set("Experience");
			if (max_items.has_value()) add_set("MaximumItems");
			if (max_energy.has_value()) add_set("MaximumEnergy");
			if (lvl.has_value()) add_set("Level");
            if (lucky.has_value()) add_set("LuckyPoints");
			if (wins.has_value()) add_set("Wins");
			if (loses.has_value()) add_set("Loses");
			if (draws.has_value()) add_set("Draws");
			if (kills.has_value()) add_set("Kills");
			if (deaths.has_value()) add_set("Deaths");
			if (assists.has_value()) add_set("Assists");
			if (headshots.has_value()) add_set("Headshots");
			if (highest_kill_streak.has_value()) add_set("HighestKillStreak");
			if (melee_kills.has_value()) add_set("MeleeKills");
			if (rifle_kills.has_value()) add_set("RifleKills");
			if (shotgun_kills.has_value()) add_set("ShotgunKills");
			if (sniper_kills.has_value()) add_set("SniperKills");
			if (gatling_kills.has_value()) add_set("GatlingKills");
			if (bazooka_kills.has_value()) add_set("BazookaKills");
			if (grenade_kills.has_value()) add_set("GrenadeKills");
			if (zombie_kills.has_value()) add_set("ZombieKills");
			if (infections.has_value()) add_set("Infections");
            if (nickname.has_value()) add_set("Nickname");

            sql += " WHERE Id = ?";
            auto* ps = Prep(sql);
            unsigned idx = 1;
            if (server_id.has_value())  ps->setUInt(idx++, server_id.value());
			if (sw_daily_attempts.has_value()) ps->setUInt(idx++, sw_daily_attempts.value());
			if (sw_high_score.has_value()) ps->setUInt(idx++, sw_high_score.value());
			if (sw_highest_wave.has_value()) ps->setUInt(idx++, sw_highest_wave.value());
			if (sw_last_update.has_value()) ps->setUInt64(idx++, sw_last_update.value());
			if (clan_kills.has_value()) ps->setUInt(idx++, clan_kills.value());
			if (clan_deaths.has_value()) ps->setUInt(idx++, clan_deaths.value());
			if (clan_assists.has_value()) ps->setUInt(idx++, clan_assists.value());
			if (clan_contribution.has_value()) ps->setUInt(idx++, clan_contribution.value());
			if (clan_wins.has_value()) ps->setUInt(idx++, clan_wins.value());
			if (clan_loses.has_value()) ps->setUInt(idx++, clan_loses.value());
			if (clan_draws.has_value()) ps->setUInt(idx++, clan_draws.value());
			if (selected_character.has_value()) ps->setUInt(idx++, selected_character.value());
			if (play_time.has_value()) ps->setUInt64(idx++, play_time.value());
			if (story.has_value()) ps->setUInt(idx++, story.value());
            if (achievement.has_value()) ps->setUInt64(idx++, achievement.value());
            if (voice.has_value()) ps->setUInt64(idx++, voice.value());
            if (tutorial.has_value()) ps->setBoolean(idx++, tutorial.value());
            if (guide.has_value()) ps->setUInt(idx++, guide.value());
			if (exp.has_value()) ps->setUInt(idx++, exp.value());
			if (max_items.has_value()) ps->setUInt(idx++, max_items.value());
			if (max_energy.has_value()) ps->setUInt(idx++, max_energy.value());
			if (lvl.has_value()) ps->setUInt(idx++, lvl.value());
            if (lucky.has_value()) ps->setUInt(idx++, lucky.value());
			if (wins.has_value()) ps->setUInt(idx++, wins.value());
			if (loses.has_value()) ps->setUInt(idx++, loses.value());
			if (draws.has_value()) ps->setUInt(idx++, draws.value());
			if (kills.has_value()) ps->setUInt(idx++, kills.value());
			if (deaths.has_value()) ps->setUInt(idx++, deaths.value());
			if (assists.has_value()) ps->setUInt(idx++, assists.value());
			if (headshots.has_value()) ps->setUInt(idx++, headshots.value());
			if (highest_kill_streak.has_value()) ps->setUInt(idx++, highest_kill_streak.value());
			if (melee_kills.has_value()) ps->setUInt(idx++, melee_kills.value());
			if (rifle_kills.has_value()) ps->setUInt(idx++, rifle_kills.value());
			if (shotgun_kills.has_value()) ps->setUInt(idx++, shotgun_kills.value());
			if (sniper_kills.has_value()) ps->setUInt(idx++, sniper_kills.value());
			if (gatling_kills.has_value()) ps->setUInt(idx++, gatling_kills.value());
			if (bazooka_kills.has_value()) ps->setUInt(idx++, bazooka_kills.value());
			if (grenade_kills.has_value()) ps->setUInt(idx++, grenade_kills.value());
			if (zombie_kills.has_value()) ps->setUInt(idx++, zombie_kills.value());
			if (infections.has_value()) ps->setUInt(idx++, infections.value());
			if (nickname.has_value()) ps->setString(idx++, nickname.value());

            ps->setUInt(idx++, v.aid);
            if (!ps->executeUpdate())
                return std::unexpected(DbError{ DbError::Type::NoRowsAffected, 0, {}, "No rows affected" });
            DEBUGLOG(green, "PersistAccountInfoPatches updated account {}", v.aid);
            return {};
        }
        catch (sql::SQLException& e)
        {
            DEBUGLOG(red, "PersistAccountInfoPatches sql exception: {}", e.what());
            return std::unexpected(CMariaDatabase::FromSQLException(e));
        }
    }
    std::expected<void, DbError> CMariaDatabase::PersistItemDeletes(ValidatedDbUpdates& v, ResultDbUpdateInfo& out)
    {
        try
        {
            if (v.items_deleted.empty()) return {};
            std::string dsql = "DELETE FROM player_items WHERE PlayerId = ? AND SerialInfo IN (" + GenerateQuestionMarks(v.items_deleted.size()) + ")";
            auto* dps = Prep(dsql);
            dps->setUInt(1, v.aid);
            for (size_t i = 0; i < v.items_deleted.size(); ++i) dps->setUInt64(2 + i, v.items_deleted[i].data);
            auto deleted = dps->executeUpdate();
            if (!deleted)
            {
                DEBUGLOG(red, "Failed to delete items for account {}: expected {}, got {}", v.aid, v.items_deleted.size(), deleted);
                return std::unexpected(DbError{ DbError::Type::NoRowsAffected,0,{},fmt::format("PersistItemDeletes: expected {} deletes, got {}", v.items_deleted.size(), deleted) });
            }
            DEBUGLOG(green, "Deleted {} items for account {}", deleted, v.aid);
            out.deleted_rows_count += deleted;
            out.deleted_serials.reserve(out.deleted_serials.size() + v.items_deleted.size());
            for (const auto& s : v.items_deleted) out.deleted_serials.push_back(s.data);
            return {};
        }
        catch (sql::SQLException& e)
        {
            DEBUGLOG(red, "PersistItemDeletes sql exception: ({})", e.what());
            return std::unexpected(CMariaDatabase::FromSQLException(e));
        }
    }
    std::expected<void, DbError> CMariaDatabase::PersistItemPatches(ValidatedDbUpdates& v, ResultDbUpdateInfo& out)
    {
        try
        {
            if (v.items_patches.empty()) return {};

            boost::unordered_flat_map<uint64_t, ItemPatchCtx> merged;
            for (const auto& rp : v.items_patches)
            {
                auto& existing = merged[rp.serial];
                if (rp.patch.new_item_id.has_value()) existing.new_item_id = rp.patch.new_item_id;
                if (rp.patch.expire_date.has_value()) existing.expire_date = rp.patch.expire_date;
                if (rp.patch.repair.has_value()) existing.repair = rp.patch.repair;
                if (rp.patch.energy.has_value()) existing.energy = rp.patch.energy;
                if (rp.patch.is_equipped.has_value()) existing.is_equipped = rp.patch.is_equipped;
                if (rp.patch.character_id.has_value()) existing.character_id = rp.patch.character_id;
            }

            if (merged.empty()) return {};

            {
                std::string checkSql = "SELECT SerialInfo FROM player_items WHERE PlayerId = ? AND SerialInfo IN (" + GenerateQuestionMarks(merged.size()) + ")";
                auto* checkPs = Prep(checkSql);
                checkPs->setUInt(1, v.aid);
                size_t idx = 2;
                for (const auto& [serial, _] : merged)
                    checkPs->setUInt64(idx++, serial);
                std::unique_ptr<sql::ResultSet> rs(checkPs->executeQuery());
                size_t foundCount = 0;
                while (rs->next()) foundCount++;
                DEBUGLOG(yellow, "PersistItemPatches: Found {} of {} items in DB for account {}", foundCount, merged.size(), v.aid);
            }


            std::string c_itemId, c_exp, c_rep, c_eng, c_equip, c_char;
            std::vector<uint64_t> serials;
            serials.reserve(merged.size());

            for (const auto& [serial, patch] : merged)
            {
                std::string s = std::to_string(serial);
                serials.push_back(serial);
                if (patch.new_item_id.has_value()) c_itemId += "WHEN " + s + " THEN " + std::to_string(patch.new_item_id.value()) + " ";
                if (patch.expire_date.has_value()) c_exp += "WHEN " + s + " THEN " + std::to_string(patch.expire_date.value()) + " ";
                if (patch.repair.has_value()) c_rep += "WHEN " + s + " THEN " + std::to_string(patch.repair.value()) + " ";
                if (patch.energy.has_value()) c_eng += "WHEN " + s + " THEN " + std::to_string(patch.energy.value()) + " ";
                if (patch.is_equipped.has_value()) c_equip += "WHEN " + s + " THEN " + std::to_string(patch.is_equipped.value()) + " ";
                if (patch.character_id.has_value()) c_char += "WHEN " + s + " THEN " + std::to_string(patch.character_id.value()) + " ";
                DEBUGLOG(green, "Merged item patch for serial {}: equipped={}", serial, patch.is_equipped.value_or(255));
            }

            if (serials.empty())
            {
                DEBUGLOG(yellow, "No serials to update in PersistItemPatches");
                return {};
            }

            std::vector<std::string> sets;
            if (!c_itemId.empty()) sets.push_back("ItemId = CASE SerialInfo " + c_itemId + "ELSE ItemId END");
            if (!c_exp.empty()) sets.push_back("ExpirationDate = CASE SerialInfo " + c_exp + "ELSE ExpirationDate END");
            if (!c_rep.empty()) sets.push_back("Repair = CASE SerialInfo " + c_rep + "ELSE Repair END");
            if (!c_eng.empty()) sets.push_back("Energy = CASE SerialInfo " + c_eng + "ELSE Energy END");
            if (!c_equip.empty()) sets.push_back("IsEquipped = CASE SerialInfo " + c_equip + "ELSE IsEquipped END");
            if (!c_char.empty()) sets.push_back("CharacterId = CASE SerialInfo " + c_char + "ELSE CharacterId END");

            if (sets.empty())
            {
                DEBUGLOG(yellow, "No item patches in PersistItemPatches");
                return {};
            }

            std::string psql = "UPDATE player_items SET " + GenerateJoinedString(sets, ", ") + " WHERE PlayerId = ? AND SerialInfo IN (" + GenerateQuestionMarks(serials.size()) + ")";
            auto* pps = Prep(psql);
            pps->setUInt(1, v.aid);
            for (size_t i = 0; i < serials.size(); ++i) pps->setUInt64(2 + i, serials[i]);
            auto patched = pps->executeUpdate();

            if (!patched)
            {
                DEBUGLOG(red, "Failed to patch items for account {}: expected {}, got {}", v.aid, serials.size(), patched);
                return std::unexpected(DbError{ DbError::Type::NoRowsAffected,0,{},fmt::format("PersistItemPatches: expected {} patches, got {}", serials.size(), patched) });
            }

            DEBUGLOG(green, "Updated {} items for account {}", patched, v.aid);
            out.patched_rows_count += patched;
            out.patched_serials.insert(out.patched_serials.end(), serials.begin(), serials.end());
            return {};
        }
        catch (sql::SQLException& e)
        {
            DEBUGLOG(red, "PersistItemPatches sql exception: ({})", e.what());
            return std::unexpected(CMariaDatabase::FromSQLException(e));
        }
    }
    std::expected<void, DbError> CMariaDatabase::PersistItemAdds(ValidatedDbUpdates& v, ResultDbUpdateInfo& out)
    {
        try
        {
            if (v.items_added.empty()) return {};
            std::string asql =
                "INSERT INTO player_items (PlayerId, SerialInfo, ItemId, ItemType, ExpirationDate, Repair, Energy, "
                "IsSealed, SealLevel, EnhanceExp, EnhanceLevel, Stock, IsEquipped, CharacterId) "
                "VALUES " + GenerateQuestionMarks(v.items_added.size(), 14);
            auto* aps = Prep(asql);
            int idx = 1;
            for (const auto& item : v.items_added) 
            {
                aps->setUInt  (idx++, v.aid);
                aps->setUInt64(idx++, item.item_info.serial_info.data);
                aps->setUInt  (idx++, item.item_info.item_number.item_id);
                aps->setUInt  (idx++, item.item_type);
                aps->setUInt  (idx++, item.item_info.expire_date);
                aps->setUInt  (idx++, item.item_info.repair);
                aps->setUInt  (idx++, item.item_info.energy);
#if defined(RELEASE_1_0_3)
                aps->setUInt(idx++, 0);
                aps->setUInt(idx++, 0);
                aps->setUInt(idx++, 0);
                aps->setUInt(idx++, 0);
#else
                aps->setUInt(idx++, item.item_info.is_sealed);
                aps->setUInt(idx++, item.item_info.seal_level);
                aps->setUInt(idx++, item.item_info.enhance_exp);
                aps->setUInt(idx++, item.item_info.enhance_level);
#endif
                aps->setUInt  (idx++, item.stock);
                aps->setUInt  (idx++, item.is_equipped);
                aps->setUInt  (idx++, item.character_id);
            }
            auto inserted = aps->executeUpdate();
            if (!inserted)
            {
                DEBUGLOG(red, "Failed to add items for account {}: expected {}, got {}", v.aid, v.items_added.size(), inserted);
                return std::unexpected(DbError{ DbError::Type::NoRowsAffected,0,{},fmt::format("PersistItemAdds: expected {} adds, got {}", v.items_added.size(), inserted) });
            }
            DEBUGLOG(green, "Added {} items for account {}", inserted, v.aid);
            out.added_rows_count += inserted;
            out.added_serials.reserve(out.added_serials.size() + v.items_added.size());
            for (const auto& it : v.items_added) out.added_serials.push_back(it.item_info.serial_info.data);
            return {};
        }
        catch (sql::SQLException& e)
        {
            DEBUGLOG(red, "PersistItemAdds sql exception: ({})", e.what());
            return std::unexpected(CMariaDatabase::FromSQLException(e));
        }
    }
    std::expected<void, DbError> CMariaDatabase::PersistMissionsPatches(ValidatedDbUpdates& v)
    {
        try 
        {
            if (v.player_missions_patches.empty()) return {};
            PlayerMissionsPatch m;
            for (const auto& p : v.player_missions_patches) 
            {
                if (p.update_time.has_value()) m.update_time = p.update_time.value();
                if (p.mission1.has_value()) m.mission1 = p.mission1.value();
                if (p.mission2.has_value()) m.mission2 = p.mission2.value();
                if (p.mission3.has_value()) m.mission3 = p.mission3.value();
                if (p.goal1.has_value()) m.goal1 = p.goal1.value();
                if (p.goal2.has_value()) m.goal2 = p.goal2.value();
                if (p.goal3.has_value()) m.goal3 = p.goal3.value();
            }
            if (!m.update_time.has_value() &&
                !m.mission1.has_value() && !m.mission2.has_value() && !m.mission3.has_value() &&
                !m.goal1.has_value() && !m.goal2.has_value() && !m.goal3.has_value()) return {};
            std::string sql = "INSERT INTO player_daily_mission (PlayerId";
            if (m.update_time.has_value()) sql += ", UpdateTime";
            if (m.mission1.has_value()) sql += ", Mission1";
            if (m.mission2.has_value()) sql += ", Mission2";
            if (m.mission3.has_value()) sql += ", Mission3";
            if (m.goal1.has_value()) sql += ", GoalMission1";
            if (m.goal2.has_value()) sql += ", GoalMission2";
            if (m.goal3.has_value()) sql += ", GoalMission3";
            sql += ") VALUES (?";
            if (m.update_time.has_value()) sql += ", ?";
            if (m.mission1.has_value()) sql += ", ?";
            if (m.mission2.has_value()) sql += ", ?";
            if (m.mission3.has_value()) sql += ", ?";
            if (m.goal1.has_value()) sql += ", ?";
            if (m.goal2.has_value()) sql += ", ?";
            if (m.goal3.has_value()) sql += ", ?";
            sql += ") ON DUPLICATE KEY UPDATE ";
            bool first = true;
            auto add_update = [&](const char* col)
            {
                if (!first) sql += ", ";
                sql += col; sql += " = VALUES("; sql += col; sql += ")";
                first = false;
            };
            if (m.update_time.has_value()) add_update("UpdateTime");
            if (m.mission1.has_value()) add_update("Mission1");
            if (m.mission2.has_value()) add_update("Mission2");
            if (m.mission3.has_value()) add_update("Mission3");
            if (m.goal1.has_value()) add_update("GoalMission1");
            if (m.goal2.has_value()) add_update("GoalMission2");
            if (m.goal3.has_value()) add_update("GoalMission3");
            if (first) sql += "PlayerId = PlayerId";
            auto* ps = Prep(sql);
            unsigned idx = 1;
            ps->setUInt(idx++, v.aid);
            if (m.update_time.has_value()) ps->setUInt64(idx++, m.update_time.value());
            if (m.mission1.has_value()) ps->setUInt(idx++, m.mission1.value());
            if (m.mission2.has_value()) ps->setUInt(idx++, m.mission2.value());
            if (m.mission3.has_value()) ps->setUInt(idx++, m.mission3.value());
            if (m.goal1.has_value()) ps->setUInt(idx++, m.goal1.value());
            if (m.goal2.has_value()) ps->setUInt(idx++, m.goal2.value());
            if (m.goal3.has_value()) ps->setUInt(idx++, m.goal3.value());
            if (!ps->executeUpdate()) 
            {
                DEBUGLOG(red, "PersistMissionPatches affected 0 rows for account {}", v.aid);
                return std::unexpected(DbError{ DbError::Type::NoRowsAffected,0,{},"PersistMissionPatches affected 0 rows" });
            }
            DEBUGLOG(green, "PersistMissionPatches upserted daily mission for account {}", v.aid);
            return {};
        } catch (sql::SQLException& e) 
        {
            DEBUGLOG(red, "PersistMissionPatches sql exception: {}", e.what());
            return std::unexpected(CMariaDatabase::FromSQLException(e));
        }
    }
    std::expected<void, DbError> CMariaDatabase::PersistMonthlyRewardsPatches(ValidatedDbUpdates& v)
    {
        try 
        {
            if (v.player_monthly_reward_patches.empty()) return {};
            PlayerMonthlyRewardPatch m;
            for (const auto& p : v.player_monthly_reward_patches) 
            {
                if (p.day_count.has_value()) m.day_count = p.day_count.value();
                if (p.last_time_update.has_value()) m.last_time_update = p.last_time_update.value();
            }
            if (!m.day_count.has_value() && !m.last_time_update.has_value()) return {};
            std::string sql = "INSERT INTO player_monthly_rewards (PlayerId";
            if (m.day_count.has_value()) sql += ", RewardCount";
            if (m.last_time_update.has_value()) sql += ", LastUpdate";
            sql += ") VALUES (?";
            if (m.day_count.has_value()) sql += ", ?";
            if (m.last_time_update.has_value()) sql += ", FROM_UNIXTIME(?)";
            sql += ") ON DUPLICATE KEY UPDATE ";
            bool first = true;
            auto add_update = [&](const char* col)
            {
                if (!first) sql += ", ";
                sql += col; sql += " = VALUES("; sql += col; sql += ")";
                first = false;
            };
            if (m.day_count.has_value()) add_update("RewardCount");
            if (m.last_time_update.has_value()) add_update("LastUpdate");
            if (first) sql += "PlayerId = PlayerId";
            auto* ps = Prep(sql);
            unsigned idx = 1;
            ps->setUInt(idx++, v.aid);
            if (m.day_count.has_value()) ps->setUInt (idx++, m.day_count.value());
            if (m.last_time_update.has_value()) ps->setUInt64(idx++, m.last_time_update.value());
            if (!ps->executeUpdate()) 
            {
                DEBUGLOG(red, "PersistMonthlyRewardPatches affected 0 rows for account {}", v.aid);
                return std::unexpected(DbError{ DbError::Type::NoRowsAffected,0,{},"PersistMonthlyRewardPatches affected 0 rows" });
            }
            DEBUGLOG(green, "PersistMonthlyRewardPatches upserted monthly reward for account {}", v.aid);
            return {};
        } catch (sql::SQLException& e)
        {
            DEBUGLOG(red, "PersistMonthlyRewardPatches sql exception: {}", e.what());
            return std::unexpected(CMariaDatabase::FromSQLException(e));
        }
    }
    std::expected<void, DbError> CMariaDatabase::PersistWeeklyRewardsPatches(ValidatedDbUpdates& v)
    {
        try
        {
            if (v.player_weekly_reward_patches.empty()) return {};
            PlayerWeeklyRewardPatch m;
            for (const auto& p : v.player_weekly_reward_patches)
            {
                if (p.day_count.has_value()) m.day_count = p.day_count.value();
                if (p.last_time_update.has_value()) m.last_time_update = p.last_time_update.value();
            }
            if (!m.day_count.has_value() && !m.last_time_update.has_value()) return {};
            std::string sql = "INSERT INTO player_weekly_rewards (PlayerId";
            if (m.day_count.has_value()) sql += ", RewardCount";
            if (m.last_time_update.has_value()) sql += ", LastUpdate";
            sql += ") VALUES (?";
            if (m.day_count.has_value()) sql += ", ?";
            if (m.last_time_update.has_value()) sql += ", FROM_UNIXTIME(?)";
            sql += ") ON DUPLICATE KEY UPDATE ";
            bool first = true;
            auto add_update = [&](const char* col)
            {
                if (!first) sql += ", ";
                sql += col; sql += " = VALUES("; sql += col; sql += ")";
                first = false;
            };
            if (m.day_count.has_value()) add_update("RewardCount");
            if (m.last_time_update.has_value()) add_update("LastUpdate");
            if (first) sql += "PlayerId = PlayerId";
            auto* ps = Prep(sql);
            unsigned idx = 1;
            ps->setUInt(idx++, v.aid);
            if (m.day_count.has_value()) ps->setUInt (idx++, m.day_count.value());
            if (m.last_time_update.has_value()) ps->setUInt64(idx++, m.last_time_update.value());
            if (!ps->executeUpdate())
            {
                DEBUGLOG(red, "PersistWeeklyRewardPatches affected 0 rows for account {}", v.aid);
                return std::unexpected(DbError{ DbError::Type::NoRowsAffected,0,{},"PersistWeeklyRewardPatches affected 0 rows" });
            }
            DEBUGLOG(green, "PersistWeeklyRewardPatches upserted weekly reward for account {}", v.aid);
            return {};
        } catch (sql::SQLException& e)
        {
            DEBUGLOG(red, "PersistWeeklyRewardPatches sql exception: {}", e.what());
            return std::unexpected(CMariaDatabase::FromSQLException(e));
        }
    }
    std::expected<void, DbError> CMariaDatabase::PersistPlaytimePatches(ValidatedDbUpdates& v)
    {
        try
        {
            if (v.player_playtime_patches.empty()) return {};
            PlayerPlaytimePatch m;
            for (const auto& p : v.player_playtime_patches)
            {
                if (p.daily_seconds.has_value()) m.daily_seconds = p.daily_seconds.value();
                if (p.claimed_stage.has_value()) m.claimed_stage = p.claimed_stage.value();
                if (p.last_time_update.has_value()) m.last_time_update = p.last_time_update.value();
            }
            if (!m.daily_seconds.has_value() && !m.claimed_stage.has_value() && !m.last_time_update.has_value()) return {};
            std::string sql = "INSERT INTO player_playtime (PlayerId";
            if (m.daily_seconds.has_value()) sql += ", DailySeconds";
            if (m.claimed_stage.has_value()) sql += ", ClaimedStage";
            if (m.last_time_update.has_value()) sql += ", LastUpdate";
            sql += ") VALUES (?";
            if (m.daily_seconds.has_value()) sql += ", ?";
            if (m.claimed_stage.has_value()) sql += ", ?";
            if (m.last_time_update.has_value()) sql += ", FROM_UNIXTIME(?)";
            sql += ") ON DUPLICATE KEY UPDATE ";
            bool first = true;
            auto add_update = [&](const char* col)
            {
                if (!first) sql += ", ";
                sql += col; sql += " = VALUES("; sql += col; sql += ")";
                first = false;
            };
            if (m.daily_seconds.has_value()) add_update("DailySeconds");
            if (m.claimed_stage.has_value()) add_update("ClaimedStage");
            if (m.last_time_update.has_value()) add_update("LastUpdate");
            if (first) sql += "PlayerId = PlayerId";
            auto* ps = Prep(sql);
            unsigned idx = 1;
            ps->setUInt(idx++, v.aid);
            if (m.daily_seconds.has_value()) ps->setUInt(idx++, m.daily_seconds.value());
            if (m.claimed_stage.has_value()) ps->setUInt(idx++, m.claimed_stage.value());
            if (m.last_time_update.has_value()) ps->setUInt64(idx++, m.last_time_update.value());
            if (!ps->executeUpdate())
            {
                DEBUGLOG(red, "PersistPlaytimePatches affected 0 rows for account {}", v.aid);
                return std::unexpected(DbError{ DbError::Type::NoRowsAffected,0,{},"PersistPlaytimePatches affected 0 rows" });
            }
            DEBUGLOG(green, "PersistPlaytimePatches upserted playtime for account {}", v.aid);
            return {};
        } catch (sql::SQLException& e)
        {
            DEBUGLOG(red, "PersistPlaytimePatches sql exception: {}", e.what());
            return std::unexpected(CMariaDatabase::FromSQLException(e));
        }
    }
    std::expected<void, DbError> CMariaDatabase::PersistBattlePassPatches(ValidatedDbUpdates& v)
    {
        try
        {
            if (v.player_battlepass_patches.empty()) return {};
            PlayerBattlePassPatch m;
            for (const auto& p : v.player_battlepass_patches)
            {
                if (p.season.has_value())             m.season = p.season;
                if (p.level.has_value())              m.level = p.level;
                if (p.xp.has_value())                 m.xp = p.xp;
                if (p.has_premium.has_value())        m.has_premium = p.has_premium;
                if (p.claimed_free.has_value())       m.claimed_free = p.claimed_free;
                if (p.claimed_premium.has_value())    m.claimed_premium = p.claimed_premium;
                if (p.current_mission_id.has_value()) m.current_mission_id = p.current_mission_id;
                if (p.mission_progress.has_value())   m.mission_progress = p.mission_progress;
                if (p.reset_count.has_value())        m.reset_count = p.reset_count;
            }
            std::string sql = "INSERT INTO player_battlepass (PlayerId";
            if (m.season.has_value())             sql += ", Season";
            if (m.level.has_value())              sql += ", Level";
            if (m.xp.has_value())                 sql += ", Xp";
            if (m.has_premium.has_value())        sql += ", HasPremium";
            if (m.claimed_free.has_value())       sql += ", ClaimedFree";
            if (m.claimed_premium.has_value())    sql += ", ClaimedPremium";
            if (m.current_mission_id.has_value()) sql += ", CurrentMissionId";
            if (m.mission_progress.has_value())   sql += ", MissionProgress";
            if (m.reset_count.has_value())        sql += ", ResetCount";
            sql += ") VALUES (?";
            for (int i = 0, n = (int)m.season.has_value() + m.level.has_value() + m.xp.has_value()
                + m.has_premium.has_value() + m.claimed_free.has_value() + m.claimed_premium.has_value()
                + m.current_mission_id.has_value() + m.mission_progress.has_value() + m.reset_count.has_value();
                 i < n; ++i) sql += ", ?";
            sql += ") ON DUPLICATE KEY UPDATE ";
            bool first = true;
            auto add_update = [&](const char* col)
            {
                if (!first) sql += ", ";
                sql += col; sql += " = VALUES("; sql += col; sql += ")";
                first = false;
            };
            if (m.season.has_value())             add_update("Season");
            if (m.level.has_value())              add_update("Level");
            if (m.xp.has_value())                 add_update("Xp");
            if (m.has_premium.has_value())        add_update("HasPremium");
            if (m.claimed_free.has_value())       add_update("ClaimedFree");
            if (m.claimed_premium.has_value())    add_update("ClaimedPremium");
            if (m.current_mission_id.has_value()) add_update("CurrentMissionId");
            if (m.mission_progress.has_value())   add_update("MissionProgress");
            if (m.reset_count.has_value())        add_update("ResetCount");
            if (first) sql += "PlayerId = PlayerId";
            auto* ps = Prep(sql);
            unsigned idx = 1;
            ps->setUInt(idx++, v.aid);
            if (m.season.has_value())             ps->setUInt(idx++, m.season.value());
            if (m.level.has_value())              ps->setUInt(idx++, m.level.value());
            if (m.xp.has_value())                 ps->setUInt(idx++, m.xp.value());
            if (m.has_premium.has_value())        ps->setUInt(idx++, m.has_premium.value());
            if (m.claimed_free.has_value())       ps->setString(idx++, BattlePassMaskToHex(*m.claimed_free));
            if (m.claimed_premium.has_value())    ps->setString(idx++, BattlePassMaskToHex(*m.claimed_premium));
            if (m.current_mission_id.has_value()) ps->setUInt(idx++, m.current_mission_id.value());
            if (m.mission_progress.has_value())   ps->setUInt(idx++, m.mission_progress.value());
            if (m.reset_count.has_value())        ps->setUInt(idx++, m.reset_count.value());
            ps->executeUpdate();
            DEBUGLOG(green, "PersistBattlePassPatches upserted battlepass for account {}", v.aid);
            return {};
        } catch (sql::SQLException& e)
        {
            DEBUGLOG(red, "PersistBattlePassPatches sql exception: {}", e.what());
            return std::unexpected(CMariaDatabase::FromSQLException(e));
        }
    }
    std::expected<void, DbError> CMariaDatabase::PersistMailboxPatches(ValidatedDbUpdates& v)
    {
        try
        {
            if (v.mailbox_patches.empty()) return {};

            std::vector<uint32_t> mark_read_ids;
            std::vector<uint32_t> del_sender_ids, del_receiver_ids;
            std::vector<size_t> insert_idx; // indices into v.mailbox_patches
            for (size_t i = 0; i < v.mailbox_patches.size(); ++i)
            {
                const auto& p = v.mailbox_patches[i];
                switch (p.op)
                {
                    case MailboxPatch::Op::MarkRead:
                        if (p.mail_id && p.read && *p.read) mark_read_ids.push_back(p.mail_id);
                        break;
                    case MailboxPatch::Op::Delete:
                        if (p.mail_id && p.side) 
                        {
                            if (*p.side == MailSide::Sender) del_sender_ids.push_back(p.mail_id);
                            else del_receiver_ids.push_back(p.mail_id);
                        }
                        break;
                    case MailboxPatch::Op::Insert:
                        if (p.insert && p.insert->receiver_nickname && p.insert->sender_nickname)
                            insert_idx.push_back(i);
                        break;
                }
            }
            if (mark_read_ids.empty() && del_sender_ids.empty() && del_receiver_ids.empty() && insert_idx.empty())
                return {};

            // mark mails as read
            if (!mark_read_ids.empty())
            {
                std::string q = "UPDATE player_mailbox SET IsNew = 0 WHERE Id IN (" + GenerateQuestionMarks(mark_read_ids.size()) + ")";
                auto* ps = Prep(q);
                for (uint32_t i = 0; i < mark_read_ids.size(); ++i)
                    ps->setUInt(i + 1, mark_read_ids[i]);
              
                if (!ps->executeUpdate()) 
                {
                    DEBUGLOG(red, "PersistMailboxPatches affected 0 rows for account {}", v.aid);
                    return std::unexpected(DbError{ DbError::Type::NoRowsAffected,0,{},"PersistMailboxPatches affected 0 rows" });
                }
            }

            // delete/flag for sender
            if (!del_sender_ids.empty())
            {
                {
                    std::string q = "DELETE FROM player_mailbox WHERE DeletedFromReceiver = 1 AND Id IN (" + GenerateQuestionMarks(del_sender_ids.size()) + ")";
                    auto* ps = Prep(q);
                    for (uint32_t i = 0; i < del_sender_ids.size(); ++i)
                        ps->setUInt(i + 1, del_sender_ids[i]);
                    
                    ps->executeUpdate();
                }
                {
                    std::string q = "UPDATE player_mailbox SET DeletedFromSender = 1 WHERE DeletedFromReceiver = 0 AND Id IN (" + GenerateQuestionMarks(del_sender_ids.size()) + ")";
                    auto* ps = Prep(q);
                    for (uint32_t i = 0; i < del_sender_ids.size(); ++i)
                        ps->setUInt(i + 1, del_sender_ids[i]);
                   
                    ps->executeUpdate();
                }
            }

            // delete/flag for receiver
            if (!del_receiver_ids.empty())
            {
                {
                    std::string q = "DELETE FROM player_mailbox WHERE DeletedFromSender = 1 AND Id IN (" + GenerateQuestionMarks(del_receiver_ids.size()) + ")";
                    auto* ps = Prep(q);
                    for (uint32_t i = 0; i < del_receiver_ids.size(); i++)
                        ps->setUInt(i + 1, del_receiver_ids[i]);
                        
                    ps->executeUpdate();
                }
                {
                    std::string q = "UPDATE player_mailbox SET DeletedFromReceiver = 1 WHERE DeletedFromSender = 0 AND Id IN (" + GenerateQuestionMarks(del_receiver_ids.size()) + ")";
                    auto* ps = Prep(q);
                    for (uint32_t i = 0; i < del_receiver_ids.size(); i++)
                        ps->setUInt(i + 1, del_receiver_ids[i]);

                    ps->executeUpdate();
                }
            }	
            // insert mails
            if (!insert_idx.empty())
            {
                static constexpr uint32_t kMailboxLimit = 100;
                auto* nick = Prep("SELECT Id FROM accounts WHERE Nickname = ? LIMIT 1");
                
                auto* is_blocked = Prep(
                        "SELECT EXISTS ("
                        "  SELECT 1 FROM player_socials "
                        "  WHERE Aid = ? AND TargetAid = ? AND State = 2"
                        ") AS IsBlocked");

                auto* count = Prep("SELECT COUNT(*) AS MailCount "
                    "FROM player_mailbox "
                    "WHERE ReceiverId = ? AND DeletedFromReceiver = 0");

                auto* ins = Prep(
                    "INSERT INTO player_mailbox "
                    "(SenderId, SenderNickname, ReceiverId, ReceiverNickname, Date, GiftItemId, Message, IsNew, DeletedFromSender, DeletedFromReceiver) "
                    "VALUES (?, ?, ?, ?, ?, ?, ?, ?, 0, 0)"
                );
				DEBUGLOG(green, "PersistMailboxPatches inserting {} new mails for account {}", insert_idx.size(), v.aid);

                for (const size_t j : insert_idx)
                {
                    auto& patch = v.mailbox_patches[j];
                    auto& in = *patch.insert;

                    nick->setString(1, in.receiver_nickname.value().c_str());
                    std::unique_ptr<sql::ResultSet> nick_result(nick->executeQuery());
                    if (!nick_result->next())
                        return std::unexpected(DbError{DbError::Type::NicknameNotFound, 0, {},fmt::format("Receiver nickname '{}' not found in accounts", in.receiver_nickname.value().c_str())});

                    const uint32_t rid = nick_result->getUInt(1);

					in.receiver_id = rid;
                    count->setUInt(1, rid);


					is_blocked->setUInt(1, rid);
					is_blocked->setUInt(2, v.aid);
                    std::unique_ptr<sql::ResultSet> blocked_result(is_blocked->executeQuery());
                    if (blocked_result->next() && blocked_result->getBoolean("IsBlocked"))
                        return std::unexpected(DbError{ DbError::Type::BlockedByReceiver, 0, {}, fmt::format("Receiver account {} has blocked sender account {}", rid, v.aid) });
							

                    std::unique_ptr<sql::ResultSet> count_result(count->executeQuery());
                    uint32_t existing = 0;

                    if (count_result->next()) 
						if (count_result->getUInt("MailCount") >= kMailboxLimit)
							return std::unexpected(DbError{ DbError::Type::MailboxFull, 0, {}, fmt::format("Receiver account {} mailbox full", rid) });

                    unsigned k = 1;
                   
                    ins->setUInt(k++, static_cast<uint32_t>(v.aid));
                    ins->setString(k++, *in.sender_nickname);
                    ins->setUInt(k++, rid);
                    ins->setString(k++, *in.receiver_nickname);
                    ins->setUInt(k++, Utility::GetUtcTimeNow());
                    ins->setUInt(k++, in.gift_item_id);
                    ins->setString(k++, in.message);
                    ins->setByte(k++, static_cast<uint8_t>(in.is_new ? 1 : 0));
                    if (!ins->executeUpdate()) 
                    {
                        DEBUGLOG(red, "PersistMailboxPatches affected 0 rows for account {}", v.aid);
                        return std::unexpected(DbError{ DbError::Type::NoRowsAffected,0,{},"PersistMailboxPatches affected 0 rows" });
                    }
                    std::unique_ptr<sql::Statement> stmt(GetConnection()->createStatement());
                    std::unique_ptr<sql::ResultSet> rs(stmt->executeQuery("SELECT LAST_INSERT_ID()"));
                    if (rs->next()) patch.mail_id = rs->getUInt(1);

					DEBUGLOG(green, "PersistMailboxPatches inserted mail id {} from aid {} to {}", patch.mail_id, v.aid, rid);
                }
            }
            return {};
        }
        catch (sql::SQLException& e) 
        {
            DEBUGLOG(red, "PersistMailboxPatches sql exception: {}", e.what());
            return std::unexpected(CMariaDatabase::FromSQLException(e));
        }
    }

    std::expected<void, DbError> CMariaDatabase::PersistMatchHistoryAdds(ValidatedDbUpdates& v)
    {
        try
        {
            if (v.match_history_adds.empty()) return {};
            std::string asql =
                "INSERT INTO player_matchhistory (MatchUniqueId, AccountId, IsWin, IsLose, IsHost, IsDraw, IsClanMatch, WinRule, TimeRule, WinRuleType, PlayTime, Level, Experience, "
                "Energy, MicroPoints, RoomIndex, RedScore, BlueScore, TeamId, RoomMode, RoomMap, SelectedCharacter, "
                "Kills, Deaths, Assists, Headshots, HighestKillStreak, MeleeKills, RifleKills, ShotgunKills, SniperKills, GatlingKills, "
                "BazookaKills, GrenadeKills, ZombieKills, Infections, MatchStartTime, MatchStartUtc, MatchEndTime, MatchEndUtc, Hair, Face, Upper, Under, Skirt, "
                "Gloves, Boots, HeadAcc, WaistAcc, BackAcc, Melee, Rifle, Shotgun, Sniper, Gatling, "
                "Bazooka, Grenade, RewardItem, IsMvp, IsEntryFragger, IsBullseye, IsSupport, IsBomba, "
                "MvpScore, EntryFraggerScore, BullseyeScore, SupportScore, BombaScore, BestKdScore, CaptureScore, WonRoundScore, ArmsRaceScore, ZombieScore, ADR, IsParty, Restriction, MaxPlayers) "
                "VALUES " + GenerateQuestionMarks(v.match_history_adds.size(), 77);
            auto* aps = Prep(asql);
            int idx = 1;
            for (const auto& match : v.match_history_adds) 
            {
                aps->setString(idx++, match.MatchUniqueId);
                aps->setUInt(idx++, match.Aid);
                aps->setBoolean(idx++, match.IsWin);
                aps->setBoolean(idx++, match.IsLose);
                aps->setBoolean(idx++, match.IsHost);
                aps->setBoolean(idx++, match.IsDraw);
                aps->setBoolean(idx++, match.IsClanMatch);
                aps->setUInt(idx++, match.WinRule);
                aps->setUInt(idx++, match.TimeRule);
                aps->setString(idx++, match.WinRuleType);
                aps->setUInt(idx++, match.PlayTime);
                aps->setUInt(idx++, match.Level);
                aps->setUInt(idx++, match.Experience);
                aps->setUInt(idx++, match.Energy);
                aps->setUInt(idx++, match.MicroPoints);
                aps->setUInt(idx++, match.room_index);
                aps->setUInt(idx++, match.redscore);
                aps->setUInt(idx++, match.bluescore);
                aps->setUInt(idx++, match.team_id);
                aps->setUInt(idx++, match.room_mode);
                aps->setUInt(idx++, match.room_map);
                aps->setUInt(idx++, match.SelectedCharacter);
                aps->setUInt(idx++, match.Kills);
                aps->setUInt(idx++, match.Deaths);
                aps->setUInt(idx++, match.Assists);
                aps->setUInt(idx++, match.Headshots);
                aps->setUInt(idx++, match.HighestKillStreak);
                aps->setUInt(idx++, match.MeleeKills);
                aps->setUInt(idx++, match.RifleKills);
                aps->setUInt(idx++, match.ShotgunKills);
                aps->setUInt(idx++, match.SniperKills);
                aps->setUInt(idx++, match.GatlingKills);
                aps->setUInt(idx++, match.BazookaKills);
                aps->setUInt(idx++, match.GrenadeKills);
                aps->setUInt(idx++, match.ZombieKills);
                aps->setUInt(idx++, match.Infections);
                aps->setUInt64(idx++, match.MatchStartTime);
                aps->setString(idx++, match.MatchStartUtc);
                aps->setUInt64(idx++, match.MatchEndTime);
                aps->setString(idx++, match.MatchEndUtc);
                aps->setUInt(idx++, match.Hair);
                aps->setUInt(idx++, match.Face);
                aps->setUInt(idx++, match.Upper);
                aps->setUInt(idx++, match.Under);
                aps->setUInt(idx++, match.Skirt);
                aps->setUInt(idx++, match.Gloves);
                aps->setUInt(idx++, match.Boots);
                aps->setUInt(idx++, match.HeadAcc);
                aps->setUInt(idx++, match.WaistAcc);
                aps->setUInt(idx++, match.BackAcc);
                aps->setUInt(idx++, match.Melee);
                aps->setUInt(idx++, match.Rifle);
                aps->setUInt(idx++, match.Shotgun);
                aps->setUInt(idx++, match.Sniper);
                aps->setUInt(idx++, match.Gatling);
                aps->setUInt(idx++, match.Bazooka);
                aps->setUInt(idx++, match.Grenade);
                aps->setUInt(idx++, match.IsItemReward ? match.reward_item : 0);
				aps->setBoolean(idx++, match.IsMvp);
				aps->setBoolean(idx++, match.IsEntryFragger);
				aps->setBoolean(idx++, match.IsBullseye);
				aps->setBoolean(idx++, match.IsSupport);
				aps->setBoolean(idx++, match.IsBomba);
                aps->setUInt(idx++, match.MvpScore);
                aps->setUInt(idx++, match.EntryFraggerScore);
                aps->setUInt(idx++, match.BullseyeScore);
                aps->setUInt(idx++, match.SupportScore);
                aps->setUInt(idx++, match.BombaScore);
                aps->setUInt(idx++, match.BestKdScore);
                aps->setUInt(idx++, match.CaptureScore);
                aps->setUInt(idx++, match.WonRoundScore);
                aps->setUInt(idx++, match.ArmsRaceScore);
                aps->setUInt(idx++, match.ZombieScore);
                aps->setUInt(idx++, match.ADR);
                aps->setBoolean(idx++, match.IsParty);
                aps->setUInt(idx++, match.Restriction);
                aps->setUInt(idx++, match.MaxPlayers);
            }
            auto inserted = aps->executeUpdate();
            if (!inserted)
            {
                DEBUGLOG(red, "Failed to add items for account {}: expected {}, got {}", v.aid, v.items_added.size(), inserted);
                return std::unexpected(DbError{ DbError::Type::NoRowsAffected,0,{},fmt::format("PersistItemAdds: expected {} adds, got {}", v.items_added.size(), inserted) });
            }
            DEBUGLOG(green, "Added {} matches to history for account {}", inserted, v.aid);
            return {};
        }
        catch (sql::SQLException& e)
        {
            DEBUGLOG(red, "PersistItemAdds sql exception: ({})", e.what());
            return std::unexpected(CMariaDatabase::FromSQLException(e));
        }
    }

    std::expected<void, DbError> CMariaDatabase::PersistPlayerSessionsPatches(ValidatedDbUpdates& v)
    {
        try
        {
            if (v.player_sessions_patches.empty()) return {};

            std::vector<std::pair<uint32_t, uint64_t>> to_insert;
            std::vector<std::pair<uint32_t, uint64_t>> to_delete;
            to_insert.reserve(v.player_sessions_patches.size());
            to_delete.reserve(v.player_sessions_patches.size());

            for (const auto& p : v.player_sessions_patches) 
            {
                if (p.aid == 0 || p.key == 0) 
                {
                    DEBUGLOG(yellow,
                                             "PersistPlayerSessionsPatches skipping invalid patch (aid={}, key={}, op={})",
                                             p.aid, p.key, static_cast<uint8_t>(p.op));
                    continue;
                }
                (p.op == PlayerSessionsPatch::Op::Insert ? to_insert : to_delete).emplace_back(p.aid, p.key);
            }
            if (to_insert.empty() && to_delete.empty()) return {};

            if (!to_delete.empty()) 
            {
                std::string q = "DELETE FROM player_sessions WHERE (PlayerId, AuthKey) IN " + GenerateInTuples(to_delete.size(), 2);

                auto* ps = Prep(q);
                std::size_t k = 1;
                for (const auto& [aid, key] : to_delete) 
                {
                    ps->setUInt(k++, aid);
                    ps->setUInt64(k++, key);
                }
                auto affected = ps->executeUpdate();
                DEBUGLOG(green, "PersistPlayerSessionsPatches deleted {} rows", affected);
            }

            if (!to_insert.empty()) 
            {
                std::string q = "INSERT INTO player_sessions (PlayerId, AuthKey) VALUES " + GenerateQuestionMarks(to_insert.size(), 2);

                auto* ps = Prep(q);
                std::size_t k = 1;
                for (const auto& [aid, key] : to_insert) 
                {
                    ps->setUInt(k++, aid);
                    ps->setUInt64(k++, key);
                }
                auto affected = ps->executeUpdate();
                DEBUGLOG(green, "PersistPlayerSessionsPatches inserted {} rows", affected);
            }
            return {};
        }
        catch (sql::SQLException& e) 
        {
            DEBUGLOG(red, "PersistPlayerSessionsPatches sql exception: {}", e.what());
            return std::unexpected(CMariaDatabase::FromSQLException(e));
        }
    }

    std::expected<void, DbError> CMariaDatabase::PersistPlayerSocialsPatches(ValidatedDbUpdates& v, ResultDbUpdateInfo& out)
    {
        try
        {
            if (v.player_social_patches.empty()) return {};

            auto* selAidByNick = Prep("SELECT Id FROM accounts WHERE Nickname = ? LIMIT 1");

            auto* selSocialState = Prep("SELECT State FROM player_socials WHERE Aid = ? AND TargetAid = ? LIMIT 1");

            std::vector<PlayerSocialPatch> to_upsert;
            std::vector<PlayerSocialPatch> to_delete;
            to_upsert.reserve(v.player_social_patches.size());
            to_delete.reserve(v.player_social_patches.size());

            constexpr uint8_t kBlockedState = NetEngine::Socials::State::Blocked;
            constexpr uint8_t kAcceptedState = NetEngine::Socials::State::Accepted;

            for (const auto& p : v.player_social_patches)
            {
                if (!p.aid) continue;

                PlayerSocialPatch tmp = p;
                if (!tmp.targetAid && tmp.TargetNickname.has_value() && !tmp.TargetNickname->empty())
                {
                    selAidByNick->setString(1, tmp.TargetNickname.value());
                    std::unique_ptr<sql::ResultSet> rs(selAidByNick->executeQuery());
                    if (rs->next()) tmp.targetAid = rs->getUInt(1);
                    else
                    {
                        out.target_not_found = true;
                        continue;
                    }
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
                    selSocialState->setUInt(1, tmp.aid);
                    selSocialState->setUInt(2, tmp.targetAid);
                    {
                        std::unique_ptr<sql::ResultSet> st(selSocialState->executeQuery());
                        if (st->next() && st->getByte(1) == kBlockedState)
                            out.target_blocked = true;
                    }
                    selSocialState->setUInt(1, tmp.targetAid);
                    selSocialState->setUInt(2, tmp.aid);
                    {
                        std::unique_ptr<sql::ResultSet> st(selSocialState->executeQuery());
                        if (st->next() && st->getByte(1) == kBlockedState)
                            out.player_blocked = true;
                    }
                    if (tmp.State.has_value() && tmp.State.value() == kAcceptedState)
                    {
                        PlayerSocialPatch mir = tmp;
                        std::swap(mir.aid, mir.targetAid);
                        to_upsert.push_back(std::move(mir));
                    }
                    if (tmp.State.has_value() && tmp.State.value() == kBlockedState)
                    {
                        PlayerSocialPatch mir = tmp;
						std::swap(mir.aid, mir.targetAid); // delete from other player's side
                        to_delete.push_back(std::move(mir)); 
                    }
                    to_upsert.push_back(std::move(tmp));
                    
                }
            }

            if (!to_delete.empty())
            {
                std::string q = "DELETE FROM player_socials WHERE (Aid, TargetAid) IN " + GenerateInTuples(to_delete.size(), 2);
                auto* ps = Prep(q);
                size_t k = 1;
                for (const auto& d : to_delete)
                {
                    ps->setUInt(k++, d.aid);
                    ps->setUInt(k++, d.targetAid);
                }
                ps->executeUpdate();
            }

            if (!to_upsert.empty())
            {
                std::string q =
                    "INSERT INTO player_socials (Aid, TargetAid, State) VALUES "
                    + GenerateQuestionMarks(to_upsert.size(), 3)
                    + " ON DUPLICATE KEY UPDATE State = VALUES(State)";
                auto* ps = Prep(q);
                size_t k = 1;
                for (const auto& u : to_upsert)
                {
                    ps->setUInt(k++, u.aid);
                    ps->setUInt(k++, u.targetAid);
                    ps->setByte(k++, u.State.value_or(0));
                }
                ps->executeUpdate();
            }

            return {};
        }
        catch (sql::SQLException& e)
        {
            DEBUGLOG(red, "PersistPlayerSocialsPatches sql exception: {}", e.what());
            return std::unexpected(CMariaDatabase::FromSQLException(e));
        }
    }

    std::expected<void, DbError> CMariaDatabase::PersistGachaPityPatches(ValidatedDbUpdates& v)
    {
        try
        {
            if (v.gacha_pity_patches.empty()) return {};

            auto* pstmt = Prep(
                "INSERT INTO player_gacha_pity (PlayerId, GachaType, LuckyPoints) VALUES (?, ?, ?) "
                "ON DUPLICATE KEY UPDATE LuckyPoints = VALUES(LuckyPoints)");

            for (const auto& patch : v.gacha_pity_patches)
            {
                pstmt->setUInt(1, v.aid);
                pstmt->setUInt(2, patch.gacha_type);
                pstmt->setUInt(3, patch.lucky_points);
                pstmt->executeUpdate();
            }
            return {};
        }
        catch (sql::SQLException& e)
        {
            DEBUGLOG(red, "PersistGachaPityPatches sql exception: {}", e.what());
            return std::unexpected(CMariaDatabase::FromSQLException(e));
        }
    }

    std::expected<void, DbError> CMariaDatabase::UpdateAccount(ValidatedDbUpdates& v, ResultDbUpdateInfo& out)
    {
        try 
        {
            if (!EnsureConnected()) return std::unexpected(DbError{ DbError::Type::ConnectionLost, 0, {}, "Not connected" });;
            std::unique_ptr<sql::Statement> stmt(GetConnection()->createStatement());
            stmt->execute("START TRANSACTION");
            TransactionGuard tx_guard;
            auto fail = [&](DbError err, std::string_view reason, std::source_location loc = std::source_location::current()) 
            {
                try { stmt->execute("ROLLBACK"); } catch(...) {}
				BaseLib::EventLog->Debug(loc, DEBUG, NONE, red, "Update account failed: {}", reason);
                return std::unexpected(err);
            };
			auto aid_str = std::to_string(v.aid);

            if (auto r = PersistCurrenciesPatches(v); !r.has_value()) 
                return fail(r.error(), "PersistCurrenciesPatches aid " + aid_str);

            if (auto r = PersistAccountInfoPatches(v); !r.has_value()) 
                return fail(r.error(), "PersistAccountInfoPatches aid " + aid_str);

            if (auto r = PersistItemDeletes(v, out); !r.has_value()) 
                return fail(r.error(), "PersistItemDeletes aid " + aid_str);

            if (auto r = PersistItemPatches(v, out); !r.has_value()) 
                return fail(r.error(), "PersistItemPatches aid " + aid_str);

            if (auto r = PersistItemAdds(v, out); !r.has_value()) 
                return fail(r.error(), "PersistItemAdds aid " + aid_str);

            if (auto r = PersistMissionsPatches(v); !r.has_value()) 
                return fail(r.error(), "PersistMissionsPatches aid " + aid_str);

			if (auto r = PersistMailboxPatches(v); !r.has_value())
				return fail(r.error(), "PersistMailboxPatches aid " + aid_str);

            if (auto r = PersistMonthlyRewardsPatches(v); !r.has_value())
                return fail(r.error(), "PersistMonthlyRewardsPatches aid " + aid_str);

            if (auto r = PersistWeeklyRewardsPatches(v); !r.has_value())
                return fail(r.error(), "PersistWeeklyRewardsPatches aid " + aid_str);

            if (auto r = PersistPlaytimePatches(v); !r.has_value())
                return fail(r.error(), "PersistPlaytimePatches aid " + aid_str);

            if (auto r = PersistBattlePassPatches(v); !r.has_value())
                return fail(r.error(), "PersistBattlePassPatches aid " + aid_str);

			if (auto r = PersistMatchHistoryAdds(v); !r.has_value())
				return fail(r.error(), "PersistMatchHistoryAdds aid " + aid_str);

			if (auto r = PersistPlayerSessionsPatches(v); !r.has_value())
				return fail(r.error(), "PersistPlayerSessionsPatches aid " + aid_str);

            if (auto r = PersistPlayerSocialsPatches(v, out); !r.has_value())
				return fail(r.error(), "PersistPlayerSocialsPatches aid " + aid_str);

            if (auto r = PersistGachaPityPatches(v); !r.has_value())
				return fail(r.error(), "PersistGachaPityPatches aid " + aid_str);

            stmt->execute("COMMIT");
            return {};
        }
        catch (sql::SQLException& e) 
        {
            try { std::unique_ptr<sql::Statement> rb(GetConnection()->createStatement()); rb->execute("ROLLBACK"); } catch(...) {}
            auto err = CMariaDatabase::FromSQLException(e);
            DEBUGLOG(red, "UpdateAccount SQL error: {} (code {}, state {})", err.message, err.error_code, err.sql_state);
            return std::unexpected(err);
        }
    }

    std::expected<void, DbError> CMariaDatabase::UpdateAccounts(std::vector<ValidatedDbUpdates>& batch, std::vector<ResultDbUpdateInfo>& results)
    {
        try 
        {
            if (!EnsureConnected()) return std::unexpected(DbError{ DbError::Type::ConnectionLost, 0, {}, "Not connected" });;
            std::unique_ptr<sql::Statement> stmt(GetConnection()->createStatement());
            stmt->execute("START TRANSACTION");
            TransactionGuard tx_guard;
            auto fail = [&](DbError err, std::string_view reason, std::source_location loc = std::source_location::current()) 
            {
                try { stmt->execute("ROLLBACK"); } catch(...) {}
                BaseLib::EventLog->Debug(loc, DEBUG, NONE, red, "Update account failed: {}", reason);
                return std::unexpected(err);
            };
			results.resize(batch.size());
            for (auto i = 0; i < batch.size(); ++i)
            {
                auto& v   = batch[i];
                auto& out = results[i];
                auto aid_str = std::to_string(v.aid);

                if (auto r = PersistCurrenciesPatches(v); !r.has_value()) 
                    return fail(r.error(), "PersistCurrenciesPatches aid " + aid_str);

                if (auto r = PersistAccountInfoPatches(v); !r.has_value()) 
                    return fail(r.error(), "PersistAccountInfoPatches aid " + aid_str);

                if (auto r = PersistItemDeletes(v, out); !r.has_value()) 
                    return fail(r.error(), "PersistItemDeletes aid " + aid_str);

                if (auto r = PersistItemPatches(v, out); !r.has_value()) 
                    return fail(r.error(), "PersistItemPatches aid " + aid_str);

                if (auto r = PersistItemAdds(v, out); !r.has_value()) 
                    return fail(r.error(), "PersistItemAdds aid " + aid_str);

                if (auto r = PersistMissionsPatches(v); !r.has_value()) 
                    return fail(r.error(), "PersistMissionsPatches aid " + aid_str);

                if (auto r = PersistMailboxPatches(v); !r.has_value())
                    return fail(r.error(), "PersistMailboxPatches aid " + aid_str);

                if (auto r = PersistMonthlyRewardsPatches(v); !r.has_value())
                    return fail(r.error(), "PersistMonthlyRewardsPatches aid " + aid_str);

                if (auto r = PersistWeeklyRewardsPatches(v); !r.has_value())
                    return fail(r.error(), "PersistWeeklyRewardsPatches aid " + aid_str);

                if (auto r = PersistPlaytimePatches(v); !r.has_value())
                    return fail(r.error(), "PersistPlaytimePatches aid " + aid_str);

                if (auto r = PersistBattlePassPatches(v); !r.has_value())
                    return fail(r.error(), "PersistBattlePassPatches aid " + aid_str);

                if (auto r = PersistMatchHistoryAdds(v); !r.has_value())
                    return fail(r.error(), "PersistMatchHistoryAdds aid " + aid_str);

                if (auto r = PersistPlayerSessionsPatches(v); !r.has_value())
                    return fail(r.error(), "PersistPlayerSessionsPatches aid " + aid_str);

                if (auto r = PersistPlayerSocialsPatches(v, out); !r.has_value())
                    return fail(r.error(), "PersistPlayerSocialsPatches aid " + aid_str);

                if (auto r = PersistGachaPityPatches(v); !r.has_value())
                    return fail(r.error(), "PersistGachaPityPatches aid " + aid_str);
            }
            stmt->execute("COMMIT");
            return {};
        }
        catch (sql::SQLException& e) 
        {
            try { std::unique_ptr<sql::Statement> rb(GetConnection()->createStatement()); rb->execute("ROLLBACK"); } catch(...) {}
            auto err = CMariaDatabase::FromSQLException(e);
            DEBUGLOG(red, "UpdateAccount SQL error: {} (code {}, state {})", err.message, err.error_code, err.sql_state);
            return std::unexpected(err);
        }
    }

    std::expected<void, DbError> CMariaDatabase::InsertAccount(const std::string& username, const std::string& password_hash, const std::string& salt, const std::string& nickname)
    {
        try
        {
            if (auto r = EnsureConnected(); !r.has_value()) return r;

            auto* pstmt = Prep(
                "INSERT INTO accounts (Username, Password, Salt, Nickname, Level, MaximumEnergy) "
                "VALUES (?, ?, ?, ?, 0, 1000)"
            );
            pstmt->setString(1, username);
            pstmt->setString(2, password_hash);
            pstmt->setString(3, salt);
            pstmt->setString(4, nickname);

            if (!pstmt->executeUpdate())
                return std::unexpected(DbError{ DbError::Type::NoRowsAffected, 0, {}, "Failed to insert account" });

            DEBUGLOG(green, "Created account: username={}, nickname={}", username, nickname);
            return {};
        }
        catch (sql::SQLException& e)
        {
            DEBUGLOG(red, "InsertAccount sql exception: {}", e.what());
            if (e.getErrorCode() == 1062) // Duplicate entry
                return std::unexpected(DbError{ DbError::Type::DuplicateNickname, e.getErrorCode(), e.getSQLState().c_str(), "Username or nickname already exists" });
            return std::unexpected(CMariaDatabase::FromSQLException(e));
        }
    }

    std::expected<int32_t, DbError> CMariaDatabase::GetAccountIdByNickname(std::string_view nickname)
    {
        try
        {
            if (auto r = EnsureConnected(); !r.has_value())
                return std::unexpected(r.error());

            auto* stmt = Prep("SELECT Id FROM accounts WHERE LOWER(Nickname) = LOWER(?) LIMIT 1");
            stmt->setString(1, std::string(nickname));
            auto result = stmt->executeQuery();
            if (!result->next())
                return std::unexpected(DbError{ DbError::Type::NicknameNotFound, 0, {}, "Nickname not found" });

            return static_cast<int32_t>(result->getUInt(1));
        }
        catch (sql::SQLException& e)
        {
            return std::unexpected(CMariaDatabase::FromSQLException(e));
        }
    }

    std::expected<bool, DbError> CMariaDatabase::AccountExists(const int32_t aid)
    {
        try
        {
            if (auto r = EnsureConnected(); !r.has_value())
                return std::unexpected(r.error());

            auto* stmt = Prep("SELECT Id FROM accounts WHERE Id = ? LIMIT 1");
            stmt->setUInt(1, static_cast<uint32_t>(aid));
            auto result = stmt->executeQuery();
            return result->next();
        }
        catch (sql::SQLException& e)
        {
            return std::unexpected(CMariaDatabase::FromSQLException(e));
        }
    }

    std::expected<void, DbError> CMariaDatabase::SetAccountMutedUntil(const int32_t aid, const uint64_t muted_until)
    {
        try
        {
            if (auto r = EnsureConnected(); !r.has_value())
                return r;

            auto* stmt = Prep("UPDATE accounts SET MutedUntil = ? WHERE Id = ?");
            stmt->setUInt64(1, muted_until);
            stmt->setUInt(2, static_cast<uint32_t>(aid));
            if (!stmt->executeUpdate())
                return std::unexpected(DbError{ DbError::Type::NoRowsAffected, 0, {}, "Failed to update mute" });

            return {};
        }
        catch (sql::SQLException& e)
        {
            return std::unexpected(CMariaDatabase::FromSQLException(e));
        }
    }

    std::expected<void, DbError> CMariaDatabase::UpsertAccountBan(const int32_t aid, const uint64_t unban_unix, const std::string_view reason)
    {
        try
        {
            if (auto r = EnsureConnected(); !r.has_value())
                return r;

            auto* delete_stmt = Prep("DELETE FROM bans WHERE AccountId = ?");
            delete_stmt->setUInt(1, static_cast<uint32_t>(aid));
            delete_stmt->executeUpdate();

            auto* insert_stmt = Prep("INSERT INTO bans (AccountId, UnbanDate, Reason) VALUES (?, FROM_UNIXTIME(?), ?)");
            insert_stmt->setUInt(1, static_cast<uint32_t>(aid));
            insert_stmt->setUInt64(2, unban_unix);
            insert_stmt->setString(3, std::string(reason.substr(0, 127)));
            if (!insert_stmt->executeUpdate())
                return std::unexpected(DbError{ DbError::Type::NoRowsAffected, 0, {}, "Failed to insert ban" });

            return {};
        }
        catch (sql::SQLException& e)
        {
            return std::unexpected(CMariaDatabase::FromSQLException(e));
        }
    }

    std::expected<void, DbError> CMariaDatabase::RemoveAccountBan(const int32_t aid)
    {
        try
        {
            if (auto r = EnsureConnected(); !r.has_value())
                return r;

            auto* stmt = Prep("DELETE FROM bans WHERE AccountId = ?");
            stmt->setUInt(1, static_cast<uint32_t>(aid));
            stmt->executeUpdate();
            return {};
        }
        catch (sql::SQLException& e)
        {
            return std::unexpected(CMariaDatabase::FromSQLException(e));
        }
    }

    std::expected<std::optional<AccountPenaltyInfo>, DbError> CMariaDatabase::GetActiveBan(const int32_t aid)
    {
        try
        {
            if (auto r = EnsureConnected(); !r.has_value())
                return std::unexpected(r.error());

            auto* stmt = Prep(
                "SELECT AccountId, UNIX_TIMESTAMP(UnbanDate) AS UnbanUnix, COALESCE(Reason, '') AS Reason "
                "FROM bans WHERE AccountId = ? AND UnbanDate > UTC_TIMESTAMP(6) "
                "ORDER BY UnbanDate DESC LIMIT 1");
            stmt->setUInt(1, static_cast<uint32_t>(aid));
            auto result = stmt->executeQuery();
            if (!result->next())
                return std::optional<AccountPenaltyInfo>{};

            return std::optional<AccountPenaltyInfo>{ AccountPenaltyInfo{
                .account_id = static_cast<int32_t>(result->getUInt("AccountId")),
                .until_unix = result->getUInt64("UnbanUnix"),
                .reason = result->getString("Reason").c_str(),
            } };
        }
        catch (sql::SQLException& e)
        {
            return std::unexpected(CMariaDatabase::FromSQLException(e));
        }
    }

    std::expected<void, DbError> CMariaDatabase::PersistChatLogs(const std::vector<ChatLogEntry>& logs)
    {
        try
        {
            if (logs.empty()) return {};

            std::string sql =
                "INSERT INTO player_chatlogs (Aid, TargetAid, ChatType, ChatLocation, ServerId, RoomId, PlazaId, ClanId, Message) VALUES "
                + GenerateQuestionMarks(logs.size(), 9);

            auto* ps = Prep(sql);
            int idx = 1;

            for (const auto& log : logs)
            {
                ps->setInt(idx++, log.aid);
                log.target_aid.has_value() ? ps->setInt(idx++, log.target_aid.value()) : ps->setNull(idx++, 0);
                ps->setString(idx++, ChatLog::TypeToString(log.chat_type));
                log.location.has_value() ? ps->setString(idx++, ChatLog::LocationToString(log.location.value())) : ps->setNull(idx++, 0);
                ps->setUInt(idx++, log.server_id);
                log.room_id.has_value() ? ps->setUInt(idx++, log.room_id.value()) : ps->setNull(idx++, 0);
                log.plaza_id.has_value() ? ps->setUInt(idx++, log.plaza_id.value()) : ps->setNull(idx++, 0);
                log.clan_id.has_value() ? ps->setUInt(idx++, log.clan_id.value()) : ps->setNull(idx++, 0);
                ps->setString(idx++, log.message.substr(0, 256));
            }

            ps->executeUpdate();
            DEBUGLOG(green, "Persisted {} chat logs", logs.size());
            return {};
        }
        catch (sql::SQLException& e)
        {
            DEBUGLOG(red, "PersistChatLogs sql exception: {}", e.what());
            return std::unexpected(CMariaDatabase::FromSQLException(e));
        }
    }

    std::expected<void, DbError> CMariaDatabase::PersistItemLogs(const std::vector<ItemLogEntry>& logs)
    {
        try
        {
            if (logs.empty()) return {};

            std::string sql =
                "INSERT INTO player_itemlogs (Aid, RelatedAid, ActionType, ItemId, ItemType, SerialInfo, OriginType, MpDelta, RtDelta, CouponDelta, EnergyDelta, NewItemId, NewRepair) VALUES "
                + GenerateQuestionMarks(logs.size(), 13);

            auto* ps = Prep(sql);
            int idx = 1;

            for (const auto& log : logs)
            {
                ps->setInt(idx++, log.aid);
                log.related_aid.has_value() ? ps->setInt(idx++, log.related_aid.value()) : ps->setNull(idx++, 0);
                ps->setString(idx++, ItemLog::ActionTypeToString(log.action_type));
                ps->setUInt(idx++, log.item_id);
                log.item_type.has_value() ? ps->setString(idx++, ItemLog::ItemTypeToString(log.item_type.value())) : ps->setNull(idx++, 0);
                log.serial_info.has_value() ? ps->setUInt64(idx++, log.serial_info.value()) : ps->setNull(idx++, 0);
                ps->setString(idx++, ItemLog::OriginTypeToString(log.origin_type));
                ps->setInt(idx++, log.mp_delta);
                ps->setInt(idx++, log.rt_delta);
                ps->setInt(idx++, log.coupon_delta);
                ps->setInt(idx++, log.energy_delta);
                log.new_item_id.has_value() ? ps->setUInt(idx++, log.new_item_id.value()) : ps->setNull(idx++, 0);
                log.new_repair.has_value() ? ps->setUInt(idx++, log.new_repair.value()) : ps->setNull(idx++, 0);
            }

            ps->executeUpdate();
            DEBUGLOG(green, "Persisted {} item logs", logs.size());
            return {};
        }
        catch (sql::SQLException& e)
        {
            DEBUGLOG(red, "PersistItemLogs sql exception: {}", e.what());
            return std::unexpected(CMariaDatabase::FromSQLException(e));
        }
    }

    std::expected<void, DbError> CMariaDatabase::PersistCurrencyLogs(const std::vector<CurrencyLogEntry>& logs)
    {
        try
        {
            if (logs.empty()) return {};

            std::string sql =
                "INSERT INTO player_currencylogs (Aid, CurrencyType, Amount, BeforeValue, AfterValue, SourceType, RelatedItemId) VALUES "
                + GenerateQuestionMarks(logs.size(), 7);

            auto* ps = Prep(sql);
            int idx = 1;

            for (const auto& log : logs)
            {
                ps->setInt(idx++, log.aid);
                ps->setString(idx++, CurrencyLog::TypeToString(log.currency_type));
                ps->setInt(idx++, log.amount);
                ps->setUInt64(idx++, log.before_value);
                ps->setUInt64(idx++, log.after_value);
                ps->setString(idx++, CurrencyLog::SourceTypeToString(log.source_type));
                log.related_item_id.has_value() ? ps->setUInt(idx++, log.related_item_id.value()) : ps->setNull(idx++, 0);
            }

            ps->executeUpdate();
            DEBUGLOG(green, "Persisted {} currency logs", logs.size());
            return {};
        }
        catch (sql::SQLException& e)
        {
            DEBUGLOG(red, "PersistCurrencyLogs sql exception: {}", e.what());
            return std::unexpected(CMariaDatabase::FromSQLException(e));
        }
    }

    std::expected<void, DbError> CMariaDatabase::PersistRoomLogs(const std::vector<RoomLogEntry>& logs)
    {
        try
        {
            if (logs.empty()) return {};

            std::string sql =
                "INSERT INTO player_roomlogs (Aid, TargetAid, EventType, ServerId, RoomId, HostAid, TeamId, NewTeamId, VoteKickReason, OldValue, NewValue) VALUES "
                + GenerateQuestionMarks(logs.size(), 11);

            auto* ps = Prep(sql);
            int idx = 1;

            for (const auto& log : logs)
            {
                ps->setInt(idx++, log.aid);
                log.target_aid.has_value() ? ps->setInt(idx++, log.target_aid.value()) : ps->setNull(idx++, 0);
                ps->setString(idx++, RoomLog::EventTypeToString(log.event_type));
                ps->setUInt(idx++, log.server_id);
                ps->setUInt(idx++, log.room_id);
                log.host_aid.has_value() ? ps->setInt(idx++, log.host_aid.value()) : ps->setNull(idx++, 0);
                log.team_id.has_value() ? ps->setUInt(idx++, log.team_id.value()) : ps->setNull(idx++, 0);
                log.new_team_id.has_value() ? ps->setUInt(idx++, log.new_team_id.value()) : ps->setNull(idx++, 0);
                log.votekick_reason.has_value() ? ps->setUInt(idx++, log.votekick_reason.value()) : ps->setNull(idx++, 0);
                log.old_value.has_value() ? ps->setInt(idx++, log.old_value.value()) : ps->setNull(idx++, 0);
                log.new_value.has_value() ? ps->setInt(idx++, log.new_value.value()) : ps->setNull(idx++, 0);
            }

            ps->executeUpdate();
            DEBUGLOG(green, "Persisted {} room logs", logs.size());
            return {};
        }
        catch (sql::SQLException& e)
        {
            DEBUGLOG(red, "PersistRoomLogs sql exception: {}", e.what());
            return std::unexpected(CMariaDatabase::FromSQLException(e));
        }
    }

    std::expected<void, DbError> CMariaDatabase::PersistPlayerMatchSessionsAdds(const std::vector<PlayerMatchSessionAdd>& adds)
    {
        try
        {
            if (adds.empty()) return {};

            std::string sql =
                "INSERT INTO player_match_sessions (MatchUniqueId, Aid, TeamId, JoinedMs, LeftMs, Reason) VALUES "
                + GenerateQuestionMarks(adds.size(), 6);

            auto* ps = Prep(sql);
            int idx = 1;

            for (const auto& add : adds)
            {
                ps->setString(idx++, add.match_unique_id);
                ps->setInt(idx++, add.aid);
                ps->setUInt(idx++, add.team_id);
                ps->setUInt64(idx++, add.joined_ms);
                ps->setUInt64(idx++, add.left_ms);
                ps->setString(idx++, add.reason);
            }

            ps->executeUpdate();
            DEBUGLOG(green, "Persisted {} match sessions", adds.size());
            return {};
        }
        catch (sql::SQLException& e)
        {
            DEBUGLOG(red, "PersistPlayerMatchSessionsAdds sql exception: {}", e.what());
            return std::unexpected(CMariaDatabase::FromSQLException(e));
        }
    }

    std::expected<void, DbError> CMariaDatabase::PersistPlayerMatchCombatAdds(const std::vector<PlayerMatchCombatAdd>& adds)
    {
        try
        {
            if (adds.empty()) return {};

            std::string sql =
                "INSERT INTO player_match_combat (MatchUniqueId, AttackerAid, VictimAid, Weapon, BodyPart, HitVariant, Damage, VictimHpAfter, IsKill, EventMs) VALUES "
                + GenerateQuestionMarks(adds.size(), 10);

            auto* ps = Prep(sql);
            int idx = 1;

            for (const auto& add : adds)
            {
                ps->setString(idx++, add.match_unique_id);
                ps->setInt(idx++, add.attacker_aid);
                ps->setInt(idx++, add.victim_aid);
                ps->setUInt(idx++, add.weapon);
                ps->setUInt(idx++, add.bodypart);
                ps->setUInt(idx++, add.hit_variant);
                ps->setUInt(idx++, add.damage);
                ps->setUInt(idx++, add.victim_hp_after);
                ps->setUInt(idx++, add.is_kill);
                ps->setUInt64(idx++, add.event_ms);
            }

            ps->executeUpdate();
            DEBUGLOG(green, "Persisted {} match combat hits", adds.size());
            return {};
        }
        catch (sql::SQLException& e)
        {
            DEBUGLOG(red, "PersistPlayerMatchCombatAdds sql exception: {}", e.what());
            return std::unexpected(CMariaDatabase::FromSQLException(e));
        }
    }

    std::expected<void, DbError> CMariaDatabase::PersistPlayerMatchEventAdds(const std::vector<PlayerMatchEventAdd>& adds)
    {
        try
        {
            if (adds.empty()) return {};

            std::string sql =
                "INSERT INTO player_match_events (MatchUniqueId, Aid, EventType, SubA, SubB, Value, EventMs) VALUES "
                + GenerateQuestionMarks(adds.size(), 7);

            auto* ps = Prep(sql);
            int idx = 1;

            for (const auto& add : adds)
            {
                ps->setString(idx++, add.match_unique_id);
                ps->setInt(idx++, add.aid);
                ps->setUInt(idx++, add.event_type);
                ps->setUInt(idx++, add.sub_a);
                ps->setUInt(idx++, add.sub_b);
                ps->setUInt(idx++, add.value);
                ps->setUInt64(idx++, add.event_ms);
            }

            ps->executeUpdate();
            DEBUGLOG(green, "Persisted {} match events", adds.size());
            return {};
        }
        catch (sql::SQLException& e)
        {
            DEBUGLOG(red, "PersistPlayerMatchEventAdds sql exception: {}", e.what());
            return std::unexpected(CMariaDatabase::FromSQLException(e));
        }
    }

    std::expected<void, DbError> CMariaDatabase::PersistAcDetectionLogs(const std::vector<AcDetectionLogEntry>& logs)
    {
        try
        {
            if (logs.empty()) return {};

            constexpr std::size_t kMaxDetectionDetailsLength = 64;
            struct NormalizedDetectionLog
            {
                int32_t aid{ 0 };
                std::string ip;
                std::string hwid;
                std::string detection_flag;
                uint32_t extra{ 0 };
                std::string details;
                uint32_t server_id{ 0 };
            };

            auto normalizeDetails = [&](const std::string& details) -> std::string
            {
                return details.size() <= kMaxDetectionDetailsLength
                    ? details
                    : details.substr(0, kMaxDetectionDetailsLength);
            };

            auto appendKeyPart = [](std::string& key, const std::string& value)
            {
                key += std::to_string(value.size());
                key.push_back(':');
                key += value;
                key.push_back('|');
            };

            auto makeDedupeKey = [&](int32_t aid,
                                     const std::string& hwid,
                                     const std::string& detectionFlag,
                                     uint32_t extra,
                                     const std::string& details) -> std::string
            {
                std::string key;
                key.reserve(hwid.size() + detectionFlag.size() + details.size() + 64);

                if (aid > 0)
                    appendKeyPart(key, std::string("aid:") + std::to_string(aid));
                else
                    appendKeyPart(key, std::string("hwid:") + hwid);

                appendKeyPart(key, detectionFlag);
                appendKeyPart(key, std::to_string(extra));
                appendKeyPart(key, details);
                return key;
            };

            std::vector<NormalizedDetectionLog> uniqueLogs;
            uniqueLogs.reserve(logs.size());

            boost::unordered_flat_set<std::string> seenInBatch;
            seenInBatch.reserve(logs.size());

            for (const auto& log : logs)
            {
                NormalizedDetectionLog normalized;
                normalized.aid = log.aid;
                normalized.ip = log.ip;
                normalized.hwid = log.hwid;
                normalized.detection_flag = AcDetection::FlagToString(log.detection_flag);
                normalized.extra = log.extra;
                normalized.details = normalizeDetails(log.details);
                normalized.server_id = log.server_id;

                if (!seenInBatch.emplace(makeDedupeKey(
                        normalized.aid,
                        normalized.hwid,
                        normalized.detection_flag,
                        normalized.extra,
                        normalized.details)).second)
                {
                    continue;
                }

                uniqueLogs.push_back(std::move(normalized));
            }

            const std::size_t skippedBatchDuplicates = logs.size() - uniqueLogs.size();

            auto* existsPs = Prep(
                "SELECT 1 FROM ac_detections "
                "WHERE DetectionFlag = ? AND Extra = ? AND Details = ? "
                "AND CreatedAt >= DATE_SUB(CURRENT_TIMESTAMP(), INTERVAL 1 DAY) "
                "AND ((? > 0 AND Aid = ?) OR (? <= 0 AND Hwid = ?)) "
                "LIMIT 1"
            );

            std::vector<NormalizedDetectionLog> logsToInsert;
            logsToInsert.reserve(uniqueLogs.size());

            for (const auto& log : uniqueLogs)
            {
                existsPs->setString(1, log.detection_flag);
                existsPs->setUInt(2, log.extra);
                existsPs->setString(3, log.details);
                existsPs->setInt(4, log.aid);
                existsPs->setInt(5, log.aid);
                existsPs->setInt(6, log.aid);
                existsPs->setString(7, log.hwid);

                std::unique_ptr<sql::ResultSet> rs(existsPs->executeQuery());
                if (!rs->next())
                    logsToInsert.push_back(log);
            }

            const std::size_t skippedExistingDuplicates = uniqueLogs.size() - logsToInsert.size();
            const std::size_t totalSkipped = skippedBatchDuplicates + skippedExistingDuplicates;

            if (logsToInsert.empty())
            {
                DEBUGLOG(dark_cyan, "Skipped {} ac detection logs duplicated within the last 24 hours", totalSkipped);
                return {};
            }

            std::string sql =
                "INSERT INTO ac_detections (Aid, Ip, Hwid, DetectionFlag, Extra, Details, ServerId) VALUES "
                + GenerateQuestionMarks(logsToInsert.size(), 7);

            auto* ps = Prep(sql);
            int idx = 1;

            for (const auto& log : logsToInsert)
            {
                ps->setInt(idx++, log.aid);
                ps->setString(idx++, log.ip);
                ps->setString(idx++, log.hwid);
                ps->setString(idx++, log.detection_flag);
                ps->setUInt(idx++, log.extra);
                ps->setString(idx++, log.details);
                ps->setUInt(idx++, log.server_id);
            }

            ps->executeUpdate();
            DEBUGLOG(green, "Persisted {} ac detection logs (skipped {} duplicates within 24h)", logsToInsert.size(), totalSkipped);
            return {};
        }
        catch (sql::SQLException& e)
        {
            DEBUGLOG(red, "PersistAcDetectionLogs sql exception: {}", e.what());
            return std::unexpected(CMariaDatabase::FromSQLException(e));
        }
    }

    std::expected<void, DbError> CMariaDatabase::PersistAuthHistory(const AuthHistoryLogEntry& entry)
    {
        try
        {
            auto* ps = Prep(
                "INSERT INTO ac_auth_history (Aid, Ip, Hwid, ServerId) VALUES (?, ?, ?, ?)");
            ps->setInt(1, entry.aid);
            ps->setString(2, entry.ip);
            ps->setString(3, entry.hwid);
            ps->setUInt(4, entry.server_id);
            ps->executeUpdate();
            return {};
        }
        catch (sql::SQLException& e)
        {
            DEBUGLOG(red, "PersistAuthHistory sql exception: {}", e.what());
            return std::unexpected(CMariaDatabase::FromSQLException(e));
        }
    }

    std::expected<void, DbError> CMariaDatabase::PersistLogs(const LogContext& ctx)
    {
        try
        {
            if (ctx.empty()) return {};
            if (!EnsureConnected()) return std::unexpected(DbError{ DbError::Type::ConnectionLost, 0, {}, "Not connected" });

            std::unique_ptr<sql::Statement> stmt(GetConnection()->createStatement());
            stmt->execute("START TRANSACTION");
            TransactionGuard tx_guard;

            auto fail = [&](DbError err, std::string_view reason)
                {
                    try { stmt->execute("ROLLBACK"); }
                    catch (...) {}
                    DEBUGLOG(red, "PersistLogs failed: {}", reason);
                    return std::unexpected(err);
                };

            if (auto r = PersistChatLogs(ctx.chat_logs); !r.has_value())
                return fail(r.error(), "PersistChatLogs");

            if (auto r = PersistItemLogs(ctx.item_logs); !r.has_value())
                return fail(r.error(), "PersistItemLogs");

            if (auto r = PersistCurrencyLogs(ctx.currency_logs); !r.has_value())
                return fail(r.error(), "PersistCurrencyLogs");

            if (auto r = PersistRoomLogs(ctx.room_logs); !r.has_value())
                return fail(r.error(), "PersistRoomLogs");

            if (auto r = PersistAcDetectionLogs(ctx.ac_detection_logs); !r.has_value())
                return fail(r.error(), "PersistAcDetectionLogs");

            stmt->execute("COMMIT");
            return {};
        }
        catch (sql::SQLException& e)
        {
            try { std::unique_ptr<sql::Statement> rb(GetConnection()->createStatement()); rb->execute("ROLLBACK"); }
            catch (...) {}
            DEBUGLOG(red, "PersistLogs SQL error: {}", e.what());
            return std::unexpected(CMariaDatabase::FromSQLException(e));
        }
    }


    bool CMariaDatabase::GetMainFrontAccount(const uint64_t authKey, uint32_t server_id, FrontAccount* outFrontAccount, ClanInfo* outClanInfo, PlayerDailyMission* outDailyMission, std::vector<Item>& inv_items, std::vector<SocialInfo>& socials, std::vector<BlockedInfo>& blockeds, std::vector<FriendInfo>& friends, std::vector<MailboxInfo>& mailbox_list, std::vector<std::uint32_t>& daily_mission_random_ids, std::vector<GachaPityEntry>& gacha_pity, SystemMonthlyRewards* outMonthlyRewards, PlayerMonthlyReward* outPlayerMonthlyReward, SystemWeeklyRewards* outWeeklyRewards, PlayerWeeklyReward* outPlayerWeeklyReward)
    {
        try
        {
            auto* conn = GetConnection();
            if (!conn || !conn->isValid()) return false;

            std::unique_ptr<sql::Statement> stmt(conn->createStatement());
            stmt->execute("START TRANSACTION");
            TransactionGuard tx_guard;

            try
            {

                // 1. Load account and clan
                auto* pstmt = Prep(R"(
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
                    d.UpdateTime, d.Mission1, d.Mission2, d.Mission3, d.GoalMission1, d.GoalMission2, d.GoalMission3
                FROM player_sessions s
                JOIN accounts a ON s.PlayerId = a.Id
                LEFT JOIN clans c ON a.ClanId = c.Id
                LEFT JOIN player_daily_mission d ON a.Id = d.PlayerId
                WHERE s.AuthKey = ?
            )");



                pstmt->setUInt64(1, authKey);
                std::unique_ptr<sql::ResultSet> result(pstmt->executeQuery());

                if (!result->next())
                {
                    stmt->execute("ROLLBACK");
                    return false;
                }

                auto accId = static_cast<std::int32_t>(result->getUInt("Id"));
                auto ServerId = result->getUInt("ServerId");
                *outFrontAccount = FrontAccount(
                    accId,
                    ServerId,
                    result->getString("Username").c_str(),
                    result->getString("Password").c_str(),
                    result->getString("Salt").c_str(),
                    result->getByte("Grade"),
                    result->getByte("PCRoom"),
                    authKey,
                    result->getUInt("ClanId"),
                    result->getUInt("ClanKills"),
                    result->getUInt("ClanDeaths"),
                    result->getUInt("ClanAssists"),
                    result->getUInt("ClanContribution"),
                    result->getUInt("ClanWins"),
                    result->getUInt("ClanLoses"),
                    result->getUInt("ClanDraws"),
                    result->getString("Nickname").c_str(),
                    result->getUInt("Level"),
                    result->getUInt("Experience"),
                    result->getBoolean("Tutorial"),
                    result->getUInt("Story"),
                    result->getUInt("GuideMission"),
                    result->getUInt64("Achievement"),
                    result->getUInt64("VoiceType"),
                    result->getUInt("VIPExperience"),
                    result->getUInt("MaximumItems"),
                    result->getUInt("MaximumEnergy"),
                    result->getUInt("SelectedCharacter"),
                    result->getUInt64("PlayTime"),
                    result->getUInt64("MutedUntil"),
                    result->getUInt("Coins"),
                    result->getUInt("Energy"),
                    result->getUInt("LuckyPoints"),
                    result->getUInt("MicroPoints"),
                    result->getUInt("RockTokens"),
                    result->getUInt("Coupons"),
                    result->getUInt("Wins"),
                    result->getUInt("Loses"),
                    result->getUInt("Draws"),
                    result->getUInt("Kills"),
                    result->getUInt("Deaths"),
                    result->getUInt("Assists"),
                    result->getUInt("Headshots"),
                    result->getUInt("HighestKillStreak"),
                    result->getUInt("MeleeKills"),
                    result->getUInt("RifleKills"),
                    result->getUInt("ShotgunKills"),
                    result->getUInt("SniperKills"),
                    result->getUInt("GatlingKills"),
                    result->getUInt("BazookaKills"),
                    result->getUInt("GrenadeKills"),
                    result->getUInt("ZombieKills"),
                    result->getUInt("Infections"),
                    result->getUInt("SingleWaveDailyAttempts"),
                    result->getUInt("SingleWaveHighestWave"),
                    result->getUInt("SingleWaveHighScore"),
                    result->getUInt64("SingleWaveLastUpdate")
                );

                if (outClanInfo && result->getUInt("ClanId") > 0)
                {
                    *outClanInfo = ClanInfo(
                        result->getUInt("ClanId"),
                        result->getUInt("OwnerId"),
                        result->getString("ClanName").c_str(),
                        result->getUInt("ClanLogoFront"),
                        result->getUInt("ClanLogoBack")
                    );
                }

                if (outDailyMission)
                {
                    uint64_t now_time = Utility::GetUtcTimeNow64();
                    uint64_t last_6am = Utility::GetLast6AMUtc();
                    bool existed = !result->isNull("UpdateTime");
                    uint64_t last_update = existed ? result->getUInt64("UpdateTime") : 0;

                    PlayerDailyMission dm = {
                        accId,
                        last_update,
                        result->getUInt("Mission1"),
                        result->getUInt("Mission2"),
                        result->getUInt("Mission3"),
                        result->getUInt("GoalMission1"),
                        result->getUInt("GoalMission2"),
                        result->getUInt("GoalMission3")
                    };

                    if (last_update < last_6am)
                    {
                        DEBUGLOG(dark_cyan, "Daily mission outdated. Generating new missions.");
                        dm.mission1 = daily_mission_random_ids[0]; dm.goal_mission1 = 0;
                        dm.mission2 = daily_mission_random_ids[1]; dm.goal_mission2 = 0;
                        dm.mission3 = daily_mission_random_ids[2]; dm.goal_mission3 = 0;
                        dm.update_time = now_time;

                        if (existed)
                        {
                            auto* updateStmt = Prep(R"(
                            UPDATE player_daily_mission SET
                                UpdateTime = ?, Mission1 = ?, Mission2 = ?, Mission3 = ?,
                                GoalMission1 = ?, GoalMission2 = ?, GoalMission3 = ?
                            WHERE PlayerId = ?
                        )");

                            updateStmt->setUInt64(1, dm.update_time);
                            updateStmt->setUInt(2, dm.mission1);
                            updateStmt->setUInt(3, dm.mission2);
                            updateStmt->setUInt(4, dm.mission3);
                            updateStmt->setUInt(5, dm.goal_mission1);
                            updateStmt->setUInt(6, dm.goal_mission2);
							updateStmt->setUInt(7, dm.goal_mission3);
                            updateStmt->setUInt(8, accId);

                            updateStmt->executeUpdate();
                        }
                        else
                        {
                            auto* insertStmt = Prep(R"(
                            INSERT INTO player_daily_mission
                                (PlayerId, UpdateTime, Mission1, Mission2, Mission3, GoalMission1, GoalMission2, GoalMission3)
                            VALUES (?, ?, ?, ?, ?, ?, ?, ?)
                        )");

                            insertStmt->setUInt(1, accId);
                            insertStmt->setUInt64(2, dm.update_time);
                            insertStmt->setUInt(3, dm.mission1);
                            insertStmt->setUInt(4, dm.mission2);
                            insertStmt->setUInt(5, dm.mission3);
                            insertStmt->setUInt(6, dm.goal_mission1);
                            insertStmt->setUInt(7, dm.goal_mission2);
                            insertStmt->setUInt(8, dm.goal_mission3);

                            insertStmt->executeUpdate();
                        }
                    }

                    *outDailyMission = dm;
                }

                // 2. Inventory
                auto* invStmt = Prep("SELECT * FROM player_items WHERE PlayerId = ?");
                invStmt->setUInt(1, accId);
                std::unique_ptr<sql::ResultSet> invRes(invStmt->executeQuery());

                while (invRes->next())
                {
                    Item newItem;
                    NetEngine::Packets::Main::InventoryItemInfo newItemInfo;

#if defined(RELEASE_1_0_3)
                    newItemInfo.serial_info.data = invRes->getUInt64("SerialInfo");
                    newItemInfo.item_number.item_id = invRes->getUInt("ItemId");
                    newItemInfo.expire_date = invRes->getUInt("ExpirationDate");
                    newItemInfo.repair = invRes->getUInt("Repair");
                    newItemInfo.energy = invRes->getUInt("Energy");
                    newItem.stock = invRes->getUInt("Stock");
                    newItemInfo.item_number.stock = newItem.stock;
                    newItem.is_equipped = invRes->getByte("IsEquipped");
                    newItem.character_id = invRes->getByte("CharacterId");
                    newItem.in_database = 1;
                    newItem.item_info = newItemInfo;
#else
                    newItemInfo.serial_info.data = invRes->getUInt64("SerialInfo");
                    newItemInfo.item_number.item_id = invRes->getUInt("ItemId");
                    newItemInfo.expire_date = invRes->getUInt("ExpirationDate");
                    newItemInfo.repair = invRes->getUInt("Repair");
                    newItemInfo.energy = invRes->getUInt("Energy");
                    newItemInfo.item_type = invRes->getUInt("ItemType");
                    newItemInfo.is_sealed = invRes->getUInt("IsSealed");
                    newItemInfo.seal_level = invRes->getUInt("SealLevel");
                    newItemInfo.enhance_exp = invRes->getUInt("EnhanceExp");
                    newItemInfo.enhance_level = invRes->getUInt("EnhanceLevel");
                    newItem.stock = invRes->getUInt("Stock");
                    newItem.is_equipped = invRes->getByte("IsEquipped");
                    newItem.character_id = invRes->getByte("CharacterId");
                    newItem.in_database = 1;
                    newItem.item_info = newItemInfo;
#endif

                    inv_items.push_back(newItem);
                }


                auto* socialStmt = Prep(
                    "SELECT s.TargetAid, s.State, a.Nickname AS TargetNickname "
                    "FROM player_socials s "
                    "JOIN accounts a ON a.Id = s.TargetAid "
                    "WHERE s.Aid = ?");
                socialStmt->setInt(1, accId);
                std::unique_ptr<sql::ResultSet> socialRes(socialStmt->executeQuery());
                while (socialRes->next())
                    socials.push_back({ accId, socialRes->getInt("TargetAid"), static_cast<uint8_t>(socialRes->getByte("State")), socialRes->getString("TargetNickname").c_str() });

                /*
                // 3. Blocked players
                auto* blockStmt = Prep(
                    "SELECT BlockedPlayerId, BlockedNickname FROM player_ignores WHERE PlayerId = ?");
                blockStmt->setInt(1, accId);
                std::unique_ptr<sql::ResultSet> blockRes(blockStmt->executeQuery());
                while (blockRes->next())
                    blockeds.push_back({ accId, blockRes->getInt("BlockedPlayerId"), 0, blockRes->getString("BlockedNickname").c_str() });

                // 4. Friends
                auto* friendStmt = Prep(
                    "SELECT FriendPlayerId, State, FriendNickname FROM player_friends WHERE PlayerId = ?");
                friendStmt->setInt(1, accId);
                std::unique_ptr<sql::ResultSet> friendRes(friendStmt->executeQuery());
                while (friendRes->next())
                    friends.push_back({ accId, friendRes->getInt("FriendPlayerId"), static_cast<uint8_t>(friendRes->getByte("State")), 0, friendRes->getString("FriendNickname").c_str() });
                */

                // 5. Mailbox
                auto* mailStmt = Prep(R"(
                SELECT Id, SenderId, SenderNickname, ReceiverId, ReceiverNickname, Date, GiftItemId, Message, IsNew, DeletedFromSender, DeletedFromReceiver
                FROM player_mailbox WHERE SenderId = ? OR ReceiverId = ?
            )");
                mailStmt->setUInt(1, accId);
                mailStmt->setUInt(2, accId);
                std::unique_ptr<sql::ResultSet> mailRes(mailStmt->executeQuery());
                while (mailRes->next())
                {
                    DEBUGLOG(dark_cyan, "adding mail to cache");
                    mailbox_list.push_back({
                        mailRes->getUInt("Id"),
                        mailRes->getUInt("SenderId"),
                        mailRes->getString("SenderNickname").c_str(),
                        mailRes->getUInt("ReceiverId"),
                        mailRes->getString("ReceiverNickname").c_str(),
                        mailRes->getUInt("Date"),
                        mailRes->getUInt("GiftItemId"),
                        mailRes->getString("Message").c_str(),
                        static_cast<bool>(mailRes->getByte("IsNew")),
                        static_cast<bool>(mailRes->getByte("DeletedFromSender")),
                        static_cast<bool>(mailRes->getByte("DeletedFromReceiver"))
                        });
                }

                auto* pityStmt = Prep("SELECT GachaType, LuckyPoints FROM player_gacha_pity WHERE PlayerId = ?");
                pityStmt->setUInt(1, accId);
                std::unique_ptr<sql::ResultSet> pityRes(pityStmt->executeQuery());
                while (pityRes->next())
                {
                    GachaPityEntry entry;
                    entry.gacha_type = pityRes->getUInt("GachaType");
                    entry.lucky_points = pityRes->getUInt("LuckyPoints");
                    gacha_pity.push_back(entry);
                }

                if (outMonthlyRewards)
                {
                    uint32_t curYear = Utility::GetCurrentYear();
                    uint32_t curMonth = Utility::GetCurrentMonth();
                    auto* monthlyStmt = Prep(
                        "SELECT * FROM system_monthly_rewards WHERE Year = ? AND Month = ?");
                    monthlyStmt->setUInt(1, curYear);
                    monthlyStmt->setUInt(2, curMonth);
                    std::unique_ptr<sql::ResultSet> monthlyRes(monthlyStmt->executeQuery());
                    if (monthlyRes->next())
                    {
                        *outMonthlyRewards = SystemMonthlyRewards(curYear, curMonth,
                            {
                                monthlyRes->getUInt("Day1"),  monthlyRes->getUInt("Day2"),  monthlyRes->getUInt("Day3"),
                                monthlyRes->getUInt("Day4"),  monthlyRes->getUInt("Day5"),  monthlyRes->getUInt("Day6"),
                                monthlyRes->getUInt("Day7"),  monthlyRes->getUInt("Day8"),  monthlyRes->getUInt("Day9"),
                                monthlyRes->getUInt("Day10"), monthlyRes->getUInt("Day11"), monthlyRes->getUInt("Day12"),
                                monthlyRes->getUInt("Day13"), monthlyRes->getUInt("Day14"), monthlyRes->getUInt("Day15"),
                                monthlyRes->getUInt("Day16"), monthlyRes->getUInt("Day17"), monthlyRes->getUInt("Day18"),
                                monthlyRes->getUInt("Day19"), monthlyRes->getUInt("Day20"), monthlyRes->getUInt("Day21"),
                                monthlyRes->getUInt("Day22"), monthlyRes->getUInt("Day23"), monthlyRes->getUInt("Day24"),
                                monthlyRes->getUInt("Day25"), monthlyRes->getUInt("Day26"), monthlyRes->getUInt("Day27"),
                                monthlyRes->getUInt("Day28"), monthlyRes->getUInt("Day29"), monthlyRes->getUInt("Day30"),
                                monthlyRes->getUInt("Day31")
                            });
                    }
                }

                if (outPlayerMonthlyReward)
                {
                    auto* playerMonthlyStmt = Prep(
                        "SELECT RewardCount, UNIX_TIMESTAMP(LastUpdate) AS LastUpdate "
                        "FROM player_monthly_rewards WHERE PlayerId = ? "
                        "ORDER BY LastUpdate DESC, ID DESC LIMIT 1");
                    playerMonthlyStmt->setUInt(1, accId);
                    std::unique_ptr<sql::ResultSet> playerMonthlyRes(playerMonthlyStmt->executeQuery());
                    if (playerMonthlyRes->next())
                    {
                        outPlayerMonthlyReward->player_account_id = accId;
                        outPlayerMonthlyReward->day_count = playerMonthlyRes->getByte("RewardCount");
                        outPlayerMonthlyReward->last_time_update = playerMonthlyRes->getUInt64("LastUpdate");
                    }
                }

                if (outWeeklyRewards)
                {
                    uint32_t curYear = Utility::GetCurrentYear();
                    uint32_t curWeek = Utility::GetCurrentWeek();
                    auto* weeklyStmt = Prep(
                        "SELECT * FROM system_weekly_rewards WHERE Year = ? AND Week = ?");
                    weeklyStmt->setUInt(1, curYear);
                    weeklyStmt->setUInt(2, curWeek);
                    std::unique_ptr<sql::ResultSet> weeklyRes(weeklyStmt->executeQuery());
                    if (weeklyRes->next())
                    {
                        *outWeeklyRewards = SystemWeeklyRewards(curYear, curWeek,
                            {
                                weeklyRes->getUInt("Day1"), weeklyRes->getUInt("Day2"), weeklyRes->getUInt("Day3"),
                                weeklyRes->getUInt("Day4"), weeklyRes->getUInt("Day5"), weeklyRes->getUInt("Day6"),
                                weeklyRes->getUInt("Day7")
                            });
                    }
                }

                if (outPlayerWeeklyReward)
                {
                    auto* playerWeeklyStmt = Prep(
                        "SELECT RewardCount, UNIX_TIMESTAMP(LastUpdate) AS LastUpdate "
                        "FROM player_weekly_rewards WHERE PlayerId = ? "
                        "ORDER BY LastUpdate DESC, ID DESC LIMIT 1");
                    playerWeeklyStmt->setUInt(1, accId);
                    std::unique_ptr<sql::ResultSet> playerWeeklyRes(playerWeeklyStmt->executeQuery());
                    if (playerWeeklyRes->next())
                    {
                        outPlayerWeeklyReward->player_account_id = accId;
                        outPlayerWeeklyReward->day_count = playerWeeklyRes->getByte("RewardCount");
                        outPlayerWeeklyReward->last_time_update = playerWeeklyRes->getUInt64("LastUpdate");
                    }
                }

                auto* updateOnlineStmt = Prep("UPDATE accounts SET ServerId = ? WHERE Id = ?");
                updateOnlineStmt->setUInt(1, server_id);
                updateOnlineStmt->setUInt(2, accId);
                updateOnlineStmt->executeUpdate();

                stmt->execute("COMMIT");
                return true;
            }
            catch (sql::SQLException& inner)
            {
                DEBUGLOG(red, "Inner SQL exception: ({})", inner.what());
                stmt->execute("ROLLBACK");
                return false;
            }
        }
        catch (sql::SQLException& outer)
        {
            DEBUGLOG(red, "Outer SQL exception: ({})", outer.what());
            return false;
        }
    }

    bool CMariaDatabase::GetFrontAccount(const std::string& ip, const std::string& username, const std::string& password, FrontAccount* outFrontAccount, ClanInfo* outClanInfo)
    {
        try
        {
            auto* conn = GetConnection();
            if (!conn || !conn->isValid()) return false;

            constexpr std::uint32_t kSessionTTLSeconds = 5u * 60u; // 5mins (sliding)
            const std::uint64_t nowUnix = Utility::GetUtcTimeNow64();
            const std::uint64_t newExp = nowUnix + kSessionTTLSeconds;

            std::unique_ptr<sql::Statement> stmt(conn->createStatement());
            stmt->execute("START TRANSACTION");
            TransactionGuard tx_guard;

			auto* pstmt = Prep(
				"SELECT a.Id, a.ServerId, a.Username, a.Password, a.Salt, a.Grade, a.ClanId, a.Level, a.Experience, "
				"a.Kills, a.Deaths, a.Assists, a.Wins, a.Loses, a.Draws, a.Nickname, "
				"c.Id as ClanId, c.OwnerId, c.ClanName, c.ClanLogoFront, c.ClanLogoBack "
				"FROM accounts a "
				"LEFT JOIN clans c ON a.ClanId = c.Id "
				"WHERE a.Username = ?"
			);

            pstmt->setString(1, username.c_str());

            std::unique_ptr<sql::ResultSet> result(pstmt->executeQuery());

            if (!result->next())
            {
                stmt->execute("ROLLBACK");
                return false;
            }
            auto password_str = result->getString("Password");
            auto password_salt_str = result->getString("Salt");
            std::string stored_hash_str = Utility::Base64::from_base64(password_str.c_str());
            std::string stored_salt_str = Utility::Base64::from_base64(password_salt_str.c_str());

            if (stored_hash_str.size() != 32 || stored_salt_str.size() != 16)
            {
                stmt->execute("ROLLBACK");
				DEBUGLOG(red, "Invalid stored hash or salt length");
                return false;
            }

            uint8_t stored_hash[32];
            uint8_t stored_salt[16];
            std::memcpy(stored_hash, stored_hash_str.data(), 32);
            std::memcpy(stored_salt, stored_salt_str.data(), 16);

            // Recompute hash
            uint8_t computed_hash[32];
            if (!Utility::HashPassword(password, stored_salt, computed_hash))
            {
                stmt->execute("ROLLBACK");
				DEBUGLOG(red, "Failed to compute password hash");
                return false;
            }

            // Verify using constant-time comparison
            if (crypto_verify32(computed_hash, stored_hash) != 0)
            {
                stmt->execute("ROLLBACK");
				DEBUGLOG(red, "Password verification failed");
                return false;
            }

            *outFrontAccount = FrontAccount(result->getUInt("Id"),
                                            result->getUInt("ServerId"),
                                            result->getString("Username").c_str(),
                                            result->getString("Password").c_str(),
                                            result->getString("Salt").c_str(),
                                            result->getByte("Grade"),
                                            0,
                                            0,
                                            result->getUInt("ClanId"),
                                            0,
                                            0,
                                            0,
                                            0,
                                            0,
                                            0,
                                            0,
                                            result->getString("Nickname").c_str(),
                                            result->getUInt("Level"),
                                            result->getUInt("Experience"),
                                            0,
                                            0,
                                            0,
                                            0,
                                            0,
                                            0,
                                            0,
                                            0,
                                            0,
                                            0,
                                            0,
                                            0,
                                            0,
                                            0,
                                            0,
                                            0,
                                            0,
                                            result->getUInt("Wins"),
                                            result->getUInt("Loses"),
                                            result->getUInt("Draws"),
                                            result->getUInt("Kills"),
                                            result->getUInt("Deaths"),
                                            result->getUInt("Assists"),
                                            0,
                                            0,
                                            0,
                                            0,
                                            0,
                                            0,
                                            0,
                                            0,
                                            0,
                                            0,
                                            0,
                                            0,
                                            0,
                                            0,
                                            0
            );

            auto clanId = result->getUInt("ClanId");
            if(clanId > 0 && outClanInfo != nullptr)
            {
                *outClanInfo = ClanInfo(
                    result->getUInt("Id"),
                    result->getUInt("OwnerId"),
                    result->getString("ClanName").c_str(),
                    result->getUInt("ClanLogoFront"),
                    result->getUInt("ClanLogoBack")
                );
            }

           


            auto* purge = Prep("DELETE FROM player_sessions WHERE ExpiresAt <= FROM_UNIXTIME(?)");
            purge->setUInt64(1, nowUnix);
            purge->execute();



            auto* selSess = Prep(
                "SELECT AuthKey, UNIX_TIMESTAMP(ExpiresAt) "
                "FROM player_sessions WHERE PlayerId = ?"
            );
            selSess->setUInt(1, outFrontAccount->Index);
            std::unique_ptr<sql::ResultSet> rsSel(selSess->executeQuery());

            if (rsSel->next())
            {
                const std::uint64_t curKey = rsSel->getUInt64(1);
                const std::uint64_t curExpiry = rsSel->getUInt64(2);

                if (curExpiry > nowUnix)
                {
                    auto* bump = Prep(
                        "UPDATE player_sessions "
                        "SET LastSeenAt = FROM_UNIXTIME(?) "
                        "WHERE PlayerId = ?"
                    );
                    bump->setUInt64(1, nowUnix);
                    bump->setUInt(2, outFrontAccount->Index);
                    bump->executeUpdate();

                    outFrontAccount->AuthKey = curKey;
                    auto* loginHistoryStmt = Prep(
                        "INSERT INTO login_history (AccountId, LoginDate, IP) VALUES (?, FROM_UNIXTIME(?), ?)"
                    );
                    loginHistoryStmt->setUInt(1, outFrontAccount->Index);
                    loginHistoryStmt->setUInt64(2, nowUnix);
                    loginHistoryStmt->setString(3, ip.c_str());
                    if (!loginHistoryStmt->executeUpdate())
                    {
                        stmt->execute("ROLLBACK");
                        DEBUGLOG(red, "Failed to log history ip");
                        return false;
                    }

                    stmt->execute("COMMIT");
                    return true;
                }
            }


            const int MAX_TRIES = 10;
            std::uint64_t newAuthKey = 0;
            thread_local Utility::SecureRandomBlake2b::Generator rng;
            for (int attempt = 0; attempt < MAX_TRIES; ++attempt)
            {
                newAuthKey = rng.GenerateAuthKey();
                try
                {
                    auto* upsert = Prep(
                        "INSERT INTO player_sessions (PlayerId, AuthKey, IssuedAt, ExpiresAt, LastSeenAt) "
                        "VALUES (?, ?, FROM_UNIXTIME(?), FROM_UNIXTIME(?), FROM_UNIXTIME(?)) "
                        "ON DUPLICATE KEY UPDATE "
                        "  AuthKey = VALUES(AuthKey), "
                        "  IssuedAt = VALUES(IssuedAt), "
                        "  ExpiresAt = VALUES(ExpiresAt), "
                        "  LastSeenAt = VALUES(LastSeenAt)"
                    );
                    upsert->setUInt(1, outFrontAccount->Index);
                    upsert->setUInt64(2, newAuthKey);
                    upsert->setUInt64(3, nowUnix);
                    upsert->setUInt64(4, newExp);
                    upsert->setUInt64(5, nowUnix);
                    upsert->executeUpdate();
                    break;
                }
                catch (sql::SQLException& e)
                {
                    if (e.getErrorCode() == 1062)
                    {
                        const std::string msg = e.what();
                        if (msg.contains("uq_authkey") || msg.contains("AuthKey"))
                            continue;
                    }
                    stmt->execute("ROLLBACK");
                    DEBUGLOG(red, "Session upsert failed: ({})", e.what());
                    return false;
                }
            }
            outFrontAccount->AuthKey = newAuthKey;

            auto* loginHistoryStmt = Prep(
                "INSERT INTO login_history (AccountId, LoginDate, IP) VALUES (?, FROM_UNIXTIME(?), ?)"
            );
            loginHistoryStmt->setUInt(1, outFrontAccount->Index);
            loginHistoryStmt->setUInt64(2, nowUnix);
            loginHistoryStmt->setString(3, ip.c_str());

            if (!loginHistoryStmt->executeUpdate())
            {
                stmt->execute("ROLLBACK");
                DEBUGLOG(red, "Failed to log history ip");
                return false;
            }

            stmt->execute("COMMIT");
            return true;
        }
        catch (sql::SQLException& e)
        {
            DEBUGLOG(red, "exception: ({})", e.what());
            return false;
        }
    }
	bool CMariaDatabase::GetFrontAccount(const uint64_t authKey, FrontAccount *outFrontAccount, ClanInfo *outClanInfo)
	{
		try
		{
			auto* conn = GetConnection();
			if (!conn || !conn->isValid()) return false;

            const std::uint64_t nowUnix = Utility::GetUtcTimeNow64();

			std::unique_ptr<sql::Statement> stmt(conn->createStatement());
			stmt->execute("START TRANSACTION");
			TransactionGuard tx_guard;

            auto* purge = Prep("DELETE FROM player_sessions WHERE ExpiresAt <= FROM_UNIXTIME(?)");
            purge->setUInt64(1, nowUnix);
            purge->execute();


			auto* pstmt = Prep(
				"SELECT a.Id, a.ServerId, a.Username, a.Password, a.Salt, a.Grade, a.ClanId, a.Level, a.Experience, "
				"a.Kills, a.Deaths, a.Assists, a.Wins, a.Loses, a.Draws, a.Nickname, "
				"c.Id as ClanId, c.OwnerId, c.ClanName, c.ClanLogoFront, c.ClanLogoBack "
                "FROM player_sessions s "
				"JOIN accounts a ON s.PlayerId = a.Id "
				"LEFT JOIN clans c ON a.ClanId = c.Id "
				"WHERE s.AuthKey = ?"
			);

			pstmt->setUInt64(1, authKey);

			std::unique_ptr<sql::ResultSet> result(pstmt->executeQuery());
            if (!result->next())
            {
                stmt->execute("ROLLBACK");
                return false; // not found or expired
            }
            *outFrontAccount = FrontAccount(result->getUInt("Id"),
                result->getUInt("ServerId"),
                result->getString("Username").c_str(),
                result->getString("Password").c_str(),
                result->getString("Salt").c_str(),
                result->getByte("Grade"),
                0,
                authKey,
                result->getUInt("ClanId"),
                0,
                0,
                0,
                0,
                0,
                0,
                0,
                result->getString("Nickname").c_str(),
                result->getUInt("Level"),
                result->getUInt("Experience"),
                0,
                0,
                0,
                0,
                0,
                0,
                0,
                0,
                0,
                0,
                0,
                0,
                0,
                0,
                0,
                0,
                0,
                result->getUInt("Wins"),
                result->getUInt("Loses"),
                result->getUInt("Draws"),
                result->getUInt("Kills"),
                result->getUInt("Deaths"),
                result->getUInt("Assists"),
                0,
                0,
                0,
                0,
                0,
                0,
                0,
                0,
                0,
                0,
                0,
                0,
                0,
                0,
                0
            );

            auto clanId = result->getUInt("ClanId");
            if (clanId > 0 && outClanInfo != nullptr)
            {
                *outClanInfo = ClanInfo(
                    clanId,
                    result->getUInt("OwnerId"),
                    result->getString("ClanName").c_str(),
                    result->getUInt("ClanLogoFront"),
                    result->getUInt("ClanLogoBack")
                );
            }


            auto* bump = Prep("UPDATE player_sessions SET LastSeenAt = FROM_UNIXTIME(?) WHERE AuthKey = ?");
            bump->setUInt64(1, nowUnix);
            bump->setUInt64(2, authKey);
            bump->executeUpdate();
            stmt->execute("COMMIT");
            return true;

		}
		catch (sql::SQLException& e)
		{
			DEBUGLOG(red, "exception: ({})", e.what());
			return false;
		}
	}

    bool CMariaDatabase::GetPlazaAuthKey(const std::string& ip, const std::string& username, const std::string& password, PlazaAuth* outPlazaAuth)
    {
        try
        {
            auto* conn = GetConnection();
            if (!conn || !conn->isValid()) return false;

            constexpr std::uint32_t kSessionTTLSeconds = 5u * 60u;
            const std::uint64_t nowUnix = Utility::GetUtcTimeNow64();
            const std::uint64_t newExp = nowUnix + kSessionTTLSeconds;

            std::unique_ptr<sql::Statement> stmt(conn->createStatement());
            stmt->execute("START TRANSACTION");
            TransactionGuard tx_guard;

            auto* pstmt = Prep(
                "SELECT Id, ServerId, Password, Salt, Grade, IsEmailVerified, TwoFactorSecret, TwoFactorEnabled "
                "FROM accounts "
                "WHERE Username = ?"
            );

            pstmt->setString(1, username.c_str());

            std::unique_ptr<sql::ResultSet> result(pstmt->executeQuery());

            if (!result->next())
            {
                stmt->execute("ROLLBACK");
                return false;
            }
            auto password_str = result->getString("Password");
            auto password_salt_str = result->getString("Salt");
            std::string stored_hash_str = Utility::Base64::from_base64(password_str.c_str());
            std::string stored_salt_str = Utility::Base64::from_base64(password_salt_str.c_str());

            if (stored_hash_str.size() != 32 || stored_salt_str.size() != 16)
            {
                stmt->execute("ROLLBACK");
                DEBUGLOG(red, "Invalid stored hash or salt length");
                return false;
            }

            uint8_t stored_hash[32];
            uint8_t stored_salt[16];
            std::memcpy(stored_hash, stored_hash_str.data(), 32);
            std::memcpy(stored_salt, stored_salt_str.data(), 16);

            // Recompute hash
            uint8_t computed_hash[32];
            if (!Utility::HashPassword(password, stored_salt, computed_hash))
            {
                stmt->execute("ROLLBACK");
                DEBUGLOG(red, "Failed to compute password hash");
                return false;
            }

            // Verify using constant-time comparison
            if (crypto_verify32(computed_hash, stored_hash) != 0)
            {
                stmt->execute("ROLLBACK");
                DEBUGLOG(red, "Password verification failed");
                return false;
            }

            *outPlazaAuth = PlazaAuth(result->getUInt("Id"),
                result->getUInt("ServerId"),
                result->getByte("Grade"),
                result->getBoolean("IsEmailVerified"),
                result->getBoolean("TwoFactorEnabled"),
                result->getString("TwoFactorSecret").c_str()
            );

            auto* purge = Prep("DELETE FROM player_sessions WHERE ExpiresAt <= FROM_UNIXTIME(?)");
            purge->setUInt64(1, nowUnix);
            purge->execute();



            auto* selSess = Prep(
                "SELECT AuthKey, UNIX_TIMESTAMP(ExpiresAt) "
                "FROM player_sessions WHERE PlayerId = ?"
            );
            selSess->setUInt(1, outPlazaAuth->Index);
            std::unique_ptr<sql::ResultSet> rsSel(selSess->executeQuery());

            if (rsSel->next())
            {
                const std::uint64_t curKey = rsSel->getUInt64(1);
                const std::uint64_t curExpiry = rsSel->getUInt64(2);

                if (curExpiry > nowUnix)
                {
                    auto* bump = Prep(
                        "UPDATE player_sessions "
                        "SET LastSeenAt = FROM_UNIXTIME(?) "
                        "WHERE PlayerId = ?"
                    );
                    bump->setUInt64(1, nowUnix);
                    bump->setUInt(2, outPlazaAuth->Index);
                    bump->executeUpdate();

                    outPlazaAuth->AuthKey = curKey;
                    auto* loginHistoryStmt = Prep(
                        "INSERT INTO login_history (AccountId, LoginDate, IP) VALUES (?, FROM_UNIXTIME(?), ?)"
                    );
                    loginHistoryStmt->setUInt(1, outPlazaAuth->Index);
                    loginHistoryStmt->setUInt64(2, nowUnix);
                    loginHistoryStmt->setString(3, ip.c_str());
                    if (!loginHistoryStmt->executeUpdate())
                    {
                        stmt->execute("ROLLBACK");
                        DEBUGLOG(red, "Failed to log history ip");
                        return false;
                    }

                    stmt->execute("COMMIT");
                    return true;
                }
            }


            const int MAX_TRIES = 10;
            std::uint64_t newAuthKey = 0;
            thread_local Utility::SecureRandomBlake2b::Generator rng;
            for (int attempt = 0; attempt < MAX_TRIES; ++attempt)
            {
                newAuthKey = rng.GenerateAuthKey();
                try
                {
                    auto* upsert = Prep(
                        "INSERT INTO player_sessions (PlayerId, AuthKey, IssuedAt, ExpiresAt, LastSeenAt) "
                        "VALUES (?, ?, FROM_UNIXTIME(?), FROM_UNIXTIME(?), FROM_UNIXTIME(?)) "
                        "ON DUPLICATE KEY UPDATE "
                        "  AuthKey = VALUES(AuthKey), "
                        "  IssuedAt = VALUES(IssuedAt), "
                        "  ExpiresAt = VALUES(ExpiresAt), "
                        "  LastSeenAt = VALUES(LastSeenAt)"
                    );
                    upsert->setUInt(1, outPlazaAuth->Index);
                    upsert->setUInt64(2, newAuthKey);
                    upsert->setUInt64(3, nowUnix);
                    upsert->setUInt64(4, newExp);
                    upsert->setUInt64(5, nowUnix);
                    upsert->executeUpdate();
                    break;
                }
                catch (sql::SQLException& e)
                {
                    if (e.getErrorCode() == 1062)
                    {
                        const std::string msg = e.what();
                        if (msg.contains("uq_authkey") || msg.contains("AuthKey"))
                            continue;
                    }
                    stmt->execute("ROLLBACK");
                    DEBUGLOG(red, "Session upsert failed: ({})", e.what());
                    return false;
                }
            }
            outPlazaAuth->AuthKey = newAuthKey;

            auto* loginHistoryStmt = Prep(
                "INSERT INTO login_history (AccountId, LoginDate, IP) VALUES (?, FROM_UNIXTIME(?), ?)"
            );
            loginHistoryStmt->setUInt(1, outPlazaAuth->Index);
            loginHistoryStmt->setUInt64(2, nowUnix);
            loginHistoryStmt->setString(3, ip.c_str());

            if (!loginHistoryStmt->executeUpdate())
            {
                stmt->execute("ROLLBACK");
                DEBUGLOG(red, "Failed to log history ip");
                return false;
            }

            stmt->execute("COMMIT");
            return true;
        }
        catch (sql::SQLException& e)
        {
            DEBUGLOG(red, "exception: ({})", e.what());
            return false;
        }
    }

    bool CMariaDatabase::GetPlazaAuthKey(const std::string& ip, const uint64_t authKey, PlazaAuth* outPlazaAuth)
    {
        try
        {
            auto* conn = GetConnection();
            if (!conn || !conn->isValid()) return false;

            constexpr std::uint32_t kSessionTTLSeconds = 5u * 60u;
            const std::uint64_t nowUnix = Utility::GetUtcTimeNow64();
            const std::uint64_t newExp = nowUnix + kSessionTTLSeconds;

            std::unique_ptr<sql::Statement> stmt(conn->createStatement());
            stmt->execute("START TRANSACTION");
            TransactionGuard tx_guard;

            auto* purge = Prep("DELETE FROM player_sessions WHERE ExpiresAt <= FROM_UNIXTIME(?)");
            purge->setUInt64(1, nowUnix);
            purge->execute();


            auto* pstmt = Prep(
                "SELECT a.Id, a.ServerId, a.Grade, a.IsEmailVerified, a.TwoFactorEnabled, a.TwoFactorSecret "
                "FROM player_sessions s "
                "JOIN accounts a ON s.PlayerId = a.Id "
                "WHERE s.AuthKey = ?"
            );

            pstmt->setUInt64(1, authKey);

            std::unique_ptr<sql::ResultSet> result(pstmt->executeQuery());
            if (!result->next())
            {
                stmt->execute("ROLLBACK");
                return false; // not found or expired
            }
            *outPlazaAuth = PlazaAuth(result->getUInt("Id"),
                result->getUInt("ServerId"),
                result->getByte("Grade"),
                result->getBoolean("IsEmailVerified"),
                result->getBoolean("TwoFactorEnabled"),
                result->getString("TwoFactorSecret").c_str(),
                authKey
            );


            auto* bump = Prep("UPDATE player_sessions SET LastSeenAt = FROM_UNIXTIME(?) WHERE AuthKey = ?");
            bump->setUInt64(1, nowUnix);
            bump->setUInt64(2, authKey);
            bump->executeUpdate();


            auto* loginHistoryStmt = Prep(
                "INSERT INTO login_history (AccountId, LoginDate, IP) VALUES (?, FROM_UNIXTIME(?), ?)"
            );
            loginHistoryStmt->setUInt(1, outPlazaAuth->Index);
            loginHistoryStmt->setUInt64(2, nowUnix);
            loginHistoryStmt->setString(3, ip.c_str());

            if (!loginHistoryStmt->executeUpdate())
            {
                stmt->execute("ROLLBACK");
                DEBUGLOG(red, "Failed to log history ip");
                return false;
            }

            stmt->execute("COMMIT");
            return true;
        }
        catch (sql::SQLException& e)
        {
            DEBUGLOG(red, "exception: ({})", e.what());
            return false;
        }
    }


    std::vector<GachaponSaleInfo> CMariaDatabase::GetGachaponSalesInfo()
    {
        std::vector<GachaponSaleInfo> sales;

        try
        {
            auto* conn = GetConnection();
            if (!conn || !conn->isValid()) return sales;

            std::unique_ptr<sql::Statement> stmt(conn->createStatement());
            stmt->execute("START TRANSACTION");
            TransactionGuard tx_guard;

            auto* pstmt = Prep(
                "SELECT GachaponId, SalePrice, "
                "UNIX_TIMESTAMP(EventStartDate) AS EventStartTimestamp, "
                "UNIX_TIMESTAMP(EventEndDate) AS EventEndTimestamp "
                "FROM system_gachapon_machine"
            );

            std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());

            while (res->next())
            {
                GachaponSaleInfo gachapon_info(
                    res->getUInt("GachaponId"),
                    res->getUInt("SalePrice"),
                    res->getUInt("EventStartTimestamp"),
                    res->getUInt("EventEndTimestamp")
                );
                sales.push_back(std::move(gachapon_info));
            }

            stmt->execute("COMMIT");
        }
        catch (const sql::SQLException& e)
        {
            DEBUGLOG(red,
                "SQL exception: {}", e.what());

            // Attempt rollback on failure
            try
            {
                if (conn && conn->isValid())
                {
                    std::unique_ptr<sql::Statement> rollbackStmt(conn->createStatement());
                    rollbackStmt->execute("ROLLBACK");
                }
            }
            catch (const sql::SQLException& rollbackEx)
            {
                DEBUGLOG(red,
                    "Rollback failed: {}", rollbackEx.what());
            }
        }

        return sales;
    }


    bool CMariaDatabase::DeleteGachaponSaleInfo(const uint32_t& gachapon_id)
    {
        try
        {
            auto* conn = GetConnection();
            if (!conn || !conn->isValid()) return false;

            std::unique_ptr<sql::Statement> stmt(conn->createStatement());
            stmt->execute("START TRANSACTION");
            TransactionGuard tx_guard;

            auto* pstmt = Prep(
                "DELETE FROM system_gachapon_machine WHERE GachaponId = ?"
            );
            pstmt->setUInt(1, gachapon_id);

            int affected_rows = pstmt->executeUpdate();

            if (affected_rows > 0)
            {
                stmt->execute("COMMIT");
                return true;
            }
            else
            {
                stmt->execute("ROLLBACK");
                return false;
            }
        }
        catch (const sql::SQLException& e)
        {
            DEBUGLOG(red,
                "SQL exception: {}", e.what());

            // Rollback if something fails
            try
            {
                if (conn && conn->isValid())
                {
                    std::unique_ptr<sql::Statement> rollbackStmt(conn->createStatement());
                    rollbackStmt->execute("ROLLBACK");
                }
            }
            catch (const sql::SQLException& rollbackEx)
            {
                DEBUGLOG(red,
                    "Rollback failed: {}", rollbackEx.what());
            }

            return false;
        }
    }

    bool CMariaDatabase::GetSystemMonthlyRewards(uint32_t year, uint32_t month, SystemMonthlyRewards* out)
    {
        try
        {
            auto* stmt = Prep(
                "SELECT * FROM system_monthly_rewards WHERE Year = ? AND Month = ?");
            stmt->setUInt(1, year);
            stmt->setUInt(2, month);
            std::unique_ptr<sql::ResultSet> res(stmt->executeQuery());
            if (!res->next()) return false;

            std::array<uint32_t, 31> rewards{};
            for (int i = 0; i < 31; ++i)
                rewards[i] = res->getUInt("Day" + std::to_string(i + 1));
            *out = SystemMonthlyRewards(year, month, rewards);
            return true;
        }
        catch (const sql::SQLException& e)
        {
            DEBUGLOG(red, "GetSystemMonthlyRewards failed: {}", e.what());
            return false;
        }
    }

    bool CMariaDatabase::GetPlayerMonthlyReward(uint32_t player_id, PlayerMonthlyReward* out)
    {
        try
        {
            auto* stmt = Prep(
                "SELECT RewardCount, UNIX_TIMESTAMP(LastUpdate) AS LastUpdate "
                "FROM player_monthly_rewards WHERE PlayerId = ? "
                "ORDER BY LastUpdate DESC, ID DESC LIMIT 1");
            stmt->setUInt(1, player_id);
            std::unique_ptr<sql::ResultSet> res(stmt->executeQuery());
            if (!res->next()) return false;

            out->player_account_id = player_id;
            out->day_count = res->getByte("RewardCount");
            out->last_time_update = res->getUInt64("LastUpdate");
            return true;
        }
        catch (const sql::SQLException& e)
        {
            DEBUGLOG(red, "GetPlayerMonthlyReward failed: {}", e.what());
            return false;
        }
    }

    bool CMariaDatabase::GetSystemPlaytimeRewards(uint32_t year, uint32_t month, SystemPlaytimeRewards* out)
    {
        try
        {
            auto* stmt = Prep("SELECT Reward1, Reward2, Reward3 FROM system_playtime_rewards WHERE Year = ? AND Month = ?");
            stmt->setUInt(1, year);
            stmt->setUInt(2, month);
            std::unique_ptr<sql::ResultSet> res(stmt->executeQuery());
            if (!res->next()) return false;
            *out = SystemPlaytimeRewards(year, month,
                { res->getUInt("Reward1"), res->getUInt("Reward2"), res->getUInt("Reward3") });
            return true;
        }
        catch (const sql::SQLException& e)
        {
            DEBUGLOG(red, "GetSystemPlaytimeRewards failed: {}", e.what());
            return false;
        }
    }

    bool CMariaDatabase::GetPlayerPlaytime(uint32_t player_id, PlayerPlaytime* out)
    {
        try
        {
            auto* stmt = Prep(
                "SELECT DailySeconds, ClaimedStage, UNIX_TIMESTAMP(LastUpdate) AS LastUpdate "
                "FROM player_playtime WHERE PlayerId = ? ORDER BY LastUpdate DESC, ID DESC LIMIT 1");
            stmt->setUInt(1, player_id);
            std::unique_ptr<sql::ResultSet> res(stmt->executeQuery());
            if (!res->next()) return false;
            out->player_account_id = player_id;
            out->daily_seconds = res->getUInt("DailySeconds");
            out->claimed_stage = res->getByte("ClaimedStage");
            out->last_time_update = res->getUInt64("LastUpdate");
            return true;
        }
        catch (const sql::SQLException& e)
        {
            DEBUGLOG(red, "GetPlayerPlaytime failed: {}", e.what());
            return false;
        }
    }

    bool CMariaDatabase::GetActiveBattlePassSeason(SystemBattlePassSeason* out)
    {
        try
        {
            auto* stmt = Prep(
                "SELECT Season, UNIX_TIMESTAMP(StartDate) AS S, UNIX_TIMESTAMP(EndDate) AS E, ResetBaseCost "
                "FROM system_battlepass_season WHERE NOW() BETWEEN StartDate AND EndDate "
                "ORDER BY Season DESC LIMIT 1");
            std::unique_ptr<sql::ResultSet> res(stmt->executeQuery());
            if (!res->next()) return false;
            out->season = res->getUInt("Season");
            out->start_date = res->getUInt64("S");
            out->end_date = res->getUInt64("E");
            out->reset_base_cost = res->getUInt("ResetBaseCost");
            return true;
        }
        catch (const sql::SQLException& e)
        {
            DEBUGLOG(red, "GetActiveBattlePassSeason failed: {}", e.what());
            return false;
        }
    }

    bool CMariaDatabase::GetSystemBattlePassLevels(uint32_t season, std::vector<SystemBattlePassLevel>* out)
    {
        try
        {
            auto* stmt = Prep(
                "SELECT Level, XpRequired, FreeItem, PremiumItem FROM system_battlepass_rewards "
                "WHERE Season = ? ORDER BY Level ASC");
            stmt->setUInt(1, season);
            std::unique_ptr<sql::ResultSet> res(stmt->executeQuery());
            out->clear();
            while (res->next())
            {
                SystemBattlePassLevel lv{};
                lv.season = season;
                lv.level = res->getUInt("Level");
                lv.xp_required = res->getUInt("XpRequired");
                lv.free_item = res->getUInt("FreeItem");
                lv.premium_item = res->getUInt("PremiumItem");
                out->push_back(lv);
            }
            return true;
        }
        catch (const sql::SQLException& e)
        {
            DEBUGLOG(red, "GetSystemBattlePassLevels failed: {}", e.what());
            return false;
        }
    }

    bool CMariaDatabase::GetSystemBattlePassMissions(std::vector<SystemBattlePassMission>* out)
    {
        try
        {
            auto* stmt = Prep(
                "SELECT MissionId, Description, CriteriaType, CriteriaTarget, XpReward "
                "FROM system_battlepass_missions ORDER BY MissionId ASC");
            std::unique_ptr<sql::ResultSet> res(stmt->executeQuery());
            out->clear();
            while (res->next())
            {
                SystemBattlePassMission m{};
                m.mission_id = res->getUInt("MissionId");
                m.description = res->getString("Description").c_str();
                m.criteria_type = res->getUInt("CriteriaType");
                m.criteria_target = res->getUInt("CriteriaTarget");
                m.xp_reward = res->getUInt("XpReward");
                out->push_back(m);
            }
            return true;
        }
        catch (const sql::SQLException& e)
        {
            DEBUGLOG(red, "GetSystemBattlePassMissions failed: {}", e.what());
            return false;
        }
    }

    bool CMariaDatabase::GetPlayerBattlePass(uint32_t player_id, PlayerBattlePass* out)
    {
        try
        {
            auto* stmt = Prep(
                "SELECT Season, Level, Xp, HasPremium, ClaimedFree, ClaimedPremium, "
                "CurrentMissionId, MissionProgress, ResetCount FROM player_battlepass WHERE PlayerId = ?");
            stmt->setUInt(1, player_id);
            std::unique_ptr<sql::ResultSet> res(stmt->executeQuery());
            if (!res->next()) return false;
            out->player_account_id = player_id;
            out->season = res->getUInt("Season");
            out->level = res->getUInt("Level");
            out->xp = res->getUInt("Xp");
            out->has_premium = static_cast<uint8_t>(res->getUInt("HasPremium"));
            out->current_mission_id = res->getUInt("CurrentMissionId");
            out->mission_progress = res->getUInt("MissionProgress");
            out->reset_count = res->getUInt("ResetCount");
            BattlePassHexToMask(res->getString("ClaimedFree").c_str(), out->claimed_free);
            BattlePassHexToMask(res->getString("ClaimedPremium").c_str(), out->claimed_premium);
            return true;
        }
        catch (const sql::SQLException& e)
        {
            DEBUGLOG(red, "GetPlayerBattlePass failed: {}", e.what());
            return false;
        }
    }

    bool CMariaDatabase::ClaimMonthlyReward(uint32_t player_id, uint8_t new_day_count, uint64_t now, const Item& reward_item)
    {
        try
        {
            auto* conn = GetConnection();
            if (!conn || !conn->isValid()) return false;
            std::unique_ptr<sql::Statement> stmt(conn->createStatement());
            stmt->execute("START TRANSACTION");
            TransactionGuard tx_guard;

            auto* upsert = Prep(
                "INSERT INTO player_monthly_rewards (PlayerId, RewardCount, LastUpdate) VALUES (?, ?, FROM_UNIXTIME(?)) "
                "ON DUPLICATE KEY UPDATE RewardCount = VALUES(RewardCount), LastUpdate = VALUES(LastUpdate)");
            upsert->setUInt(1, player_id);
            upsert->setUInt(2, new_day_count);
            upsert->setUInt64(3, now);
            upsert->executeUpdate();

            if (reward_item.item_info.item_number.item_id > 0)
            {
                auto* itemStmt = Prep(
                    "INSERT INTO player_items (PlayerId, SerialInfo, ItemId, ItemType, IsSealed, IsEquipped, Stock, ExpirationDate, Repair, Energy, CharacterId, SealLevel, EnhanceExp, EnhanceLevel) "
                    "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, 0, 0, 0)");
                itemStmt->setUInt(1, player_id);
                itemStmt->setUInt64(2, reward_item.item_info.serial_info.data);
                itemStmt->setUInt(3, reward_item.item_info.item_number.item_id);
                itemStmt->setUInt(4, reward_item.item_type);
                itemStmt->setUInt(5, 0);
                itemStmt->setUInt(6, reward_item.is_equipped);
                itemStmt->setUInt(7, reward_item.stock);
                itemStmt->setUInt(8, reward_item.item_info.expire_date);
                itemStmt->setUInt(9, reward_item.item_info.repair);
                itemStmt->setUInt(10, reward_item.item_info.energy);
                itemStmt->setUInt(11, reward_item.character_id);
                itemStmt->executeUpdate();
            }

            stmt->execute("COMMIT");
            return true;
        }
        catch (const sql::SQLException& e)
        {
            DEBUGLOG(red, "ClaimMonthlyReward failed: {}", e.what());
            try
            {
                if (conn && conn->isValid())
                {
                    std::unique_ptr<sql::Statement> rollbackStmt(conn->createStatement());
                    rollbackStmt->execute("ROLLBACK");
                }
            }
            catch (...) {}
            return false;
        }
    }

    bool CMariaDatabase::GetPlayerMiscInvisible(int32_t aid)
    {
        try
        {
            if (auto r = EnsureConnected(); !r.has_value())
                return false;

            auto* stmt = Prep("SELECT IsInvisible FROM player_misc WHERE AccountId = ?");
            stmt->setUInt(1, static_cast<uint32_t>(aid));
            auto result = stmt->executeQuery();
            if (!result->next())
                return false;
            return result->getInt("IsInvisible") != 0;
        }
        catch (sql::SQLException&)
        {
            return false;
        }
    }

    std::expected<void, DbError> CMariaDatabase::SetPlayerMiscInvisible(int32_t aid, bool val)
    {
        try
        {
            if (auto r = EnsureConnected(); !r.has_value())
                return r;

            auto* stmt = Prep(
                "INSERT INTO player_misc (AccountId, IsInvisible) VALUES (?, ?) "
                "ON DUPLICATE KEY UPDATE IsInvisible = VALUES(IsInvisible)");
            stmt->setUInt(1, static_cast<uint32_t>(aid));
            stmt->setInt(2, val ? 1 : 0);
            stmt->executeUpdate();
            return {};
        }
        catch (sql::SQLException& e)
        {
            return std::unexpected(CMariaDatabase::FromSQLException(e));
        }
    }

    std::string CMariaDatabase::GetDatabaseName()
    {
        return database_name;
    }

    std::unique_ptr<IDatabase> Database = std::make_unique<CMariaDatabase>();
}
