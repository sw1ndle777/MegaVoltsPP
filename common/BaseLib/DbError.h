#pragma once
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>
#include <array>
#include <optional>
#include <expected>
#include <variant>
#include <NetEngine/Packets/PacketData.h>
#include <BaseLib/CLogging.h>
#include <BaseLib/Platform.h>

namespace BaseLib
{
#pragma pack(push, 1)
    union ItemUpdateInfo
    {
        struct
        {
            uint32_t id : 1;
            uint32_t expire_date : 1;
            uint32_t repair : 1;
            uint32_t energy : 1;
            uint32_t is_sealed : 1;
            uint32_t stock : 1;
            uint32_t is_equipped : 1;
            uint32_t character_id : 1;
        };
        uint8_t data;

        ItemUpdateInfo(uint8_t data = 0)
        {
            std::memset(this, 0, sizeof(ItemUpdateInfo));
            this->data = data;
        }
    };
#pragma pack(pop)
    struct PlazaAuth
    {
        int32_t Index;
        uint32_t ServerId;
        uint8_t Grade;
        bool emailVerified;
        bool has2fa;
        std::string secret2fa;
        uint64_t AuthKey{ 0 };
        bool isVerified2fa;

    };
    struct AccountPenaltyInfo
    {
        int32_t account_id{ 0 };
        uint64_t until_unix{ 0 };
        std::string reason{};
    };
    struct FrontAccount
    {
        int32_t Index;
        uint32_t ServerId;
        std::string Username;
        std::string Password;
        std::string Salt;
        uint8_t Grade;
        uint8_t PCRoom;
        uint64_t AuthKey{ 0 };
        uint32_t ClanId;
        uint32_t ClanKills;
        uint32_t ClanDeaths;
        uint32_t ClanAssists;
        uint32_t ClanContribution;
        uint32_t ClanWins;
        uint32_t ClanLoses;
        uint32_t ClanDraws;
        std::string Nickname;
        uint32_t Level;
        uint32_t Experience;
        bool Tutorial;
        uint32_t Story;
        uint8_t GuideMission;
        uint64_t Achievement;
        uint64_t VoiceType;
        uint32_t VIPExperience;
        uint32_t MaximumItems;
        uint32_t MaximumEnergy;
        uint32_t SelectedCharacter;
        uint64_t PlayTime;
        uint64_t MutedUntil;
        uint32_t Coins;
        uint32_t Energy;
        uint32_t LuckyPoints;
        uint32_t MicroPoints;
        uint32_t RockTokens;
        uint32_t Coupons;
        uint32_t Wins;
        uint32_t Loses;
        uint32_t Draws;
        uint32_t Kills;
        uint32_t Deaths;
        uint32_t Assists;
        uint32_t Headshots;
        uint32_t HighestKillStreak;
        uint32_t MeleeKills;
        uint32_t RifleKills;
        uint32_t ShotgunKills;
        uint32_t SniperKills;
        uint32_t GatlingKills;
        uint32_t BazookaKills;
        uint32_t GrenadeKills;
        uint32_t ZombieKills;
        uint32_t Infections;
        uint32_t SingleWaveDailyAttempts;
        uint32_t SingleWaveHighestWave;
        uint32_t SingleWaveHighScore;
        uint64_t SingleWaveLastUpdate;

