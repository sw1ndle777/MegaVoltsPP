#pragma once
#include "CLog.h"

#include <mariadb/conncpp.hpp>
#include <iostream>

#include "Utility.h"
#include <array>
#include <NetEngine/Packets/PacketData.h>
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
    struct FrontAccount
    {
        int32_t Index;
		bool IsOnline;
        std::string Username;
        std::string Password;
        std::string Salt;
        uint8_t Grade;
        uint8_t PCRoom;
        uint64_t AuthKey;
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
			this->IsOnline = false;
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
            const uint32_t& index,
			const bool& is_online,
            const std::string&   username,         const std::string&   password,       const std::string&   salt,               const uint8_t&  grade,
            const uint8_t& pc_room,           const uint64_t& auth_key,       const uint32_t& clan_id,            const uint32_t& clan_kills,
            const uint32_t& clan_deaths,
            const uint32_t& clan_assists,     const uint32_t& clan_contrib,   const uint32_t& clan_wins,          const uint32_t& clan_loses, 
            const uint32_t& clan_draws,       const std::string&   nickname,       const uint32_t& level,
            const uint32_t& experience,       const bool&          tutorial,       const uint32_t& story,              const uint8_t& guide_mission,
            const uint64_t& achievement,      const uint64_t& voice_type,     const uint32_t& vip_exp,
            const uint32_t& max_items,        const uint32_t& max_energy,     const uint32_t& selected_char,      const uint64_t& playtime,
            const uint64_t& muteduntil,       const uint32_t& coins,          const uint32_t& energy,             const uint32_t& luckypoints,
            const uint32_t& micropoints,      const uint32_t& rocktokens,     const uint32_t& coupons,            const uint32_t& wins,
            const uint32_t& loses,            const uint32_t& draws,          const uint32_t& kills,              const uint32_t& deaths,
            const uint32_t& assists,          const uint32_t& headshots,      const uint32_t& highest_killstreak, const uint32_t& melee_kills,
            const uint32_t& rifle_kills,      const uint32_t& shotgun_kills,  const uint32_t& sniper_kills,       const uint32_t& gatling_kills,
            const uint32_t& bazooka_kills,    const uint32_t& grenade_kills,  const uint32_t& zombie_kills,       const uint32_t& infections, 
            const uint32_t& sw_dailyattempts, const uint32_t& sw_highestwave, const uint32_t& sw_highscore,       const uint64_t& sw_lastupdate
        )
        {
            this->Index = index;
			this->IsOnline = is_online;
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
        uint32_t month;
        std::array<uint32_t, 31> rewards;
        SystemMonthlyRewards(uint32_t m, std::array<uint32_t, 31> r)
            : month(m), rewards(r) {
        }

        SystemMonthlyRewards()
            : month(0), rewards{} {
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
        uint32_t id;
        uint32_t owner_id;
        std::string name;
        uint32_t logo_front;
        uint32_t logo_back;
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
            in_database(inDb)
        {
        }
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

    class CDatabase
    {

    public:
        void Initialize(const std::string& database, const std::string& host, const uint16_t& port, const std::string& user, const std::string& password);
        bool CreateTable(const std::string& table_name, const std::string& data_collumns);
        bool CreateDatabase(const std::string& name);
        bool UpdateEndMatchInfo(std::vector<EndMatchUpdateDatabaseInfo>& playerUpdates);
		bool GetFrontAccount(const std::string& ip, const std::string& username, const std::string& password, FrontAccount* outFrontAccount, ClanInfo* outClanInfo);
		bool GetFrontAccount(const uint64_t& authKey, FrontAccount* outFrontAccount, ClanInfo* outClanInfo);
        bool SetAccountOffline(const std::uint32_t& accountId);
        bool GetMainFrontAccount(const uint64_t& authKey, FrontAccount* outFrontAccount, ClanInfo* outClanInfo, PlayerDailyMission* outDailyMission, std::vector<Item>& inv_items, std::vector<BlockedInfo>& blockeds, std::vector<FriendInfo>& friends, std::vector<MailboxInfo>& mailbox_list, std::vector<std::uint32_t>& daily_mission_random_ids);
        bool NicknameExists(const std::string_view& nickname);
        bool NicknameExists(const std::string_view& nickname, uint32_t& account_id);;
        bool UpdateNickname(const std::string_view& nickname, const uint64_t& authKey);
        bool UpdateSelectedCharacter(const uint32_t& character, const uint64_t& authKey);
        bool UpdateEnergy(const uint32_t& energy, const uint64_t& authKey);
        //bool GetFrontAccount(const uint64_t& authKey, FrontAccount* outFrontAccount);
        bool UpdateFrontAccount(const FrontAccount& front_acc);
        bool GetInventoryItems(const uint32_t& acc_id, std::vector<Item>& inv_items);
        bool UpdateInventoryItems(const uint32_t& acc_id, const std::vector<Item>& inv_items);
        bool InsertInventoryItems(const uint32_t& acc_id, const std::vector<Item>& inv_items);
        bool InsertInventoryitemsMicroTransactions(const uint32_t& acc_id, const std::vector<Item>& inv_items, const uint32_t mp, const uint32_t rt, const uint32_t coupons);
        bool DeleteInventoryItems(const uint32_t& acc_id, const std::vector<Item>& inv_items);
        bool NewDeleteInventoryItems(const uint32_t& acc_id, const std::vector<NetEngine::Packets::Main::ItemSerialInfo>& del_items);
        bool GetPlayerFriends(const int32_t& acc_id, std::vector<FriendInfo>& friends);
        bool GetPlayerBlockeds(const int32_t& acc_id, std::vector<BlockedInfo>& blockeds);
        bool InsertPlayerFriends(const std::vector<FriendInfo>& friends);
        bool DeletePlayerFriends(const std::vector<FriendInfo>& friends);
        bool UpdatePlayerFriends(const int32_t& acc_id, const uint32_t& friend_acc_id, const uint8_t& state);
        bool InsertPlayerBlockeds(const std::vector<BlockedInfo>& blockeds);
        bool DeletePlayerBlockeds(const std::vector<BlockedInfo>& blockeds);
        bool RegisterClan(const std::string& name, const uint32_t& owner_id, const uint32_t& logo_front, const uint32_t& logo_back);
        bool GetClanInfo(const uint32_t& clan_id, ClanInfo* outClanInfo);
        bool UpdateClanInfo(const uint32_t& clan_id, const std::string& name, const uint32_t& owner_id, const uint32_t& logo_front, const uint32_t& logo_back);
        bool InsertPlayerMailbox(const MailboxInfo& mailbox_info, uint32_t& out_mail_id);
        bool DeletePlayerMailbox(const std::vector<MailboxInfo>& mails);
        std::vector<MailboxInfo> GetPlayerMailbox(const int32_t& acc_id);
        uint32_t GetPlayerReceiverMailboxCount(const int32_t& acc_id);
        uint32_t GetPlayerReceiverGiftboxCount(const int32_t& acc_id);
        bool UpdateMailboxIsNew(const std::vector<uint32_t>& mail_ids, bool is_new);
        bool UpdateOrDeleteMailboxForSender(const std::vector<uint32_t>& mail_ids);
        bool UpdateOrDeleteMailboxForReceiver(const std::vector<uint32_t>& mail_ids);
        bool GetSystemMonthlyRewards(const uint32_t& month, SystemMonthlyRewards* outMonthlyRewards);
        bool GetPlayerMonthlyDayCount(const uint32_t& acc_id, PlayerMonthlyReward* outMonthlyRewards);
        bool InsertPlayerMonthlyDayCount(const uint32_t& acc_id, const uint8_t& reward_count, const uint64_t& last_update);
        bool UpdatePlayerMonthlyDayCount(const uint32_t& acc_id, const uint8_t& reward_count, const uint64_t& last_update);
        bool GetPlayerDailyMission(const int32_t& acc_id, PlayerDailyMission* outDailyMission);
        bool InsertPlayerDailyMission(const uint32_t& acc_id, const PlayerDailyMission& dailyMission);
        bool UpdatePlayerDailyMission(const uint32_t& acc_id, const PlayerDailyMission& dailyMission);
        std::vector<GachaponSaleInfo> GetGachaponSalesInfo();
        bool DeleteGachaponSaleInfo(const uint32_t& gachapon_id);
        std::string GetDatabaseName();
        CDatabase() {};
        ~CDatabase() {};

    private:
        std::string database_name;
        sql::Driver* driver = nullptr;
        sql::Connection* conn = nullptr;
        sql::Properties properties;
    };

    extern std::unique_ptr<CDatabase> Database;
}

//#endif