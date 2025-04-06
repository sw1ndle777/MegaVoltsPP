#include "CDatabase.h"
//#include <fmt/color.h>
#include "fmt/color.h"
namespace BaseLib
{
    void CDatabase::Initialize(const std::string& database, const std::string& host, const uint16_t& port, const std::string& user, const std::string& password)
    {
        try
        {
            this->database_name = database;
            this->driver = sql::mariadb::get_driver_instance();
            this->properties["hostName"] = std::string(host + ":" + std::to_string(port)).c_str();
            this->properties["userName"] = user.c_str();
            this->properties["password"] = password.c_str();
            this->properties["autoReconnect"] = "true";
            conn = driver->connect(this->properties);

            if (conn)
            {
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "connected to ({}:{})", host.c_str(), port);

            
                if (CreateDatabase(database))
                {

                    BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "created database ({})", database.c_str());

                    conn->setSchema(database);
                    CreateTable("accounts", R"(
                    Id int unsigned NOT NULL AUTO_INCREMENT,
					IsOnline bit(1) NOT NULL DEFAULT 0, 
                    Username varchar(16) NOT NULL, 
                    Password varchar(127) NOT NULL, 
                    Salt varchar(127) NOT NULL, 
                    Grade tinyint unsigned NOT NULL,
                    PCRoom tinyint unsigned NOT NULL,
                    AuthKey bigint unsigned NOT NULL, 
                    ClanId int unsigned DEFAULT NULL, 
                    Nickname varchar(16) NOT NULL, 
                    Level int unsigned NOT NULL, 
                    Experience int unsigned NOT NULL, 
                    Tutoral bit(1) NOT NULL, 
                    Story int unsigned NOT NULL,
                    GuideMission tinyint unsigned NOT NULL,
                    Achievement bigint unsigned NOT NULL,
                    VoiceType bigint unsigned NOT NULL,
                    VIPExperience int unsigned NOT NULL, 
                    MaximumItems int unsigned NOT NULL, 
                    MaximumEnergy int unsigned NOT NULL, 
                    SelectedCharacter int unsigned NOT NULL, 
                    PlayTime bigint unsigned NOT NULL, 
                    MutedUntil bigint unsigned NOT NULL, 
                    Coins int unsigned NOT NULL,
                    Energy int unsigned NOT NULL,
                    LuckyPoints int unsigned NOT NULL,
                    MicroPoints bigint unsigned NOT NULL,
                    RockTokens bigint unsigned NOT NULL,
                    Coupons int unsigned NOT NULL,
                    Wins int unsigned NOT NULL,
                    Loses int unsigned NOT NULL,
                    Draws int unsigned NOT NULL,
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
                    SingleWaveDailyAttempts int unsigned NOT NULL,
                    SingleWaveHighestWave int unsigned NOT NULL,
                    SingleWaveHighScore int unsigned NOT NULL,
                    SingleWaveLastUpdate bigint unsigned NOT NULL,
                    PRIMARY KEY(Id))");

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
                    UnbanDate datetime(6) NOT NULL, 
                    IP varchar(15) NOT NULL, 
                    PRIMARY KEY(Id), 
                    KEY IX_login_history_AccountId (AccountId), 
                    CONSTRAINT FK_login_history_accounts_AccountId FOREIGN KEY (AccountId) REFERENCES accounts (Id) ON DELETE CASCADE)");

                    CreateTable("clans", R"(
                    Id int unsigned NOT NULL AUTO_INCREMENT, 
                    OwnerId int unsigned DEFAULT NULL, 
                    ClanName varchar(16) NOT NULL, 
                    ClanLogoFront smallint unsigned NOT NULL, 
                    ClanLogoBack smallint unsigned NOT NULL, 
                    ClanContribution int unsigned NOT NULL, 
                    ClanWin int unsigned NOT NULL, 
                    ClanLose int unsigned NOT NULL, 
                    ClanDraw int unsigned NOT NULL, 
                    Kills int unsigned NOT NULL, 
                    Deaths int unsigned NOT NULL, 
                    Assists int unsigned NOT NULL, 
                    PRIMARY KEY(Id), 
                    KEY IX_clans_OwnerId (OwnerId), 
                    CONSTRAINT FK_clans_accounts_OwnerId FOREIGN KEY (OwnerId) REFERENCES accounts (Id) ON DELETE CASCADE)");


                    CreateTable("player_friends", R"(
                    PlayerId int unsigned NOT NULL,
                    FriendPlayerId int unsigned NOT NULL,
                    State tinyint unsigned NOT NULL,
                    FriendNickname varchar(16) NOT NULL,
                    KEY IX_player_friends_FriendPlayerId (FriendPlayerId),
                    KEY IX_player_friends_PlayerId (PlayerId),
                    CONSTRAINT FK_player_friends_accounts_FriendPlayerId FOREIGN KEY (FriendPlayerId) REFERENCES accounts (Id) ON DELETE CASCADE,
                    CONSTRAINT FK_player_friends_accounts_PlayerId FOREIGN KEY (PlayerId) REFERENCES accounts (Id) ON DELETE CASCADE)");

                    CreateTable("player_ignores", R"(
                    PlayerId int unsigned NOT NULL,
                    BlockedPlayerId int unsigned NOT NULL,
                    BlockedNickname varchar(16) NOT NULL,
                    KEY IX_player_ignores_BlockedPlayerId (BlockedPlayerId),
                    KEY IX_player_ignores_PlayerId (PlayerId),
                    CONSTRAINT FK_player_ignores_accounts_BlockedPlayerId FOREIGN KEY (BlockedPlayerId) REFERENCES accounts (Id) ON DELETE CASCADE,
                    CONSTRAINT FK_player_ignores_accounts_PlayerId FOREIGN KEY (PlayerId) REFERENCES accounts (Id) ON DELETE CASCADE)");

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
                    PRIMARY KEY (SerialInfo),
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
                    KEY IX_player_monthly_rewards_PlayerId (PlayerId),
                    CONSTRAINT FK_player_monthly_rewards_accounts_PlayerId FOREIGN KEY (PlayerId) REFERENCES accounts (Id) ON DELETE CASCADE)");

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
                    Month int unsigned NOT NULL AUTO_INCREMENT,
                    Day1 int unsigned NOT NULL,
                    Day2 int unsigned NOT NULL,
                    Day3 int unsigned NOT NULL,
                    Day4 int unsigned NOT NULL,
                    Day5 int unsigned NOT NULL,
                    Day6 int unsigned NOT NULL,
                    Day7 int unsigned NOT NULL,
                    Day8 int unsigned NOT NULL,
                    Day9 int unsigned NOT NULL,
                    Day10 int unsigned NOT NULL,
                    Day11 int unsigned NOT NULL,
                    Day12 int unsigned NOT NULL,
                    Day13 int unsigned NOT NULL,
                    Day14 int unsigned NOT NULL,
                    Day15 int unsigned NOT NULL,
                    Day16 int unsigned NOT NULL,
                    Day17 int unsigned NOT NULL,
                    Day18 int unsigned NOT NULL,
                    Day19 int unsigned NOT NULL,
                    Day20 int unsigned NOT NULL,
                    Day21 int unsigned NOT NULL,
                    Day22 int unsigned NOT NULL,
                    Day23 int unsigned NOT NULL,
                    Day24 int unsigned NOT NULL,
                    Day25 int unsigned NOT NULL,
                    Day26 int unsigned NOT NULL,
                    Day27 int unsigned NOT NULL,
                    Day28 int unsigned NOT NULL,
                    Day29 int unsigned NOT NULL,
                    Day30 int unsigned NOT NULL,
                    Day31 int unsigned NOT NULL,
                    PRIMARY KEY (Month))");

                    CreateTable("player_daily_mission", R"(
                    ID int unsigned NOT NULL AUTO_INCREMENT,
                    PlayerId int unsigned NOT NULL,
                    UpdateTime BIGINT UNSIGNED NOT NULL,
                    Mission1 int unsigned NOT NULL,
                    Mission2 int unsigned NOT NULL,
                    Mission3 int unsigned NOT NULL,
                    GoalMission1 int unsigned NOT NULL,
                    GoalMission2 int unsigned NOT NULL,
                    GoalMission3 int unsigned NOT NULL,
                    PRIMARY KEY (ID),
                    KEY IX_player_daily_mission_PlayerId (PlayerId),
                    CONSTRAINT FK_player_daily_mission_accounts_PlayerId FOREIGN KEY (PlayerId) REFERENCES accounts (Id) ON DELETE CASCADE)");
                }
                
                else
                {
                    conn->setSchema(database);
                    /*
                    CreateTable("accounts", R"(
                    Id int unsigned NOT NULL AUTO_INCREMENT, 
                    Username varchar(16) NOT NULL, 
                    Password varchar(127) NOT NULL, 
                    Salt varchar(127) NOT NULL, 
                    Grade tinyint unsigned NOT NULL, 
                    AuthKey bigint unsigned NOT NULL, 
                    ClanId int unsigned DEFAULT NULL, 
                    Nickname varchar(16) NOT NULL, 
                    Level int unsigned NOT NULL, 
                    Experience int unsigned NOT NULL, 
                    Tutoral bit(1) NOT NULL, 
                    Story int unsigned NOT NULL, 
                    VIPExperience int unsigned NOT NULL, 
                    MaximumItems int unsigned NOT NULL, 
                    MaximumEnergy int unsigned NOT NULL, 
                    SelectedCharacter int unsigned NOT NULL, 
                    PlayTime bigint unsigned NOT NULL, 
                    MutedUntil bigint unsigned NOT NULL, 
                    Coins int unsigned NOT NULL,
                    Energy int unsigned NOT NULL,
                    LuckyPoints int unsigned NOT NULL,
                    MicroPoints bigint unsigned NOT NULL,
                    RockTokens bigint unsigned NOT NULL,
                    Coupons int unsigned NOT NULL,
                    Wins int unsigned NOT NULL,
                    Loses int unsigned NOT NULL,
                    Draws int unsigned NOT NULL,
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
                    SingleWaveDailyAttempts int unsigned NOT NULL,
                    SingleWaveHighestWave int unsigned NOT NULL,
                    SingleWaveHighScore int unsigned NOT NULL,
                    SingleWaveLastUpdate bigint unsigned NOT NULL,
                    PRIMARY KEY(Id))");

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
                    UnbanDate datetime(6) NOT NULL, 
                    IP varchar(15) NOT NULL, 
                    PRIMARY KEY(Id), 
                    KEY IX_login_history_AccountId (AccountId), 
                    CONSTRAINT FK_login_history_accounts_AccountId FOREIGN KEY (AccountId) REFERENCES accounts (Id) ON DELETE CASCADE)");

                    CreateTable("clans", R"(
                    Id int unsigned NOT NULL AUTO_INCREMENT, 
                    OwnerId int unsigned DEFAULT NULL, 
                    ClanName varchar(16) NOT NULL, 
                    ClanLogoFront smallint unsigned NOT NULL, 
                    ClanLogoBack smallint unsigned NOT NULL, 
                    ClanContribution int unsigned NOT NULL, 
                    ClanWin int unsigned NOT NULL, 
                    ClanLose int unsigned NOT NULL, 
                    ClanDraw int unsigned NOT NULL, 
                    Kills int unsigned NOT NULL, 
                    Deaths int unsigned NOT NULL, 
                    Assists int unsigned NOT NULL, 
                    PRIMARY KEY(Id), 
                    KEY IX_clans_OwnerId (OwnerId), 
                    CONSTRAINT FK_clans_accounts_OwnerId FOREIGN KEY (OwnerId) REFERENCES accounts (Id) ON DELETE CASCADE)");


                    CreateTable("player_friends", R"(
                    PlayerId int unsigned NOT NULL,
                    FriendPlayerId int unsigned NOT NULL,
                    State tinyint unsigned NOT NULL,
                    FriendNickname varchar(16) NOT NULL,
                    KEY IX_player_friends_FriendPlayerId (FriendPlayerId),
                    KEY IX_player_friends_PlayerId (PlayerId),
                    CONSTRAINT FK_player_friends_accounts_FriendPlayerId FOREIGN KEY (FriendPlayerId) REFERENCES accounts (Id) ON DELETE CASCADE,
                    CONSTRAINT FK_player_friends_accounts_PlayerId FOREIGN KEY (PlayerId) REFERENCES accounts (Id) ON DELETE CASCADE)");

                    CreateTable("player_ignores", R"(
                    PlayerId int unsigned NOT NULL,
                    BlockedPlayerId int unsigned NOT NULL,
                    BlockedNickname varchar(16) NOT NULL,
                    KEY IX_player_ignores_BlockedPlayerId (BlockedPlayerId),
                    KEY IX_player_ignores_PlayerId (PlayerId),
                    CONSTRAINT FK_player_ignores_accounts_BlockedPlayerId FOREIGN KEY (BlockedPlayerId) REFERENCES accounts (Id) ON DELETE CASCADE,
                    CONSTRAINT FK_player_ignores_accounts_PlayerId FOREIGN KEY (PlayerId) REFERENCES accounts (Id) ON DELETE CASCADE)");

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
                    PRIMARY KEY (SerialInfo),
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
                    LastUpdate bigint unsigned NOT NULL,
                    PRIMARY KEY (ID),
                    KEY IX_player_monthly_rewards_PlayerId (PlayerId),
                    CONSTRAINT FK_player_monthly_rewards_accounts_PlayerId FOREIGN KEY (PlayerId) REFERENCES accounts (Id) ON DELETE CASCADE)");

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
                    Month int unsigned NOT NULL AUTO_INCREMENT,
                    Day1 int unsigned NOT NULL,
                    Day2 int unsigned NOT NULL,
                    Day3 int unsigned NOT NULL,
                    Day4 int unsigned NOT NULL,
                    Day5 int unsigned NOT NULL,
                    Day6 int unsigned NOT NULL,
                    Day7 int unsigned NOT NULL,
                    Day8 int unsigned NOT NULL,
                    Day9 int unsigned NOT NULL,
                    Day10 int unsigned NOT NULL,
                    Day11 int unsigned NOT NULL,
                    Day12 int unsigned NOT NULL,
                    Day13 int unsigned NOT NULL,
                    Day14 int unsigned NOT NULL,
                    Day15 int unsigned NOT NULL,
                    Day16 int unsigned NOT NULL,
                    Day17 int unsigned NOT NULL,
                    Day18 int unsigned NOT NULL,
                    Day19 int unsigned NOT NULL,
                    Day20 int unsigned NOT NULL,
                    Day21 int unsigned NOT NULL,
                    Day22 int unsigned NOT NULL,
                    Day23 int unsigned NOT NULL,
                    Day24 int unsigned NOT NULL,
                    Day25 int unsigned NOT NULL,
                    Day26 int unsigned NOT NULL,
                    Day27 int unsigned NOT NULL,
                    Day28 int unsigned NOT NULL,
                    Day29 int unsigned NOT NULL,
                    Day30 int unsigned NOT NULL,
                    Day31 int unsigned NOT NULL,
                    PRIMARY KEY (Month))");
                    */
                }  
                
            }
        }
        catch (sql::SQLException& e)
        {
            BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::red, "exception: ({})", e.what());
        }
    }

    bool CDatabase::CreateTable(const std::string& table_name, const std::string& data_collumns)
    {
        try
        {

            if (!conn || !conn->isValid())
            {
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::yellow, "Reconnecting to the database...");
                conn = driver->connect(this->properties);
                if (conn)
                    BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "Successfully reconnected to database");
            }

            std::unique_ptr<sql::PreparedStatement> check_stmt(conn->prepareStatement("SHOW TABLES LIKE ?"));
            check_stmt->setString(1, table_name);

            std::unique_ptr<sql::ResultSet> res(check_stmt->executeQuery());
            if (res->next())
                return true;

            std::string create_query = "CREATE TABLE " + table_name + " (" + data_collumns + ")";
            std::unique_ptr<sql::PreparedStatement> create_stmt(conn->prepareStatement(create_query));

            bool retn = !create_stmt->execute();

            if (retn)
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::green, "Created table: ({})", table_name);

            return retn;
        }
        catch (sql::SQLException& e)
        {
            BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::red, "exception: ({})", e.what());;
            return false;
        }
    }

    bool CDatabase::CreateDatabase(const std::string& name)
    {
        try
        {
            if (!conn || !conn->isValid())
            {
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::yellow, "Reconnecting to the database...");
                conn = driver->connect(this->properties);
                if (conn)
                    BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "Successfully reconnected to database");
            }

            std::unique_ptr<sql::Statement> stmt(conn->createStatement());
            return !stmt->execute(fmt::format("CREATE DATABASE {0}", name.c_str()));
        }
        catch (sql::SQLException& e)
        {
            if (std::string(e.what()).find("database exists") == std::string::npos)
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::red, "exception: ({})", e.what());

            return false;
        }
    }
    /*
    bool CDatabase::RegisterAccount(const std::string& username, const std::string& password, const uint8_t& grade, const uint32_t& mp, const uint32_t& rt, const uint32_t& coupons, const uint32_t& coins, const uint32_t& energy, const uint32_t& max_items, const uint32_t& max_battery, const std::string& nickname)
    {

        auto hash = Utility::Hash(password);
        auto auth_key = Utility::GenerateAuthKey(username, password);

        try
        {
            if (!conn || !conn->isValid())
            {
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::yellow, "Reconnecting to the database...");
                conn = driver->connect(this->properties);
                if (conn)
                    BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "Successfully reconnected to database");
            }

            std::unique_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement(
                "INSERT INTO accounts (Username, Password, Salt, Grade, AuthKey, ClanId, ClanKills, ClanDeaths, ClanAssists, ClanContribution, ClanWins, ClanLoses, ClanDraws, Nickname, Level, Experience, Tutoral, Story, VIPExperience, MaximumItems, MaximumEnergy, SelectedCharacter, PlayTime, MutedUntil, Coins, Energy, LuckyPoints, MicroPoints, RockTokens, Coupons, Wins, Loses, Draws, Kills, Deaths, Assists, Headshots, HighestKillStreak, MeleeKills, RifleKills, ShotgunKills, SniperKills, GatlingKills, BazookaKills, GrenadeKills, ZombieKills, Infections, SingleWaveDailyAttempts, SingleWaveHighestWave, SingleWaveHighScore, SingleWaveLastUpdate) "
                "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)"));


            std::time_t now = std::time(nullptr);
            std::tm tm_time;
            auto time_point = std::chrono::system_clock::from_time_t(now);
            std::chrono::duration<int, std::ratio<1>> second(1);
            auto remainder = std::chrono::duration_cast<std::chrono::seconds>(time_point.time_since_epoch() % second);
            time_point -= remainder;

            std::time_t truncated_time = std::chrono::system_clock::to_time_t(time_point);
            tm_time = *std::gmtime(&truncated_time);


            std::string datetime_str = fmt::format("{:04d}-{:02d}-{:02d} {:02d}:{:02d}:{:02d}",
                tm_time.tm_year + 1900, tm_time.tm_mon + 1, tm_time.tm_mday,
                tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec);

            pstmt->setString(1, username.c_str());
            pstmt->setString(2, hash.first.c_str());
            pstmt->setString(3, hash.second.c_str());
            pstmt->setByte(4, grade);
            pstmt->setUInt64(5, auth_key);
            pstmt->setUInt(6, 0);//clan
            pstmt->setUInt(7, 0);//ClanKills
            pstmt->setUInt(8, 0);//ClanDeaths
            pstmt->setUInt(9, 0);//ClanAssists
            pstmt->setUInt64(10, 0);//ClanContribution
            pstmt->setUInt64(11, 0);//ClanWins
            pstmt->setUInt64(12, 0);//ClanLoses
            pstmt->setUInt64(13, 0);//ClanDraws
            pstmt->setString(14, nickname.c_str());
            pstmt->setUInt(15, 0);//level
            pstmt->setUInt(16, 0);//exp
            pstmt->setByte(17, false);//tutorial
            pstmt->setUInt(18, 0);//story
            pstmt->setUInt(19, 0);//vip exp
            pstmt->setUInt(20, max_items);//max items
            pstmt->setUInt(21, max_battery);//max battery
            pstmt->setUInt(22, 0);//selected char
            pstmt->setUInt64(23, 0);//playtime
            pstmt->setUInt64(24, 0);//muteduntil
            pstmt->setUInt(25, coins);//coins
            pstmt->setUInt(26, energy);//energy
            pstmt->setUInt(27, 0);//lucky points
            pstmt->setUInt(28, mp);//mp
            pstmt->setUInt(29, rt);//rt
            pstmt->setUInt(30, coupons);//coupons
            pstmt->setUInt(31, 0);//Wins
            pstmt->setUInt(32, 0);//Loses
            pstmt->setUInt(33, 0);//Draws
            pstmt->setUInt(34, 0);//Kills
            pstmt->setUInt(35, 0);//Deaths
            pstmt->setUInt(36, 0);//Assists
            pstmt->setUInt(37, 0);//Headshots
            pstmt->setUInt(38, 0);//HighestKillStreak
            pstmt->setUInt(39, 0);//MeleeKills
            pstmt->setUInt(40, 0);//RifleKills
            pstmt->setUInt(41, 0);//ShotgunKills
            pstmt->setUInt(42, 0);//SniperKills
            pstmt->setUInt(43, 0);//GatlingKills
            pstmt->setUInt(44, 0);//BazookaKills
            pstmt->setUInt(45, 0);//GrenadeKills
            pstmt->setUInt(46, 0);//ZombieKills
            pstmt->setUInt(47, 0);//Infections
            pstmt->setUInt(48, 0);//SingleWaveDailyAttempts
            pstmt->setUInt(49, 0);//SingleWaveHighestWave
            pstmt->setUInt(50, 0);//SingleWaveHighScore
            pstmt->setUInt64(51, 0);//SingleWaveLastUpdate
            return !pstmt->execute();
        }
        catch (sql::SQLException& e)
        {
            BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::red, "exception: ({})", e.what());
            return false;
        }
    }
    */
    bool CDatabase::UpdateFrontAccount(const FrontAccount& front_acc)
    {
        try
        {
            if (!conn || !conn->isValid())
            {
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::yellow, "Reconnecting to the database...");
                conn = driver->connect(this->properties);
                if (conn)
                    BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "Successfully reconnected to database");
            }

            std::unique_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement(
                "UPDATE accounts SET "
                "Username=?, Password=?, Salt=?, Grade=?, PCRoom = ?, AuthKey=?, ClanId=?, ClanKills=?, ClanDeaths=?, ClanAssists=?, "
                "ClanContribution=?, ClanWins=?, ClanLoses=?, ClanDraws=?, Nickname=?, Level=?, Experience=?, Tutoral=?, "
                "Story=?, GuideMission=?, Achievement=?, VoiceType=?, VIPExperience=?, MaximumItems=?, MaximumEnergy=?, SelectedCharacter=?, PlayTime=?, "
                "MutedUntil=?, Coins=?, Energy=?, LuckyPoints=?, MicroPoints=?, "
                "RockTokens=?, Coupons=?, Wins=?, Loses=?, Draws=?, Kills=?, Deaths=?, "
                "Assists=?, Headshots=?, HighestKillStreak=?, MeleeKills=?, RifleKills=?, "
                "ShotgunKills=?, SniperKills=?, GatlingKills=?, BazookaKills=?, "
                "GrenadeKills=?, ZombieKills=?, Infections=?, SingleWaveDailyAttempts=?, "
                "SingleWaveHighestWave=?, SingleWaveHighScore=?, SingleWaveLastUpdate=? "
                "WHERE Id=?"));

            int index = 1;
            pstmt->setString(index++, front_acc.Username);
            pstmt->setString(index++, front_acc.Password);
            pstmt->setString(index++, front_acc.Salt);
            pstmt->setByte(index++, front_acc.Grade);
            pstmt->setByte(index++, front_acc.PCRoom);
            pstmt->setUInt64(index++, front_acc.AuthKey);
            pstmt->setUInt(index++, front_acc.ClanId);
            pstmt->setUInt(index++, front_acc.ClanKills);
            pstmt->setUInt(index++, front_acc.ClanDeaths);
            pstmt->setUInt(index++, front_acc.ClanAssists);
            pstmt->setUInt64(index++, front_acc.ClanContribution);
            pstmt->setUInt64(index++, front_acc.ClanWins);
            pstmt->setUInt64(index++, front_acc.ClanLoses);
            pstmt->setUInt64(index++, front_acc.ClanDraws);
            pstmt->setString(index++, front_acc.Nickname);
            pstmt->setUInt(index++, front_acc.Level);
            pstmt->setUInt(index++, front_acc.Experience);
            pstmt->setBoolean(index++, front_acc.Tutorial);
            pstmt->setUInt(index++, front_acc.Story);
            pstmt->setUInt(index++, front_acc.GuideMission);
            pstmt->setUInt64(index++, front_acc.Achievement);
            pstmt->setUInt64(index++, front_acc.VoiceType);
            pstmt->setUInt(index++, front_acc.VIPExperience);
            pstmt->setUInt(index++, front_acc.MaximumItems);
            pstmt->setUInt(index++, front_acc.MaximumEnergy);
            pstmt->setUInt(index++, front_acc.SelectedCharacter);
            pstmt->setUInt64(index++, front_acc.PlayTime);
            pstmt->setUInt64(index++, front_acc.MutedUntil);
            pstmt->setUInt(index++, front_acc.Coins);
            pstmt->setUInt(index++, front_acc.Energy);
            pstmt->setUInt(index++, front_acc.LuckyPoints);
            pstmt->setUInt(index++, front_acc.MicroPoints);
            pstmt->setUInt(index++, front_acc.RockTokens);
            pstmt->setUInt(index++, front_acc.Coupons);
            pstmt->setUInt(index++, front_acc.Wins);
            pstmt->setUInt(index++, front_acc.Loses);
            pstmt->setUInt(index++, front_acc.Draws);
            pstmt->setUInt(index++, front_acc.Kills);
            pstmt->setUInt(index++, front_acc.Deaths);
            pstmt->setUInt(index++, front_acc.Assists);
            pstmt->setUInt(index++, front_acc.Headshots);
            pstmt->setUInt(index++, front_acc.HighestKillStreak);
            pstmt->setUInt(index++, front_acc.MeleeKills);
            pstmt->setUInt(index++, front_acc.RifleKills);
            pstmt->setUInt(index++, front_acc.ShotgunKills);
            pstmt->setUInt(index++, front_acc.SniperKills);
            pstmt->setUInt(index++, front_acc.GatlingKills);
            pstmt->setUInt(index++, front_acc.BazookaKills);
            pstmt->setUInt(index++, front_acc.GrenadeKills);
            pstmt->setUInt(index++, front_acc.ZombieKills);
            pstmt->setUInt(index++, front_acc.Infections);
            pstmt->setUInt(index++, front_acc.SingleWaveDailyAttempts);
            pstmt->setUInt(index++, front_acc.SingleWaveHighestWave);
            pstmt->setUInt(index++, front_acc.SingleWaveHighScore);
            pstmt->setUInt64(index++, front_acc.SingleWaveLastUpdate);

            pstmt->setUInt(index, front_acc.Index); 

            int updateCount = pstmt->executeUpdate();
            return updateCount > 0;
        }
        catch (sql::SQLException& e)
        {
            BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::red, "exception: ({})", e.what());
            return false;
        }
    }
    bool CDatabase::GetInventoryItems(const uint32_t& acc_id, std::vector<Item>& inv_items)
    {
        try
        {
            if (!conn || !conn->isValid())
            {
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::yellow, "Reconnecting to the database...");
                conn = driver->connect(this->properties);
                if (conn)
                    BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "Successfully reconnected to database");
            }

            std::unique_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement("SELECT * FROM player_items WHERE PlayerId = ?"));
            pstmt->setUInt(1, acc_id);
            std::unique_ptr<sql::ResultSet> result(pstmt->executeQuery());
            bool foundItems = false;
            while (result->next())
            {
                foundItems = true;
                Item newItem;
                NetEngine::Packets::Main::InventoryItemInfo newItemInfo;
            #if defined(RELEASE_1_0_3)
                newItemInfo.serial_info.data = result->getUInt64("SerialInfo");
                newItemInfo.item_number.item_id = result->getUInt("ItemId");
                newItemInfo.expire_date = result->getUInt("ExpirationDate");
                newItemInfo.repair = result->getUInt("Repair");
                newItemInfo.energy = result->getUInt("Energy");
                newItem.stock = result->getUInt("Stock");
                newItemInfo.item_number.stock = newItem.stock;
                newItem.is_equipped = result->getByte("IsEquipped");
                newItem.character_id = result->getByte("CharacterId");
                newItem.in_database = 1;
                newItem.item_info = newItemInfo;
            #else
                newItemInfo.serial_info.data = result->getUInt64("SerialInfo");
                newItemInfo.item_number.item_id = result->getUInt("ItemId");
                newItemInfo.expire_date = result->getUInt("ExpirationDate");
                newItemInfo.repair = result->getUInt("Repair");
                newItemInfo.energy = result->getUInt("Energy");
                newItemInfo.item_type = result->getUInt("ItemType");
                newItemInfo.is_sealed = result->getUInt("IsSealed");
                newItemInfo.seal_level = result->getUInt("SealLevel");
                newItemInfo.enhance_exp = result->getUInt("EnhanceExp");
                newItemInfo.enhance_level = result->getUInt("EnhanceLevel");
                newItem.stock = result->getUInt("Stock");
                newItem.is_equipped = result->getByte("IsEquipped");
                newItem.character_id = result->getByte("CharacterId");
                newItem.in_database = 1;
                newItem.item_info = newItemInfo;
            #endif
                
                inv_items.push_back(newItem);
            }
            return foundItems;
        }
        catch (sql::SQLException& e)
        {
            BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::red, "exception: ({})", e.what());
            return false;
        }
    }
    bool CDatabase::UpdateInventoryItems(const uint32_t& acc_id, const std::vector<Item>& inv_items)
    {
        if (inv_items.empty()) return false;

        try {
            if (!conn || !conn->isValid())
            {
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::yellow, "Reconnecting to the database...");
                conn = driver->connect(this->properties);
                if (conn)
                    BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "Successfully reconnected to database");
            }

            std::string query = "UPDATE player_items SET ";
            std::vector<std::string> caseStatements;
            std::string whereClause = " WHERE PlayerId = ? AND SerialInfo IN (";
            std::vector<std::string> whereSerials;

            std::string itemIdCase = "ItemId = CASE SerialInfo ";
            std::string itemTypeCase = "ItemType = CASE SerialInfo ";
            std::string expirationDateCase = "ExpirationDate = CASE SerialInfo ";
            std::string repairCase = "Repair = CASE SerialInfo ";
            std::string energyCase = "Energy = CASE SerialInfo ";
            std::string isSealedCase = "IsSealed = CASE SerialInfo ";
            std::string sealLevelCase = "SealLevel = CASE SerialInfo ";
            std::string enhanceExpCase = "EnhanceExp = CASE SerialInfo ";
            std::string enhanceLevelCase = "EnhanceLevel = CASE SerialInfo ";
            std::string stockCase = "Stock = CASE SerialInfo ";
            std::string isEquippedCase = "IsEquipped = CASE SerialInfo ";
            std::string characterIdCase = "CharacterId = CASE SerialInfo ";

            auto join = [](const std::vector<std::string>& vec, const std::string& delim) -> std::string {
                std::string result;
                for (size_t i = 0; i < vec.size(); ++i) {
                    if (i != 0) result += delim;
                    result += vec[i];
                }
                return result;
                };

            for (const auto& item : inv_items) 
            {
                auto serial = std::to_string(item.item_info.serial_info.data);
            #if defined(RELEASE_1_0_3)

                itemIdCase += "WHEN " + serial + " THEN " + std::to_string(item.item_info.item_number.item_id) + " ";
                itemTypeCase += "WHEN " + serial + " THEN " + std::to_string(0) + " ";
                expirationDateCase += "WHEN " + serial + " THEN " + std::to_string(item.item_info.expire_date) + " ";
                repairCase += "WHEN " + serial + " THEN " + std::to_string(item.item_info.repair) + " ";
                energyCase += "WHEN " + serial + " THEN " + std::to_string(item.item_info.energy) + " ";
                isSealedCase += "WHEN " + serial + " THEN " + std::to_string(0) + " ";
                sealLevelCase += "WHEN " + serial + " THEN " + std::to_string(0) + " ";
                enhanceExpCase += "WHEN " + serial + " THEN " + std::to_string(0) + " ";
                enhanceLevelCase += "WHEN " + serial + " THEN " + std::to_string(0) + " ";
                stockCase += "WHEN " + serial + " THEN " + std::to_string(item.stock) + " ";
                isEquippedCase += "WHEN " + serial + " THEN " + std::to_string(item.is_equipped) + " ";
                characterIdCase += "WHEN " + serial + " THEN " + std::to_string(item.character_id) + " ";
            #else

                itemIdCase += "WHEN " + serial + " THEN " + std::to_string(item.item_info.item_number.item_id) + " ";
                itemTypeCase += "WHEN " + serial + " THEN " + std::to_string(0) + " ";
                expirationDateCase += "WHEN " + serial + " THEN " + std::to_string(item.item_info.expire_date) + " ";
                repairCase += "WHEN " + serial + " THEN " + std::to_string(item.item_info.repair) + " ";
                energyCase += "WHEN " + serial + " THEN " + std::to_string(item.item_info.energy) + " ";
                isSealedCase += "WHEN " + serial + " THEN " + std::to_string(0) + " ";
                sealLevelCase += "WHEN " + serial + " THEN " + std::to_string(0l) + " ";
                enhanceExpCase += "WHEN " + serial + " THEN " + std::to_string(0) + " ";
                enhanceLevelCase += "WHEN " + serial + " THEN " + std::to_string(0) + " ";
                stockCase += "WHEN " + serial + " THEN " + std::to_string(item.stock) + " ";
                isEquippedCase += "WHEN " + serial + " THEN " + std::to_string(item.is_equipped) + " ";
                characterIdCase += "WHEN " + serial + " THEN " + std::to_string(item.character_id) + " ";
            #endif

                whereSerials.push_back(serial);
            }

            std::string endCase = " END, ";
            query += itemIdCase + endCase + itemTypeCase + endCase + expirationDateCase + endCase +
                repairCase + endCase + energyCase + endCase + isSealedCase + endCase +
                sealLevelCase + endCase + enhanceExpCase + endCase + enhanceLevelCase + endCase +
                stockCase + endCase + isEquippedCase + endCase + characterIdCase.substr(0, characterIdCase.size() - 1) + " END";

            query += whereClause + join(whereSerials, ", ") + ")";
            auto pstmt = conn->prepareStatement(query);
            pstmt->setUInt(1, acc_id);
            pstmt->executeUpdate();
            return true;
        }
        catch (const sql::SQLException& e) 
        {
            BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::red, "exception: ({})", e.what());
            return false;
        }
    }
    bool CDatabase::InsertInventoryItems(const uint32_t& acc_id, const std::vector<Item>& inv_items)
    {
        if (inv_items.empty()) return false;

        try
        {
            if (!conn || !conn->isValid())
            {
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::yellow, "Reconnecting to the database...");
                conn = driver->connect(this->properties);
                if (conn)
                    BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "Successfully reconnected to database");
            }

            std::string query = "INSERT INTO player_items (PlayerId, SerialInfo, ItemId, ItemType, ExpirationDate, Repair, Energy, IsSealed, SealLevel, EnhanceExp, EnhanceLevel, Stock, IsEquipped, CharacterId) VALUES ";
            std::string placeholders;
            std::string valuePlaceholder = "(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";
            for (size_t i = 0; i < inv_items.size(); ++i) {
                placeholders += (i == 0 ? "" : ", ") + valuePlaceholder;
            }
            query += placeholders;

            std::unique_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement(query));

            int paramIndex = 1;
            for (const auto& item : inv_items)
            {
            #if defined(RELEASE_1_0_3)
                pstmt->setUInt(paramIndex++, acc_id);
                pstmt->setUInt64(paramIndex++, item.item_info.serial_info.data);
                pstmt->setUInt(paramIndex++, item.item_info.item_number.item_id);
                pstmt->setUInt(paramIndex++, 0);
                pstmt->setUInt(paramIndex++, item.item_info.expire_date);
                pstmt->setUInt(paramIndex++, item.item_info.repair);
                pstmt->setUInt(paramIndex++, item.item_info.energy);
                pstmt->setUInt(paramIndex++, 0);
                pstmt->setUInt(paramIndex++, 0);
                pstmt->setUInt(paramIndex++, 0);
                pstmt->setUInt(paramIndex++, 0);
                pstmt->setUInt(paramIndex++, item.stock);
                pstmt->setUInt(paramIndex++, item.is_equipped);
                pstmt->setUInt(paramIndex++, item.character_id);
            #else
                pstmt->setUInt(paramIndex++, acc_id);
                pstmt->setUInt64(paramIndex++, item.item_info.serial_info.data);
                pstmt->setUInt(paramIndex++, item.item_info.item_number.item_id);
                pstmt->setUInt(paramIndex++, 0);
                pstmt->setUInt(paramIndex++, item.item_info.expire_date);
                pstmt->setUInt(paramIndex++, item.item_info.repair);
                pstmt->setUInt(paramIndex++, item.item_info.energy);
                pstmt->setUInt(paramIndex++, 0);
                pstmt->setUInt(paramIndex++, 0);
                pstmt->setUInt(paramIndex++, 0);
                pstmt->setUInt(paramIndex++, 0);
                pstmt->setUInt(paramIndex++, item.stock);
                pstmt->setUInt(paramIndex++, item.is_equipped);
                pstmt->setUInt(paramIndex++, item.character_id);
            #endif
            }

            pstmt->executeUpdate();
            return true;
        }
        catch (sql::SQLException& e)
        {
            BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::red, "exception: ({})", e.what());
            return false;
        }
    }
    bool CDatabase::DeleteInventoryItems(const uint32_t& acc_id, const std::vector<Item>& inv_items)
    {
        if (inv_items.empty()) return false;

        try
        {
            if (!conn || !conn->isValid())
            {
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::yellow, "Reconnecting to the database...");
                conn = driver->connect(this->properties);
                if (conn)
                    BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "Successfully reconnected to database");
            }

            std::string query = "DELETE FROM player_items WHERE PlayerId = ? AND SerialInfo IN (";
            std::string placeholders;
            for (size_t i = 0; i < inv_items.size(); ++i)  placeholders += (i == 0 ? "?" : ", ?");
            query += placeholders + ")";

            std::unique_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement(query));

            pstmt->setUInt(1, acc_id); 
            int paramIndex = 2;
            for (const auto& item : inv_items)
                pstmt->setUInt64(paramIndex++, item.item_info.serial_info.data);

            pstmt->executeUpdate();
            return true;
        }
        catch (sql::SQLException& e)
        {
            BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::red, "exception: ({})", e.what());
            return false;
        }
    }
    bool CDatabase::GetPlayerFriends(const int32_t& acc_id, std::vector<FriendInfo>& friends)
    {
        try
        {
            if (!conn || !conn->isValid())
            {
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::yellow, "Reconnecting to the database...");
                conn = driver->connect(this->properties);
                if (conn)
                    BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "Successfully reconnected to database");
            }

            std::unique_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement("SELECT FriendPlayerId, State, FriendNickname FROM player_friends WHERE PlayerId = ?"));
            pstmt->setInt(1, acc_id);
            std::unique_ptr<sql::ResultSet> result(pstmt->executeQuery());

            while (result->next())
                friends.push_back({ acc_id, result->getInt("FriendPlayerId"), static_cast<uint8_t>(result->getByte("State")), 0, result->getString("FriendNickname").c_str() });

            return !friends.empty();
        }
        catch (sql::SQLException& e)
        {
            BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::red, "exception: ({})", e.what());
            return false;
        }
    }
    bool CDatabase::GetPlayerBlockeds(const int32_t& acc_id, std::vector<BlockedInfo>& blockeds)
    {
        try
        {
            if (!conn || !conn->isValid())
            {
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::yellow, "Reconnecting to the database...");
                conn = driver->connect(this->properties);
                if (conn)
                    BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "Successfully reconnected to database");
            }

            std::unique_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement("SELECT BlockedPlayerId, BlockedNickname FROM player_ignores WHERE PlayerId = ?"));
            pstmt->setInt(1, acc_id);
            std::unique_ptr<sql::ResultSet> result(pstmt->executeQuery());

            while (result->next())
                blockeds.push_back({ acc_id ,result->getInt("BlockedPlayerId"), 0,  result->getString("BlockedNickname").c_str() });

            return !blockeds.empty();
        }
        catch (sql::SQLException& e)
        {
            BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::red, "exception: ({})", e.what());
            return false;
        }
    }
    bool CDatabase::InsertPlayerFriends(const std::vector<FriendInfo>& friends)
    {
        if (friends.empty()) return false;

        try
        {
            if (!conn || !conn->isValid())
            {
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::yellow, "Reconnecting to the database...");
                conn = driver->connect(this->properties);
                if (conn)
                    BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "Successfully reconnected to database");
            }

            std::string query = "INSERT INTO player_friends (PlayerId, FriendPlayerId, State, FriendNickname) VALUES ";
            std::string placeholders;
            std::string valuePlaceholder = "(?, ?, ?, ?)";
            for (size_t i = 0; i < friends.size(); ++i) {
                placeholders += (i == 0 ? "" : ", ") + valuePlaceholder;
            }
            query += placeholders;

            std::unique_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement(query));

            int paramIndex = 1;
            for (const auto& friend_info : friends)
            {
                pstmt->setInt(paramIndex++, friend_info.player_account_id);
                pstmt->setInt(paramIndex++, friend_info.friend_account_id);
                pstmt->setByte(paramIndex++, friend_info.state);
                pstmt->setString(paramIndex++, friend_info.friend_nickname);
            }

            pstmt->executeUpdate();
            return true;
        }
        catch (sql::SQLException& e)
        {
            BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::red, "exception: ({})", e.what());
            return false;
        }
    }
    bool CDatabase::DeletePlayerFriends(const std::vector<FriendInfo>& friends)
    {
        if (friends.empty()) return false;

        try
        {
            if (!conn || !conn->isValid())
            {
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::yellow, "Reconnecting to the database...");
                conn = driver->connect(this->properties);
                if (conn)
                    BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "Successfully reconnected to database");
            }

            std::string query = "DELETE FROM player_friends WHERE (PlayerId, FriendPlayerId) IN (";
            std::string placeholders;
            for (size_t i = 0; i < friends.size(); ++i) {
                placeholders += (i == 0 ? "(?, ?)" : ", (?, ?)");
            }
            query += placeholders + ")";

            std::unique_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement(query));

            int paramIndex = 1;
            for (const auto& friend_info : friends)
            {
                pstmt->setInt(paramIndex++, friend_info.player_account_id);
                pstmt->setInt(paramIndex++, friend_info.friend_account_id);
            }

            pstmt->executeUpdate();
            return true;
        }
        catch (sql::SQLException& e)
        {
            BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::red, "exception: ({})", e.what());
            return false;
        }
    }
    bool CDatabase::UpdatePlayerFriends(const int32_t& acc_id, const uint32_t& friend_acc_id, const uint8_t& state)
    {
        try
        {
            if (!conn || !conn->isValid())
            {
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::yellow, "Reconnecting to the database...");
                conn = driver->connect(this->properties);
                if (conn)
                    BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "Successfully reconnected to database");
            }

            std::unique_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement("UPDATE player_friends SET State = ? WHERE PlayerId = ? AND FriendPlayerId = ?"));
            pstmt->setByte(1, state);
            pstmt->setInt(2, acc_id);
            pstmt->setInt(3, friend_acc_id);
            int32_t affectedRows = pstmt->executeUpdate();
            return affectedRows > 0;
        }
        catch (sql::SQLException& e)
        {
            BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::red, "exception: ({})", e.what());
            return false;
        }
    }
    bool CDatabase::InsertPlayerBlockeds(const std::vector<BlockedInfo>& blockeds)
    {
        if (!conn || !conn->isValid())
        {
            BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::yellow, "Reconnecting to the database...");
            conn = driver->connect(this->properties);
            if (conn)
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "Successfully reconnected to database");
        }

        if (blockeds.empty()) return false;

        try
        {
            std::string query = "INSERT INTO player_ignores (PlayerId, BlockedPlayerId, BlockedNickname) VALUES ";
            std::string placeholders;
            std::string valuePlaceholder = "(?, ?)";
            for (size_t i = 0; i < blockeds.size(); ++i) {
                placeholders += (i == 0 ? "" : ", ") + valuePlaceholder;
            }
            query += placeholders;

            std::unique_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement(query));

            int paramIndex = 1;
            for (const auto& blocked_info : blockeds)
            {
                pstmt->setInt(paramIndex++, blocked_info.player_account_id);
                pstmt->setInt(paramIndex++, blocked_info.blocked_account_id);
                pstmt->setString(paramIndex++, blocked_info.blocked_nickname);
            }

            pstmt->executeUpdate();
            return true;
        }
        catch (sql::SQLException& e)
        {
            BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::red, "exception: ({})", e.what());
            return false;
        }
    }
    bool CDatabase::DeletePlayerBlockeds(const std::vector<BlockedInfo>& blockeds)
    {
        if (blockeds.empty()) return false;

        try
        {
            if (!conn || !conn->isValid())
            {
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::yellow, "Reconnecting to the database...");
                conn = driver->connect(this->properties);
                if (conn)
                    BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "Successfully reconnected to database");
            }

            std::string query = "DELETE FROM player_ignores WHERE (PlayerId, BlockedPlayerId) IN (";
            std::string placeholders;
            for (size_t i = 0; i < blockeds.size(); ++i) {
                placeholders += (i == 0 ? "(?, ?)" : ", (?, ?)");
            }
            query += placeholders + ")";

            std::unique_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement(query));

            int paramIndex = 1;
            for (const auto& blocked_info : blockeds)
            {
                pstmt->setInt(paramIndex++, blocked_info.player_account_id);
                pstmt->setInt(paramIndex++, blocked_info.blocked_account_id);
            }

            pstmt->executeUpdate();
            return true;
        }
        catch (sql::SQLException& e)
        {
            BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::red, "exception: ({})", e.what());
            return false;
        }
    }

    

    bool CDatabase::GetFrontAccount(const uint64_t& authKey, FrontAccount* outFrontAccount)
    {
        try
        {
            if (!conn || !conn->isValid())
            {
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::yellow, "Reconnecting to the database...");
                conn = driver->connect(this->properties);
                if (conn)
                    BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "Successfully reconnected to database");
            }

            std::unique_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement("SELECT * FROM accounts WHERE AuthKey = ?"));
            pstmt->setUInt64(1, authKey);

            std::unique_ptr<sql::ResultSet> result(pstmt->executeQuery());
            if (result->next())
            {
                *outFrontAccount = FrontAccount(result->getUInt("Id"),
					result->getBoolean("IsOnline"),
                    result->getString("Username").c_str(),
                    result->getString("Password").c_str(),
                    result->getString("Salt").c_str(),
                    result->getByte("Grade"),
                    result->getByte("PCRoom"),
                    result->getUInt64("AuthKey"),
                    result->getUInt("ClanId"),
                    result->getUInt("ClanKills"),
                    result->getUInt("ClanDeaths"),
                    result->getUInt("ClanAssists"),
                    result->getUInt64("ClanContribution"),
                    result->getUInt64("ClanWins"),
                    result->getUInt64("ClanLoses"),
                    result->getUInt64("ClanDraws"),
                    result->getString("Nickname").c_str(),
                    result->getUInt("Level"),
                    result->getUInt("Experience"),
                    result->getBoolean("Tutoral"),
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

                return true;
            }
            else return false;
        }
        catch (sql::SQLException& e)
        {
            BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::red, "exception: ({})", e.what());
            return false;
        }
    }


    bool CDatabase::SetAccountOffline(const std::uint32_t& accountId)
    {
        try
        {
            if (!conn || !conn->isValid())
            {
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::yellow, "Reconnecting to the database...");
                conn = driver->connect(this->properties);
                if (conn)
                    BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "Successfully reconnected to database");
            }

            std::unique_ptr<sql::Statement> stmt(conn->createStatement());
            stmt->execute("START TRANSACTION");

            try
            {
                std::unique_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement(
                    "UPDATE accounts SET IsOnline = FALSE WHERE Id = ?"));
                pstmt->setUInt(1, accountId);

                int affected = pstmt->executeUpdate();
                stmt->execute("COMMIT");
                return affected > 0;
            }
            catch (sql::SQLException& inner)
            {
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::red, "Inner SQL error in SetAccountOffline: ({})", inner.what());
                stmt->execute("ROLLBACK");
                return false;
            }
        }
        catch (sql::SQLException& outer)
        {
            BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::red, "Outer SQL error in SetAccountOffline: ({})", outer.what());
            return false;
        }
    }
    bool CDatabase::GetMainFrontAccount(const uint64_t& authKey, FrontAccount* outFrontAccount, ClanInfo* outClanInfo, PlayerDailyMission* outDailyMission, std::vector<Item>& inv_items, std::vector<BlockedInfo>& blockeds, std::vector<FriendInfo>& friends, std::vector<MailboxInfo>& mailbox_list, std::vector<std::uint32_t>& daily_mission_random_ids)
    {
        try
        {
            if (!conn || !conn->isValid())
            {
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::yellow, "Reconnecting to the database...");
                conn = driver->connect(this->properties);
                if (conn)
                    BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "Successfully reconnected to database");
            }

            std::unique_ptr<sql::Statement> stmt(conn->createStatement());
            stmt->execute("START TRANSACTION");

            try
            {
                // 1. Load account and clan
                std::unique_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement(R"(
                SELECT 
                    a.Id, a.IsOnline, a.Username, a.Password, a.Salt, a.Grade, a.PCRoom, a.AuthKey, a.ClanId,
                    a.ClanKills, a.ClanDeaths, a.ClanAssists, a.ClanContribution, a.ClanWins, a.ClanLoses, a.ClanDraws,
                    a.Nickname, a.Level, a.Experience, a.Tutoral, a.Story, a.GuideMission, a.Achievement, a.VoiceType, a.VIPExperience,
                    a.MaximumItems, a.MaximumEnergy, a.SelectedCharacter, a.PlayTime, a.MutedUntil, a.Coins, a.Energy, a.LuckyPoints,
                    a.MicroPoints, a.RockTokens, a.Coupons, a.Wins, a.Loses, a.Draws, a.Kills, a.Deaths, a.Assists, a.Headshots,
                    a.HighestKillStreak, a.MeleeKills, a.RifleKills, a.ShotgunKills, a.SniperKills, a.GatlingKills, a.BazookaKills,
                    a.GrenadeKills, a.ZombieKills, a.Infections, a.SingleWaveDailyAttempts, a.SingleWaveHighestWave, a.SingleWaveHighScore,
                    a.SingleWaveLastUpdate,
                    c.OwnerId, c.ClanName, c.ClanLogoFront, c.ClanLogoBack,
                    d.UpdateTime, d.Mission1, d.Mission2, d.Mission3, d.GoalMission1, d.GoalMission2, d.GoalMission3
                FROM accounts a
                LEFT JOIN clans c ON a.ClanId = c.Id
                LEFT JOIN player_daily_mission d ON a.Id = d.PlayerId
                WHERE a.AuthKey = ?
            )"));

                pstmt->setUInt64(1, authKey);
                std::unique_ptr<sql::ResultSet> result(pstmt->executeQuery());

                if (!result->next())
                {
                    stmt->execute("ROLLBACK");
                    return false;
                }

                auto accId = static_cast<std::int32_t>(result->getUInt("Id"));
                auto isOnline = result->getBoolean("IsOnline");
                *outFrontAccount = FrontAccount(
                    accId,
                    isOnline,
                    result->getString("Username").c_str(),
                    result->getString("Password").c_str(),
                    result->getString("Salt").c_str(),
                    result->getByte("Grade"),
                    result->getByte("PCRoom"),
                    result->getUInt64("AuthKey"),
                    result->getUInt("ClanId"),
                    result->getUInt("ClanKills"),
                    result->getUInt("ClanDeaths"),
                    result->getUInt("ClanAssists"),
                    result->getUInt64("ClanContribution"),
                    result->getUInt64("ClanWins"),
                    result->getUInt64("ClanLoses"),
                    result->getUInt64("ClanDraws"),
                    result->getString("Nickname").c_str(),
                    result->getUInt("Level"),
                    result->getUInt("Experience"),
                    result->getBoolean("Tutoral"),
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
                        BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "Daily mission outdated. Generating new missions.");
                        dm.mission1 = daily_mission_random_ids[0]; dm.goal_mission1 = 0;
                        dm.mission2 = daily_mission_random_ids[1]; dm.goal_mission2 = 0;
                        dm.mission3 = daily_mission_random_ids[2]; dm.goal_mission3 = 0;
                        dm.update_time = now_time;

                        if (existed)
                        {
                            std::unique_ptr<sql::PreparedStatement> updateStmt(conn->prepareStatement(R"(
                            UPDATE player_daily_mission SET
                                UpdateTime = ?, Mission1 = ?, Mission2 = ?, Mission3 = ?,
                                GoalMission1 = ?, GoalMission2 = ?, GoalMission3 = ?
                            WHERE PlayerId = ?
                        )"));

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
                            std::unique_ptr<sql::PreparedStatement> insertStmt(conn->prepareStatement(R"(
                            INSERT INTO player_daily_mission
                                (PlayerId, UpdateTime, Mission1, Mission2, Mission3, GoalMission1, GoalMission2, GoalMission3)
                            VALUES (?, ?, ?, ?, ?, ?, ?, ?)
                        )"));

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
                std::unique_ptr<sql::PreparedStatement> invStmt(conn->prepareStatement("SELECT * FROM player_items WHERE PlayerId = ?"));
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

                // 3. Blocked players
                std::unique_ptr<sql::PreparedStatement> blockStmt(conn->prepareStatement(
                    "SELECT BlockedPlayerId, BlockedNickname FROM player_ignores WHERE PlayerId = ?"));
                blockStmt->setInt(1, accId);
                std::unique_ptr<sql::ResultSet> blockRes(blockStmt->executeQuery());
                while (blockRes->next())
                    blockeds.push_back({ accId, blockRes->getInt("BlockedPlayerId"), 0, blockRes->getString("BlockedNickname").c_str() });

                // 4. Friends
                std::unique_ptr<sql::PreparedStatement> friendStmt(conn->prepareStatement(
                    "SELECT FriendPlayerId, State, FriendNickname FROM player_friends WHERE PlayerId = ?"));
                friendStmt->setInt(1, accId);
                std::unique_ptr<sql::ResultSet> friendRes(friendStmt->executeQuery());
                while (friendRes->next())
                    friends.push_back({ accId, friendRes->getInt("FriendPlayerId"), static_cast<uint8_t>(friendRes->getByte("State")), 0, friendRes->getString("FriendNickname").c_str() });

                // 5. Mailbox
                std::unique_ptr<sql::PreparedStatement> mailStmt(conn->prepareStatement(R"(
                SELECT Id, SenderId, SenderNickname, ReceiverId, ReceiverNickname, Date, GiftItemId, Message, IsNew, DeletedFromSender, DeletedFromReceiver
                FROM player_mailbox WHERE SenderId = ? OR ReceiverId = ?
            )"));
                mailStmt->setUInt(1, accId);
                mailStmt->setUInt(2, accId);
                std::unique_ptr<sql::ResultSet> mailRes(mailStmt->executeQuery());
                while (mailRes->next())
                {
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

                if (!isOnline)
                {
                    std::unique_ptr<sql::PreparedStatement> updateOnlineStmt(conn->prepareStatement(
                        "UPDATE accounts SET IsOnline = TRUE WHERE Id = ?"));
                    updateOnlineStmt->setUInt(1, accId);
                    updateOnlineStmt->executeUpdate();
                }


                // All succeeded — commit
                stmt->execute("COMMIT");
                return true;
            }
            catch (sql::SQLException& inner)
            {
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::red, "Inner SQL exception: ({})", inner.what());
                stmt->execute("ROLLBACK");
                return false;
            }
        }
        catch (sql::SQLException& outer)
        {
            BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::red, "Outer SQL exception: ({})", outer.what());
            return false;
        }
    }

    bool CDatabase::UpdateEndMatchInfo(std::vector<EndMatchUpdateDatabaseInfo>& updates)
    {
        if (updates.empty())
        {
            BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::yellow,
                "UpdateEndMatchInfo called with empty playerUpdates");
            return true;
        }

        try
        {
            if (!conn || !conn->isValid())
            {
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::yellow,
                    "Reconnecting to the database...");
                conn = driver->connect(this->properties);
                if (conn)
                    BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan,
                        "Successfully reconnected to database");
            }

            std::unique_ptr<sql::Statement> stmt(conn->createStatement());
            stmt->execute("START TRANSACTION");

            try
            {
                std::ostringstream sql;
                sql << "UPDATE accounts SET ";

                const std::vector<std::string> fields = {
                    "ClanKills", "ClanDeaths", "ClanAssists", "ClanContribution",
                    "ClanWins", "ClanLoses", "ClanDraws", "Level", "Experience",
                    "PlayTime", "SelectedCharacter", "Energy", "MicroPoints",
                    "Wins", "Loses", "Draws", "Kills", "Deaths", "Assists",
                    "Headshots", "HighestKillStreak", "MeleeKills", "RifleKills",
                    "ShotgunKills", "SniperKills", "GatlingKills", "BazookaKills",
                    "GrenadeKills", "ZombieKills", "Infections"
                };

                // Build CASE statements
                for (size_t i = 0; i < fields.size(); ++i)
                {
                    sql << fields[i] << " = CASE Id ";
                    for (size_t j = 0; j < updates.size(); ++j)
                    {
                        sql << "WHEN ? THEN ? ";
                    }
                    sql << "ELSE " << fields[i] << " END";
                    if (i != fields.size() - 1) sql << ", ";
                }

                // WHERE Id IN (?, ?, ...)
                sql << " WHERE Id IN (";
                for (size_t i = 0; i < updates.size(); ++i)
                {
                    sql << "?";
                    if (i != updates.size() - 1) sql << ", ";
                }
                sql << ");";

                std::string finalQuery = sql.str();
                std::unique_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement(finalQuery));

                // Safe manual field binding
                int paramIndex = 1;

                for (size_t fieldIndex = 0; fieldIndex < fields.size(); ++fieldIndex)
                {
                    const auto& field = fields[fieldIndex];
                    for (const auto& player : updates)
                    {
                        pstmt->setUInt(paramIndex++, player.Id);

                        // Safely access the correct member
                        switch (fieldIndex)
                        {
                        case 0: pstmt->setUInt(paramIndex++, player.ClanKills); break;
                        case 1: pstmt->setUInt(paramIndex++, player.ClanDeaths); break;
                        case 2: pstmt->setUInt(paramIndex++, player.ClanAssists); break;
                        case 3: pstmt->setUInt64(paramIndex++, player.ClanContribution); break;
                        case 4: pstmt->setUInt64(paramIndex++, player.ClanWins); break;
                        case 5: pstmt->setUInt64(paramIndex++, player.ClanLoses); break;
                        case 6: pstmt->setUInt64(paramIndex++, player.ClanDraws); break;
                        case 7: pstmt->setUInt(paramIndex++, player.Level); break;
                        case 8: pstmt->setUInt(paramIndex++, player.Experience); break;
                        case 9: pstmt->setUInt64(paramIndex++, player.PlayTime); break;
                        case 10: pstmt->setUInt(paramIndex++, player.SelectedCharacter); break;
                        case 11: pstmt->setUInt(paramIndex++, player.Energy); break;
                        case 12: pstmt->setUInt(paramIndex++, player.MicroPoints); break;
                        case 13: pstmt->setUInt(paramIndex++, player.Wins); break;
                        case 14: pstmt->setUInt(paramIndex++, player.Loses); break;
                        case 15: pstmt->setUInt(paramIndex++, player.Draws); break;
                        case 16: pstmt->setUInt(paramIndex++, player.Kills); break;
                        case 17: pstmt->setUInt(paramIndex++, player.Deaths); break;
                        case 18: pstmt->setUInt(paramIndex++, player.Assists); break;
                        case 19: pstmt->setUInt(paramIndex++, player.Headshots); break;
                        case 20: pstmt->setUInt(paramIndex++, player.HighestKillStreak); break;
                        case 21: pstmt->setUInt(paramIndex++, player.MeleeKills); break;
                        case 22: pstmt->setUInt(paramIndex++, player.RifleKills); break;
                        case 23: pstmt->setUInt(paramIndex++, player.ShotgunKills); break;
                        case 24: pstmt->setUInt(paramIndex++, player.SniperKills); break;
                        case 25: pstmt->setUInt(paramIndex++, player.GatlingKills); break;
                        case 26: pstmt->setUInt(paramIndex++, player.BazookaKills); break;
                        case 27: pstmt->setUInt(paramIndex++, player.GrenadeKills); break;
                        case 28: pstmt->setUInt(paramIndex++, player.ZombieKills); break;
                        case 29: pstmt->setUInt(paramIndex++, player.Infections); break;
                        }
                    }
                }


                // Bind WHERE Id IN (...)
                for (const auto& player : updates)
                {
                    pstmt->setUInt(paramIndex++, player.Id);
                }

                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::cyan,
                    "Executing CASE update with {} bound parameters", paramIndex - 1);

                pstmt->executeUpdate();
                stmt->execute("COMMIT");

                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::green,
                    "Successfully updated end match info for {} players", updates.size());

                return true;
            }
            catch (sql::SQLException& inner)
            {
                stmt->execute("ROLLBACK");
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::red,
                    "Inner SQL exception in UpdateEndMatchInfo: {}", inner.what());
                return false;
            }
        }
        catch (sql::SQLException& outer)
        {
            BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::red,
                "Outer SQL exception in UpdateEndMatchInfo: {}", outer.what());
            return false;
        }
    }


    bool CDatabase::GetFrontAccount(const std::string& username, FrontAccount* outFrontAccount)
    {
        try
        {
            if (!conn || !conn->isValid())
            {
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::yellow, "Reconnecting to the database...");
                conn = driver->connect(this->properties);
                if (conn)
                    BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "Successfully reconnected to database");
            }

            std::unique_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement("SELECT * FROM accounts WHERE Username = ?"));
                //"SELECT * FROM accounts WHERE Username = ? Id, Username, Password, Salt, Grade, AuthKey, ClanId, Nickname, Level, Experience, Tutoral, Story, VIPExperience, MaximumItems, MaximumEnergy, SelectedCharacter, PlayTime, MutedUntil, Coins, Energy, LuckyPoints, MicroPoints, RockTokens, Coupons, Wins, Loses, Draws, Kills, Deaths, Assists, Headshots, HighestKillStreak, MeleeKills, RifleKills, ShotgunKills, SniperKills, GatlingKills, BazookaKills, GrenadeKills, ZombieKills, Infections, SingleWaveDailyAttempts, SingleWaveHighestWave, SingleWaveHighScore, SingleWaveLastUpdate"));

            pstmt->setString(1, username.c_str());

            std::unique_ptr<sql::ResultSet> result(pstmt->executeQuery());
            if (result->next())
            {
                *outFrontAccount = FrontAccount(result->getUInt("Id"),
					result->getBoolean("IsOnline"),
                    result->getString("Username").c_str(),
                    result->getString("Password").c_str(),
                    result->getString("Salt").c_str(),
                    result->getByte("Grade"),
                    result->getByte("PCRoom"),
                    result->getUInt64("AuthKey"),
                    result->getUInt("ClanId"),
                    result->getUInt("ClanKills"),
                    result->getUInt("ClanDeaths"),
                    result->getUInt("ClanAssists"),
                    result->getUInt64("ClanContribution"),
                    result->getUInt64("ClanWins"),
                    result->getUInt64("ClanLoses"),
                    result->getUInt64("ClanDraws"),
                    result->getString("Nickname").c_str(),
                    result->getUInt("Level"),
                    result->getUInt("Experience"),
                    result->getBoolean("Tutoral"),
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

                return true;
            }
            else return false;
        }
        catch (sql::SQLException& e)
        {
            BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::red, "exception: ({})", e.what());
            return false;
        }
    }
    bool CDatabase::GetFrontAccount(const std::string& username, const std::string& password, FrontAccount* outFrontAccount, ClanInfo* outClanInfo)
    {
        try
        {
            if (!conn || !conn->isValid())
            {
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::yellow, "Reconnecting to the database...");
                conn = driver->connect(this->properties);
                if (conn)
                    BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "Successfully reconnected to database");
            }

            // Begin transaction
            std::unique_ptr<sql::Statement> stmt(conn->createStatement());
            stmt->execute("START TRANSACTION");

			std::unique_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement(
				"SELECT a.Id, a.IsOnline, a.Username, a.Password, a.Salt, a.Grade, a.AuthKey, a.ClanId, a.Level, a.Experience, "
				"a.Kills, a.Deaths, a.Assists, a.Wins, a.Loses, a.Draws, a.Nickname, "
				"c.Id as ClanId, c.OwnerId, c.ClanName, c.ClanLogoFront, c.ClanLogoBack "
				"FROM accounts a "
				"LEFT JOIN clans c ON a.ClanId = c.Id "
				"WHERE a.Username = ?"
			));

            pstmt->setString(1, username.c_str());
            //pstmt->setString(2, password.c_str());
            //pstmt->setString(3, salt.c_str());

            std::unique_ptr<sql::ResultSet> result(pstmt->executeQuery());

            if (!result->next())
            {
                stmt->execute("ROLLBACK");
                return false;
            }

            // Extract stored hash and salt
            std::string stored_hash_str = base64::from_base64(result->getString("Password").c_str());
            std::string stored_salt_str = base64::from_base64(result->getString("Salt").c_str());

            if (stored_hash_str.size() != 32 || stored_salt_str.size() != 16)
            {
                stmt->execute("ROLLBACK");
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
                return false;
            }

            // Verify using constant-time comparison
            if (crypto_verify32(computed_hash, stored_hash) != 0)
            {
                stmt->execute("ROLLBACK");
                return false;
            }

            *outFrontAccount = FrontAccount(result->getUInt("Id"),
                                            result->getBoolean("IsOnline"),
                                            result->getString("Username").c_str(),
                                            result->getString("Password").c_str(),
                                            result->getString("Salt").c_str(),
                                            result->getByte("Grade"),
                                            0,
                                            result->getUInt64("AuthKey"),
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

            // Commit transaction
            stmt->execute("COMMIT");
            return true;
        }
        catch (sql::SQLException& e)
        {
            BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::red, "exception: ({})", e.what());
            return false;
        }
    }
	bool CDatabase::GetFrontAccount(const uint64_t& authKey, FrontAccount *outFrontAccount, ClanInfo *outClanInfo)
	{
		try
		{
			if (!conn || !conn->isValid())
			{
				BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::yellow, "Reconnecting to the database...");
				conn = driver->connect(this->properties);
				if (conn)
					BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "Successfully reconnected to database");
			}

			// Begin transaction
			std::unique_ptr<sql::Statement> stmt(conn->createStatement());
			stmt->execute("START TRANSACTION");

			std::unique_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement(
				"SELECT a.Id, a.IsOnline, a.Username, a.Password, a.Salt, a.Grade, a.AuthKey, a.ClanId, a.Level, a.Experience, "
				"a.Kills, a.Deaths, a.Assists, a.Wins, a.Loses, a.Draws, a.Nickname, "
				"c.Id as ClanId, c.OwnerId, c.ClanName, c.ClanLogoFront, c.ClanLogoBack "
				"FROM accounts a "
				"LEFT JOIN clans c ON a.ClanId = c.Id "
				"WHERE a.AuthKey = ?"
			));

			pstmt->setUInt64(1, authKey);

			std::unique_ptr<sql::ResultSet> result(pstmt->executeQuery());

			if (result->next())
			{

				*outFrontAccount = FrontAccount(result->getUInt("Id"),
					result->getBoolean("IsOnline"),
					result->getString("Username").c_str(),
					result->getString("Password").c_str(),
					result->getString("Salt").c_str(),
					result->getByte("Grade"),
					0,
					result->getUInt64("AuthKey"),
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
                        clanId,
						result->getUInt("OwnerId"),
						result->getString("ClanName").c_str(),
						result->getUInt("ClanLogoFront"),
						result->getUInt("ClanLogoBack")
					);
				}

				// Commit transaction
				stmt->execute("COMMIT");
				return true;
			}
			else
			{
				// Rollback transaction if not found
				stmt->execute("ROLLBACK");
				return false;
			}
		}
		catch (sql::SQLException& e)
		{
			BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::red, "exception: ({})", e.what());
			return false;
		}
	}
    bool CDatabase::UpdateNickname(const std::string_view& nickname, const uint64_t& authKey)
    {
        try
        {
            if (!conn || !conn->isValid())
            {
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::yellow, "Reconnecting to the database...");
                conn = driver->connect(this->properties);
                if (conn)
                    BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "Successfully reconnected to database");
            }

            std::unique_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement("UPDATE accounts SET Nickname = ? WHERE AuthKey = ? LIMIT 1"));
            pstmt->setString(1, std::string(nickname).c_str()); 
            pstmt->setUInt64(2, authKey);
            int32_t affectedRows = pstmt->executeUpdate();
            return affectedRows > 0;
        }
        catch (sql::SQLException& e)
        {
            BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::red, "exception: ({})", e.what());
            return false;
        }
    }
    bool CDatabase::UpdateSelectedCharacter(const uint32_t& character, const uint64_t& authKey)
    {
        try
        {
            if (!conn || !conn->isValid())
            {
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::yellow, "Reconnecting to the database...");
                conn = driver->connect(this->properties);
                if (conn)
                    BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "Successfully reconnected to database");
            }

            std::unique_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement("UPDATE accounts SET SelectedCharacter = ? WHERE AuthKey = ? LIMIT 1"));
            pstmt->setInt(1, character);
            pstmt->setUInt64(2, authKey);
            int32_t affectedRows = pstmt->executeUpdate();
            return affectedRows > 0;
        }
        catch (sql::SQLException& e)
        {
            BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::red, "exception: ({})", e.what());
            return false;
        }
    }
    bool CDatabase::NicknameExists(const std::string_view& nickname)
    {
        try
        {
            if (!conn || !conn->isValid())
            {
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::yellow, "Reconnecting to the database...");
                conn = driver->connect(this->properties);
                if (conn)
                    BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "Successfully reconnected to database");
            }

            std::unique_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement("SELECT Id FROM accounts WHERE Nickname = ? LIMIT 1"));

            pstmt->setString(1, std::string(nickname).c_str());
            std::unique_ptr<sql::ResultSet> result(pstmt->executeQuery());
            return result->next();
        }
        catch (sql::SQLException& e)
        {
            BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::red, "exception: ({})", e.what());
            return false;
        }
    }

    bool CDatabase::NicknameExists(const std::string_view& nickname, uint32_t& account_id)
    {
        try
        {
            if (!conn || !conn->isValid())
            {
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::yellow, "Reconnecting to the database...");
                conn = driver->connect(this->properties);
                if (conn)
                    BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "Successfully reconnected to database");
            }

            std::unique_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement("SELECT Id FROM accounts WHERE Nickname = ? LIMIT 1"));

            pstmt->setString(1, std::string(nickname).c_str());
            std::unique_ptr<sql::ResultSet> result(pstmt->executeQuery());
            if (result->next())
            {
                account_id = result->getUInt("Id");
                return true;
            }
            return false;;
        }
        catch (sql::SQLException& e)
        {
            BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::red, "exception: ({})", e.what());
            return false;
        }
    }

    std::string CDatabase::GetDatabaseName()
    {
        return this->database_name;
    }

    bool CDatabase::RegisterClan(const std::string& name, const uint32_t& owner_id, const uint32_t& logo_front, const uint32_t& logo_back)
    {
       
        try
        {
            if (!conn || !conn->isValid())
            {
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::yellow, "Reconnecting to the database...");
                conn = driver->connect(this->properties);
                if (conn)
                    BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "Successfully reconnected to database");
            }

            std::unique_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement(
                "INSERT INTO clans (OwnerId, ClanName, ClanLogoFront, ClanLogoBack) "
                "VALUES (?, ?, ?, ?)"));


            pstmt->setUInt(1, owner_id);
            pstmt->setString(2, name.c_str());
            pstmt->setUInt(3, logo_front);
            pstmt->setUInt(4, logo_back);
            return !pstmt->execute();
        }
        catch (sql::SQLException& e)
        {
            BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::red, "exception: ({})", e.what());
            return false;
        }
    }
    bool CDatabase::GetClanInfo(const uint32_t& clanId, ClanInfo* outClanInfo)
    {
        try
        {
            if (!conn || !conn->isValid())
            {
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::yellow, "Reconnecting to the database...");
                conn = driver->connect(this->properties);
                if (conn)
                    BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "Successfully reconnected to database");
            }

            // Begin transaction
            std::unique_ptr<sql::Statement> stmt(conn->createStatement());
            stmt->execute("START TRANSACTION");

            // Selecting only necessary columns instead of SELECT *
            std::unique_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement(
                "SELECT Id, OwnerId, ClanName, ClanLogoFront, ClanLogoBack FROM clans WHERE Id = ?"
            ));
            pstmt->setUInt(1, clanId);

            std::unique_ptr<sql::ResultSet> result(pstmt->executeQuery());
            if (result->next())
            {
                *outClanInfo = ClanInfo(
                    result->getUInt("Id"),
                    result->getUInt("OwnerId"),
                    result->getString("ClanName").c_str(),
                    result->getUInt("ClanLogoFront"),
                    result->getUInt("ClanLogoBack")
                );

                // Commit transaction
                stmt->execute("COMMIT");
                return true;
            }
            else
            {
                // Rollback transaction if no result
                stmt->execute("ROLLBACK");
                return false;
            }
        }
        catch (sql::SQLException& e)
        {
            BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::red, "exception: ({})", e.what());

            // Rollback transaction on exception
            if (conn->isValid())
            {
                std::unique_ptr<sql::Statement> stmt(conn->createStatement());
                stmt->execute("ROLLBACK");
            }

            return false;
        }
    }

    bool CDatabase::UpdateClanInfo(const uint32_t& clan_id, const std::string& name, const uint32_t& owner_id, const uint32_t& logo_front, const uint32_t& logo_back)
    {
        try
        {
            if (!conn || !conn->isValid())
            {
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::yellow, "Reconnecting to the database...");
                conn = driver->connect(this->properties);
                if (conn)
                    BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "Successfully reconnected to database");
            }

            std::unique_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement(
                "UPDATE clans SET "
                "OwnerId=?, ClanName=?, ClanLogoFront=?, ClanLogoBack=? "
                "WHERE Id=?"));

            int index = 1;
            pstmt->setUInt(index++, owner_id);
            pstmt->setString(index++, name);
            pstmt->setUInt(index++, logo_front);
            pstmt->setUInt(index++, logo_back);


            pstmt->setUInt(index, clan_id);

            int updateCount = pstmt->executeUpdate();
            return updateCount > 0;
        }
        catch (sql::SQLException& e)
        {
            BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::red, "exception: ({})", e.what());
            return false;
        }
    }

    bool CDatabase::InsertPlayerMailbox(const MailboxInfo& mailbox_info, uint32_t& out_mail_id)
    {
        try
        {
            if (!conn || !conn->isValid())
            {
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::yellow, "Reconnecting to the database...");
                conn = driver->connect(this->properties);
                if (conn)
                    BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "Successfully reconnected to database");
            }
            conn->setAutoCommit(false);
            std::unique_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement(
                "INSERT INTO player_mailbox (SenderId, SenderNickname, ReceiverId, ReceiverNickname, Date, GiftItemId, Message, IsNew, DeletedFromSender, DeletedFromReceiver) "
                "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)"));

            int index = 1;
            pstmt->setUInt(index++, mailbox_info.sender_account_id);
            pstmt->setString(index++, mailbox_info.sender_nickname.c_str());
            pstmt->setUInt(index++, mailbox_info.receiver_account_id);
            pstmt->setString(index++, mailbox_info.receiver_nickname.c_str());
            pstmt->setUInt(index++, mailbox_info.time);
            pstmt->setUInt(index++, mailbox_info.gift_itemid);
            pstmt->setString(index++, mailbox_info.message.c_str());
            pstmt->setByte(index++, mailbox_info.is_new);
            pstmt->setByte(index++, mailbox_info.deleted_from_sender);
            pstmt->setByte(index++, mailbox_info.deleted_from_receiver);
            
            if (!pstmt->execute())
            {
                std::unique_ptr<sql::Statement> stmt(conn->createStatement());
                std::unique_ptr<sql::ResultSet> res(stmt->executeQuery("SELECT LAST_INSERT_ID()"));

                if (res->next())
                {
                    out_mail_id = res->getUInt(1);
                    conn->commit(); // Commit the transaction
                    conn->setAutoCommit(true); // Reset to default auto-commit mode
                    return true;
                }
            }
            conn->rollback(); 
            conn->setAutoCommit(true);
        }
        catch (sql::SQLException& e)
        {
            BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::red, "exception: ({})", e.what());
            if (conn)
            {
                try
                {
                    conn->rollback(); // Ensure rollback in case of error
                    conn->setAutoCommit(true); // Reset auto-commit mode
                }
                catch (...) {}
            }
        }
        return false;
    }
    bool CDatabase::DeletePlayerMailbox(const std::vector<MailboxInfo>& mails)
    {
        if (mails.empty()) return false;

        try
        {
            if (!conn || !conn->isValid())
            {
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::yellow, "Reconnecting to the database...");
                conn = driver->connect(this->properties);
                if (conn)
                    BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "Successfully reconnected to database");
            }

            std::string query = "DELETE FROM player_mailbox WHERE (Id) IN (";
            std::string placeholders;
            for (size_t i = 0; i < mails.size(); ++i)
            {
                placeholders += (i == 0 ? "(?, ?)" : ", (?, ?)");
            }
            query += placeholders + ")";

            std::unique_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement(query));

            int paramIndex = 1;
            for (const auto& mail_info : mails)
                pstmt->setInt(paramIndex++, mail_info.mail_id);

            pstmt->executeUpdate();
            return true;
        }
        catch (sql::SQLException& e)
        {
            BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::red, "exception: ({})", e.what());
            return false;
        }
    }

    std::vector<MailboxInfo> CDatabase::GetPlayerMailbox(const int32_t& acc_id)
    {
        std::vector<MailboxInfo> mailbox_list;
        try
        {
            if (!conn || !conn->isValid())
            {
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::yellow, "Reconnecting to the database...");
                conn = driver->connect(this->properties);
                if (conn)
                    BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "Successfully reconnected to database");
            }

            std::unique_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement(
                "SELECT Id, SenderId, SenderNickname, ReceiverId, ReceiverNickname, Date, GiftItemId, Message, IsNew, DeletedFromSender, DeletedFromReceiver "
                "FROM player_mailbox WHERE SenderId = ? OR ReceiverId = ?"));

            pstmt->setUInt(1, acc_id);
            pstmt->setUInt(2, acc_id);

            std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());

            while (res->next())
            {
                MailboxInfo mailbox_info = 
                { 
                    res->getUInt("Id") , res->getUInt("SenderId"), res->getString("SenderNickname").c_str(), res->getUInt("ReceiverId"), res->getString("ReceiverNickname").c_str(),
                    res->getUInt("Date"), res->getUInt("GiftItemId"), res->getString("Message").c_str(), static_cast<bool>(res->getByte("IsNew")), static_cast<bool>(res->getByte("DeletedFromSender")), static_cast<bool>(res->getByte("DeletedFromReceiver"))
                };
                mailbox_list.push_back(mailbox_info);
            }
        }
        catch (sql::SQLException& e)
        {
            BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::red, "exception: ({})", e.what());
        }
        return mailbox_list;
    }

    uint32_t CDatabase::GetPlayerReceiverMailboxCount(const int32_t& acc_id)
    {
        try
        {
            if (!conn || !conn->isValid())
            {
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::yellow, "Reconnecting to the database...");
                conn = driver->connect(this->properties);
                if (conn)
                    BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "Successfully reconnected to database");
            }

            std::unique_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement(
                "SELECT COUNT(*) AS MailCount FROM player_mailbox WHERE ReceiverId = ? AND GiftItemId = 0"));

            pstmt->setUInt(1, acc_id);

            std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());

            if (res->next())
            {
                return res->getInt("MailCount");
            }
        }
        catch (sql::SQLException& e)
        {
            BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::red, "exception: ({})", e.what());
        }

        return 0;
    }
    uint32_t CDatabase::GetPlayerReceiverGiftboxCount(const int32_t& acc_id)
    {
        try
        {
            if (!conn || !conn->isValid())
            {
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::yellow, "Reconnecting to the database...");
                conn = driver->connect(this->properties);
                if (conn)
                    BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "Successfully reconnected to database");
            }

            std::unique_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement(
                "SELECT COUNT(*) AS MailCount FROM player_mailbox WHERE ReceiverId = ? AND GiftItemId != 0"));

            pstmt->setUInt(1, acc_id);

            std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());

            if (res->next())
            {
                return res->getInt("MailCount");
            }
        }
        catch (sql::SQLException& e)
        {
            BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::red, "exception: ({})", e.what());
        }

        return 0;
    }
    bool CDatabase::UpdateMailboxIsNew(const std::vector<uint32_t>& mail_ids, bool is_new)
    {
        try
        {
            if (!conn || !conn->isValid())
            {
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::yellow, "Reconnecting to the database...");
                conn = driver->connect(this->properties);
                if (conn)
                    BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "Successfully reconnected to database");
            }

            if (mail_ids.empty())
            {
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::yellow, "No mail IDs provided for updating IsNew.");
                return false;
            }

            conn->setAutoCommit(false);

            std::string query = "UPDATE player_mailbox SET IsNew = ? WHERE Id IN (";
            for (size_t i = 0; i < mail_ids.size(); ++i)
            {
                query += "?";
                if (i < mail_ids.size() - 1)
                    query += ",";
            }
            query += ")";

            std::unique_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement(query));
            pstmt->setByte(1, static_cast<unsigned char>(is_new));

            for (size_t i = 0; i < mail_ids.size(); ++i)
            {
                pstmt->setUInt(static_cast<unsigned int>(i + 2), mail_ids[i]);
            }

            int affected_rows = pstmt->executeUpdate();
            if (affected_rows > 0)
            {
                conn->commit(); // Commit the transaction
                conn->setAutoCommit(true); // Reset to default auto-commit mode
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "Updated IsNew for {} rows", affected_rows);
                return true;
            }

            conn->rollback(); // Rollback in case no rows were updated
            conn->setAutoCommit(true);
        }
        catch (sql::SQLException& e)
        {
            BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::red, "SQL exception: {}", e.what());
            if (conn)
            {
                try
                {
                    conn->rollback(); // Ensure rollback in case of error
                    conn->setAutoCommit(true); // Reset auto-commit mode
                }
                catch (...) {}
            }
        }
        return false;
    }

    bool CDatabase::UpdateOrDeleteMailboxForSender(const std::vector<uint32_t>& mail_ids)
    {
        try
        {
            if (!conn || !conn->isValid())
            {
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::yellow, "Reconnecting to the database...");
                conn = driver->connect(this->properties);
                if (conn)
                    BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "Successfully reconnected to database");
            }

            if (mail_ids.empty())
            {
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::yellow, "No mail IDs provided for updating or deleting.");
                return false;
            }

            conn->setAutoCommit(false);

            // Step 1: Delete rows where DeletedFromReceiver is already true
            std::string delete_query = "DELETE FROM player_mailbox WHERE Id IN (";
            for (size_t i = 0; i < mail_ids.size(); ++i)
            {
                delete_query += "?";
                if (i < mail_ids.size() - 1)
                    delete_query += ",";
            }
            delete_query += ") AND DeletedFromReceiver = 1";

            std::unique_ptr<sql::PreparedStatement> delete_stmt(conn->prepareStatement(delete_query));
            for (size_t i = 0; i < mail_ids.size(); ++i)
            {
                delete_stmt->setUInt(static_cast<unsigned int>(i + 1), mail_ids[i]);
            }

            int deleted_rows = delete_stmt->executeUpdate();

            // Step 2: Update DeletedFromSender for remaining rows
            std::string update_query = "UPDATE player_mailbox SET DeletedFromSender = 1 WHERE Id IN (";
            for (size_t i = 0; i < mail_ids.size(); ++i)
            {
                update_query += "?";
                if (i < mail_ids.size() - 1)
                    update_query += ",";
            }
            update_query += ") AND DeletedFromReceiver = 0";

            std::unique_ptr<sql::PreparedStatement> update_stmt(conn->prepareStatement(update_query));
            for (size_t i = 0; i < mail_ids.size(); ++i)
            {
                update_stmt->setUInt(static_cast<unsigned int>(i + 1), mail_ids[i]);
            }

            int updated_rows = update_stmt->executeUpdate();

            conn->commit(); // Commit the transaction
            conn->setAutoCommit(true); // Reset to default auto-commit mode

            BaseLib::EventLog->Debug(
                std::source_location::current(),
                fmt::color::dark_cyan,
                "Deleted {} rows and updated {} rows.",
                deleted_rows,
                updated_rows
            );

            return true;
        }
        catch (sql::SQLException& e)
        {
            BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::red, "SQL exception: {}", e.what());
            if (conn)
            {
                try
                {
                    conn->rollback(); // Ensure rollback in case of error
                    conn->setAutoCommit(true); // Reset auto-commit mode
                }
                catch (...) {}
            }
        }
        return false;
    }

    bool CDatabase::UpdateOrDeleteMailboxForReceiver(const std::vector<uint32_t>& mail_ids)
    {
        try
        {
            if (!conn || !conn->isValid())
            {
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::yellow, "Reconnecting to the database...");
                conn = driver->connect(this->properties);
                if (conn)
                    BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "Successfully reconnected to database");
            }

            if (mail_ids.empty())
            {
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::yellow, "No mail IDs provided for updating or deleting.");
                return false;
            }

            conn->setAutoCommit(false);

            // Step 1: Delete rows where DeletedFromSender is already true
            std::string delete_query = "DELETE FROM player_mailbox WHERE Id IN (";
            for (size_t i = 0; i < mail_ids.size(); ++i)
            {
                delete_query += "?";
                if (i < mail_ids.size() - 1)
                    delete_query += ",";
            }
            delete_query += ") AND DeletedFromSender = 1";

            std::unique_ptr<sql::PreparedStatement> delete_stmt(conn->prepareStatement(delete_query));
            for (size_t i = 0; i < mail_ids.size(); ++i)
            {
                delete_stmt->setUInt(static_cast<unsigned int>(i + 1), mail_ids[i]);
            }

            int deleted_rows = delete_stmt->executeUpdate();

            // Step 2: Update DeletedFromReceiver for remaining rows
            std::string update_query = "UPDATE player_mailbox SET DeletedFromReceiver = 1 WHERE Id IN (";
            for (size_t i = 0; i < mail_ids.size(); ++i)
            {
                update_query += "?";
                if (i < mail_ids.size() - 1)
                    update_query += ",";
            }
            update_query += ") AND DeletedFromSender = 0";

            std::unique_ptr<sql::PreparedStatement> update_stmt(conn->prepareStatement(update_query));
            for (size_t i = 0; i < mail_ids.size(); ++i)
            {
                update_stmt->setUInt(static_cast<unsigned int>(i + 1), mail_ids[i]);
            }

            int updated_rows = update_stmt->executeUpdate();

            conn->commit(); // Commit the transaction
            conn->setAutoCommit(true); // Reset to default auto-commit mode

            BaseLib::EventLog->Debug(
                std::source_location::current(),
                fmt::color::dark_cyan,
                "Deleted {} rows and updated {} rows.",
                deleted_rows,
                updated_rows
            );

            return true;
        }
        catch (sql::SQLException& e)
        {
            BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::red, "SQL exception: {}", e.what());
            if (conn)
            {
                try
                {
                    conn->rollback(); // Ensure rollback in case of error
                    conn->setAutoCommit(true); // Reset auto-commit mode
                }
                catch (...) {}
            }
        }
        return false;
    }

    bool CDatabase::GetSystemMonthlyRewards(const uint32_t& month, SystemMonthlyRewards* outMonthlyRewards)
    {
        try
        {
            if (!conn || !conn->isValid())
            {
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::yellow, "Reconnecting to the database...");
                conn = driver->connect(this->properties);
                if (conn)
                    BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "Successfully reconnected to database");
            }

            std::unique_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement("SELECT * FROM system_monthly_rewards WHERE Month = ?"));
            pstmt->setUInt(1, month);

            std::unique_ptr<sql::ResultSet> result(pstmt->executeQuery());
            if (result->next())
            {
                *outMonthlyRewards = SystemMonthlyRewards(result->getUInt("Month"),
                    {
                        result->getUInt("Day1"), result->getUInt("Day2"), result->getUInt("Day3"), result->getUInt("Day4"), result->getUInt("Day5"),
                        result->getUInt("Day6"), result->getUInt("Day7"), result->getUInt("Day8"), result->getUInt("Day9"), result->getUInt("Day10"),
                        result->getUInt("Day11"), result->getUInt("Day12"), result->getUInt("Day13"), result->getUInt("Day14"), result->getUInt("Day15"),
                        result->getUInt("Day16"), result->getUInt("Day17"), result->getUInt("Day18"), result->getUInt("Day19"), result->getUInt("Day20"),
                        result->getUInt("Day21"), result->getUInt("Day22"), result->getUInt("Day23"), result->getUInt("Day24"), result->getUInt("Day25"),
                        result->getUInt("Day26"), result->getUInt("Day27"), result->getUInt("Day28"), result->getUInt("Day29"), result->getUInt("Day30"),
                        result->getUInt("Day31")
                    }
                );

                return true;
            }
            else return false;
        }
        catch (sql::SQLException& e)
        {
            BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::red, "exception: ({})", e.what());
            return false;
        }
    }

    bool CDatabase::GetPlayerMonthlyDayCount(const uint32_t& acc_id, PlayerMonthlyReward* outMonthlyRewards)
    {
        try
        {
            if (!conn || !conn->isValid())
            {
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::yellow, "Reconnecting to the database...");
                conn = driver->connect(this->properties);
                if (conn)
                    BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "Successfully reconnected to database");
            }

            std::unique_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement("SELECT * FROM player_monthly_rewards WHERE PlayerId = ?"));
            pstmt->setUInt(1, acc_id);

            std::unique_ptr<sql::ResultSet> result(pstmt->executeQuery());
            if (result->next())
            {
                *outMonthlyRewards = PlayerMonthlyReward(acc_id, result->getByte("RewardCount"), result->getUInt64("LastUpdate"));

                return true;
            }
            else return false;
        }
        catch (sql::SQLException& e)
        {
            BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::red, "exception: ({})", e.what());
            return false;
        }
    }

    bool CDatabase::InsertPlayerMonthlyDayCount(const uint32_t& acc_id, const uint8_t& reward_count, const uint64_t& last_update)
    {
        try
        {
            if (!conn || !conn->isValid())
            {
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::yellow, "Reconnecting to the database...");
                conn = driver->connect(this->properties);
                if (conn)
                    BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "Successfully reconnected to database");
            }

            std::unique_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement(
                "INSERT INTO player_monthly_rewards (PlayerId, RewardCount, LastUpdate) VALUES (?, ?, ?)"));
            pstmt->setUInt(1, acc_id);
            pstmt->setUInt(2, reward_count);
            pstmt->setUInt64(3, last_update);

            pstmt->executeUpdate();
            return true;
        }
        catch (sql::SQLException& e)
        {
            BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::red, "exception: ({})", e.what());
            return false;
        }
    }

    bool CDatabase::UpdatePlayerMonthlyDayCount(const uint32_t& acc_id, const uint8_t& reward_count, const uint64_t& last_update)
    {
        try
        {
            if (!conn || !conn->isValid())
            {
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::yellow, "Reconnecting to the database...");
                conn = driver->connect(this->properties);
                if (conn)
                    BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "Successfully reconnected to database");
            }

            std::unique_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement(
                "UPDATE player_monthly_rewards SET RewardCount = ?, LastUpdate = ? WHERE PlayerId = ?"));
            pstmt->setUInt(1, reward_count);
            pstmt->setUInt64(2, last_update);
            pstmt->setUInt(3, acc_id);

            pstmt->executeUpdate();
            return true;
        }
        catch (sql::SQLException& e)
        {
            BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::red, "exception: ({})", e.what());
            return false;
        }
    }

    bool CDatabase::GetPlayerDailyMission(const uint32_t& acc_id, PlayerDailyMission* outDailyMission) {
        try {
            if (!conn || !conn->isValid()) {
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::yellow, "Reconnecting to the database...");
                conn = driver->connect(this->properties);
                if (conn)
                    BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "Successfully reconnected to database");
            }

            std::unique_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement("SELECT * FROM player_daily_mission WHERE PlayerId = ?"));
            pstmt->setUInt(1, acc_id);

            std::unique_ptr<sql::ResultSet> result(pstmt->executeQuery());
            if (result->next()) {
                *outDailyMission = PlayerDailyMission{
                    acc_id,
                    result->getUInt64("UpdateTime"),
                    result->getUInt("Mission1"),
                    result->getUInt("Mission2"),
                    result->getUInt("Mission3"),
                    result->getUInt("GoalMission1"),
                    result->getUInt("GoalMission2"),
                    result->getUInt("GoalMission3")
                };
                return true;
            }
            return false;
        }
        catch (sql::SQLException& e) {
            BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::red, "exception: ({})", e.what());
            return false;
        }
    }

    bool CDatabase::InsertPlayerDailyMission(const uint32_t& acc_id, const PlayerDailyMission& dailyMission) {
        try {
            if (!conn || !conn->isValid()) {
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::yellow, "Reconnecting to the database...");
                conn = driver->connect(this->properties);
                if (conn)
                    BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "Successfully reconnected to database");
            }

            std::unique_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement(
                "INSERT INTO player_daily_mission (PlayerId, UpdateTime, Mission1, Mission2, Mission3, GoalMission1, GoalMission2, GoalMission3) VALUES (?, ?, ?, ?, ?, ?, ?, ?)"));
            pstmt->setUInt(1, acc_id);
            pstmt->setUInt64(2, dailyMission.update_time);
            pstmt->setUInt(3, dailyMission.mission1);
            pstmt->setUInt(4, dailyMission.mission2);
            pstmt->setUInt(5, dailyMission.mission3);
            pstmt->setUInt(6, dailyMission.goal_mission1);
            pstmt->setUInt(7, dailyMission.goal_mission2);
            pstmt->setUInt(8, dailyMission.goal_mission3);

            pstmt->executeUpdate();
            return true;
        }
        catch (sql::SQLException& e) {
            BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::red, "exception: ({})", e.what());
            return false;
        }
    }

    bool CDatabase::UpdatePlayerDailyMission(const uint32_t& acc_id, const PlayerDailyMission& dailyMission) {
        try {
            if (!conn || !conn->isValid()) {
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::yellow, "Reconnecting to the database...");
                conn = driver->connect(this->properties);
                if (conn)
                    BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "Successfully reconnected to database");
            }

            std::unique_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement(
                "UPDATE player_daily_mission SET UpdateTime = ?, Mission1 = ?, Mission2 = ?, Mission3 = ?, GoalMission1 = ?, GoalMission2 = ?, GoalMission3 = ? WHERE PlayerId = ?"));
            pstmt->setUInt64(1, dailyMission.update_time);
            pstmt->setUInt(2, dailyMission.mission1);
            pstmt->setUInt(3, dailyMission.mission2);
            pstmt->setUInt(4, dailyMission.mission3);
            pstmt->setUInt(5, dailyMission.goal_mission1);
            pstmt->setUInt(6, dailyMission.goal_mission2);
            pstmt->setUInt(7, dailyMission.goal_mission3);
            pstmt->setUInt(8, acc_id);

            pstmt->executeUpdate();
            return true;
        }
        catch (sql::SQLException& e) {
            BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::red, "exception: ({})", e.what());
            return false;
        }
    }

    std::vector<GachaponSaleInfo> CDatabase::GetGachaponSalesInfo()
    {
        std::vector<GachaponSaleInfo> sales;
        try
        {
            if (!conn || !conn->isValid())
            {
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::yellow, "Reconnecting to the database...");
                conn = driver->connect(this->properties);
                if (conn)
                    BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "Successfully reconnected to database");
            }

            std::unique_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement(
                "SELECT GachaponId, SalePrice, UNIX_TIMESTAMP(EventStartDate) AS EventStartTimestamp, UNIX_TIMESTAMP(EventEndDate) AS EventEndTimestamp FROM system_gachapon_machine"
            ));


            std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
            
            while (res->next())
            {
                GachaponSaleInfo gachapon_info =
                {

                    res->getUInt("GachaponId") , res->getUInt("SalePrice"), 
                    res->getUInt("EventStartTimestamp"), res->getUInt("EventEndTimestamp")
                };
                sales.push_back(gachapon_info);
            }
        }
        catch (sql::SQLException& e)
        {
            BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::red, "exception: ({})", e.what());
        }
        return sales;
    }

    bool CDatabase::DeleteGachaponSaleInfo(const uint32_t& gachapon_id)
    {
        try
        {
            if (!conn || !conn->isValid())
            {
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::yellow, "Reconnecting to the database...");
                conn = driver->connect(this->properties);
                if (conn)
                    BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "Successfully reconnected to database");
            }
            std::unique_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement("DELETE FROM system_gachapon_machine WHERE GachaponId = ?"));
            pstmt->setUInt(1, gachapon_id);
            return !pstmt->execute();
        }
        catch (sql::SQLException& e)
        {
            BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::red, "exception: ({})", e.what());
            return false;
        }
    }

    std::unique_ptr<CDatabase> Database = std::make_unique<CDatabase>();
}