        FrontAccount()
        {
            this->Index = -1;
			this->ServerId = 0;
            this->Username = "";
            this->Password = "";
            this->Salt = "";
            this->Grade = -1;
            this->PCRoom = -1;
            this->AuthKey = -1;
            this->ClanId = -1;
            this->ClanKills = -1;
            this->ClanDeaths = -1;
            this->ClanAssists = -1;
            this->ClanContribution = -1;
            this->ClanWins = -1;
            this->ClanLoses = -1;
            this->ClanDraws = -1;
            this->Nickname = "";
            this->Level = -1;
            this->Experience = -1;
            this->Tutorial = false;
            this->Story = -1;
            this->GuideMission = -1;
            this->Achievement = -1;
            this->VoiceType = -1;
            this->VIPExperience = -1;
            this->MaximumItems = -1;
            this->MaximumEnergy = -1;
            this->SelectedCharacter = -1;
            this->PlayTime = 0;
            this->MutedUntil = 0;
            this->Coins = -1;
            this->Energy = -1;
            this->LuckyPoints = -1;
            this->MicroPoints = -1;
            this->RockTokens = -1;
            this->Coupons = -1;
            this->RockTokens = -1;
            this->Wins = -1;
            this->Loses = -1;
            this->Draws = -1;
            this->Kills = -1;
            this->Deaths = -1;
            this->Assists = -1;
            this->Headshots = -1;
            this->HighestKillStreak = -1;
            this->MeleeKills = -1;
            this->RifleKills = -1;
            this->ShotgunKills = -1;
            this->SniperKills = -1;
            this->GatlingKills = -1;
            this->BazookaKills = -1;
            this->GrenadeKills = -1;
            this->ZombieKills = -1;
            this->Infections = -1;
            this->SingleWaveDailyAttempts = -1;
            this->SingleWaveHighestWave = -1;
            this->SingleWaveHighScore = -1;
            this->SingleWaveLastUpdate = -1;
        }
        FrontAccount(
            const uint32_t index,
			const uint32_t server_id,
            const std::string& username,
            const std::string& password,
            const std::string& salt,
            const uint8_t grade,
            const uint8_t pc_room,
            const uint64_t auth_key,
            const uint32_t clan_id,
            const uint32_t clan_kills,
            const uint32_t clan_deaths,
            const uint32_t clan_assists,
            const uint32_t clan_contrib,
            const uint32_t clan_wins,
            const uint32_t clan_loses,
            const uint32_t clan_draws,
            const std::string& nickname,
            const uint32_t level,
            const uint32_t experience,
            const bool tutorial,
            const uint32_t story,
            const uint8_t guide_mission,
            const uint64_t achievement,
            const uint64_t voice_type,
            const uint32_t vip_exp,
            const uint32_t max_items,
            const uint32_t max_energy,
            const uint32_t selected_char,
            const uint64_t playtime,
            const uint64_t muteduntil,
            const uint32_t coins,
            const uint32_t energy,
            const uint32_t luckypoints,
            const uint32_t micropoints,
            const uint32_t rocktokens,
            const uint32_t coupons,
            const uint32_t wins,
            const uint32_t loses,
            const uint32_t draws,
            const uint32_t kills,
            const uint32_t deaths,
            const uint32_t assists,
            const uint32_t headshots,
            const uint32_t highest_killstreak,
            const uint32_t melee_kills,
            const uint32_t rifle_kills,
            const uint32_t shotgun_kills,
            const uint32_t sniper_kills,
            const uint32_t gatling_kills,
            const uint32_t bazooka_kills,
            const uint32_t grenade_kills,
            const uint32_t zombie_kills,
            const uint32_t infections,
            const uint32_t sw_dailyattempts,
            const uint32_t sw_highestwave,
            const uint32_t sw_highscore,
            const uint64_t sw_lastupdate
        )
        {
            this->Index = index;
			this->ServerId = server_id;
            this->Username = username;
            this->Password = password;
            this->Salt = salt;
            this->Grade = grade;
            this->PCRoom = pc_room;
            this->AuthKey = auth_key;
            this->ClanId = clan_id;
            this->ClanKills = clan_kills;
            this->ClanDeaths = clan_deaths;
            this->ClanAssists = clan_assists;
            this->ClanContribution = clan_contrib;
            this->ClanWins = clan_wins;
            this->ClanLoses = clan_loses;
            this->ClanDraws = clan_draws;
            this->Nickname = nickname;
            this->Level = level;
            this->Experience = experience;
            this->Tutorial = tutorial;
            this->Story = story;
            this->GuideMission = guide_mission;
            this->Achievement = achievement;
            this->VoiceType = voice_type;
            this->VIPExperience = vip_exp;
            this->MaximumItems = max_items;
            this->MaximumEnergy = max_energy;
            this->SelectedCharacter = selected_char;
            this->PlayTime = playtime;
            this->MutedUntil = muteduntil;
            this->Coins = coins;
            this->Energy = energy;
            this->LuckyPoints = luckypoints;
            this->MicroPoints = micropoints;
            this->RockTokens = rocktokens;
            this->Coupons = coupons;
            this->RockTokens = rocktokens;
            this->Wins = wins;
            this->Loses = loses;
            this->Draws = draws;
            this->Kills = kills;
            this->Deaths = deaths;
            this->Assists = assists;
            this->Headshots = headshots;
            this->HighestKillStreak = highest_killstreak;
            this->MeleeKills = melee_kills;
            this->RifleKills = rifle_kills;
            this->ShotgunKills = shotgun_kills;
            this->SniperKills = sniper_kills;
            this->GatlingKills = gatling_kills;
            this->BazookaKills = bazooka_kills;
            this->GrenadeKills = grenade_kills;
            this->ZombieKills = zombie_kills;
            this->Infections = infections;
            this->SingleWaveDailyAttempts = sw_dailyattempts;
            this->SingleWaveHighestWave = sw_highestwave;
            this->SingleWaveHighScore = sw_highscore;
            this->SingleWaveLastUpdate = sw_lastupdate;
        }
    };

