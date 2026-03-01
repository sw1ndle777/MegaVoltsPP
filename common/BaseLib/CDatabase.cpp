#include "CDatabase.h"
#include <fmt/color.h>
#include <boost_unordered.hpp>
namespace BaseLib
{
	using enum fmt::color;
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
                    AccountId int unsigned NOT NULL,
                    IsHost bool NOT NULL,
                    IsDraw bool NOT NULL,
                    IsClanMatch bool NOT NULL,
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
                    MatchEndTime bigint unsigned NOT NULL,
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
                    PRIMARY KEY(Id),
                    KEY IX_player_matchhistory_AccountId (AccountId),
                    CONSTRAINT FK_player_matchhistory_accounts_AccountId FOREIGN KEY (AccountId) REFERENCES accounts (Id) ON DELETE CASCADE)");


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
                    KEY IX_player_monthly_rewards_PlayerId (PlayerId),
                    CONSTRAINT FK_player_monthly_rewards_accounts_PlayerId FOREIGN KEY (PlayerId) REFERENCES accounts (Id) ON DELETE CASCADE)");

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
                    PRIMARY KEY (Year, Month))");

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
                    EventType enum('RoomCreated','RoomJoined','RoomLeft','RoomKicked','TeamChanged','VoteKickStarted','VoteKickAgreed','VoteKickSucceeded','VoteKickFailed') NOT NULL,
                    ServerId int unsigned NOT NULL DEFAULT 0,
                    RoomId int unsigned NOT NULL,
                    HostAid int unsigned DEFAULT NULL,
                    TeamId tinyint unsigned DEFAULT NULL,
                    NewTeamId tinyint unsigned DEFAULT NULL,
                    VoteKickReason tinyint unsigned DEFAULT NULL,
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

                CreateTable("player_gacha_pity", R"(
                    PlayerId int unsigned NOT NULL,
                    GachaId int unsigned NOT NULL,
                    LuckyPoints int unsigned NOT NULL DEFAULT 0,
                    PRIMARY KEY (PlayerId, GachaId),
                    CONSTRAINT FK_gacha_pity_accounts FOREIGN KEY (PlayerId) REFERENCES accounts (Id) ON DELETE CASCADE
                )");

                CreateTable("ac_detections", R"(
                    Id bigint unsigned NOT NULL AUTO_INCREMENT,
                    Aid int unsigned NOT NULL,
                    Ip varchar(45) NOT NULL,
                    Hwid varchar(64) NOT NULL,
                    DetectionFlag enum('MemoryManipulation','SpeedHack','WallHack','AimbotDetected','FileIntegrityFail','DebuggerDetected','InjectionDetected','HeartbeatTimeout','InvalidResponse','ProcessAnomaly','NetworkManipulation','ClientModified','UnknownFlag') NOT NULL,
                    Extra int unsigned NOT NULL DEFAULT 0,
                    ServerId int unsigned NOT NULL DEFAULT 0,
                    CreatedAt datetime NOT NULL DEFAULT current_timestamp(),
                    PRIMARY KEY (Id),
                    KEY IX_ac_detections_Aid (Aid),
                    KEY IX_ac_detections_Flag (DetectionFlag),
                    KEY IX_ac_detections_CreatedAt (CreatedAt),
                    CONSTRAINT FK_ac_detections_Aid FOREIGN KEY (Aid) REFERENCES accounts (Id) ON DELETE CASCADE)");

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

                // Create default admin account if it doesn't exist
                try
                {
                    std::unique_ptr<sql::PreparedStatement> checkAdmin(conn->prepareStatement(
                        "SELECT Id FROM accounts WHERE Username = 'admin' LIMIT 1"
                    ));
                    std::unique_ptr<sql::ResultSet> adminResult(checkAdmin->executeQuery());

                    if (!adminResult->next())
                    {
                        // Admin doesn't exist, create it
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

                        // Hash password
                        uint8_t hash[32];
                        if (Utility::HashPassword(adminPassword, salt, hash))
                        {
                            // Base64 encode salt and hash
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

    bool CDatabase::CreateTable(const std::string& table_name, const std::string& data_columns)
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
                conn->setSchema(database_name); // Re-set schema after reconnect
                DEBUGLOG(dark_cyan, "Successfully reconnected to database");
            }

            // Ensure schema is set
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

    bool CDatabase::CreateDatabase(const std::string& name)
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
            if (std::string(e.what()).find("database exists") == std::string::npos)
                DEBUGLOG(red, "exception=({})", e.what());

            return false;
        }
    }

    std::string CDatabase::GenerateQuestionMarks(size_t n)
    {
        if (n == 0) return {};
        std::string result;
        result.reserve(n * 3);
        for (size_t i = 0; i < n; i++)
            result += (i > 0) ? ", ?" : "?";
        return result;
    }
    std::string CDatabase::GenerateQuestionMarks(size_t rows, size_t cols)
    {
        if (rows == 0 || cols == 0) return {};
        auto one_row = "(" + GenerateQuestionMarks(cols) + ")";
        std::string result;
        result.reserve(rows * one_row.size());
        for (size_t i = 0; i < rows; i++)
            result += (i > 0 ? ", " : "") + one_row;
        return result;
    }
    std::string CDatabase::GenerateInTuples(size_t rows, size_t cols)
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
    std::string CDatabase::GenerateJoinedString(const std::vector<std::string>& vec, const std::string& delim)
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
    /*
    bool CDatabase::UpdateFrontAccount(const FrontAccount& front_acc)
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

            std::unique_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement(
                "UPDATE accounts SET "
                "Username=?, Password=?, Salt=?, Grade=?, PCRoom = ?, AuthKey=?, ClanId=?, ClanKills=?, ClanDeaths=?, ClanAssists=?, "
                "ClanContribution=?, ClanWins=?, ClanLoses=?, ClanDraws=?, Nickname=?, Level=?, Experience=?, Tutorial=?, "
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
            pstmt->setUInt(index++, front_acc.ClanContribution);
            pstmt->setUInt(index++, front_acc.ClanWins);
            pstmt->setUInt(index++, front_acc.ClanLoses);
            pstmt->setUInt(index++, front_acc.ClanDraws);
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
            return pstmt->executeUpdate() > 0;
        }
        catch (sql::SQLException& e)
        {
            DEBUGLOG(red, "exception: ({})", e.what());
            return false;
        }
    }
    bool CDatabase::GetInventoryItems(const uint32_t acc_id, std::vector<Item>& inv_items)
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

            std::unique_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement("SELECT * FROM player_items WHERE PlayerId = ?"));
            pstmt->setUInt(1, acc_id);
            std::unique_ptr<sql::ResultSet> result(pstmt->executeQuery());
            bool foundItems = false;
            while (result->next())
            {
                foundItems = true;
                Item newItem;
                NetEngine::Packets::Main::InventoryItemInfo newItemInfo;
                newItemInfo.serial_info.data = result->getUInt64("SerialInfo");
                newItemInfo.item_number.item_id = result->getUInt("ItemId");
                newItemInfo.expire_date = result->getUInt("ExpirationDate");
                newItemInfo.repair = result->getUInt("Repair");
                newItemInfo.energy = result->getUInt("Energy");
            #if !defined(RELEASE_1_0_3)
                newItemInfo.item_type = result->getUInt("ItemType");
                newItemInfo.is_sealed = result->getUInt("IsSealed");
                newItemInfo.seal_level = result->getUInt("SealLevel");
                newItemInfo.enhance_exp = result->getUInt("EnhanceExp");
                newItemInfo.enhance_level = result->getUInt("EnhanceLevel");
            #endif
                newItem.stock = result->getUInt("Stock");
                newItemInfo.item_number.stock = newItem.stock;
                newItem.is_equipped = result->getByte("IsEquipped");
                newItem.character_id = result->getByte("CharacterId");
                newItem.in_database = 1;
                newItem.item_info = newItemInfo;
                inv_items.push_back(newItem);
            }
            return foundItems;
        }
        catch (sql::SQLException& e)
        {
            DEBUGLOG(red, "exception: ({})", e.what());
            return false;
        }
    }
    */
    std::expected<void, DbError> CDatabase::EnsureConnected()
    {
        if (conn && conn->isValid()) return {};
        DEBUGLOG(yellow, "Reconnecting to the database...");
        conn = driver->connect(this->properties);
        const bool ok = conn && conn->isValid();
        DEBUGLOG(dark_cyan, ok ? "Successfully reconnected to database" : "Failed to (re)connect to the database");
        if(!ok) return std::unexpected(DbError{ DbError::Type::ConnectionLost, 0, {}, "No database connection" });
        return {};
    }
    std::expected<void, DbError> CDatabase::PersistCurrenciesPatches(ValidatedDbUpdates& v)
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
            auto ps = conn->prepareStatement(sql);
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
            return std::unexpected(DbError::FromSQLException(e));
		}
    }
    std::expected<void, DbError> CDatabase::PersistAccountInfoPatches(ValidatedDbUpdates& v)
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
            auto ps = conn->prepareStatement(sql);
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
            return std::unexpected(DbError::FromSQLException(e));
        }
    }
    std::expected<void, DbError> CDatabase::PersistItemDeletes(ValidatedDbUpdates& v, ResultDbUpdateInfo& out)
    {
        try
        {
            if (v.items_deleted.empty()) return {};
            std::string dsql = "DELETE FROM player_items WHERE PlayerId = ? AND SerialInfo IN (" + GenerateQuestionMarks(v.items_deleted.size()) + ")";
            auto dps = conn->prepareStatement(dsql);
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
            return std::unexpected(DbError::FromSQLException(e));
        }
    }
    std::expected<void, DbError> CDatabase::PersistItemPatches(ValidatedDbUpdates& v, ResultDbUpdateInfo& out)
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
                auto checkPs = conn->prepareStatement(checkSql);
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
            auto pps = conn->prepareStatement(psql);
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
            return std::unexpected(DbError::FromSQLException(e));
        }
    }
    std::expected<void, DbError> CDatabase::PersistItemAdds(ValidatedDbUpdates& v, ResultDbUpdateInfo& out)
    {
        try
        {
            if (v.items_added.empty()) return {};
            std::string asql =
                "INSERT INTO player_items (PlayerId, SerialInfo, ItemId, ItemType, ExpirationDate, Repair, Energy, "
                "IsSealed, SealLevel, EnhanceExp, EnhanceLevel, Stock, IsEquipped, CharacterId) "
                "VALUES " + GenerateQuestionMarks(v.items_added.size(), 14);
            auto aps = conn->prepareStatement(asql);
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
            return std::unexpected(DbError::FromSQLException(e));
        }
    }
    std::expected<void, DbError> CDatabase::PersistMissionsPatches(ValidatedDbUpdates& v)
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
            auto ps = conn->prepareStatement(sql);
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
            return std::unexpected(DbError::FromSQLException(e));
        }
    }
    std::expected<void, DbError> CDatabase::PersistMonthlyRewardsPatches(ValidatedDbUpdates& v)
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
            if (m.last_time_update.has_value()) sql += ", ?";
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
            auto ps = conn->prepareStatement(sql);
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
            return std::unexpected(DbError::FromSQLException(e));
        }
    }
    std::expected<void, DbError> CDatabase::PersistMailboxPatches(ValidatedDbUpdates& v)
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
                std::unique_ptr<sql::PreparedStatement> ps(conn->prepareStatement(q));
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
                    std::unique_ptr<sql::PreparedStatement> ps(conn->prepareStatement(q));
                    for (uint32_t i = 0; i < del_sender_ids.size(); ++i)
                        ps->setUInt(i + 1, del_sender_ids[i]);
                    
                    ps->executeUpdate();
                }
                {
                    std::string q = "UPDATE player_mailbox SET DeletedFromSender = 1 WHERE DeletedFromReceiver = 0 AND Id IN (" + GenerateQuestionMarks(del_sender_ids.size()) + ")";
                    std::unique_ptr<sql::PreparedStatement> ps(conn->prepareStatement(q));
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
                    std::unique_ptr<sql::PreparedStatement> ps(conn->prepareStatement(q));
                    for (uint32_t i = 0; i < del_receiver_ids.size(); i++)
                        ps->setUInt(i + 1, del_receiver_ids[i]);
                        
                    ps->executeUpdate();
                }
                {
                    std::string q = "UPDATE player_mailbox SET DeletedFromReceiver = 1 WHERE DeletedFromSender = 0 AND Id IN (" + GenerateQuestionMarks(del_receiver_ids.size()) + ")";
                    std::unique_ptr<sql::PreparedStatement> ps(conn->prepareStatement(q));
                    for (uint32_t i = 0; i < del_receiver_ids.size(); i++)
                        ps->setUInt(i + 1, del_receiver_ids[i]);

                    ps->executeUpdate();
                }
            }	
            // insert mails
            if (!insert_idx.empty())
            {
                static constexpr uint32_t kMailboxLimit = 100;
                std::unique_ptr<sql::PreparedStatement> nick(conn->prepareStatement("SELECT Id FROM accounts WHERE Nickname = ? LIMIT 1"));
                
                std::unique_ptr<sql::PreparedStatement> is_blocked(
                    conn->prepareStatement(
                        "SELECT EXISTS ("
                        "  SELECT 1 FROM player_socials "
                        "  WHERE Aid = ? AND TargetAid = ? AND State = 2"
                        ") AS IsBlocked"));

                std::unique_ptr<sql::PreparedStatement> count(
                    conn->prepareStatement("SELECT COUNT(*) AS MailCount "
                    "FROM player_mailbox "
                    "WHERE ReceiverId = ? AND DeletedFromReceiver = 0"));

                std::unique_ptr<sql::PreparedStatement> ins(conn->prepareStatement(
                    "INSERT INTO player_mailbox "
                    "(SenderId, SenderNickname, ReceiverId, ReceiverNickname, Date, GiftItemId, Message, IsNew, DeletedFromSender, DeletedFromReceiver) "
                    "VALUES (?, ?, ?, ?, ?, ?, ?, ?, 0, 0)"
                ));
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
                    std::unique_ptr<sql::Statement> stmt(conn->createStatement());
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
            return std::unexpected(DbError::FromSQLException(e));
        }
    }

    std::expected<void, DbError> CDatabase::PersistMatchHistoryAdds(ValidatedDbUpdates& v)
    {
        try
        {
            if (v.match_history_adds.empty()) return {};
            std::string asql =
                "INSERT INTO player_matchhistory (AccountId, IsHost, IsDraw, IsClanMatch, PlayTime, Level, Experience, "
                "Energy, MicroPoints, RoomIndex, RedScore, BlueScore, TeamId, RoomMode, RoomMap, SelectedCharacter, "
                "Kills, Deaths, Assists, Headshots, HighestKillStreak, MeleeKills, RifleKills, ShotgunKills, SniperKills, GatlingKills, "
                "BazookaKills, GrenadeKills, ZombieKills, Infections, MatchEndTime, Hair, Face, Upper, Under, Skirt, "
                "Gloves, Boots, HeadAcc, WaistAcc, BackAcc, Melee, Rifle, Shotgun, Sniper, Gatling, "
                "Bazooka, Grenade, RewardItem, IsMvp, IsEntryFragger, IsBullseye, IsSupport, IsBomba) "
                "VALUES " + GenerateQuestionMarks(v.match_history_adds.size(), 54);
            auto aps = conn->prepareStatement(asql);
            int idx = 1;
            for (const auto& match : v.match_history_adds) 
            {
                aps->setUInt(idx++, match.Aid);
                aps->setBoolean(idx++, match.IsHost);
                aps->setBoolean(idx++, match.IsDraw);
                aps->setBoolean(idx++, match.IsClanMatch);
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
                aps->setUInt64(idx++, match.MatchEndTime);
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
            return std::unexpected(DbError::FromSQLException(e));
        }
    }

    std::expected<void, DbError> CDatabase::PersistPlayerSessionsPatches(ValidatedDbUpdates& v)
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

                std::unique_ptr<sql::PreparedStatement> ps(conn->prepareStatement(q));
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

                std::unique_ptr<sql::PreparedStatement> ps(conn->prepareStatement(q));
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
            return std::unexpected(DbError::FromSQLException(e));
        }
    }

    std::expected<void, DbError> CDatabase::PersistPlayerSocialsPatches(ValidatedDbUpdates& v, ResultDbUpdateInfo& out)
    {
        try
        {
            if (v.player_social_patches.empty()) return {};

            std::unique_ptr<sql::PreparedStatement> selAidByNick(
                conn->prepareStatement("SELECT Id FROM accounts WHERE Nickname = ? LIMIT 1"));

            std::unique_ptr<sql::PreparedStatement> selSocialState(
                conn->prepareStatement("SELECT State FROM player_socials WHERE Aid = ? AND TargetAid = ? LIMIT 1"));

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
                auto ps = conn->prepareStatement(q);
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
                auto ps = conn->prepareStatement(q);
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
            return std::unexpected(DbError::FromSQLException(e));
        }
    }

    std::expected<void, DbError> CDatabase::PersistGachaPityPatches(ValidatedDbUpdates& v)
    {
        try
        {
            if (v.gacha_pity_patches.empty()) return {};

            std::unique_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement(
                "INSERT INTO player_gacha_pity (PlayerId, GachaId, LuckyPoints) VALUES (?, ?, ?) "
                "ON DUPLICATE KEY UPDATE LuckyPoints = VALUES(LuckyPoints)"));

            for (const auto& patch : v.gacha_pity_patches)
            {
                pstmt->setUInt(1, v.aid);
                pstmt->setUInt(2, patch.gacha_id);
                pstmt->setUInt(3, patch.lucky_points);
                pstmt->executeUpdate();
            }
            return {};
        }
        catch (sql::SQLException& e)
        {
            DEBUGLOG(red, "PersistGachaPityPatches sql exception: {}", e.what());
            return std::unexpected(DbError::FromSQLException(e));
        }
    }

    std::expected<void, DbError> CDatabase::UpdateAccount(ValidatedDbUpdates& v, ResultDbUpdateInfo& out)
    {
        try 
        {
            if (!EnsureConnected()) return std::unexpected(DbError{ DbError::Type::ConnectionLost, 0, {}, "Not connected" });;
            std::unique_ptr<sql::Statement> stmt(conn->createStatement());
            stmt->execute("START TRANSACTION");
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
            try { std::unique_ptr<sql::Statement> rb(conn->createStatement()); rb->execute("ROLLBACK"); } catch(...) {}
            auto err = DbError::FromSQLException(e);
            DEBUGLOG(red, "UpdateAccount SQL error: {} (code {}, state {})", err.message, err.error_code, err.sql_state);
            return std::unexpected(err);
        }
    }

    std::expected<void, DbError> CDatabase::UpdateAccounts(std::vector<ValidatedDbUpdates>& batch, std::vector<ResultDbUpdateInfo>& results)
    {
        try 
        {
            if (!EnsureConnected()) return std::unexpected(DbError{ DbError::Type::ConnectionLost, 0, {}, "Not connected" });;
            std::unique_ptr<sql::Statement> stmt(conn->createStatement());
            stmt->execute("START TRANSACTION");
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
            try { std::unique_ptr<sql::Statement> rb(conn->createStatement()); rb->execute("ROLLBACK"); } catch(...) {}
            auto err = DbError::FromSQLException(e);
            DEBUGLOG(red, "UpdateAccount SQL error: {} (code {}, state {})", err.message, err.error_code, err.sql_state);
            return std::unexpected(err);
        }
    }

    std::expected<void, DbError> CDatabase::InsertAccount(const std::string& username, const std::string& password_hash, const std::string& salt, const std::string& nickname)
    {
        try
        {
            if (auto r = EnsureConnected(); !r.has_value()) return r;

            std::unique_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement(
                "INSERT INTO accounts (Username, Password, Salt, Nickname, Level, MaximumEnergy) "
                "VALUES (?, ?, ?, ?, 0, 1000)"
            ));
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
            return std::unexpected(DbError::FromSQLException(e));
        }
    }

    std::expected<void, DbError> CDatabase::PersistChatLogs(const std::vector<ChatLogEntry>& logs)
    {
        try
        {
            if (logs.empty()) return {};

            std::string sql =
                "INSERT INTO player_chatlogs (Aid, TargetAid, ChatType, ChatLocation, ServerId, RoomId, PlazaId, ClanId, Message) VALUES "
                + GenerateQuestionMarks(logs.size(), 9);

            auto ps = conn->prepareStatement(sql);
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
            return std::unexpected(DbError::FromSQLException(e));
        }
    }

    std::expected<void, DbError> CDatabase::PersistItemLogs(const std::vector<ItemLogEntry>& logs)
    {
        try
        {
            if (logs.empty()) return {};

            std::string sql =
                "INSERT INTO player_itemlogs (Aid, RelatedAid, ActionType, ItemId, ItemType, SerialInfo, OriginType, MpDelta, RtDelta, CouponDelta, EnergyDelta, NewItemId, NewRepair) VALUES "
                + GenerateQuestionMarks(logs.size(), 13);

            auto ps = conn->prepareStatement(sql);
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
            return std::unexpected(DbError::FromSQLException(e));
        }
    }

    std::expected<void, DbError> CDatabase::PersistCurrencyLogs(const std::vector<CurrencyLogEntry>& logs)
    {
        try
        {
            if (logs.empty()) return {};

            std::string sql =
                "INSERT INTO player_currencylogs (Aid, CurrencyType, Amount, BeforeValue, AfterValue, SourceType, RelatedItemId) VALUES "
                + GenerateQuestionMarks(logs.size(), 7);

            auto ps = conn->prepareStatement(sql);
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
            return std::unexpected(DbError::FromSQLException(e));
        }
    }

    std::expected<void, DbError> CDatabase::PersistRoomLogs(const std::vector<RoomLogEntry>& logs)
    {
        try
        {
            if (logs.empty()) return {};

            std::string sql =
                "INSERT INTO player_roomlogs (Aid, TargetAid, EventType, ServerId, RoomId, HostAid, TeamId, NewTeamId, VoteKickReason) VALUES "
                + GenerateQuestionMarks(logs.size(), 9);

            auto ps = conn->prepareStatement(sql);
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
            }

            ps->executeUpdate();
            DEBUGLOG(green, "Persisted {} room logs", logs.size());
            return {};
        }
        catch (sql::SQLException& e)
        {
            DEBUGLOG(red, "PersistRoomLogs sql exception: {}", e.what());
            return std::unexpected(DbError::FromSQLException(e));
        }
    }

    std::expected<void, DbError> CDatabase::PersistAcDetectionLogs(const std::vector<AcDetectionLogEntry>& logs)
    {
        try
        {
            if (logs.empty()) return {};

            std::string sql =
                "INSERT INTO ac_detections (Aid, Ip, Hwid, DetectionFlag, Extra, ServerId) VALUES "
                + GenerateQuestionMarks(logs.size(), 6);

            auto ps = conn->prepareStatement(sql);
            int idx = 1;

            for (const auto& log : logs)
            {
                ps->setInt(idx++, log.aid);
                ps->setString(idx++, log.ip);
                ps->setString(idx++, log.hwid);
                ps->setString(idx++, AcDetection::FlagToString(log.detection_flag));
                ps->setUInt(idx++, log.extra);
                ps->setUInt(idx++, log.server_id);
            }

            ps->executeUpdate();
            DEBUGLOG(green, "Persisted {} ac detection logs", logs.size());
            return {};
        }
        catch (sql::SQLException& e)
        {
            DEBUGLOG(red, "PersistAcDetectionLogs sql exception: {}", e.what());
            return std::unexpected(DbError::FromSQLException(e));
        }
    }

    std::expected<void, DbError> CDatabase::PersistAuthHistory(const AuthHistoryLogEntry& entry)
    {
        try
        {
            auto ps = conn->prepareStatement(
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
            return std::unexpected(DbError::FromSQLException(e));
        }
    }

    std::expected<void, DbError> CDatabase::PersistLogs(const LogContext& ctx)
    {
        try
        {
            if (ctx.empty()) return {};
            if (!EnsureConnected()) return std::unexpected(DbError{ DbError::Type::ConnectionLost, 0, {}, "Not connected" });

            std::unique_ptr<sql::Statement> stmt(conn->createStatement());
            stmt->execute("START TRANSACTION");

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
            try { std::unique_ptr<sql::Statement> rb(conn->createStatement()); rb->execute("ROLLBACK"); }
            catch (...) {}
            DEBUGLOG(red, "PersistLogs SQL error: {}", e.what());
            return std::unexpected(DbError::FromSQLException(e));
        }
    }
    /*
    bool CDatabase::UpdateInventoryItems(const uint32_t acc_id, const std::vector<Item>& inv_items)
    {
        if (inv_items.empty()) return false;

        try {
            if (!conn || !conn->isValid())
            {
                DEBUGLOG(yellow, "Reconnecting to the database...");
                conn = driver->connect(this->properties);
                if (conn)
                    DEBUGLOG(dark_cyan, "Successfully reconnected to database");
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

            auto join = [](const std::vector<std::string> &vec,
                           const std::string &delim) -> std::string {
              std::string result;
              for (size_t i = 0; i < vec.size(); ++i) {
                if (i != 0)
                  result += delim;
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
            DEBUGLOG(red, "exception: ({})", e.what());
            return false;
        }
    }

    bool CDatabase::UpdateInventoryItems(const uint32_t acc_id, const std::vector<ItemUpdateCtx>& changes, std::optional<uint32_t> mp_cost)
    {
        if (changes.empty()) return false;

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
            stmt->execute("START TRANSACTION");

            try
            {
                if (mp_cost.has_value()) // deduct MicroPoints if required
                {
                    std::unique_ptr<sql::PreparedStatement> checkTransactionsStmt(conn->prepareStatement(
                        "UPDATE accounts SET MicroPoints = MicroPoints - ? "
                        "WHERE Id = ? AND MicroPoints >= ?"
                    ));
                    checkTransactionsStmt->setUInt(1, mp_cost.value());
                    checkTransactionsStmt->setUInt(2, acc_id);
                    checkTransactionsStmt->setUInt(3, mp_cost.value());

                    int affected = checkTransactionsStmt->executeUpdate();
                    if (affected == 0)
                    {
                        DEBUGLOG(red, "Insufficient MicroPoints for account {} (cost: {})", acc_id, mp_cost.value());
                        stmt->execute("ROLLBACK");
                        return false;
                    }
                }

                std::string itemIdCase, expireDateCase, repairCase, energyCase, equipCase, characterIdCase;
                std::vector<uint64_t> serials;

                for (const auto& change : changes)
                {
                    if (!change.serial_info.has_value()) continue;

                    uint64_t serial = change.serial_info->data;
                    std::string s = std::to_string(serial);
                    serials.push_back(serial);

                    if (change.new_item_id.has_value()) itemIdCase += "WHEN " + s + " THEN " + std::to_string(change.new_item_id.value()) + " ";
                    if (change.expire_date.has_value()) expireDateCase += "WHEN " + s + " THEN " + std::to_string(change.expire_date.value()) + " ";
                    if (change.repair.has_value()) repairCase += "WHEN " + s + " THEN " + std::to_string(change.repair.value()) + " ";
                    if (change.energy.has_value()) energyCase += "WHEN " + s + " THEN " + std::to_string(change.energy.value()) + " ";
                    if (change.is_equipped.has_value()) equipCase += "WHEN " + s + " THEN " + std::to_string(change.is_equipped.value()) + " ";
                    if (change.character_id.has_value()) characterIdCase += "WHEN " + s + " THEN " + std::to_string(change.character_id.value()) + " ";
                }

                if (serials.empty())
                {
					DEBUGLOG(yellow, "No serials to update in UpdateInventoryItems");
                    stmt->execute("ROLLBACK");
                    return false;
                }

                std::vector<std::string> sets;
                if (!itemIdCase.empty())
                    sets.push_back("ItemId = CASE SerialInfo " + itemIdCase + "ELSE ItemId END");

                if (!expireDateCase.empty())
                    sets.push_back("ExpirationDate = CASE SerialInfo " + expireDateCase + "ELSE ExpirationDate END");

                if (!repairCase.empty())
                    sets.push_back("Repair = CASE SerialInfo " + repairCase + "ELSE Repair END");

                if (!energyCase.empty())
                    sets.push_back("Energy = CASE SerialInfo " + energyCase + "ELSE Energy END");

                if (!equipCase.empty())
                    sets.push_back("IsEquipped = CASE SerialInfo " + equipCase + "ELSE IsEquipped END");

                if (!characterIdCase.empty())
                    sets.push_back("CharacterId = CASE SerialInfo " + characterIdCase + "ELSE CharacterId END");


                if (sets.empty())
                {
					DEBUGLOG(yellow, "No changes to apply in UpdateInventoryItems");
                    stmt->execute("ROLLBACK");
                    return false;
                }

                std::string query = "UPDATE player_items SET " + GenerateJoinedString(sets, ", ") +
                    " WHERE PlayerId = ? AND SerialInfo IN (" + GenerateQuestionMarks(serials.size()) + ")";

                std::unique_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement(query));
                pstmt->setUInt(1, acc_id);
                for (size_t i = 0; i < serials.size(); ++i)
                    pstmt->setUInt64(2 + i, serials[i]);

                int affected = pstmt->executeUpdate();
                DEBUGLOG(red, "Patch SQL: {}", query.c_str());
                if (!affected)
                {
					DEBUGLOG(yellow, "UpdateInventoryItems affected {} rows", affected);
                    stmt->execute("ROLLBACK");
                    return false;
                }

                stmt->execute("COMMIT");
            }
            catch (sql::SQLException& inner)
            {
                DEBUGLOG(red, "Inner SQL error in UpdateInventoryItems: ({})", inner.what());
                stmt->execute("ROLLBACK");
                return false;
            }
            return true;
        }
        catch (const sql::SQLException& e) 
        {
            DEBUGLOG(red, "exception: ({})", e.what());
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
                DEBUGLOG(yellow, "Reconnecting to the database...");
                conn = driver->connect(this->properties);
                if (conn)
                    DEBUGLOG(dark_cyan, "Successfully reconnected to database");
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
            DEBUGLOG(red, "exception: ({})", e.what());
            return false;
        }
    }
    
    bool CDatabase::InsertInventoryitemsMicroTransactions(const uint32_t acc_id, const std::vector<Item>& inv_items, const uint32_t mp, const uint32_t rt, const uint32_t coupons)
    {
        if (inv_items.empty()) return false;

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
            stmt->execute("START TRANSACTION");

            try
            {

                std::unique_ptr<sql::PreparedStatement> checkTransactionsStmt(conn->prepareStatement(
                    "UPDATE accounts SET "
                    "RockTokens = RockTokens - ?, "
                    "MicroPoints = MicroPoints - ?, "
                    "Coupons = Coupons - ? "
                    "WHERE Id = ? "
                    "AND RockTokens >= ? "
                    "AND MicroPoints >= ? "
                    "AND Coupons >= ?"
                ));

                checkTransactionsStmt->setUInt(1, rt);
                checkTransactionsStmt->setUInt(2, mp);
                checkTransactionsStmt->setUInt(3, coupons);
                checkTransactionsStmt->setUInt(4, acc_id);
                checkTransactionsStmt->setUInt(5, rt);
                checkTransactionsStmt->setUInt(6, mp);
                checkTransactionsStmt->setUInt(7, coupons);

                int rows = checkTransactionsStmt->executeUpdate();

                if (rows == 0) return false;


                std::string query = "INSERT INTO player_items (PlayerId, SerialInfo, ItemId, ItemType, ExpirationDate, Repair, Energy, IsSealed, SealLevel, EnhanceExp, EnhanceLevel, Stock, IsEquipped, CharacterId) VALUES ";

				query += GenerateQuestionMarks(inv_items.size(), 14);
                std::unique_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement(query));

                int paramIndex = 1;
                for (const auto& item : inv_items)
                {
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
                }
                int affected = pstmt->executeUpdate();
                if (affected != static_cast<int>(inv_items.size())) 
                {
                    stmt->execute("ROLLBACK");
                    DEBUGLOG(red, "InsertInventoryitemsMicroTransactions: Expected {} inserts but got {}", inv_items.size(), affected);
                    return false;
                }
                stmt->execute("COMMIT");
                return true;
            }
            catch (sql::SQLException& inner)
            {
                DEBUGLOG(red, "Inner SQL error in InsertInventoryitemsMicroTransactions: ({})", inner.what());
                stmt->execute("ROLLBACK");
                return false;
            }
            return true;
        }
        catch (sql::SQLException& e)
        {
            DEBUGLOG(red, "exception: ({})", e.what());
            return false;
        }
    }
    bool CDatabase::DeleteInventoryItemsAndRewardMP(const uint32_t acc_id, const std::vector<NetEngine::Packets::Main::ItemSerialInfo>& del_items, const uint32_t mp_reward)
    {
        if (del_items.empty()) return false;
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
            stmt->execute("START TRANSACTION");

            try 
            {
                const std::string delete_sql = "DELETE FROM player_items WHERE SerialInfo IN (" + GenerateQuestionMarks(del_items.size()) + ") AND PlayerId = ?";
                std::unique_ptr<sql::PreparedStatement> deleteStmt(conn->prepareStatement(delete_sql));
                size_t i = 1;
                for (const auto& serial : del_items) deleteStmt->setUInt64(i++, serial.data);
                deleteStmt->setUInt(i, acc_id);
                int deleted = deleteStmt->executeUpdate();
                if (deleted != static_cast<int>(del_items.size())) 
                {
                    stmt->execute("ROLLBACK");
                    DEBUGLOG(red, "DeleteInventoryItemsAndRewardMP: Expected {} deletions, got {}", del_items.size(), deleted);
                    return false;
                }
                if (mp_reward > 0) // if item is sellable it would reward mp
                {
                    std::unique_ptr<sql::PreparedStatement> rewardStmt(conn->prepareStatement("UPDATE accounts SET MicroPoints = MicroPoints + ? WHERE Id = ?"));
                    rewardStmt->setUInt(1, mp_reward);
                    rewardStmt->setUInt(2, acc_id);
                    int updated = rewardStmt->executeUpdate();
                    if (updated != 1) {
                        stmt->execute("ROLLBACK");
                        DEBUGLOG(red, "DeleteInventoryItemsAndRewardMP: Failed to reward {} MP to account {}", mp_reward, acc_id);
                        return false;
                    }
                }
                stmt->execute("COMMIT");
                return true;
            }
            catch (const sql::SQLException& inner) {
                stmt->execute("ROLLBACK");
                DEBUGLOG(red, "Inner SQL exception in DeleteInventoryItemsAndRewardMP: {}", inner.what());
                return false;
            }
        }
        catch (const sql::SQLException& e)
        {
            DEBUGLOG(red, "Outer SQL exception in DeleteInventoryItemsAndRewardMP: {}", e.what());
            return false;
        }
    }
    bool CDatabase::NewDeleteInventoryItems(const uint32_t& acc_id, const std::vector<NetEngine::Packets::Main::ItemSerialInfo>& del_items)
    {
        if (del_items.empty()) return false;

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
            stmt->execute("START TRANSACTION");

            try
            {
                std::unique_ptr<sql::PreparedStatement> deleteStmt(conn->prepareStatement(
                    "DELETE FROM player_items WHERE SerialInfo in (" + GenerateQuestionMarks(del_items.size()) + ") AND PlayerId = ?"
                ));
                auto last_idx = del_items.size() + 1;
                for (int i = 1; i < last_idx; i++)
                {
                    DEBUGLOG(dark_cyan, "delete item serial ({})", del_items[i - 1].data);
                    deleteStmt->setUInt64(i, del_items[i - 1].data);
                }
                deleteStmt->setUInt(last_idx, acc_id);
                auto affected = deleteStmt->executeUpdate();
                if (affected != static_cast<int32_t>(del_items.size())) {
                    stmt->execute("ROLLBACK");
                    DEBUGLOG(red,
                        "NewDeleteInventoryItems: Expected {} delets but got {}", del_items.size(), affected);
                    return false;
                }
                stmt->execute("COMMIT");
                return true;

            }
            catch (sql::SQLException& inner)
            {
                DEBUGLOG(red, "Inner SQL error: ({})", inner.what());
                stmt->execute("ROLLBACK");
                return false;
            }


            return true;
        }
        catch (sql::SQLException& e)
        {
            DEBUGLOG(red, "exception: ({})", e.what());
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
                DEBUGLOG(yellow, "Reconnecting to the database...");
                conn = driver->connect(this->properties);
                if (conn)
                    DEBUGLOG(dark_cyan, "Successfully reconnected to database");
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
            DEBUGLOG(red, "exception: ({})", e.what());
            return false;
        }
    }
    bool CDatabase::GetPlayerFriends(const int32_t& acc_id, std::vector<FriendInfo>& friends)
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

            std::unique_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement("SELECT FriendPlayerId, State, FriendNickname FROM player_friends WHERE PlayerId = ?"));
            pstmt->setInt(1, acc_id);
            std::unique_ptr<sql::ResultSet> result(pstmt->executeQuery());

            while (result->next())
                friends.push_back({ acc_id, result->getInt("FriendPlayerId"), static_cast<uint8_t>(result->getByte("State")), 0, result->getString("FriendNickname").c_str() });

            return !friends.empty();
        }
        catch (sql::SQLException& e)
        {
            DEBUGLOG(red, "exception: ({})", e.what());
            return false;
        }
    }
    bool CDatabase::GetPlayerBlockeds(const int32_t& acc_id, std::vector<BlockedInfo>& blockeds)
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

            std::unique_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement("SELECT BlockedPlayerId, BlockedNickname FROM player_ignores WHERE PlayerId = ?"));
            pstmt->setInt(1, acc_id);
            std::unique_ptr<sql::ResultSet> result(pstmt->executeQuery());

            while (result->next())
                blockeds.push_back({ acc_id ,result->getInt("BlockedPlayerId"), 0,  result->getString("BlockedNickname").c_str() });

            return !blockeds.empty();
        }
        catch (sql::SQLException& e)
        {
            DEBUGLOG(red, "exception: ({})", e.what());
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
                DEBUGLOG(yellow, "Reconnecting to the database...");
                conn = driver->connect(this->properties);
                if (conn)
                    DEBUGLOG(dark_cyan, "Successfully reconnected to database");
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
            DEBUGLOG(red, "exception: ({})", e.what());
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
                DEBUGLOG(yellow, "Reconnecting to the database...");
                conn = driver->connect(this->properties);
                if (conn)
                    DEBUGLOG(dark_cyan, "Successfully reconnected to database");
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
            DEBUGLOG(red, "exception: ({})", e.what());
            return false;
        }
    }
    bool CDatabase::UpdatePlayerFriends(const int32_t& acc_id, const uint32_t& friend_acc_id, const uint8_t& state)
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

            std::unique_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement("UPDATE player_friends SET State = ? WHERE PlayerId = ? AND FriendPlayerId = ?"));
            pstmt->setByte(1, state);
            pstmt->setInt(2, acc_id);
            pstmt->setInt(3, friend_acc_id);
            int32_t affectedRows = pstmt->executeUpdate();
            return affectedRows > 0;
        }
        catch (sql::SQLException& e)
        {
            DEBUGLOG(red, "exception: ({})", e.what());
            return false;
        }
    }
    bool CDatabase::InsertPlayerBlockeds(const std::vector<BlockedInfo>& blockeds)
    {
        if (!conn || !conn->isValid())
        {
            DEBUGLOG(yellow, "Reconnecting to the database...");
            conn = driver->connect(this->properties);
            if (conn)
                DEBUGLOG(dark_cyan, "Successfully reconnected to database");
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
            DEBUGLOG(red, "exception: ({})", e.what());
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
                DEBUGLOG(yellow, "Reconnecting to the database...");
                conn = driver->connect(this->properties);
                if (conn)
                    DEBUGLOG(dark_cyan, "Successfully reconnected to database");
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
            DEBUGLOG(red, "exception: ({})", e.what());
            return false;
        }
    }
    */

    bool CDatabase::GetMainFrontAccount(const uint64_t authKey, uint32_t server_id, FrontAccount* outFrontAccount, ClanInfo* outClanInfo, PlayerDailyMission* outDailyMission, std::vector<Item>& inv_items, std::vector<SocialInfo>& socials, std::vector<BlockedInfo>& blockeds, std::vector<FriendInfo>& friends, std::vector<MailboxInfo>& mailbox_list, std::vector<std::uint32_t>& daily_mission_random_ids, std::vector<GachaPityEntry>& gacha_pity, SystemMonthlyRewards* outMonthlyRewards, PlayerMonthlyReward* outPlayerMonthlyReward)
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
            stmt->execute("START TRANSACTION");

            try
            {

                // 1. Load account and clan
                std::unique_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement(R"(
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
            )"));



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


                std::unique_ptr<sql::PreparedStatement> socialStmt(conn->prepareStatement(
                    "SELECT s.TargetAid, s.State, a.Nickname AS TargetNickname "
                    "FROM player_socials s "
                    "JOIN accounts a ON a.Id = s.TargetAid "
                    "WHERE s.Aid = ?"));
                socialStmt->setInt(1, accId);
                std::unique_ptr<sql::ResultSet> socialRes(socialStmt->executeQuery());
                while (socialRes->next())
                    socials.push_back({ accId, socialRes->getInt("TargetAid"), static_cast<uint8_t>(socialRes->getByte("State")), socialRes->getString("TargetNickname").c_str() });

                /*
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
                */

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

                std::unique_ptr<sql::PreparedStatement> pityStmt(conn->prepareStatement("SELECT GachaId, LuckyPoints FROM player_gacha_pity WHERE PlayerId = ?"));
                pityStmt->setUInt(1, accId);
                std::unique_ptr<sql::ResultSet> pityRes(pityStmt->executeQuery());
                while (pityRes->next())
                {
                    GachaPityEntry entry;
                    entry.gacha_id = pityRes->getUInt("GachaId");
                    entry.lucky_points = pityRes->getUInt("LuckyPoints");
                    gacha_pity.push_back(entry);
                }

                if (outMonthlyRewards)
                {
                    uint32_t curYear = Utility::GetCurrentYear();
                    uint32_t curMonth = Utility::GetCurrentMonth();
                    std::unique_ptr<sql::PreparedStatement> monthlyStmt(conn->prepareStatement(
                        "SELECT * FROM system_monthly_rewards WHERE Year = ? AND Month = ?"));
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
                    std::unique_ptr<sql::PreparedStatement> playerMonthlyStmt(conn->prepareStatement(
                        "SELECT RewardCount, UNIX_TIMESTAMP(LastUpdate) AS LastUpdate FROM player_monthly_rewards WHERE PlayerId = ?"));
                    playerMonthlyStmt->setUInt(1, accId);
                    std::unique_ptr<sql::ResultSet> playerMonthlyRes(playerMonthlyStmt->executeQuery());
                    if (playerMonthlyRes->next())
                    {
                        outPlayerMonthlyReward->player_account_id = accId;
                        outPlayerMonthlyReward->day_count = playerMonthlyRes->getByte("RewardCount");
                        outPlayerMonthlyReward->last_time_update = playerMonthlyRes->getUInt64("LastUpdate");
                    }
                }

                std::unique_ptr<sql::PreparedStatement> updateOnlineStmt(conn->prepareStatement("UPDATE accounts SET ServerId = ? WHERE Id = ?"));
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
    /*
    bool CDatabase::UpdateEndMatchInfo(std::vector<EndMatchUpdateDatabaseInfo>& updates, std::vector<MatchInfoHistoryDatabaseInfo>& matchHistory)
    {
        if (updates.empty())
        {
            DEBUGLOG(yellow,
                "UpdateEndMatchInfo called with empty playerUpdates");
            return true;
        }

        try
        {
            if (!conn || !conn->isValid())
            {
                DEBUGLOG(yellow,
                    "Reconnecting to the database...");
                conn = driver->connect(this->properties);
                if (conn)
                    DEBUGLOG(dark_cyan,
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

                int paramIndex = 1;

                for (size_t fieldIndex = 0; fieldIndex < fields.size(); ++fieldIndex)
                {
                    for (const auto& player : updates)
                    {
                        pstmt->setUInt(paramIndex++, player.Id);
                        switch (fieldIndex)
                        {
                        case 0: pstmt->setUInt(paramIndex++, player.ClanKills); break;
                        case 1: pstmt->setUInt(paramIndex++, player.ClanDeaths); break;
                        case 2: pstmt->setUInt(paramIndex++, player.ClanAssists); break;
                        case 3: pstmt->setUInt(paramIndex++, player.ClanContribution); break;
                        case 4: pstmt->setUInt(paramIndex++, player.ClanWins); break;
                        case 5: pstmt->setUInt(paramIndex++, player.ClanLoses); break;
                        case 6: pstmt->setUInt(paramIndex++, player.ClanDraws); break;
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


                for (const auto& player : updates)
                    pstmt->setUInt(paramIndex++, player.Id);

               
                pstmt->executeUpdate();

                std::unique_ptr<sql::PreparedStatement> insertStmt(conn->prepareStatement(R"(INSERT INTO player_matchhistory (
                        AccountId, IsHost, IsDraw, IsClanMatch, PlayTime, Level, Experience,
                        Energy, MicroPoints, RoomIndex, RedScore, BlueScore, TeamId,
                        RoomMode, RoomMap, SelectedCharacter, Kills, Deaths, Assists,
                        Headshots, HighestKillStreak, MeleeKills, RifleKills, ShotgunKills,
                        SniperKills, GatlingKills, BazookaKills, GrenadeKills, ZombieKills,
                        Infections, MatchEndTime, Hair, Face, Upper, 
                        Under, Skirt, Gloves, Boots, HeadAcc,
                        WaistAcc, BackAcc, Melee, Rifle, Shotgun,
                        Sniper, Gatling, Bazooka, Grenade, RewardItem
                    ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, FROM_UNIXTIME(?), ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
                )"));

                for (const auto& info : matchHistory)
                {
                    int idx = 1;
                    insertStmt->setUInt(idx++, info.Aid);
                    insertStmt->setBoolean(idx++, info.IsHost);
                    insertStmt->setBoolean(idx++, info.IsDraw);
                    insertStmt->setBoolean(idx++, info.IsClanMatch);
                    insertStmt->setUInt(idx++, info.PlayTime);
                    insertStmt->setUInt(idx++, info.Level);
                    insertStmt->setUInt(idx++, info.Experience);
                    insertStmt->setUInt(idx++, info.Energy);
                    insertStmt->setUInt(idx++, info.MicroPoints);
                    insertStmt->setUInt(idx++, info.room_index);
                    insertStmt->setUInt(idx++, info.redscore);
                    insertStmt->setUInt(idx++, info.bluescore);
                    insertStmt->setUInt(idx++, info.team_id);
                    insertStmt->setUInt(idx++, info.room_mode);
                    insertStmt->setUInt(idx++, info.room_map);
                    insertStmt->setUInt(idx++, info.SelectedCharacter);
                    insertStmt->setUInt(idx++, info.Kills);
                    insertStmt->setUInt(idx++, info.Deaths);
                    insertStmt->setUInt(idx++, info.Assists);
                    insertStmt->setUInt(idx++, info.Headshots);
                    insertStmt->setUInt(idx++, info.HighestKillStreak);
                    insertStmt->setUInt(idx++, info.MeleeKills);
                    insertStmt->setUInt(idx++, info.RifleKills);
                    insertStmt->setUInt(idx++, info.ShotgunKills);
                    insertStmt->setUInt(idx++, info.SniperKills);
                    insertStmt->setUInt(idx++, info.GatlingKills);
                    insertStmt->setUInt(idx++, info.BazookaKills);
                    insertStmt->setUInt(idx++, info.GrenadeKills);
                    insertStmt->setUInt(idx++, info.ZombieKills);
                    insertStmt->setUInt(idx++, info.Infections);
                    insertStmt->setUInt64(idx++, info.MatchEndTime);
                    insertStmt->setUInt(idx++, info.Hair);
					insertStmt->setUInt(idx++, info.Face);
					insertStmt->setUInt(idx++, info.Upper);
					insertStmt->setUInt(idx++, info.Under);
					insertStmt->setUInt(idx++, info.Skirt);
					insertStmt->setUInt(idx++, info.Gloves);
					insertStmt->setUInt(idx++, info.Boots);
					insertStmt->setUInt(idx++, info.HeadAcc);
					insertStmt->setUInt(idx++, info.WaistAcc);
					insertStmt->setUInt(idx++, info.BackAcc);
					insertStmt->setUInt(idx++, info.Melee);
					insertStmt->setUInt(idx++, info.Rifle);
					insertStmt->setUInt(idx++, info.Shotgun);
					insertStmt->setUInt(idx++, info.Sniper);
					insertStmt->setUInt(idx++, info.Gatling);
					insertStmt->setUInt(idx++, info.Bazooka);
					insertStmt->setUInt(idx++, info.Grenade);
					insertStmt->setUInt(idx++, info.IsItemReward ? info.reward_item.item_info.item_number.item_id : 0);
                    insertStmt->executeUpdate();
                }

                stmt->execute("COMMIT");

                DEBUGLOG(green,
                    "Successfully updated end match info for {} players", updates.size());

                return true;
            }
            catch (sql::SQLException& inner)
            {
                stmt->execute("ROLLBACK");
                DEBUGLOG(red,
                    "Inner SQL exception in UpdateEndMatchInfo: {}", inner.what());
                return false;
            }
        }
        catch (sql::SQLException& outer)
        {
            DEBUGLOG(red,
                "Outer SQL exception in UpdateEndMatchInfo: {}", outer.what());
            return false;
        }
    }
    */
    bool CDatabase::GetFrontAccount(const std::string& ip, const std::string& username, const std::string& password, FrontAccount* outFrontAccount, ClanInfo* outClanInfo)
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

            constexpr std::uint32_t kSessionTTLSeconds = 5u * 60u; // 5mins (sliding)
            const std::uint64_t nowUnix = Utility::GetUtcTimeNow64();
            const std::uint64_t newExp = nowUnix + kSessionTTLSeconds;

            // Begin transaction
            std::unique_ptr<sql::Statement> stmt(conn->createStatement());
            stmt->execute("START TRANSACTION");

			std::unique_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement(
				"SELECT a.Id, a.ServerId, a.Username, a.Password, a.Salt, a.Grade, a.ClanId, a.Level, a.Experience, "
				"a.Kills, a.Deaths, a.Assists, a.Wins, a.Loses, a.Draws, a.Nickname, "
				"c.Id as ClanId, c.OwnerId, c.ClanName, c.ClanLogoFront, c.ClanLogoBack "
				"FROM accounts a "
				"LEFT JOIN clans c ON a.ClanId = c.Id "
				"WHERE a.Username = ?"
			));

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

           


            std::unique_ptr<sql::PreparedStatement> purge(conn->prepareStatement("DELETE FROM player_sessions WHERE ExpiresAt <= FROM_UNIXTIME(?)"));
            purge->setUInt64(1, nowUnix);
            purge->execute();



            std::unique_ptr<sql::PreparedStatement> selSess(conn->prepareStatement(
                "SELECT AuthKey, UNIX_TIMESTAMP(ExpiresAt) "
                "FROM player_sessions WHERE PlayerId = ?"
            ));
            selSess->setUInt(1, outFrontAccount->Index);
            std::unique_ptr<sql::ResultSet> rsSel(selSess->executeQuery());

            if (rsSel->next())
            {
                const std::uint64_t curKey = rsSel->getUInt64(1);
                const std::uint64_t curExpiry = rsSel->getUInt64(2);

                if (curExpiry > nowUnix)
                {
                    std::unique_ptr<sql::PreparedStatement> bump(conn->prepareStatement(
                        "UPDATE player_sessions "
                        "SET LastSeenAt = FROM_UNIXTIME(?) "
                        "WHERE PlayerId = ?"
                    ));
                    bump->setUInt64(1, nowUnix);
                    bump->setUInt(2, outFrontAccount->Index);
                    bump->executeUpdate();

                    outFrontAccount->AuthKey = curKey;
                    std::unique_ptr<sql::PreparedStatement> loginHistoryStmt(conn->prepareStatement(
                        "INSERT INTO login_history (AccountId, LoginDate, IP) VALUES (?, FROM_UNIXTIME(?), ?)"
                    ));
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
                    std::unique_ptr<sql::PreparedStatement> upsert(conn->prepareStatement(
                        "INSERT INTO player_sessions (PlayerId, AuthKey, IssuedAt, ExpiresAt, LastSeenAt) "
                        "VALUES (?, ?, FROM_UNIXTIME(?), FROM_UNIXTIME(?), FROM_UNIXTIME(?)) "
                        "ON DUPLICATE KEY UPDATE "
                        "  AuthKey = VALUES(AuthKey), "
                        "  IssuedAt = VALUES(IssuedAt), "
                        "  ExpiresAt = VALUES(ExpiresAt), "
                        "  LastSeenAt = VALUES(LastSeenAt)"
                    ));
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
                        if (msg.find("uq_authkey") != std::string::npos || msg.find("AuthKey") != std::string::npos)
                            continue;
                    }
                    stmt->execute("ROLLBACK");
                    DEBUGLOG(red, "Session upsert failed: ({})", e.what());
                    return false;
                }
            }
            outFrontAccount->AuthKey = newAuthKey;

            std::unique_ptr<sql::PreparedStatement> loginHistoryStmt(conn->prepareStatement(
                "INSERT INTO login_history (AccountId, LoginDate, IP) VALUES (?, FROM_UNIXTIME(?), ?)"
            ));
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
	bool CDatabase::GetFrontAccount(const uint64_t authKey, FrontAccount *outFrontAccount, ClanInfo *outClanInfo)
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

            const std::uint64_t nowUnix = Utility::GetUtcTimeNow64();


			std::unique_ptr<sql::Statement> stmt(conn->createStatement());
			stmt->execute("START TRANSACTION");

            std::unique_ptr<sql::PreparedStatement> purge(conn->prepareStatement("DELETE FROM player_sessions WHERE ExpiresAt <= FROM_UNIXTIME(?)"));
            purge->setUInt64(1, nowUnix);
            purge->execute();


			std::unique_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement(
				"SELECT a.Id, a.ServerId, a.Username, a.Password, a.Salt, a.Grade, a.ClanId, a.Level, a.Experience, "
				"a.Kills, a.Deaths, a.Assists, a.Wins, a.Loses, a.Draws, a.Nickname, "
				"c.Id as ClanId, c.OwnerId, c.ClanName, c.ClanLogoFront, c.ClanLogoBack "
                "FROM player_sessions s "
				"JOIN accounts a ON s.PlayerId = a.Id "
				"LEFT JOIN clans c ON a.ClanId = c.Id "
				"WHERE s.AuthKey = ?"
			));

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


            std::unique_ptr<sql::PreparedStatement> bump(
                conn->prepareStatement("UPDATE player_sessions SET LastSeenAt = FROM_UNIXTIME(?) WHERE AuthKey = ?"));
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

    bool CDatabase::GetPlazaAuthKey(const std::string& ip, const std::string& username, const std::string& password, PlazaAuth* outPlazaAuth)
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

            constexpr std::uint32_t kSessionTTLSeconds = 5u * 60u; // 5mins (sliding)
            const std::uint64_t nowUnix = Utility::GetUtcTimeNow64();
            const std::uint64_t newExp = nowUnix + kSessionTTLSeconds;

            // Begin transaction
            std::unique_ptr<sql::Statement> stmt(conn->createStatement());
            stmt->execute("START TRANSACTION");

            std::unique_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement(
                "SELECT Id, ServerId, Password, Salt, Grade, IsEmailVerified, TwoFactorSecret, TwoFactorEnabled "
                "FROM accounts "
                "WHERE Username = ?"
            ));

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

            std::unique_ptr<sql::PreparedStatement> purge(conn->prepareStatement("DELETE FROM player_sessions WHERE ExpiresAt <= FROM_UNIXTIME(?)"));
            purge->setUInt64(1, nowUnix);
            purge->execute();



            std::unique_ptr<sql::PreparedStatement> selSess(conn->prepareStatement(
                "SELECT AuthKey, UNIX_TIMESTAMP(ExpiresAt) "
                "FROM player_sessions WHERE PlayerId = ?"
            ));
            selSess->setUInt(1, outPlazaAuth->Index);
            std::unique_ptr<sql::ResultSet> rsSel(selSess->executeQuery());

            if (rsSel->next())
            {
                const std::uint64_t curKey = rsSel->getUInt64(1);
                const std::uint64_t curExpiry = rsSel->getUInt64(2);

                if (curExpiry > nowUnix)
                {
                    std::unique_ptr<sql::PreparedStatement> bump(conn->prepareStatement(
                        "UPDATE player_sessions "
                        "SET LastSeenAt = FROM_UNIXTIME(?) "
                        "WHERE PlayerId = ?"
                    ));
                    bump->setUInt64(1, nowUnix);
                    bump->setUInt(2, outPlazaAuth->Index);
                    bump->executeUpdate();

                    outPlazaAuth->AuthKey = curKey;
                    std::unique_ptr<sql::PreparedStatement> loginHistoryStmt(conn->prepareStatement(
                        "INSERT INTO login_history (AccountId, LoginDate, IP) VALUES (?, FROM_UNIXTIME(?), ?)"
                    ));
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
                    std::unique_ptr<sql::PreparedStatement> upsert(conn->prepareStatement(
                        "INSERT INTO player_sessions (PlayerId, AuthKey, IssuedAt, ExpiresAt, LastSeenAt) "
                        "VALUES (?, ?, FROM_UNIXTIME(?), FROM_UNIXTIME(?), FROM_UNIXTIME(?)) "
                        "ON DUPLICATE KEY UPDATE "
                        "  AuthKey = VALUES(AuthKey), "
                        "  IssuedAt = VALUES(IssuedAt), "
                        "  ExpiresAt = VALUES(ExpiresAt), "
                        "  LastSeenAt = VALUES(LastSeenAt)"
                    ));
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
                        if (msg.find("uq_authkey") != std::string::npos || msg.find("AuthKey") != std::string::npos)
                            continue;
                    }
                    stmt->execute("ROLLBACK");
                    DEBUGLOG(red, "Session upsert failed: ({})", e.what());
                    return false;
                }
            }
            outPlazaAuth->AuthKey = newAuthKey;

            std::unique_ptr<sql::PreparedStatement> loginHistoryStmt(conn->prepareStatement(
                "INSERT INTO login_history (AccountId, LoginDate, IP) VALUES (?, FROM_UNIXTIME(?), ?)"
            ));
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

    bool CDatabase::GetPlazaAuthKey(const std::string& ip, const uint64_t authKey, PlazaAuth* outPlazaAuth)
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

            constexpr std::uint32_t kSessionTTLSeconds = 5u * 60u; // 5mins (sliding)
            const std::uint64_t nowUnix = Utility::GetUtcTimeNow64();
            const std::uint64_t newExp = nowUnix + kSessionTTLSeconds;

            // Begin transaction
            std::unique_ptr<sql::Statement> stmt(conn->createStatement());
            stmt->execute("START TRANSACTION");

            std::unique_ptr<sql::PreparedStatement> purge(conn->prepareStatement("DELETE FROM player_sessions WHERE ExpiresAt <= FROM_UNIXTIME(?)"));
            purge->setUInt64(1, nowUnix);
            purge->execute();


            std::unique_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement(
                "SELECT a.Id, a.ServerId, a.Grade, a.IsEmailVerified, a.TwoFactorEnabled, a.TwoFactorSecret "
                "FROM player_sessions s "
                "JOIN accounts a ON s.PlayerId = a.Id "
                "WHERE s.AuthKey = ?"
            ));

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


            std::unique_ptr<sql::PreparedStatement> bump(conn->prepareStatement("UPDATE player_sessions SET LastSeenAt = FROM_UNIXTIME(?) WHERE AuthKey = ?"));
            bump->setUInt64(1, nowUnix);
            bump->setUInt64(2, authKey);
            bump->executeUpdate();


            std::unique_ptr<sql::PreparedStatement> loginHistoryStmt(conn->prepareStatement(
                "INSERT INTO login_history (AccountId, LoginDate, IP) VALUES (?, FROM_UNIXTIME(?), ?)"
            ));
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
    

    /*
    bool CDatabase::NicknameExists(const std::string_view& nickname)
    {
        try
        {
            // Ensure connection is valid
            if (!conn || !conn->isValid())
            {
                DEBUGLOG(yellow,
                    "Reconnecting to the database...");
                conn = driver->connect(this->properties);
                if (conn)
                {
                    DEBUGLOG(dark_cyan,
                        "Successfully reconnected to database");
                }
            }

            // Start transaction
            std::unique_ptr<sql::Statement> stmt(conn->createStatement());
            stmt->execute("START TRANSACTION");

            // Prepare and execute query
            std::unique_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement(
                "SELECT Id FROM accounts WHERE Nickname = ? LIMIT 1"
            ));
            pstmt->setString(1, std::string(nickname));

            std::unique_ptr<sql::ResultSet> result(pstmt->executeQuery());

            bool exists = result->next();

            // Commit read-only transaction
            stmt->execute("COMMIT");

            return exists;
        }
        catch (const sql::SQLException& e)
        {
            DEBUGLOG(red,
                "SQL exception: {}", e.what());

            // Attempt rollback if connection is still valid
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


    bool CDatabase::NicknameExists(const std::string_view& nickname, uint32_t& account_id)
    {
        try
        {
            // Ensure the connection is valid
            if (!conn || !conn->isValid())
            {
                DEBUGLOG(yellow,
                    "Reconnecting to the database...");
                conn = driver->connect(this->properties);
                if (conn)
                {
                    DEBUGLOG(dark_cyan,
                        "Successfully reconnected to database");
                }
            }

            // Start transaction
            std::unique_ptr<sql::Statement> stmt(conn->createStatement());
            stmt->execute("START TRANSACTION");

            // Prepare and execute the SELECT statement
            std::unique_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement(
                "SELECT Id FROM accounts WHERE Nickname = ? LIMIT 1"
            ));
            pstmt->setString(1, std::string(nickname)); // Convert string_view safely

            std::unique_ptr<sql::ResultSet> result(pstmt->executeQuery());

            bool found = false;

            if (result->next())
            {
                account_id = result->getUInt("Id");
                found = true;
            }

            // Commit the transaction
            stmt->execute("COMMIT");
            return found;
        }
        catch (const sql::SQLException& e)
        {
            DEBUGLOG(red,
                "SQL exception: {}", e.what());

            // Attempt rollback on error
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
                DEBUGLOG(yellow, "Reconnecting to the database...");
                conn = driver->connect(this->properties);
                if (conn)
                    DEBUGLOG(dark_cyan, "Successfully reconnected to database");
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
            DEBUGLOG(red, "exception: ({})", e.what());
            return false;
        }
    }
    bool CDatabase::GetClanInfo(const uint32_t& clanId, ClanInfo* outClanInfo)
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
            DEBUGLOG(red, "exception: ({})", e.what());

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
            // Validate or reconnect to the database
            if (!conn || !conn->isValid())
            {
                DEBUGLOG(yellow,
                    "Reconnecting to the database...");
                conn = driver->connect(this->properties);
                if (conn)
                {
                    DEBUGLOG(dark_cyan,
                        "Successfully reconnected to database");
                }
            }

            // Start transaction
            std::unique_ptr<sql::Statement> stmt(conn->createStatement());
            stmt->execute("START TRANSACTION");

            // Prepare the update statement
            std::unique_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement(
                "UPDATE clans SET OwnerId = ?, ClanName = ?, ClanLogoFront = ?, ClanLogoBack = ? WHERE Id = ?"
            ));

            int index = 1;
            pstmt->setUInt(index++, owner_id);
            pstmt->setString(index++, name);
            pstmt->setUInt(index++, logo_front);
            pstmt->setUInt(index++, logo_back);
            pstmt->setUInt(index++, clan_id);

            int updateCount = pstmt->executeUpdate();

            if (updateCount > 0)
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

            // Attempt rollback if transaction failed
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


    bool CDatabase::InsertPlayerMailbox(const MailboxInfo& mailbox_info, uint32_t& out_mail_id)
    {
        try
        {
            // Ensure connection is valid
            if (!conn || !conn->isValid())
            {
                DEBUGLOG(yellow,
                    "Reconnecting to the database...");
                conn = driver->connect(this->properties);
                if (conn)
                {
                    DEBUGLOG(dark_cyan,
                        "Successfully reconnected to database");
                }
            }

            // Begin transaction
            std::unique_ptr<sql::Statement> stmt(conn->createStatement());
            stmt->execute("START TRANSACTION");

            // Insert mailbox entry
            std::unique_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement(
                "INSERT INTO player_mailbox (SenderId, SenderNickname, ReceiverId, ReceiverNickname, Date, GiftItemId, Message, IsNew, DeletedFromSender, DeletedFromReceiver) "
                "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)"
            ));

            int index = 1;
            pstmt->setUInt(index++, mailbox_info.sender_account_id);
            pstmt->setString(index++, mailbox_info.sender_nickname);
            pstmt->setUInt(index++, mailbox_info.receiver_account_id);
            pstmt->setString(index++, mailbox_info.receiver_nickname);
            pstmt->setUInt(index++, mailbox_info.time);
            pstmt->setUInt(index++, mailbox_info.gift_itemid);
            pstmt->setString(index++, mailbox_info.message);
            pstmt->setByte(index++, mailbox_info.is_new);
            pstmt->setByte(index++, mailbox_info.deleted_from_sender);
            pstmt->setByte(index++, mailbox_info.deleted_from_receiver);

            pstmt->executeUpdate();

            // Get inserted ID
            std::unique_ptr<sql::ResultSet> res(stmt->executeQuery("SELECT LAST_INSERT_ID()"));
            if (res->next())
            {
                out_mail_id = res->getUInt(1);
                stmt->execute("COMMIT");
                return true;
            }

            stmt->execute("ROLLBACK");
            return false;
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

            return false;
        }
    }

    bool CDatabase::DeletePlayerMailbox(const std::vector<MailboxInfo>& mails)
    {
        if (mails.empty())
            return false;

        try
        {
            // Ensure database connection is valid
            if (!conn || !conn->isValid())
            {
                DEBUGLOG(yellow,
                    "Reconnecting to the database...");
                conn = driver->connect(this->properties);
                if (conn)
                {
                    DEBUGLOG(dark_cyan,
                        "Successfully reconnected to database");
                }
            }

            // Begin transaction
            std::unique_ptr<sql::Statement> stmt(conn->createStatement());
            stmt->execute("START TRANSACTION");

            // Construct query: DELETE FROM player_mailbox WHERE Id IN (?, ?, ?, ...)
            std::string query = "DELETE FROM player_mailbox WHERE Id IN (";
            for (size_t i = 0; i < mails.size(); ++i)
            {
                query += (i > 0 ? ", ?" : "?");
            }
            query += ")";

            std::unique_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement(query));

            int paramIndex = 1;
            for (const auto& mail_info : mails)
            {
                pstmt->setUInt(paramIndex++, mail_info.mail_id);
            }

            int affectedRows = pstmt->executeUpdate();

            if (affectedRows > 0)
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

            // Rollback on error
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


    std::vector<MailboxInfo> CDatabase::GetPlayerMailbox(const int32_t& acc_id)
    {
        std::vector<MailboxInfo> mailbox_list;

        try
        {
            // Ensure database connection is valid
            if (!conn || !conn->isValid())
            {
                DEBUGLOG(yellow,
                    "Reconnecting to the database...");
                conn = driver->connect(this->properties);
                if (conn)
                {
                    DEBUGLOG(dark_cyan,
                        "Successfully reconnected to database");
                }
            }

            // Start transaction
            std::unique_ptr<sql::Statement> stmt(conn->createStatement());
            stmt->execute("START TRANSACTION");

            // Prepare the SELECT statement
            std::unique_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement(
                "SELECT Id, SenderId, SenderNickname, ReceiverId, ReceiverNickname, Date, GiftItemId, Message, IsNew, DeletedFromSender, DeletedFromReceiver "
                "FROM player_mailbox WHERE SenderId = ? OR ReceiverId = ?"
            ));

            pstmt->setUInt(1, acc_id);
            pstmt->setUInt(2, acc_id);

            std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());

            while (res->next())
            {
                
                std::string senderNick = static_cast<std::string>(res->getString("SenderNickname"));
                std::string receiverNick = static_cast<std::string>(res->getString("ReceiverNickname"));
                std::string message = static_cast<std::string>(res->getString("Message"));

                MailboxInfo mailbox_info(
                    res->getUInt("Id"),
                    res->getUInt("SenderId"),
                    senderNick,
                    res->getUInt("ReceiverId"),
                    receiverNick,
                    res->getUInt("Date"),
                    res->getUInt("GiftItemId"),
                    message,
                    static_cast<bool>(res->getByte("IsNew")),
                    static_cast<bool>(res->getByte("DeletedFromSender")),
                    static_cast<bool>(res->getByte("DeletedFromReceiver"))
                );

                mailbox_list.push_back(std::move(mailbox_info));
            }

            stmt->execute("COMMIT");
        }
        catch (const sql::SQLException& e)
        {
            DEBUGLOG(red,
                "SQL exception: {}", e.what());

            // Rollback on error
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

        return mailbox_list;
    }


    uint32_t CDatabase::GetPlayerReceiverMailboxCount(const int32_t& acc_id)
    {
        try
        {
            // Validate or reconnect to the database
            if (!conn || !conn->isValid())
            {
                DEBUGLOG(yellow,
                    "Reconnecting to the database...");
                conn = driver->connect(this->properties);
                if (conn)
                {
                    DEBUGLOG(dark_cyan,
                        "Successfully reconnected to database");
                }
            }

            // Begin transaction
            std::unique_ptr<sql::Statement> stmt(conn->createStatement());
            stmt->execute("START TRANSACTION");

            std::unique_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement(
                "SELECT COUNT(*) AS MailCount FROM player_mailbox WHERE ReceiverId = ? AND GiftItemId = 0"
            ));
            pstmt->setUInt(1, acc_id);

            std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());

            uint32_t count = 0;
            if (res->next())
            {
                count = res->getUInt("MailCount");
            }

            stmt->execute("COMMIT");
            return count;
        }
        catch (const sql::SQLException& e)
        {
            DEBUGLOG(red,
                "SQL exception: {}", e.what());

            // Rollback if failed
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

            return 0;
        }
    }

    uint32_t CDatabase::GetPlayerReceiverGiftboxCount(const int32_t& acc_id)
    {
        try
        {
            // Reconnect if needed
            if (!conn || !conn->isValid())
            {
                DEBUGLOG(yellow,
                    "Reconnecting to the database...");
                conn = driver->connect(this->properties);
                if (conn)
                {
                    DEBUGLOG(dark_cyan,
                        "Successfully reconnected to database");
                }
            }

            // Start transaction
            std::unique_ptr<sql::Statement> stmt(conn->createStatement());
            stmt->execute("START TRANSACTION");

            // Prepare and execute count query
            std::unique_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement(
                "SELECT COUNT(*) AS MailCount FROM player_mailbox WHERE ReceiverId = ? AND GiftItemId != 0"
            ));
            pstmt->setUInt(1, acc_id);

            std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());

            uint32_t count = 0;
            if (res->next())
            {
                count = res->getUInt("MailCount");
            }

            stmt->execute("COMMIT");
            return count;
        }
        catch (const sql::SQLException& e)
        {
            DEBUGLOG(red,
                "SQL exception: {}", e.what());

            // Attempt rollback
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

            return 0;
        }
    }

    bool CDatabase::UpdateMailboxIsNew(const std::vector<uint32_t>& mail_ids, bool is_new)
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

            if (mail_ids.empty())
            {
                DEBUGLOG(yellow, "No mail IDs provided for updating IsNew.");
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
                DEBUGLOG(dark_cyan, "Updated IsNew for {} rows", affected_rows);
                return true;
            }

            conn->rollback(); // Rollback in case no rows were updated
            conn->setAutoCommit(true);
        }
        catch (sql::SQLException& e)
        {
            DEBUGLOG(red, "SQL exception: {}", e.what());
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
                DEBUGLOG(yellow, "Reconnecting to the database...");
                conn = driver->connect(this->properties);
                if (conn)
                    DEBUGLOG(dark_cyan, "Successfully reconnected to database");
            }

            if (mail_ids.empty())
            {
                DEBUGLOG(yellow, "No mail IDs provided for updating or deleting.");
                return false;
            }

            conn->setAutoCommit(false);

            
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

			DEBUGLOG(dark_cyan, "Deleted {} rows and updated {} rows.", deleted_rows, updated_rows);

            return true;
        }
        catch (sql::SQLException& e)
        {
            DEBUGLOG(red, "SQL exception: {}", e.what());
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
                DEBUGLOG(yellow, "Reconnecting to the database...");
                conn = driver->connect(this->properties);
                if (conn)
                    DEBUGLOG(dark_cyan, "Successfully reconnected to database");
            }

            if (mail_ids.empty())
            {
                DEBUGLOG(yellow, "No mail IDs provided for updating or deleting.");
                return false;
            }

            conn->setAutoCommit(false);

            
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

            conn->commit(); 
            conn->setAutoCommit(true); 

			DEBUGLOG(dark_cyan, "Deleted {} rows and updated {} rows.", deleted_rows, updated_rows);

            return true;
        }
        catch (sql::SQLException& e)
        {
            DEBUGLOG(red, "SQL exception: {}", e.what());
            if (conn)
            {
                try
                {
                    conn->rollback(); 
                    conn->setAutoCommit(true); 
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
                DEBUGLOG(yellow,
                    "Reconnecting to the database...");
                conn = driver->connect(this->properties);
                if (conn)
                {
                    DEBUGLOG(dark_cyan,
                        "Successfully reconnected to database");
                }
            }
            std::unique_ptr<sql::Statement> stmt(conn->createStatement());
            stmt->execute("START TRANSACTION");

            std::unique_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement(
                "SELECT * FROM system_monthly_rewards WHERE Month = ?"
            ));
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
            DEBUGLOG(red, "SQL exception: {}", e.what());

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


    bool CDatabase::GetPlayerMonthlyDayCount(const uint32_t& acc_id, PlayerMonthlyReward* outMonthlyRewards)
    {
        try
        {
            if (!conn || !conn->isValid())
            {
                DEBUGLOG(yellow,
                    "Reconnecting to the database...");
                conn = driver->connect(this->properties);
                if (conn)
                {
                    DEBUGLOG(dark_cyan,
                        "Successfully reconnected to database");
                }
            }

            // Start transaction
            std::unique_ptr<sql::Statement> stmt(conn->createStatement());
            stmt->execute("START TRANSACTION");

            std::unique_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement(
                "SELECT * FROM player_monthly_rewards WHERE PlayerId = ?"
            ));
            pstmt->setUInt(1, acc_id);

            std::unique_ptr<sql::ResultSet> result(pstmt->executeQuery());

            if (result->next())
            {
                *outMonthlyRewards = PlayerMonthlyReward(
                    acc_id,
                    result->getByte("RewardCount"),
                    result->getUInt64("LastUpdate")
                );

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
            DEBUGLOG(red, "SQL exception: {}", e.what());
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
                DEBUGLOG(red, "Rollback failed: {}", rollbackEx.what());
            }

            return false;
        }
    }


    bool CDatabase::InsertPlayerMonthlyDayCount(const uint32_t& acc_id, const uint8_t& reward_count, const uint64_t& last_update)
    {
        try
        {
            // Ensure connection is valid
            if (!conn || !conn->isValid())
            {
                DEBUGLOG(yellow,
                    "Reconnecting to the database...");
                conn = driver->connect(this->properties);
                if (conn)
                {
                    DEBUGLOG(dark_cyan,
                        "Successfully reconnected to database");
                }
            }

            // Begin transaction
            std::unique_ptr<sql::Statement> stmt(conn->createStatement());
            stmt->execute("START TRANSACTION");

            // Insert player reward data
            std::unique_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement(
                "INSERT INTO player_monthly_rewards (PlayerId, RewardCount, LastUpdate) VALUES (?, ?, ?)"
            ));
            pstmt->setUInt(1, acc_id);
            pstmt->setUInt(2, reward_count);
            pstmt->setUInt64(3, last_update);

            pstmt->executeUpdate();

            stmt->execute("COMMIT");
            return true;
        }
        catch (const sql::SQLException& e)
        {
            DEBUGLOG(red,
                "SQL exception: {}", e.what());

            // Rollback if transaction fails
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


    bool CDatabase::UpdatePlayerMonthlyDayCount(const uint32_t& acc_id, const uint8_t& reward_count, const uint64_t& last_update)
    {
        try
        {
            // Ensure database connection is valid
            if (!conn || !conn->isValid())
            {
                DEBUGLOG(yellow,
                    "Reconnecting to the database...");
                conn = driver->connect(this->properties);
                if (conn)
                {
                    DEBUGLOG(dark_cyan,
                        "Successfully reconnected to database");
                }
            }

            // Begin transaction
            std::unique_ptr<sql::Statement> stmt(conn->createStatement());
            stmt->execute("START TRANSACTION");

            std::unique_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement(
                "UPDATE player_monthly_rewards SET RewardCount = ?, LastUpdate = ? WHERE PlayerId = ?"
            ));
            pstmt->setUInt(1, reward_count);
            pstmt->setUInt64(2, last_update);
            pstmt->setUInt(3, acc_id);

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

            return false;
        }
    }


    bool CDatabase::GetPlayerDailyMission(const int32_t& acc_id, PlayerDailyMission* outDailyMission)
    {
        try
        {
            // Ensure database connection is valid
            if (!conn || !conn->isValid())
            {
                DEBUGLOG(yellow,
                    "Reconnecting to the database...");
                conn = driver->connect(this->properties);
                if (conn)
                {
                    DEBUGLOG(dark_cyan,
                        "Successfully reconnected to database");
                }
            }

            // Begin transaction
            std::unique_ptr<sql::Statement> stmt(conn->createStatement());
            stmt->execute("START TRANSACTION");

            std::unique_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement(
                "SELECT * FROM player_daily_mission WHERE PlayerId = ?"
            ));
            pstmt->setUInt(1, acc_id);

            std::unique_ptr<sql::ResultSet> result(pstmt->executeQuery());

            if (result->next())
            {
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

            // Rollback on error
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


    bool CDatabase::InsertPlayerDailyMission(const uint32_t& acc_id, const PlayerDailyMission& dailyMission)
    {
        try
        {
            // Ensure connection is valid
            if (!conn || !conn->isValid())
            {
                DEBUGLOG(yellow,
                    "Reconnecting to the database...");
                conn = driver->connect(this->properties);
                if (conn)
                {
                    DEBUGLOG(dark_cyan,
                        "Successfully reconnected to database");
                }
            }

            // Begin transaction
            std::unique_ptr<sql::Statement> stmt(conn->createStatement());
            stmt->execute("START TRANSACTION");

            // Insert daily mission data
            std::unique_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement(
                "INSERT INTO player_daily_mission "
                "(PlayerId, UpdateTime, Mission1, Mission2, Mission3, GoalMission1, GoalMission2, GoalMission3) "
                "VALUES (?, ?, ?, ?, ?, ?, ?, ?)"
            ));

            pstmt->setUInt(1, acc_id);
            pstmt->setUInt64(2, dailyMission.update_time);
            pstmt->setUInt(3, dailyMission.mission1);
            pstmt->setUInt(4, dailyMission.mission2);
            pstmt->setUInt(5, dailyMission.mission3);
            pstmt->setUInt(6, dailyMission.goal_mission1);
            pstmt->setUInt(7, dailyMission.goal_mission2);
            pstmt->setUInt(8, dailyMission.goal_mission3);

            pstmt->executeUpdate();

            stmt->execute("COMMIT");
            return true;
        }
        catch (const sql::SQLException& e)
        {
            DEBUGLOG(red,
                "SQL exception: {}", e.what());

            // Rollback on error
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


    bool CDatabase::UpdatePlayerDailyMission(const uint32_t& acc_id, const PlayerDailyMission& dailyMission)
    {
        try
        {
            // Ensure database connection is valid
            if (!conn || !conn->isValid())
            {
                DEBUGLOG(yellow,
                    "Reconnecting to the database...");
                conn = driver->connect(this->properties);
                if (conn)
                {
                    DEBUGLOG(dark_cyan,
                        "Successfully reconnected to database");
                }
            }

            // Start transaction
            std::unique_ptr<sql::Statement> stmt(conn->createStatement());
            stmt->execute("START TRANSACTION");

            std::unique_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement(
                "UPDATE player_daily_mission SET "
                "UpdateTime = ?, Mission1 = ?, Mission2 = ?, Mission3 = ?, "
                "GoalMission1 = ?, GoalMission2 = ?, GoalMission3 = ? "
                "WHERE PlayerId = ?"
            ));

            pstmt->setUInt64(1, dailyMission.update_time);
            pstmt->setUInt(2, dailyMission.mission1);
            pstmt->setUInt(3, dailyMission.mission2);
            pstmt->setUInt(4, dailyMission.mission3);
            pstmt->setUInt(5, dailyMission.goal_mission1);
            pstmt->setUInt(6, dailyMission.goal_mission2);
            pstmt->setUInt(7, dailyMission.goal_mission3);
            pstmt->setUInt(8, acc_id);

            int affectedRows = pstmt->executeUpdate();

            if (affectedRows > 0)
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

            // Rollback in case of failure
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
    */

    std::vector<GachaponSaleInfo> CDatabase::GetGachaponSalesInfo()
    {
        std::vector<GachaponSaleInfo> sales;

        try
        {
            // Ensure connection is valid
            if (!conn || !conn->isValid())
            {
                DEBUGLOG(yellow,
                    "Reconnecting to the database...");
                conn = driver->connect(this->properties);
                if (conn)
                {
                    DEBUGLOG(dark_cyan,
                        "Successfully reconnected to database");
                }
            }

            // Begin transaction
            std::unique_ptr<sql::Statement> stmt(conn->createStatement());
            stmt->execute("START TRANSACTION");

            std::unique_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement(
                "SELECT GachaponId, SalePrice, "
                "UNIX_TIMESTAMP(EventStartDate) AS EventStartTimestamp, "
                "UNIX_TIMESTAMP(EventEndDate) AS EventEndTimestamp "
                "FROM system_gachapon_machine"
            ));

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


    bool CDatabase::DeleteGachaponSaleInfo(const uint32_t& gachapon_id)
    {
        try
        {
            // Ensure database connection is valid
            if (!conn || !conn->isValid())
            {
                DEBUGLOG(yellow,
                    "Reconnecting to the database...");
                conn = driver->connect(this->properties);
                if (conn)
                {
                    DEBUGLOG(dark_cyan,
                        "Successfully reconnected to database");
                }
            }

            // Begin transaction
            std::unique_ptr<sql::Statement> stmt(conn->createStatement());
            stmt->execute("START TRANSACTION");

            std::unique_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement(
                "DELETE FROM system_gachapon_machine WHERE GachaponId = ?"
            ));
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

    std::unique_ptr<CDatabase> Database = std::make_unique<CDatabase>();
}
