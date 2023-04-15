#include "CDatabase.h"
namespace BaseLib
{
    void CDatabase::Initialize(const std::string& database, const std::string& host, const std::uint16_t& port, const std::string& user, const std::string& password)
    {
        try
        {
            this->database_name = database;
            this->driver = sql::mariadb::get_driver_instance();
            sql::Properties properties;
            properties["hostName"] = std::string(host + ":" + std::to_string(port)).c_str();
            properties["userName"] = user.c_str();
            properties["password"] = password.c_str();
            conn = driver->connect(properties);

            if (conn)
            {
                EventLog->Info("CDatabase() - Connected to '%s'", std::string(host + ":" + std::to_string(port)).c_str());
                std::printf("CDatabase() - Connected '%s'\n", std::string(host + ":" + std::to_string(port)).c_str());
                if (CreateDatabase(database))
                {

                    EventLog->Info("CDatabase() - Created database '%s'", database.c_str());
                    std::printf("CDatabase() - Created database '%s'\n", database.c_str());
                    conn->setSchema(database);
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
                    PlayTime datetime(6) NOT NULL, 
                    MutedUntil datetime(6) NOT NULL, 
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
                    GetlingKills int unsigned NOT NULL,
                    BazookaKills int unsigned NOT NULL,
                    GranadeKills int unsigned NOT NULL,
                    ZombieKills int unsigned NOT NULL,
                    Infections int unsigned NOT NULL,
                    SingleWaveDailyAttempts int unsigned NOT NULL,
                    SingleWaveHighestWave int unsigned NOT NULL,
                    SingleWaveHighScore int unsigned NOT NULL,
                    SingleWaveLastUpdate datetime(6) NOT NULL,
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


                    CreateTable("player_characters", R"(
                    Id int unsigned NOT NULL AUTO_INCREMENT,
                    PlayerId int unsigned NOT NULL,
                    CharacterId int unsigned NOT NULL,
                    MeleeId bigint unsigned DEFAULT NULL,
                    RifleId bigint unsigned DEFAULT NULL,
                    ShotgunId bigint unsigned DEFAULT NULL,
                    SniperId bigint unsigned DEFAULT NULL,
                    GatlingId bigint unsigned DEFAULT NULL,
                    BazookaId bigint unsigned DEFAULT NULL,
                    GrenadeId bigint unsigned DEFAULT NULL,
                    HairId bigint unsigned DEFAULT NULL,
                    FaceId bigint unsigned DEFAULT NULL,
                    UpperId bigint unsigned DEFAULT NULL,
                    UnderId bigint unsigned DEFAULT NULL,
                    PantsId bigint unsigned DEFAULT NULL,
                    ShirtId bigint unsigned DEFAULT NULL,
                    BootsId bigint unsigned DEFAULT NULL,
                    GlassesId bigint unsigned DEFAULT NULL,
                    BackId bigint unsigned DEFAULT NULL,
                    WaistId bigint unsigned DEFAULT NULL,
                    FootingId bigint unsigned DEFAULT NULL,
                    ObjectId bigint unsigned DEFAULT NULL,
                    SetId bigint unsigned DEFAULT NULL,
                    PRIMARY KEY (Id),
                    KEY IX_player_characters_PlayerId (PlayerId),
                    CONSTRAINT FK_player_characters_accounts_PlayerId FOREIGN KEY (PlayerId) REFERENCES accounts (Id) ON DELETE CASCADE)");

                    CreateTable("player_friends", R"(
                    Id int unsigned NOT NULL AUTO_INCREMENT,
                    PlayerId int unsigned NOT NULL,
                    FriendPlayerId int unsigned NOT NULL,
                    State tinyint unsigned NOT NULL,
                    PRIMARY KEY (Id),
                    KEY IX_player_friends_FriendPlayerId (FriendPlayerId),
                    KEY IX_player_friends_PlayerId (PlayerId),
                    CONSTRAINT FK_player_friends_accounts_FriendPlayerId FOREIGN KEY (FriendPlayerId) REFERENCES accounts (Id) ON DELETE CASCADE,
                    CONSTRAINT FK_player_friends_accounts_PlayerId FOREIGN KEY (PlayerId) REFERENCES accounts (Id) ON DELETE CASCADE)");

                    CreateTable("player_ignores", R"(
                    Id int unsigned NOT NULL AUTO_INCREMENT,
                    PlayerId int unsigned NOT NULL,
                    BlockedPlayerId int unsigned NOT NULL,
                    PRIMARY KEY (Id),
                    KEY IX_player_ignores_BlockedPlayerId (BlockedPlayerId),
                    KEY IX_player_ignores_PlayerId (PlayerId),
                    CONSTRAINT FK_player_ignores_accounts_BlockedPlayerId FOREIGN KEY (BlockedPlayerId) REFERENCES accounts (Id) ON DELETE CASCADE,
                    CONSTRAINT FK_player_ignores_accounts_PlayerId FOREIGN KEY (PlayerId) REFERENCES accounts (Id) ON DELETE CASCADE)");

                    CreateTable("player_items", R"(
                    SerialInfo bigint unsigned NOT NULL,
                    PlayerId int unsigned NOT NULL,
                    ItemId int unsigned NOT NULL,
                    ItemType int unsigned NOT NULL,
                    ExpirationDate datetime(6) NOT NULL,
                    Repair smallint unsigned NOT NULL,
                    Energy smallint unsigned NOT NULL,
                    IsSealed int unsigned NOT NULL,
                    SealLevel int unsigned NOT NULL,
                    EnhanceExp int unsigned NOT NULL,
                    EnhanceLevel int unsigned NOT NULL,
                    Stock int unsigned NOT NULL,
                    PRIMARY KEY (SerialInfo),
                    KEY IX_player_items_PlayerId (PlayerId),
                    CONSTRAINT FK_player_items_accounts_PlayerId FOREIGN KEY (PlayerId) REFERENCES accounts (Id) ON DELETE CASCADE)");

                    CreateTable("player_mailbox", R"(
                    Id int unsigned NOT NULL AUTO_INCREMENT,
                    SenderId int unsigned NOT NULL,
                    ReceiverId int unsigned NOT NULL,
                    Date datetime(6) NOT NULL,
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
                    GachaponId int unsigned NOT NULL AUTO_INCREMENT,
                    Description  varchar(127) DEFAULT NULL,
                    SalePrice int unsigned NOT NULL,
                    EventStartDate datetime(6) NOT NULL,
                    EventEndDate datetime(6) NOT NULL,
                    PRIMARY KEY (GachaponId))");

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
                }
                else
                    conn->setSchema(database);
            }
        }
        catch (sql::SQLException& e)
        {
            EventLog->Info("CDatabase() - Exception (%s)", e.what());
            std::printf("CDatabase() - Exception (%s)'\n", e.what());
        }
    }