    struct PlayerMonthlyReward
    {
        uint32_t player_account_id;
        uint8_t day_count;
        uint64_t last_time_update;
    };
    struct SystemMonthlyRewards
    {
        uint32_t year{};
        uint32_t month{};
        std::array<uint32_t, 31> rewards{};
        SystemMonthlyRewards(uint32_t y, uint32_t m, std::array<uint32_t, 31> r)
            : year(y), month(m), rewards(r) {
        }

        SystemMonthlyRewards()
            : year(0), month(0), rewards{} {
        }
    };
    struct PlayerDailyMission {
        int32_t player_account_id;
        uint64_t update_time;
        uint32_t mission1;
        uint32_t mission2;
        uint32_t mission3;
        uint32_t goal_mission1;
        uint32_t goal_mission2;
        uint32_t goal_mission3;
    };
    struct ClanInfo
    {
        uint32_t id{ 0 };
        uint32_t owner_id{ 0 };
        std::string name = "";
        uint32_t logo_front{ 0 };
        uint32_t logo_back{ 0 };
        std::vector<uint16_t> online_members;
        ClanInfo(const uint32_t& id = 0, const uint32_t& owner_id = 0, const std::string& name = "", const uint32_t& logo_front = 0, const uint32_t& logo_back = 0) :
            id(id), owner_id(owner_id), name(name), logo_front(logo_front), logo_back(logo_back) {}

    };
    struct Item
    {
        NetEngine::Packets::Main::InventoryItemInfo item_info;
        uint32_t stock;
        uint8_t is_equipped;
        uint8_t character_id;
        uint8_t in_database;
        uint8_t item_type;
        Item(
            const NetEngine::Packets::Main::InventoryItemInfo& itemInfo = NetEngine::Packets::Main::InventoryItemInfo(),
            const uint32_t& stockVal = 0,
            const uint8_t& equippedVal = 0,
            const uint8_t& charIdVal = 0,
            const uint8_t& inDb = 0)
            : item_info(itemInfo),
            stock(stockVal),
            is_equipped(equippedVal),
            character_id(charIdVal),
            in_database(inDb),
            item_type(0)
        {
        }
    };
    struct SocialInfo
    {
        int32_t Aid{};
        int32_t targetAid{};
        uint8_t State{};
        std::string TargetNickname{};
        SocialInfo(const int32_t& Aid = 0, const int32_t& targetAid = 0, const uint8_t& State = 0, const std::string& TargetNickname = "") :
            Aid(Aid), targetAid(targetAid), State(State), TargetNickname(TargetNickname) { }

        SocialInfo() {}
    };
    struct FriendInfo
    {
        int32_t player_account_id{};
        int32_t friend_account_id{};
        uint8_t state{};
        uint32_t friend_session_id{};
        std::string friend_nickname{};
        FriendInfo(const int32_t& player_account_id = 0, const int32_t& friend_account_id = 0, const uint8_t& state = 0, const uint32_t& friend_session_id = 0, const std::string& friend_nickname = "") :
            player_account_id(player_account_id), friend_account_id(friend_account_id), state(state), friend_session_id(friend_session_id), friend_nickname(friend_nickname) {}

        FriendInfo() {}
    };
    struct BlockedInfo
    {
        int32_t player_account_id{};
        int32_t blocked_account_id{};
        uint32_t blocked_session_id{};
        std::string blocked_nickname{};
        BlockedInfo(const int32_t& player_account_id = 0, const int32_t& blocked_account_id = 0, const uint32_t& blocked_session_id = 0, const std::string& blocked_nickname = "") :
            player_account_id(player_account_id), blocked_account_id(blocked_account_id), blocked_session_id(blocked_session_id), blocked_nickname(blocked_nickname) {}

