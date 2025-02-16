#pragma once
#ifdef _WIN64
#pragma comment(lib, "mariadbcpp.lib")
#endif
#include <conncpp.hpp>
#include <iostream>
#include "CLog.h"
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
            std::uint32_t id : 1;
            std::uint32_t expire_date : 1;
            std::uint32_t repair : 1;
            std::uint32_t energy : 1;
            std::uint32_t is_sealed : 1;
            std::uint32_t stock : 1;
            std::uint32_t is_equipped : 1;
            std::uint32_t character_id : 1;
        };
        std::uint8_t data;

        ItemUpdateInfo(std::uint8_t data = 0)
        {
            std::memset(this, 0, sizeof(ItemUpdateInfo));
            this->data = data;
        }
    };
#pragma pack(pop)
    struct FrontAccount
    {
        std::int32_t Index;
        std::string Username;
        std::string Password;
        std::string Salt;
        std::uint8_t Grade;
        std::uint64_t AuthKey;
        std::uint32_t ClanId;
        std::uint32_t ClanKills;
        std::uint32_t ClanDeaths;
        std::uint32_t ClanAssists;
        std::uint64_t ClanContribution;
        std::uint64_t ClanWins;
        std::uint64_t ClanLoses;
        std::uint64_t ClanDraws;
        std::string Nickname;
        std::uint32_t Level;
        std::uint32_t Experience;
        bool Tutorial;
        std::uint32_t Story;
        std::uint32_t VIPExperience;
        std::uint32_t MaximumItems;
        std::uint32_t MaximumEnergy;
        std::uint32_t SelectedCharacter;
        std::uint64_t PlayTime;
        std::uint64_t MutedUntil;
        std::uint32_t Coins;
        std::uint32_t Energy;
        std::uint32_t LuckyPoints;
        std::uint32_t MicroPoints;
        std::uint32_t RockTokens;
        std::uint32_t Coupons;
        std::uint32_t Wins;
        std::uint32_t Loses;
        std::uint32_t Draws;
        std::uint32_t Kills;
        std::uint32_t Deaths;
        std::uint32_t Assists;
        std::uint32_t Headshots;
        std::uint32_t HighestKillStreak;
        std::uint32_t MeleeKills;
        std::uint32_t RifleKills;
        std::uint32_t ShotgunKills;
        std::uint32_t SniperKills;
        std::uint32_t GatlingKills;
        std::uint32_t BazookaKills;
        std::uint32_t GrenadeKills;
        std::uint32_t ZombieKills;
        std::uint32_t Infections;
        std::uint32_t SingleWaveDailyAttempts;
        std::uint32_t SingleWaveHighestWave;
        std::uint32_t SingleWaveHighScore;
        std::uint64_t SingleWaveLastUpdate;

        FrontAccount()
        {
            this->Index = -1;
            this->Username = "";
            this->Password = "";
            this->Salt = "";
            this->Grade = -1;
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
            const std::uint32_t& index,            
            const std::string&   username,         const std::string&   password,       const std::string&   salt,               const std::uint8_t&  grade,
            const std::uint64_t& auth_key,         const std::uint32_t& clan_id,        const std::uint32_t& clan_kills,         const std::uint32_t& clan_deaths,
            const std::uint32_t& clan_assists,     const std::uint64_t& clan_contrib,   const std::uint64_t& clan_wins,          const std::uint64_t& clan_loses, 
            const std::uint64_t& clan_draws,       const std::string&   nickname,       const std::uint32_t& level,
            const std::uint32_t& experience,       const bool&          tutorial,       const std::uint32_t& story,              const std::uint32_t& vip_exp,
            const std::uint32_t& max_items,        const std::uint32_t& max_energy,     const std::uint32_t& selected_char,      const std::uint64_t& playtime,
            const std::uint64_t& muteduntil,       const std::uint32_t& coins,          const std::uint32_t& energy,             const std::uint32_t& luckypoints,
            const std::uint32_t& micropoints,      const std::uint32_t& rocktokens,     const std::uint32_t& coupons,            const std::uint32_t& wins,
            const std::uint32_t& loses,            const std::uint32_t& draws,          const std::uint32_t& kills,              const std::uint32_t& deaths,
            const std::uint32_t& assists,          const std::uint32_t& headshots,      const std::uint32_t& highest_killstreak, const std::uint32_t& melee_kills,
            const std::uint32_t& rifle_kills,      const std::uint32_t& shotgun_kills,  const std::uint32_t& sniper_kills,       const std::uint32_t& gatling_kills,
            const std::uint32_t& bazooka_kills,    const std::uint32_t& grenade_kills,  const std::uint32_t& zombie_kills,       const std::uint32_t& infections, 
            const std::uint32_t& sw_dailyattempts, const std::uint32_t& sw_highestwave, const std::uint32_t& sw_highscore,       const std::uint64_t& sw_lastupdate
        )
        {
            this->Index = index;
            this->Username = username;
            this->Password = password;
            this->Salt = salt;
            this->Grade = grade;
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
        std::uint32_t player_account_id;
        std::uint8_t day_count;
        std::uint64_t last_time_update;
    };
    struct SystemMonthlyRewards
    {
        std::uint32_t month;
        std::array<std::uint32_t, 31> rewards;
        SystemMonthlyRewards(std::uint32_t m, std::array<std::uint32_t, 31> r)
            : month(m), rewards(r) {
        }

        SystemMonthlyRewards()
            : month(0), rewards{} {
        }
    };
    struct ClanInfo
    {
        std::uint32_t id;
        std::uint32_t owner_id;
        std::string name;
        std::uint32_t logo_front;
        std::uint32_t logo_back;
        std::vector<std::uint16_t> online_members;
        ClanInfo(const std::uint32_t& id = 0, const std::uint32_t& owner_id = 0, const std::string& name = "", const std::uint32_t& logo_front = 0, const std::uint32_t& logo_back = 0) :
            id(id), owner_id(owner_id), name(name), logo_front(logo_front), logo_back(logo_back) {}
    };
    struct Item
    {
        NetEngine::Packets::Main::InventoryItemInfo item_info;
        std::uint32_t stock;
        std::uint8_t is_equipped;
        std::uint8_t character_id;
        std::uint8_t in_database;
        Item(
            const NetEngine::Packets::Main::InventoryItemInfo& itemInfo = NetEngine::Packets::Main::InventoryItemInfo(),
            const std::uint32_t& stockVal = 0,
            const std::uint8_t& equippedVal = 0,
            const std::uint8_t& charIdVal = 0,
            const std::uint8_t& inDb = 0)
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
        std::int32_t player_account_id{};
        std::int32_t friend_account_id{};
        std::uint8_t state{};
        std::uint32_t friend_session_id{};
        std::string friend_nickname{};
        FriendInfo(const std::int32_t& player_account_id = 0, const std::int32_t& friend_account_id = 0, const std::uint8_t& state = 0, const std::uint32_t& friend_session_id = 0, const std::string& friend_nickname = "") :
            player_account_id(player_account_id), friend_account_id(friend_account_id), state(state), friend_session_id(friend_session_id), friend_nickname(friend_nickname) {}

        FriendInfo() {}
    };
    struct BlockedInfo
    {
        std::int32_t player_account_id{};
        std::int32_t blocked_account_id{};
        std::uint32_t blocked_session_id{};
        std::string blocked_nickname{};
        BlockedInfo(const std::int32_t& player_account_id = 0, const std::int32_t& blocked_account_id = 0, const std::uint32_t& blocked_session_id = 0, const std::string& blocked_nickname = "") :
            player_account_id(player_account_id), blocked_account_id(blocked_account_id), blocked_session_id(blocked_session_id), blocked_nickname(blocked_nickname) {}

        BlockedInfo() {}
    };
    struct MailboxInfo
    {
        std::uint32_t mail_id{};
        std::uint32_t sender_account_id{};
        std::string sender_nickname{};
        std::uint32_t receiver_account_id{};
        std::string receiver_nickname{};
        std::uint32_t time{};
        std::uint32_t gift_itemid{};
        std::string message{};
        bool is_new{};
        bool deleted_from_sender{};
        bool deleted_from_receiver{};
        MailboxInfo(const std::uint32_t& mail_id = 0, const std::uint32_t& sender_account_id = 0, const std::string& sender_nickname = "", const std::uint32_t& receiver_account_id = 0, const std::string& receiver_nickname = "", const std::uint32_t& time = 0, const std::uint32_t& gift_itemid = 0, const std::string& message = "", const bool& is_new = false, const bool& deleted_from_sender = false, const bool& deleted_from_receiver = false) :
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
        std::uint32_t gachapon_id;
        std::uint32_t sale_price;
        std::uint32_t start_date;
        std::uint32_t end_date;
        GachaponSaleInfo(const std::uint32_t& gachapon_id = 0, const std::uint32_t& sale_price = 0, const std::uint32_t& start_date = 0, const std::uint32_t& end_date = 0) :
            gachapon_id(gachapon_id), sale_price(sale_price), start_date(start_date), end_date(end_date) {
        }
    };

    class CDatabase
    {

    public:
        void Initialize(const std::string& database, const std::string& host, const std::uint16_t& port, const std::string& user, const std::string& password);
        bool CreateTable(const std::string& table_name, const std::string& data_collumns);
        bool CreateDatabase(const std::string& name);
        bool RegisterAccount(const std::string& username, const std::string& password, const std::uint8_t& grade, const std::uint32_t& mp, const std::uint32_t& rt, const std::uint32_t& coupons = 0, const std::uint32_t& coins = 0, const std::uint32_t& energy = 0, const std::uint32_t& max_items = 1000, const std::uint32_t& max_battery = 5000, const std::string& nickname = "");
        bool GetFrontAccount(const std::string& username, FrontAccount* outFrontAccount);
        bool NicknameExists(const std::string_view& nickname);
        bool NicknameExists(const std::string_view& nickname, std::uint32_t& account_id);;
        bool UpdateNickname(const std::string_view& nickname, const std::uint64_t& authKey);
        bool UpdateSelectedCharacter(const std::uint32_t& character, const std::uint64_t& authKey);
        bool GetFrontAccount(const std::uint64_t& authKey, FrontAccount* outFrontAccount);
        bool UpdateFrontAccount(const FrontAccount& front_acc);
        bool GetInventoryItems(const std::uint32_t& acc_id, std::vector<Item>& inv_items);
        bool UpdateInventoryItems(const std::uint32_t& acc_id, const std::vector<Item>& inv_items);
        bool InsertInventoryItems(const std::uint32_t& acc_id, const std::vector<Item>& inv_items);
        bool DeleteInventoryItems(const std::uint32_t& acc_id, const std::vector<Item>& inv_items);
        bool GetPlayerFriends(const std::int32_t& acc_id, std::vector<FriendInfo>& friends);
        bool GetPlayerBlockeds(const std::int32_t& acc_id, std::vector<BlockedInfo>& blockeds);
        bool InsertPlayerFriends(const std::vector<FriendInfo>& friends);
        bool DeletePlayerFriends(const std::vector<FriendInfo>& friends);
        bool UpdatePlayerFriends(const std::int32_t& acc_id, const std::uint32_t& friend_acc_id, const std::uint8_t& state);
        bool InsertPlayerBlockeds(const std::vector<BlockedInfo>& blockeds);
        bool DeletePlayerBlockeds(const std::vector<BlockedInfo>& blockeds);
        bool RegisterClan(const std::string& name, const std::uint32_t& owner_id, const std::uint32_t& logo_front, const std::uint32_t& logo_back);
        bool GetClanInfo(const std::uint32_t& clan_id, ClanInfo* outClanInfo);
        bool UpdateClanInfo(const std::uint32_t& clan_id, const std::string& name, const std::uint32_t& owner_id, const std::uint32_t& logo_front, const std::uint32_t& logo_back);
        bool InsertPlayerMailbox(const MailboxInfo& mailbox_info, std::uint32_t& out_mail_id);
        bool DeletePlayerMailbox(const std::vector<MailboxInfo>& mails);
        std::vector<MailboxInfo> GetPlayerMailbox(const std::int32_t& acc_id);
        std::uint32_t GetPlayerReceiverMailboxCount(const std::int32_t& acc_id);
        std::uint32_t GetPlayerReceiverGiftboxCount(const std::int32_t& acc_id);
        bool UpdateMailboxIsNew(const std::vector<std::uint32_t>& mail_ids, bool is_new);
        bool UpdateOrDeleteMailboxForSender(const std::vector<std::uint32_t>& mail_ids);
        bool UpdateOrDeleteMailboxForReceiver(const std::vector<std::uint32_t>& mail_ids);
        bool GetSystemMonthlyRewards(const std::uint32_t& month, SystemMonthlyRewards* outMonthlyRewards);
        bool GetPlayerMonthlyDayCount(const std::uint32_t& acc_id, PlayerMonthlyReward* outMonthlyRewards);
        bool InsertPlayerMonthlyDayCount(const std::uint32_t& acc_id, const std::uint8_t& reward_count, const std::uint64_t& last_update);
        bool UpdatePlayerMonthlyDayCount(const std::uint32_t& acc_id, const std::uint8_t& reward_count, const std::uint64_t& last_update);
        std::vector<GachaponSaleInfo> GetGachaponSalesInfo();
        bool DeleteGachaponSaleInfo(const std::uint32_t& gachapon_id);
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