    bool CDatabase::CreateTable(const std::string& table_name, const std::string& data_collumns)
    {
        try
        {
            std::unique_ptr<sql::Statement> stmt(conn->createStatement());
            auto retn = !stmt->execute(std::format("CREATE TABLE IF NOT EXISTS {0} ({1})", table_name.c_str(), data_collumns.c_str()));
            if (retn)
            {
                EventLog->Info("CDatabase() - Created table '%s'", table_name.c_str());
                std::printf("CDatabase() - Created table '%s'\n", table_name.c_str());
            }
            return retn;
        }
        catch (sql::SQLException& e)
        {
            EventLog->Info("CDatabase() - Failed to create table '%s'", table_name.c_str());
            std::printf("CDatabase() - Failed to create '%s'\n", table_name.c_str());
            EventLog->Info("CDatabase() - Exception (%s)", e.what());
            std::printf("CDatabase() - Exception (%s)'\n", e.what());
            return false;
        }
    }

    bool CDatabase::CreateDatabase(const std::string& name)
    {
        try
        {
            std::unique_ptr<sql::Statement> stmt(conn->createStatement());
            return !stmt->execute(std::format("CREATE DATABASE {0}", name.c_str()));
        }
        catch (sql::SQLException& e)
        {
            if (!std::string(e.what()).contains("database exists"))
            {
                EventLog->Info("CDatabase() - Exception (%s)", e.what());
                std::printf("CDatabase() - Exception (%s)'\n", e.what());
            }
            return false;
        }
    }