        BlockedInfo() {}
    };
    struct MailboxInfo
    {
        uint32_t mail_id{};
        uint32_t sender_account_id{};
        std::string sender_nickname{};
        uint32_t receiver_account_id{};
        std::string receiver_nickname{};
        uint32_t time{};
        uint32_t gift_itemid{};
        std::string message{};
        bool is_new{};
        bool deleted_from_sender{};
        bool deleted_from_receiver{};
        MailboxInfo(const uint32_t& mail_id = 0, const uint32_t& sender_account_id = 0, const std::string& sender_nickname = "", const uint32_t& receiver_account_id = 0, const std::string& receiver_nickname = "", const uint32_t& time = 0, const uint32_t& gift_itemid = 0, const std::string& message = "", const bool& is_new = false, const bool& deleted_from_sender = false, const bool& deleted_from_receiver = false) :
            mail_id(mail_id), sender_account_id(sender_account_id), sender_nickname(sender_nickname), receiver_account_id(receiver_account_id), receiver_nickname(receiver_nickname), time(time), gift_itemid(gift_itemid), message(message), is_new(is_new), deleted_from_sender(deleted_from_sender), deleted_from_receiver(deleted_from_receiver) {
        }

        MailboxInfo() {
            mail_id = 0;
            sender_account_id = 0;
            sender_nickname = "";
            receiver_account_id = 0;
            receiver_nickname = "";
            time = 0;
            gift_itemid = 0;
            message = "";
            is_new = false;
            deleted_from_sender = false;
            deleted_from_receiver = false;
        }
    };
    struct GachaPityEntry
    {
        uint32_t gacha_id{ 0 };
        uint32_t lucky_points{ 0 };
    };
    struct GachaPityPatch
    {
        uint32_t gacha_id{ 0 };
        uint32_t lucky_points{ 0 };
    };
    struct GachaponSaleInfo
    {
        uint32_t gachapon_id;
        uint32_t sale_price;
        uint32_t start_date;
        uint32_t end_date;
        GachaponSaleInfo(const uint32_t& gachapon_id = 0, const uint32_t& sale_price = 0, const uint32_t& start_date = 0, const uint32_t& end_date = 0) :
            gachapon_id(gachapon_id), sale_price(sale_price), start_date(start_date), end_date(end_date) {
        }
    };
    struct EndMatchUpdateDatabaseInfo {
        int32_t Id;
        uint32_t ClanKills;
        uint32_t ClanDeaths;
        uint32_t ClanAssists;
        uint32_t ClanContribution;
        uint32_t ClanWins;
        uint32_t ClanLoses;
        uint32_t ClanDraws;
        uint32_t Level;
        uint32_t Experience;
        uint64_t PlayTime;
        uint32_t SelectedCharacter;
        uint32_t Energy;
        uint32_t MicroPoints;
        uint32_t Wins;
        uint32_t Loses;
        uint32_t Draws;
        uint32_t Kills;
        uint32_t Deaths;
        uint32_t Assists;
        uint32_t Headshots;
        uint32_t HighestKillStreak;
        uint32_t MeleeKills;
        uint32_t RifleKills;
        uint32_t ShotgunKills;
        uint32_t SniperKills;
        uint32_t GatlingKills;
        uint32_t BazookaKills;
        uint32_t GrenadeKills;
        uint32_t ZombieKills;
        uint32_t Infections;
    };
    struct MatchInfoHistoryAdd
    {
        std::string MatchUniqueId;
        uint16_t Sid;
        int32_t Aid;
        bool IsWin;
        bool IsLose;
        bool IsHost;
        bool IsDraw;
        bool IsClanMatch;
        uint32_t WinRule;
        uint32_t TimeRule;
        std::string WinRuleType;
        uint32_t PlayTime;
        uint32_t Level;
        uint32_t Experience;
        uint32_t Energy;
        uint32_t MicroPoints;
        uint32_t room_index;
        uint32_t redscore;
        uint32_t bluescore;
        uint32_t team_id;
        uint32_t room_mode;
        uint32_t room_map;
        uint32_t SelectedCharacter;
        uint32_t Kills;
        uint32_t Deaths;
        uint32_t Assists;
        uint32_t Headshots;
        uint32_t HighestKillStreak;
        uint32_t MeleeKills;
        uint32_t RifleKills;
        uint32_t ShotgunKills;
        uint32_t SniperKills;
        uint32_t GatlingKills;
        uint32_t BazookaKills;
        uint32_t GrenadeKills;
        uint32_t ZombieKills;
        uint32_t Infections;
        uint64_t MatchStartTime;
        std::string MatchStartUtc;
        uint64_t MatchEndTime;
        std::string MatchEndUtc;
        uint32_t Hair, Face, Upper, Under, Skirt, Gloves, Boots;
        uint32_t HeadAcc, WaistAcc, BackAcc;
        uint32_t Melee, Rifle, Shotgun, Sniper, Gatling, Bazooka, Grenade;
        bool IsItemReward;
        uint32_t reward_item;
        bool IsMvp;
        bool IsEntryFragger;
        bool IsBullseye;
        bool IsSupport;
        bool IsBomba;
        uint32_t MvpScore;
        uint32_t EntryFraggerScore;
        uint32_t BullseyeScore;
        uint32_t SupportScore;
        uint32_t BombaScore;
        uint32_t BestKdScore;
        uint32_t CaptureScore;
        uint32_t WonRoundScore;
        uint32_t ArmsRaceScore;
        uint32_t ZombieScore;
        uint32_t ADR;
    };

    enum class ItemUpdateCtxType
    {
        Equip,
        Repair,
        Energy,
        Upgrade,
        ExpireDate
    };
    enum class ItemUpdateCtxError
    {
        ItemNotFound
    };
    struct ItemUpdateCtx
    {
        ItemUpdateCtxType change_type;
        std::optional<NetEngine::Packets::Main::ItemSerialInfo> serial_info;
        std::optional<uint32_t> item_type;
        std::optional<bool> is_equipped;
        std::optional<uint8_t> character_id;
        std::optional<uint32_t> repair;
        std::optional<uint32_t> energy;
        std::optional<uint32_t> new_item_id;
        std::optional<uint32_t> expire_date;
    };

    enum class DbUpdateError
    {
        ItemNotFound,
        NotEnoughSerialInfos,
		InventoryFull,
        InsufficientMP,
        InsufficientRT,
        InsufficientCOUPONS,
        InsufficientENERGY,
		MpFull,
		RtFull,
		CouponsFull,
        EnergyFull,
        MaxEnergyReachedAlready,
		MaxInventoryItemsReachedAlready,
        InvalidOp,
        InvalidAid,
		InvalidTargetAid,
        InvalidMailId,
        InvalidMailInserts,
        InvalidMailSenderNick,
        InvalidMailReceiverAid,
        InvalidMailReceiverNick,
        InvalidMailMsg,
        SelfFriend,
        SelfBlock,
        SqlError,
        AVA_CREATE_OVERLAPPEDNAME, // already exists
        AVA_CREATE_SHORTNAME, // doesn't meet length requirements
        AVA_CREATE_BANNAME,
        MEMO_MAIL_NOT_FOUND,
        MEMO_MAIL_SEND_MYSELF,
        MEMO_MAIL_FULL_SENDER,
        MEMO_MAIL_FULL_RECEIVER,
        MEMO_MAIL_BLOCKEDBY_SENDER,
        MEMO_MAIL_BLOCKEDBY_RECEIVER,
        MEMO_GIFT_FULL_SENDER,
        MEMO_GIFT_FULL_RECEIVER
    };
    struct ItemSelector
    {
        std::optional<NetEngine::Packets::Main::ItemSerialInfo> serial;
        std::optional<uint32_t> item_type;
        std::optional<uint8_t>  character_id;
    };
    struct ItemPatchCtx
    {
        ItemSelector sel;
        std::optional<bool> is_equipped;
        std::optional<uint8_t> character_id;
        std::optional<uint32_t> repair;
        std::optional<uint32_t> energy;
        std::optional<uint32_t> new_item_id;
        std::optional<uint32_t> expire_date;
    };
    struct ItemAddCtx
    {
        std::vector<Item> items;
    };
    struct ItemDeleteCtx
    {
        std::vector<NetEngine::Packets::Main::ItemSerialInfo> serials;
    };
    enum class CurrencyType { MP, RT, COUPONS, ENERGY };
    struct AccountCurrencyDelta
    {
        CurrencyType type;
        uint32_t value;
        bool is_reward{true}; // for costs; rewards can ignore
    };
    struct AccountInfoPatch
    {
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
        std::optional<uint64_t> achievement_tier1;
        std::optional<uint8_t> guide_mission;
		std::optional<uint64_t> voice_type;
        std::optional<bool> bTutorial;
		std::optional<uint32_t> maximum_items;
		std::optional<uint32_t> maximum_energy;
        std::optional<uint32_t> experience;
        std::optional<uint32_t> level;
		std::optional<uint32_t> lucky_points;
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
    };
    struct PlayerMissionsPatch
    {