    bool CDatabase::RegisterAccount(const std::string& username, const std::string& password, const std::uint8_t& grade, const std::uint32_t& mp, const std::uint32_t& rt, const std::uint32_t& coupons, const std::uint32_t& coins, const std::uint32_t& energy, const std::uint32_t& max_items, const std::uint32_t& max_battery, const std::string& nickname)
    {

        auto hash = Utility::Hash(password);
        auto auth_key = Utility::GenerateAuthKey(username, password);

        try
        {
            std::unique_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement(
                "INSERT INTO accounts(Username, Password, Salt, Grade, AuthKey, Nickname, Level, Experience, Tutoral, Story, VIPExperience, MaximumItems, MaximumEnergy, SelectedCharacter, PlayTime, MutedUntil, Coins, Energy, LuckyPoints, MicroPoints, RockTokens, Coupons, Wins, Loses, Draws, Kills, Deaths, Assists, Headshots, HighestKillStreak, MeleeKills, RifleKills, ShotgunKills, SniperKills, GetlingKills, BazookaKills, GranadeKills, ZombieKills, Infections, SingleWaveDailyAttempts, SingleWaveHighestWave, SingleWaveHighScore, SingleWaveLastUpdate) "
                "VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)"));


            std::time_t now = std::time(nullptr);
            sql::SQLString datetime_str = sql::SQLString(std::to_string(now));

            pstmt->setString(1, username.c_str());
            pstmt->setString(2, hash.first.c_str());
            pstmt->setString(3, hash.second.c_str());
            pstmt->setByte(4, grade);
            pstmt->setUInt64(5, auth_key);
            pstmt->setString(6, nickname.c_str());
            pstmt->setUInt(7, 1);//level
            pstmt->setUInt(8, 0);//exp
            pstmt->setByte(9, false);//tutorial
            pstmt->setUInt(10, 0);//vip exp
            pstmt->setUInt(11, max_items);//max items
            pstmt->setUInt(12, max_battery);//max battery
            pstmt->setUInt(13, 0);//selected char
            pstmt->setString(14, "FROM_UNIXTIME(" + datetime_str + ")");//playtime
            pstmt->setString(15, "FROM_UNIXTIME(" + datetime_str + ")");//muteduntil
            pstmt->setUInt(16, coins);//coins
            pstmt->setUInt(17, energy);//energy
            pstmt->setUInt(18, 0);//lucky points
            pstmt->setUInt(19, mp);//mp
            pstmt->setUInt(20, rt);//rt
            pstmt->setUInt(21, coupons);//coupons
            pstmt->setUInt(22, 0);//Wins
            pstmt->setUInt(23, 0);//Loses
            pstmt->setUInt(24, 0);//Draws
            pstmt->setUInt(25, 0);//Kills
            pstmt->setUInt(26, 0);//Deaths
            pstmt->setUInt(27, 0);//Assists
            pstmt->setUInt(28, 0);//Headshots
            pstmt->setUInt(29, 0);//HighestKillStreak
            pstmt->setUInt(30, 0);//MeleeKills
            pstmt->setUInt(31, 0);//RifleKills
            pstmt->setUInt(32, 0);//ShotgunKills
            pstmt->setUInt(33, 0);//SniperKills
            pstmt->setUInt(34, 0);//GetlingKills
            pstmt->setUInt(35, 0);//BazookaKills
            pstmt->setUInt(36, 0);//GranadeKills
            pstmt->setUInt(37, 0);//ZombieKills
            pstmt->setUInt(38, 0);//Infections
            pstmt->setUInt(39, 0);//SingleWaveDailyAttempts
            pstmt->setUInt(40, 0);//SingleWaveHighestWave
            pstmt->setUInt(41, 0);//SingleWaveHighScore
            return !pstmt->execute();
        }
        catch (sql::SQLException& e)
        {
            EventLog->Info("CDatabase() - Exception (%s)", e.what());
            std::printf("CDatabase() - Exception (%s)'\n", e.what());
            return false;
        }
    }

    bool CDatabase::InsertFrontAccount(const std::string& username, const std::string& password, const std::string& salt, std::uint8_t grade, std::uint64_t auth_key)
    {
        try
        {
            std::unique_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement(
                "INSERT INTO front_accounts(Username, Password, Salt, Grade, AuthKey) "
                "VALUES(?, ?, ?, ?, ?)")); 
            pstmt->setString(1, username.c_str());
            pstmt->setString(2, password.c_str());
            pstmt->setString(3, salt.c_str());
            pstmt->setByte(4, grade);
            pstmt->setUInt64(5, auth_key);
            return !pstmt->execute();
        }
        catch (sql::SQLException& e)
        {
            EventLog->Info("CDatabase() - Exception (%s)", e.what());
            std::printf("CDatabase() - Exception (%s)'\n", e.what());
            return false;
        }
    }
    bool CDatabase::InsertPlayers(
        const std::string& nickname, const std::uint32_t& level, const std::uint32_t& experience, 
        const bool& tutorial, const std::uint32_t& story, const std::uint32_t& vip_experience, 
        const std::uint32_t& max_items, const std::uint32_t& max_energy, const std::uint32_t& selected_character, std::uint32_t& outPlayerID)
    {
        try
        {
            std::unique_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement(
                "INSERT INTO players(Nickname, Level, Experience, Tutoral, Story, VIPExperience, MaximumItems, MaximumEnergy, SelectedCharacter, PlayTime, MutedUntil) "
                "VALUES(?, ?, ?, ?, ?)", sql::Statement::RETURN_GENERATED_KEYS));

            std::time_t now = std::time(nullptr);
            sql::SQLString datetime_str = sql::SQLString(std::to_string(now));

            pstmt->setString(1, nickname.c_str());
            pstmt->setUInt(2, level);
            pstmt->setUInt(3, experience);
            pstmt->setByte(4, tutorial);
            pstmt->setUInt(5, story);
            pstmt->setUInt(6, vip_experience);
            pstmt->setUInt(7, max_items);
            pstmt->setUInt(8, max_energy);
            pstmt->setUInt(9, selected_character);
            pstmt->setString(10, "FROM_UNIXTIME(" + datetime_str + ")");
            pstmt->setString(11, "FROM_UNIXTIME(" + datetime_str + ")");
            auto retn = !pstmt->execute();
            auto rs = pstmt->getGeneratedKeys();
            if (rs->next())
                outPlayerID = rs->getUInt(1);
            //pstmt->get

            return retn;
        }
        catch (sql::SQLException& e)
        {
            EventLog->Info("CDatabase() - Exception (%s)", e.what());
            std::printf("CDatabase() - Exception (%s)'\n", e.what());
            return false;
        }
    }
    bool CDatabase::GetFrontAccount(const std::string& username, FrontAccount* outFrontAccount)
    {
        try
        {
            std::unique_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement(
                "SELECT Id, Username, Password, Salt, Grade, AuthKey "
                "FROM front_accounts WHERE Username = ?"));

            pstmt->setString(1, username.c_str());

            auto result = pstmt->executeQuery();
            if (result->next())
            {
                *outFrontAccount = FrontAccount(result->getUInt("Id"),
                    result->getString("Username").c_str(),
                    result->getString("Password").c_str(),
                    result->getString("Salt").c_str(),
                    result->getByte("Grade"),
                    result->getUInt64("AuthKey"));

                return true;
            }
            else return false;
        }
        catch (sql::SQLException& e)
        {
            EventLog->Info("CDatabase() - Exception (%s)", e.what());
            std::printf("CDatabase() - Exception (%s)'\n", e.what());
            return false;
        }
    }

    std::future<bool> CDatabase::GetFrontAccountAsync(const std::string& username, FrontAccount* outFrontAccount)
    {

        return std::future<bool>();
    }

    std::string CDatabase::GetDatabaseName()
    {
        return this->database_name;
    }

    std::unique_ptr<CDatabase> Database = std::make_unique<CDatabase>();
}