        std::optional<uint32_t> mission1, mission2, mission3;
        std::optional<uint32_t> goal1, goal2, goal3;
        std::optional<uint64_t> update_time;
    };
    struct PlayerMonthlyRewardPatch
    {
        std::optional<uint32_t> day_count;
        std::optional<uint64_t> last_time_update;
    };

    enum class MailSide : uint8_t { Sender, Receiver };
    struct MailInsert
    {
        std::optional<uint32_t> receiver_id;
        std::optional<std::string> sender_nickname;
        std::optional<std::string> receiver_nickname;
        std::string message;
        uint32_t gift_item_id{0};
        bool is_new{true};
    };
    struct MailboxPatch
    {
        enum class Op : uint8_t { MarkRead, Delete, Insert } op{Op::MarkRead};
        uint32_t mail_id{ 0 };
        std::optional<bool> read;
        std::optional<MailSide> side;
        std::optional<MailInsert> insert;
    };

    struct PlayerSessionsPatch
    {
        enum class Op : uint8_t { Delete, Insert } op{Op::Delete};
        int32_t aid{ 0 };
        uint64_t key{ 0 };
    };

    struct PlayerSocialPatch
    {
        enum class Op : uint8_t { Delete, InsertOrUpdate } op{ Op::Delete };
        int32_t aid{ 0 };
        int32_t targetAid{ 0 };
        std::optional<uint8_t> State;
        std::optional<std::string> TargetNickname;
    };

    using DbOp = std::variant<
        ItemPatchCtx,
        ItemAddCtx,
        ItemDeleteCtx,
        AccountCurrencyDelta,
        AccountInfoPatch,
        PlayerMissionsPatch,
        PlayerMonthlyRewardPatch,
        MailboxPatch,
        MatchInfoHistoryAdd,
		PlayerSessionsPatch,
        PlayerSocialPatch,
        GachaPityPatch
    >;

    struct DatabaseUpdateCtx
    {
        uint32_t sid;
        int32_t aid;
        std::vector<DbOp> ops;
    };

    struct ResolvedItemPatch
    {
        uint64_t serial;
        ItemPatchCtx patch;
    };

    struct ValidatedDbUpdates
    {
        uint32_t sid;
        int32_t aid;
        std::vector<ResolvedItemPatch> items_patches;
        std::vector<Item> items_added;
        std::vector<NetEngine::Packets::Main::ItemSerialInfo> items_deleted;
        std::vector<AccountCurrencyDelta> currency_updates;
		std::vector<AccountInfoPatch> acc_info_patches;
		std::vector<PlayerMissionsPatch> player_missions_patches;
		std::vector<PlayerMonthlyRewardPatch> player_monthly_reward_patches;
		std::vector<MailboxPatch> mailbox_patches;
		std::vector<MatchInfoHistoryAdd> match_history_adds;
		std::vector<PlayerSessionsPatch> player_sessions_patches;
        std::vector<PlayerSocialPatch> player_social_patches;
        std::vector<GachaPityPatch> gacha_pity_patches;
    };

    struct ApplyCacheUpdateResult
    {
        std::vector<uint64_t> patched_serials;
        std::vector<uint64_t> deleted_serials;
        std::vector<uint64_t> added_serials;
        uint32_t new_mp{ 0 }, new_rt{ 0 }, new_coupons{ 0 }, new_energy{ 0 };
    };

    struct ResultLevelUpInfo
    {
        uint32_t sid{ 0 };
		bool level_up{ false };
        uint32_t new_level{ 0 };
		std::optional<uint32_t> reward_mp;
        std::optional<NetEngine::Packets::Main::ShopItem> reward_item;
    };
    struct ResultDbUpdateInfo
    {
        std::vector<uint64_t> patched_serials;
		int32_t patched_rows_count{0};
        std::vector<uint64_t> deleted_serials;
        int32_t deleted_rows_count{0};
        std::vector<uint64_t> added_serials;
        int32_t added_rows_count{0};
		bool target_not_found{ false };
		bool target_blocked{ false };
		bool player_blocked{ false };
    };


    struct DbError
    {
        enum class Type
        {
            None,
            ConnectionLost,
            DuplicateNickname,
            ConstraintViolation,
            NoRowsAffected,
            NicknameNotFound,
            MailboxFull,
            BlockedByReceiver,
            SqlException,
            Unknown
        };
        Type type{Type::None};
        int error_code{ 0 };
        std::string sql_state;
        std::string message;
    };
}
