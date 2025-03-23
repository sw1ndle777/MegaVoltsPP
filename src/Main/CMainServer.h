#pragma once
#include <string>
#include <functional>
#include <unordered_set>
#include <optional>
#include <numeric>
#include <ranges>

#include "BaseLib/CLog.h"

#include "NetEngine/CServer.h"
#include "NetEngine/CSession.h"
#include "NetEngine/Constants.h"

#include "NetEngine/Packets/PacketStruct.h"
#include "NetEngine/Packets/PacketData.h"
#include <boost/unordered/unordered_flat_map.hpp>
#include <boost/unordered/unordered_flat_set.hpp>
#include "BaseLib/CDatabase.h"
#include "BaseLib/CDBData.h"

#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

namespace Game
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Main;


    struct Disconnect
    {
        enum Reason : std::uint8_t
        {
            Generic = 0x00,
            Block = 0x2A, //Multiple user login
            Offline = 0x2F, //Server is in maintenance
            Close = 0x1B, //Server close message
            DataError = 0x04, //No client response
            Busy = 0x05, //Autorization failed
            Deny = 0x23  //Removed by moderator
        };
    };
    struct ItemExpire
    {
        enum Type : std::uint32_t
        {
            Unlimited = 0,
            Unused = 1,
            Destroyed = 2,
            Expired = 3
        };
    };
    struct PlazaJoin
    {
        enum Result : std::uint8_t
        {
            Success = 0x01,
            Full = 0x07,
            NoPlaza = 0x23
        };
    };
    struct FrontAuthorize
    {
        enum Type : std::uint8_t
        {
            Wrong = 0x00,
            Success = 0x01,
            DataError = 0x04,
            Busy = 0x05,
            DontExist = 0x0D,
            TimeExpire2 = 0x18,
            Blocked = 0x2A,
            Shutdown = 0x50
        };
    };
    struct ChannelInfo
    {
        enum Status : std::uint8_t
        {
            Normal = 0x00,
            Busy = 0x01,
            VeryBusy = 0x02,
            Offline = 0x03
        };
    };
    struct VersionCheckInfo
    {
        enum Result : std::uint8_t
        {
            NicknameDialog = 0x00,
            Full = 0x01,
            Empty = 0x02,
            Deny = 0x03,
            WrongVersion = 0x10,
            Offline = 0x2F
        };
    };
    struct NicknameCreationInfo
    {
        enum Result : std::uint8_t
        {
            Continue = 0x00,
            CreationFailed = 0x01,//no result
            AlreadyInUse = 0x05,
            Full = 0x07,//error
            NotExist = 0x0D,//error
            ShortName = 0x0E,//error
            NoPermission = 0x10//error
        };
    };
    struct CharacterSelectInfo
    {
        enum Result : std::uint8_t
        {
            Ok = 0x01,
            NotExist = 0x0D,
            Deny = 0x23,
            Fail
        };
    };

    struct PlayerInfo
    {

        enum State : std::uint8_t
        {
            Standby = 0,
            CharacterSelection = 1,
            SceneLoading = 2,
            SceneLoaded = 3,
            LobbyAgora = 4,
            Inventory = 5,
            GachaponMachine = 6,
            Waiting = 7,
            HostReady = 8,
            PlayerReady = 9,
            Loaded = 10,
            Normal = 11,
            Test = 12
        };
    };
    struct ProbabilityStruct
    {
        std::uint32_t id;
        std::uint32_t prob;
        ProbabilityStruct(const std::uint32_t& id, const std::uint32_t& prob) : id(id), prob(prob) {}
    };
    struct Player
    {
        std::shared_mutex mutex;
        std::uint16_t session_id;
        std::uint32_t ping;
        std::uint8_t fps_limit;
        std::uint32_t room_id;
        std::uint32_t plaza_id;
        std::uint32_t party_id;
        bool playing;
        std::uint32_t slot_id;
        std::uint8_t team_id;
        std::uint8_t state;
        std::uint64_t server_time;
        bool in_room;
        bool in_plaza;
        bool in_party;
        bool sent_ping_once;
        std::uint8_t voice_id;
        std::uint64_t match_loaded_time;
        std::uint32_t earnt_battery;
        std::uint32_t zombie_team;
        PlayerDailyMission daily_mission_info;
        BaseLib::FrontAccount acc_info;
        std::vector<Item> inventory_items;
        std::vector<ItemSerialInfo> items_deleted;
        std::vector<ItemSerialInfo> items_added;
        std::vector<ItemSerialInfo> items_updated;
        std::vector<BaseLib::FriendInfo> friends_deleted;
        std::vector<BaseLib::FriendInfo> friends_accepted;
        std::vector<BaseLib::FriendInfo> friends_pendings;
        std::vector<BaseLib::BlockedInfo> blockeds_deleted;
        std::vector<BaseLib::BlockedInfo> blockeds_added;
        Player(const std::uint16_t& sessionId, const std::uint64_t& serverTime, const BaseLib::FrontAccount& accountInfo, const std::vector<Item>& inventoryItems)
            : session_id(sessionId), server_time(serverTime), acc_info(accountInfo), inventory_items(inventoryItems)
        {
            plaza_id = 0;
            party_id = 0;
            ping = 0;
            fps_limit = 0;
            room_id = 0;
            playing = false;
            slot_id = 0;
            team_id = 0;
            state = 0;
            in_room = false;
            in_plaza = false;
            in_party = false;
            sent_ping_once = false;
            voice_id = 0;
            match_loaded_time = 0;
            earnt_battery = 0;
            zombie_team = 0;
            items_deleted.clear();
            items_added.clear();
            items_updated.clear();
            friends_deleted.clear();
            friends_accepted.clear();
            friends_pendings.clear();
            blockeds_deleted.clear();
            blockeds_added.clear();
        }
        Player(const Player& other)
        {
            session_id = other.session_id;
            ping = other.ping;
            fps_limit = other.fps_limit;
            room_id = other.room_id;
            plaza_id = other.plaza_id;
            party_id = other.party_id;
            playing = other.playing;
            slot_id = other.slot_id;
            team_id = other.team_id;
            state = other.state;
            server_time = other.server_time;
            in_room = other.in_room;
            in_plaza = other.in_plaza;
            in_party = other.in_party;
            sent_ping_once = other.sent_ping_once;
            zombie_team = other.zombie_team;
            voice_id = other.voice_id;
            match_loaded_time = other.match_loaded_time;
            earnt_battery = other.earnt_battery;
            acc_info = other.acc_info;
            inventory_items = other.inventory_items;
            items_deleted = other.items_deleted;
            items_added = other.items_added;
            items_updated = other.items_updated;
            friends_deleted = other.friends_deleted;
            friends_accepted = other.friends_accepted;
            friends_pendings = other.friends_pendings;
            blockeds_deleted = other.blockeds_deleted;
            blockeds_added = other.blockeds_added;
            daily_mission_info = other.daily_mission_info;
        }
        Player& operator=(const Player& other)
        {
            if (this == &other) return *this;
            session_id = other.session_id;
            ping = other.ping;
            fps_limit = other.fps_limit;
            room_id = other.room_id;
            plaza_id = other.plaza_id;
            party_id = other.party_id;
            playing = other.playing;
            slot_id = other.slot_id;
            team_id = other.team_id;
            state = other.state;
            server_time = other.server_time;
            in_room = other.in_room;
            in_plaza = other.in_plaza;
            in_party = other.in_party;
            sent_ping_once = other.sent_ping_once;
            zombie_team = other.zombie_team;
            voice_id = other.voice_id;
            match_loaded_time = other.match_loaded_time;
            earnt_battery = other.earnt_battery;
            acc_info = other.acc_info;
            inventory_items = other.inventory_items;
            items_deleted = other.items_deleted;
            items_added = other.items_added;
            items_updated = other.items_updated;
            friends_deleted = other.friends_deleted;
            friends_accepted = other.friends_accepted;
            friends_pendings = other.friends_pendings;
            blockeds_deleted = other.blockeds_deleted;
            blockeds_added = other.blockeds_added;
            daily_mission_info = other.daily_mission_info;
            return *this;
        }
        Player()
        {
            plaza_id = 0;
            party_id = 0;
            acc_info = BaseLib::FrontAccount();
            session_id = 0;
            server_time = 0;
            ping = 0;
            fps_limit = 0;
            room_id = 0;
            playing = false;
            slot_id = 0;
            team_id = 0;
            state = 0;
            earnt_battery = 0;
            in_plaza = false;
            in_room = false;
            in_party = false;
            sent_ping_once = false;
            zombie_team = 0;
            voice_id = 0;
            match_loaded_time = 0;
            inventory_items.clear();
            items_deleted.clear();
            items_added.clear();
            items_updated.clear();
            friends_deleted.clear();
            friends_accepted.clear();
            friends_pendings.clear();
            blockeds_deleted.clear();
            blockeds_added.clear();
        }
    };
    struct Room
    {
        std::shared_mutex mutex;
        std::uint16_t room_id;
        std::uint16_t channel_id;
        std::string title;
        std::string password;
        NetEngine::Room::Map::Index MapIndex;
        NetEngine::Room::Map::Index RandomMapIndex;
        NetEngine::Room::Mode::Index ModeIndex;
        NetEngine::Room::Restriction::Type Restriction;
        NetEngine::Room::Balance::State TeamBalance;
        std::uint32_t max_players;
        std::uint32_t score_rule;
        std::uint32_t time_rule;
        bool allow_intruders;
        bool allow_drops;
        bool allow_observers;
        bool is_playing;
        bool has_password;
        std::uint16_t host_session_id;
        std::vector<std::uint16_t> neutralteam_session_ids;
        std::vector<std::uint16_t> blueteam_session_ids;
        std::vector<std::uint16_t> redteam_session_ids;
        std::vector<std::uint16_t> observers_session_ids;
        std::vector<std::uint16_t> kicked_session_ids;
        std::vector<std::uint16_t> kick_voters_session_ids;
        std::vector<std::uint16_t> voters_session_ids;
        std::uint16_t vote_kick_target_session_id;
        bool is_kick_vote_running = false;
        bool is_clan_room;
        std::uint32_t clan_id_1;
        std::uint32_t clan_id_2;
        Room(const std::uint16_t& roomId = 0, const std::uint16_t& channelId = 0, const std::string& title = "", const std::string& password = "",
            const NetEngine::Room::Map::Index& mapIndex = NetEngine::Room::Map::Index::Chess, const NetEngine::Room::Mode::Index& modeIndex = NetEngine::Room::Mode::Index::TeamDeathMatch,
            const NetEngine::Room::Restriction::Type& restriction = NetEngine::Room::Restriction::AllWeapons, const NetEngine::Room::Balance::State& teamBalance = NetEngine::Room::Balance::State::Disabled,
            const std::uint32_t& maxPlayers = 0, const std::uint32_t& scoreRule = 0, const std::uint32_t& timeRule = 0,
            const bool& allowIntruders = true, const bool& allowDrops = true, const bool& allowObservers = true, const bool& isPlaying = false, const bool& hasPassword = false,
            const std::uint16_t& hostSessionId = 0)
            : room_id(roomId), channel_id(channelId), title(title), password(password), MapIndex(mapIndex), ModeIndex(modeIndex), Restriction(restriction), TeamBalance(teamBalance), 
            max_players(maxPlayers), score_rule(scoreRule), time_rule(timeRule), allow_intruders(allowIntruders), allow_drops(allowDrops), allow_observers(allowObservers),
            is_playing(isPlaying), has_password(hasPassword), host_session_id(hostSessionId), is_kick_vote_running(false), vote_kick_target_session_id(0)
        {
            neutralteam_session_ids.clear();
            blueteam_session_ids.clear();
            redteam_session_ids.clear();
            observers_session_ids.clear();
            kicked_session_ids.clear();
            kick_voters_session_ids.clear();
            voters_session_ids.clear();
        }
        Room(const Room& other)
        {
            room_id = other.room_id;
            channel_id = other.channel_id;
            title = other.title;
            password = other.password;
            MapIndex = other.MapIndex;
            RandomMapIndex = other.RandomMapIndex;
            ModeIndex = other.ModeIndex;
            Restriction = other.Restriction;
            TeamBalance = other.TeamBalance;
            max_players = other.max_players;
            score_rule = other.score_rule;
            time_rule = other.time_rule;
            allow_intruders = other.allow_intruders;
            allow_drops = other.allow_drops;
            allow_observers = other.allow_observers;
            is_playing = other.is_playing;
            has_password = other.has_password;
            host_session_id = other.host_session_id;
            neutralteam_session_ids = other.neutralteam_session_ids;
            blueteam_session_ids = other.blueteam_session_ids;
            redteam_session_ids = other.redteam_session_ids;
            observers_session_ids = other.observers_session_ids;
            kicked_session_ids = other.kicked_session_ids;
            kick_voters_session_ids = other.kick_voters_session_ids;
            voters_session_ids = other.voters_session_ids;
            is_kick_vote_running = other.is_kick_vote_running;
            vote_kick_target_session_id = other.vote_kick_target_session_id;
            is_clan_room = other.is_clan_room;
            clan_id_1 = other.clan_id_1;
            clan_id_2 = other.clan_id_2;
        }
        Room& operator=(const Room& other)
        {
            if (this == &other) return *this;
            room_id = other.room_id;
            channel_id = other.channel_id;
            title = other.title;
            password = other.password;
            MapIndex = other.MapIndex;
            RandomMapIndex = other.RandomMapIndex;
            ModeIndex = other.ModeIndex;
            Restriction = other.Restriction;
            TeamBalance = other.TeamBalance;
            max_players = other.max_players;
            score_rule = other.score_rule;
            time_rule = other.time_rule;
            allow_intruders = other.allow_intruders;
            allow_drops = other.allow_drops;
            allow_observers = other.allow_observers;
            is_playing = other.is_playing;
            has_password = other.has_password;
            host_session_id = other.host_session_id;
            neutralteam_session_ids = other.neutralteam_session_ids;
            blueteam_session_ids = other.blueteam_session_ids;
            redteam_session_ids = other.redteam_session_ids;
            observers_session_ids = other.observers_session_ids;
            kicked_session_ids = other.kicked_session_ids;
            kick_voters_session_ids = other.kick_voters_session_ids;
            voters_session_ids = other.voters_session_ids;
            is_kick_vote_running = other.is_kick_vote_running;
            vote_kick_target_session_id = other.vote_kick_target_session_id;
            is_clan_room = other.is_clan_room;
            clan_id_1 = other.clan_id_1;
            clan_id_2 = other.clan_id_2;
            return *this;
        }
    };
    struct Plaza
    {
        std::shared_mutex mutex;
        std::uint16_t plaza_id;
        std::vector<std::uint16_t> session_ids;
        std::uint16_t max_players;
        Plaza(const std::uint16_t& plazaId = 0, const std::uint16_t& maxSize = 2) : plaza_id(plazaId), max_players(maxSize) { session_ids.clear(); }
        Plaza(const Plaza& other)
        {
            plaza_id = other.plaza_id;
            session_ids = other.session_ids;
            max_players = other.max_players;
        }
        Plaza& operator=(const Plaza& other)
        {
            if (this == &other) return *this;
            plaza_id = other.plaza_id;
            session_ids = other.session_ids;
            max_players = other.max_players;
            return *this;
        }
    };

    struct ClanMatch
    {
        std::shared_mutex mutex;
        bool is_playing;
        std::uint32_t room_id;
        std::uint16_t host_session_id;
        std::vector<std::uint16_t> team_sessions;
        ClanMatch(const bool& is_playing = false, const std::uint32_t& room_id = 0, const std::uint16_t& host_session_id = 0) : is_playing(is_playing), room_id(room_id), host_session_id(host_session_id) { team_sessions.clear(); }
        ClanMatch(const ClanMatch& other)
        {
            is_playing = other.is_playing;
            room_id = other.room_id;
            host_session_id = other.host_session_id;
            team_sessions = other.team_sessions;
        }
        ClanMatch& operator=(const ClanMatch& other)
        {
            if (this == &other) return *this;
            is_playing = other.is_playing;
            room_id = other.room_id;
            host_session_id = other.host_session_id;
            team_sessions = other.team_sessions;
            return *this;
        }
    };

    struct Clan
    {
        std::shared_mutex mutex;
        std::uint32_t clan_id;
        std::string clan_name;
        std::uint32_t logo_front;
        std::uint32_t logo_back;
        std::vector<std::uint16_t> online_members;
        std::vector<ClanMatch> clan_matches;
        Clan(const std::uint32_t& clan_id = 0, const std::string& clan_name = "", const std::uint32_t& logo_front = 0, const std::uint32_t& logo_back = 0) : clan_id(clan_id), logo_front(logo_front), logo_back(logo_back) { online_members.clear(); clan_matches.clear(); }
        Clan(const Clan& other)
        {
            clan_id = other.clan_id;
            clan_name = other.clan_name;
            logo_front = other.logo_front;
            logo_back = other.logo_back;
            online_members = other.online_members;
            clan_matches = other.clan_matches;
        }
        Clan& operator=(const Clan& other)
        {
            if (this == &other) return *this;
            clan_id = other.clan_id;
            clan_name = other.clan_name;
            logo_front = other.logo_front;
            logo_back = other.logo_back;
            online_members = other.online_members;
            clan_matches = other.clan_matches;
            return *this;
        }
    };

    struct Party
    {
        std::shared_mutex mutex;
        std::uint32_t party_id;
        bool is_playing;
        bool is_queueing;
        bool is_registered;
        bool is_clan;
        bool has_password;
        std::string password;
        std::uint8_t max_members;
        std::uint16_t clan_id;
        std::uint16_t party_host_session_id;
        std::vector<std::uint16_t> members;
        std::vector<std::uint16_t> kicked_members;
        std::uint16_t mod_id;
        std::uint16_t map_id;
        Party(const bool& is_playing = false, const bool& is_queueing = false, const bool& is_clan = false, const std::uint8_t& max_members = 4, const std::uint16_t& party_host_session_id = 0, const std::uint16_t& clan_id = 0, const std::uint16_t& mod_id = 0, const std::uint16_t& map_id = 0) : is_playing(is_playing), is_queueing(is_queueing), is_clan(is_clan), max_members(max_members), party_host_session_id(party_host_session_id), clan_id(clan_id), mod_id(mod_id), map_id(map_id), is_registered(is_registered) {
            members.clear(); kicked_members.clear();
        }
        Party(const Party& other)
        {
            is_playing = other.is_playing;
            is_queueing = other.is_queueing;
            is_clan = other.is_clan;
            max_members = other.max_members;
            clan_id = other.clan_id;
            mod_id = other.mod_id;
            map_id = other.map_id;
            party_host_session_id = other.party_host_session_id;
            members = other.members;
            kicked_members = other.kicked_members;
            is_registered = other.is_registered;
        }
        Party& operator=(const Party& other)
        {
            if (this == &other) return *this;
            is_playing = other.is_playing;
            is_queueing = other.is_queueing;
            is_clan = other.is_clan;
            party_host_session_id = other.party_host_session_id;
            members = other.members;
            kicked_members = other.kicked_members;
            is_registered = other.is_registered;
            return *this;
        }
    };

    struct MailboxData
    {
        std::shared_mutex mutex;
        std::uint32_t mail_id{};
        std::int32_t sender_account_id{};
        std::string sender_nickname{};
        std::int32_t receiver_account_id{};
        std::string receiver_nickname{};
        std::uint32_t time{};
        std::uint32_t gift_itemid{};
        std::string message{};
        bool is_new{};
        bool deleted_from_sender{};
        bool deleted_from_receiver{};
        MailboxData(const std::uint32_t& mailId = 0, const std::int32_t& senderAccountId = 0, const std::string& senderNickname = "", const std::int32_t& receiverAccountId = 0, const std::string& receiverNickname = "",
            const std::uint32_t& time = 0, const std::uint32_t& giftItemid = 0, const std::string& message = "", const bool& isNew = false, const bool& deletedFromSender = false, const bool& deletedFromReceiver = false)
            : mail_id(mailId), sender_account_id(senderAccountId), sender_nickname(senderNickname), receiver_account_id(receiverAccountId), receiver_nickname(receiverNickname),
            time(time), gift_itemid(giftItemid), message(message), is_new(isNew), deleted_from_sender(deletedFromSender), deleted_from_receiver(deletedFromReceiver) {
        }
        MailboxData(const MailboxData& other)
        {
            mail_id = other.mail_id;
            sender_account_id = other.sender_account_id;
            sender_nickname = other.sender_nickname;
            receiver_account_id = other.receiver_account_id;
            receiver_nickname = other.receiver_nickname;
            time = other.time;
            gift_itemid = other.gift_itemid;
            message = other.message;
            is_new = other.is_new;
            deleted_from_sender = other.deleted_from_sender;
            deleted_from_receiver = other.deleted_from_receiver;  
        }
        MailboxData& operator=(const MailboxData& other)
        {
            if (this == &other) return *this;
            mail_id = other.mail_id;
            sender_account_id = other.sender_account_id;
            sender_nickname = other.sender_nickname;
            receiver_account_id = other.receiver_account_id;
            receiver_nickname = other.receiver_nickname;
            time = other.time;
            gift_itemid = other.gift_itemid;
            message = other.message;
            is_new = other.is_new;
            deleted_from_sender = other.deleted_from_sender;
            deleted_from_receiver = other.deleted_from_receiver;
            return *this;
        }
        MailboxData(const MailboxInfo& other)
        {
            mail_id = other.mail_id;
            sender_account_id = other.sender_account_id;
            sender_nickname = other.sender_nickname;
            receiver_account_id = other.receiver_account_id;
            receiver_nickname = other.receiver_nickname;
            time = other.time;
            gift_itemid = other.gift_itemid;
            message = other.message;
            is_new = other.is_new;
            deleted_from_sender = other.deleted_from_sender;
            deleted_from_receiver = other.deleted_from_receiver;
        }
    };

    extern std::shared_mutex items_info_mutex;
    extern std::shared_mutex effect_info_mutex;
    extern std::shared_mutex collection_info_mutex;
    extern std::shared_mutex dailymission_info_mutex;
    extern std::shared_mutex setitems_info_mutex;
    extern std::shared_mutex vendors_info_mutex;
    extern std::shared_mutex upgrades_info_mutex;
    extern std::shared_mutex gachapons_info_mutex;
    extern std::shared_mutex packages_info_mutex;
    extern std::shared_mutex vendor_item_ids_mutex;
    extern std::shared_mutex dailymission_ids_mutex;
    extern std::shared_mutex roomoptionsinfo_cache_mutex;
    extern std::shared_mutex grades_info_mutex;
    extern std::shared_mutex rewards_info_mutex;
    extern std::shared_mutex friends_cache_mutex;
    extern std::shared_mutex blockeds_cache_mutex;
    extern std::shared_mutex accounts_cache_mutex;
    extern std::shared_mutex rooms_cache_mutex;
    extern std::shared_mutex plaza_cache_mutex;
    extern std::shared_mutex room_ids_mutex;
    extern std::shared_mutex party_ids_mutex;
    extern std::shared_mutex clan_cache_mutex;
    extern std::shared_mutex party_cache_mutex;
    extern std::shared_mutex mailbox_data_cache_mutex;
    extern std::shared_mutex mailbox_sent_cache_mutex;
    extern std::shared_mutex mailbox_recv_cache_mutex;
    extern std::shared_mutex giftbox_recv_cache_mutex;
    extern std::shared_mutex gachapon_sale_cache_mutex;
    extern std::shared_mutex gachapon_ids_sale_cache_mutex;

    


    extern boost::unordered_flat_map<std::uint32_t, BaseLib::ItemInfo> items_info; //read only
    extern boost::unordered_flat_map<std::uint32_t, BaseLib::EffectInfo> effect_info; //read only
    extern boost::unordered_flat_map<std::uint32_t, BaseLib::CollectionInfo> collection_info;
    extern boost::unordered_flat_map<std::uint32_t, BaseLib::DailyMissionInfo> dailymission_info;
    extern boost::unordered_flat_map<std::uint32_t, BaseLib::SetItemInfo> setitems_info; //read only
    extern std::vector<BaseLib::VendorInfo> vendors_info; //read only
    extern boost::unordered_flat_map<std::uint32_t, boost::unordered_flat_map<Items::Upgrade::Type, std::vector<BaseLib::UpgradeInfo>>> upgrades_info; //read only
    extern boost::unordered_flat_map<std::uint32_t, BaseLib::GachaponInfo> gachapons_info; //read only
    extern boost::unordered_flat_map<std::uint32_t, boost::unordered_flat_map<std::uint32_t, std::vector<BaseLib::PackageInfo>>> packages_info; //read only
    extern std::vector<std::uint32_t> vendor_item_ids; //read only
    extern std::vector<std::uint32_t> dailymission_ids; //read only
    extern boost::unordered_flat_map<std::uint32_t, boost::unordered_flat_map<std::uint32_t, std::vector<BaseLib::RoomOptionInfo>>> roomoptionsinfo_cache; //read only
    extern boost::unordered_flat_map<std::uint32_t, BaseLib::GradeInfo> grades_info; //read only
    extern boost::unordered_flat_map<std::uint32_t, BaseLib::RewardInfo> rewards_info; //read only
    extern boost::unordered_flat_map<std::uint32_t, std::vector<BaseLib::FriendInfo>> friends_cache; //read & write
    extern boost::unordered_flat_map<std::uint32_t, std::vector<BaseLib::BlockedInfo>> blockeds_cache; //read & write
    extern boost::unordered_flat_map<std::uint32_t, Player> accounts_cache; //read & write
    extern boost::unordered_flat_map<std::uint32_t, Room> rooms_cache; //read & write
    extern boost::unordered_flat_map<std::uint32_t, Plaza> plaza_cache; //read & write
    extern std::vector<std::uint32_t> room_ids; //read & write
    extern std::vector<std::uint32_t> party_ids; //read & write
    extern boost::unordered_flat_map<std::uint32_t, Clan> clan_cache; //read & write
    extern boost::unordered_flat_map<std::uint16_t, Party> party_cache; //read & write
    extern boost::unordered_flat_map<std::uint32_t, MailboxData> mailbox_data_cache; //read & write access by mail id
    extern boost::unordered_flat_map<std::uint32_t, std::vector<std::uint32_t>> mailbox_sent_cache; //read & write access by acc id, get vector of mail sent mail ids
    extern boost::unordered_flat_map<std::uint32_t, std::vector<std::uint32_t>> mailbox_recv_cache; //read & write access by acc id, get vector of mail recv mail ids
    extern boost::unordered_flat_map<std::uint32_t, std::vector<std::uint32_t>> giftbox_recv_cache; //read & write access by acc id, get vector of mail recv mail ids
    extern boost::unordered_flat_map<std::uint32_t, BaseLib::GachaponSaleInfo> gachapon_sales_info; //read & write access by gachapon id
    extern std::vector<std::uint32_t> gachapon_ids_sale; //read & write

    using AccCacheResource = LockedResource<std::unique_lock<std::shared_mutex>, Player>;
    using AccCacheSharedResource = LockedResource<std::shared_lock<std::shared_mutex>, Player>;
    using RoomCacheResource = LockedResource<std::unique_lock<std::shared_mutex>, Room>;
    using RoomCacheSharedResource = LockedResource<std::shared_lock<std::shared_mutex>, Room>;
    using PlazaCacheResource = LockedResource<std::unique_lock<std::shared_mutex>, Plaza>;
    using PlazaCacheSharedResource = LockedResource<std::shared_lock<std::shared_mutex>, Plaza>;
    using ClanCacheResource = LockedResource<std::unique_lock<std::shared_mutex>, Clan>;
    using ClanCacheSharedResource = LockedResource<std::shared_lock<std::shared_mutex>, Clan>;
    using RoomOptionsCacheResource = LockedResource<std::shared_lock<std::shared_mutex>, boost::unordered_flat_map<std::uint32_t, std::vector<BaseLib::RoomOptionInfo>>>;
    using GachaponCacheResource = LockedResource<std::shared_lock<std::shared_mutex>, BaseLib::GachaponInfo>;
    using PackageCacheResource = LockedResource<std::shared_lock<std::shared_mutex>, boost::unordered_flat_map<std::uint32_t, std::vector<BaseLib::PackageInfo>>>;
    using UpgradeCacheResource = LockedResource<std::shared_lock<std::shared_mutex>, std::vector<BaseLib::UpgradeInfo>>;
    using BlockedCacheResource = LockedResource<std::shared_lock<std::shared_mutex>, std::vector<BaseLib::BlockedInfo>>;
    using FriendCacheResource = LockedResource<std::shared_lock<std::shared_mutex>, std::vector<BaseLib::FriendInfo>>;

    /*
    extern std::unordered_map<std::uint32_t, BaseLib::ItemInfo> items_info; //read only
    extern std::unordered_map<std::uint32_t, BaseLib::SetItemInfo> setitems_info; //read only
    extern std::vector<BaseLib::VendorInfo> vendors_info; //read only
    extern std::unordered_map<std::uint32_t, std::unordered_map<Items::Upgrade::Type, std::vector<BaseLib::UpgradeInfo>>> upgrades_info; //read only
    extern std::unordered_map<std::uint32_t, BaseLib::GachaponInfo> gachapons_info; //read only
    extern std::unordered_map<std::uint32_t, std::unordered_map<std::uint32_t, std::vector<BaseLib::PackageInfo>>> packages_info; //read only
    extern std::vector<std::uint32_t> vendor_item_ids; //read only
    extern std::unordered_map<std::uint32_t, std::unordered_map<std::uint32_t, std::vector<BaseLib::RoomOptionInfo>>> roomoptionsinfo_cache; //read only
    extern std::unordered_map<std::uint32_t, BaseLib::GradeInfo> grades_info; //read only
    extern std::unordered_map<std::uint32_t, BaseLib::RewardInfo> rewards_info; //read only
    extern std::unordered_map<std::uint32_t, std::vector<BaseLib::FriendInfo>> friends_cache; //read & write
    extern std::unordered_map<std::uint32_t, std::vector<BaseLib::BlockedInfo>> blockeds_cache; //read & write
    extern std::unordered_map<std::uint32_t, Player> accounts_cache; //read & write
    extern std::unordered_map<std::uint32_t, Room> rooms_cache; //read & write
    extern std::unordered_map<std::uint32_t, Plaza> plaza_cache; //read & write
    extern std::vector<std::uint32_t> room_ids; //read & write 

    using AccCacheResource = LockedResource<std::unique_lock<std::shared_mutex>, Player>;
    using AccCacheSharedResource = LockedResource<std::shared_lock<std::shared_mutex>, Player>;
    using RoomCacheResource = LockedResource<std::unique_lock<std::shared_mutex>, Room>;
    using RoomCacheSharedResource = LockedResource<std::shared_lock<std::shared_mutex>, Room>;
    using PlazaCacheResource = LockedResource<std::unique_lock<std::shared_mutex>, Plaza>;
    using PlazaCacheSharedResource = LockedResource<std::shared_lock<std::shared_mutex>, Plaza>;
    using RoomOptionsCacheResource = LockedResource<std::shared_lock<std::shared_mutex>, std::unordered_map<std::uint32_t, std::vector<BaseLib::RoomOptionInfo>>>;
    using GachaponCacheResource = LockedResource<std::shared_lock<std::shared_mutex>, BaseLib::GachaponInfo>;
    using PackageCacheResource = LockedResource<std::shared_lock<std::shared_mutex>, std::unordered_map<std::uint32_t, std::vector<BaseLib::PackageInfo>>>;
    using UpgradeCacheResource = LockedResource<std::shared_lock<std::shared_mutex>, std::vector<BaseLib::UpgradeInfo>>;
    using BlockedCacheResource = LockedResource<std::shared_lock<std::shared_mutex>, std::vector<BaseLib::BlockedInfo>>;
    using FriendCacheResource = LockedResource<std::shared_lock<std::shared_mutex>, std::vector<BaseLib::FriendInfo>>;
    */

    class CMainServer : public NetEngine::CServer
    {
    public:
        CMainServer();
        ~CMainServer();

        auto GetOwnedCharacters(const std::vector<Character::Type>& ownedCharacters)
        {
            std::uint32_t owned = 0;
            for (const auto& character : ownedCharacters) owned |= 1 << static_cast<std::uint32_t>(character);
            return owned;
        }
        auto IsSessionIdAlready(const std::uint16_t& session_id, const std::vector<std::uint16_t>& session_ids)
        {
            auto findit = std::find(session_ids.begin(), session_ids.end(), session_id);
            return findit != session_ids.end();
        }

        void AddAccCache(const std::uint16_t& id, Player front_acc)
        {
            auto accounts_locked = LockedResource{ std::unique_lock(accounts_cache_mutex), accounts_cache };
            front_acc.session_id = id;
            auto [it, inserted] = accounts_locked->emplace(id, std::move(front_acc));

            if (!inserted)
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "Attempted to add a player with session id: ({}), but it already exists ", id);
        }
        void RemoveAccCache(const std::uint16_t& id)
        {
            auto accounts_locked = LockedResource{ std::unique_lock(accounts_cache_mutex), accounts_cache };
            accounts_locked->erase(id);
        }
        auto GetAccCacheSharedBySessionId(const std::uint16_t& session_id)
        {
            std::shared_lock lock(accounts_cache_mutex);
            auto it = accounts_cache.find(session_id);
            if (it != accounts_cache.end())
                return LockedResource{ std::shared_lock(it->second.mutex), it->second };
            else
            {
                static thread_local std::shared_mutex null_player_mutex;
                static thread_local Player null_player;
                return LockedResource{ std::shared_lock(null_player_mutex), null_player };
            }
        }
        auto GetAccCacheUniqueBySessionId(const std::uint16_t& session_id)
        {
            std::shared_lock lock(accounts_cache_mutex);
            auto it = accounts_cache.find(session_id);
            if (it != accounts_cache.end())
                return LockedResource{ std::unique_lock(it->second.mutex), it->second };
            else
            {
                static thread_local std::shared_mutex null_player_mutex;
                static thread_local Player null_player;
                return LockedResource{ std::unique_lock(null_player_mutex), null_player };
            }
        }
        auto GetAccCacheSharedByAccountId(const std::uint32_t& account_id)
        {
            std::shared_lock lock(accounts_cache_mutex);

            auto findit = accounts_cache.begin();
            findit = std::find_if(accounts_cache.begin(), accounts_cache.end(),
                [&](const auto& pair) { return pair.second.acc_info.Index == account_id; });

            if (findit != accounts_cache.end())
                return LockedResource{ std::shared_lock(findit->second.mutex), findit->second };
            else
            {
                static thread_local std::shared_mutex null_player_mutex;
                static thread_local Player null_player;
                return LockedResource{ std::shared_lock(null_player_mutex), null_player };
            }
        }
        auto GetAccCacheSharedByAuthKey(const std::uint64_t& auth_key)
        {
            std::shared_lock lock(accounts_cache_mutex);

            auto findit = accounts_cache.begin();
            findit = std::find_if(accounts_cache.begin(), accounts_cache.end(),
                [&](const auto& pair) { return pair.second.acc_info.AuthKey == auth_key; });

            if (findit != accounts_cache.end())
                return LockedResource{ std::shared_lock(findit->second.mutex), findit->second };
            else
            {
                static thread_local std::shared_mutex null_player_mutex;
                static thread_local Player null_player;
                return LockedResource{ std::shared_lock(null_player_mutex), null_player };
            }
        }
        auto GetAccCacheSharedByNickname(const std::string& nickname)
        {
            std::shared_lock lock(accounts_cache_mutex);

            auto findit = accounts_cache.begin();
            findit = std::find_if(accounts_cache.begin(), accounts_cache.end(), [&](const auto& pair)
            {
                return Utility::ToLowercase(pair.second.acc_info.Nickname) == Utility::ToLowercase(nickname);
            });

            if (findit != accounts_cache.end())
                return LockedResource{ std::shared_lock(findit->second.mutex), findit->second };
            else
            {
                static thread_local std::shared_mutex null_player_mutex;
                static thread_local Player null_player;
                return LockedResource{ std::shared_lock(null_player_mutex), null_player };
            }
        }
       
        auto GetAccCacheUniqueByAccountId(const std::uint32_t& account_id)
        {
            std::shared_lock lock(accounts_cache_mutex);

            auto findit = accounts_cache.begin();
            findit = std::find_if(accounts_cache.begin(), accounts_cache.end(),
                [&](const auto& pair) { return pair.second.acc_info.Index == account_id; });

            if (findit != accounts_cache.end())
                return LockedResource{ std::unique_lock(findit->second.mutex), findit->second };
            else
            {
                static thread_local std::shared_mutex null_player_mutex;
                static thread_local Player null_player;
                return LockedResource{ std::unique_lock(null_player_mutex), null_player };
            }
        }
        auto GetAccCacheUniqueByAuthKey(const std::uint64_t& auth_key)
        {
            std::shared_lock lock(accounts_cache_mutex);

            auto findit = accounts_cache.begin();
            findit = std::find_if(accounts_cache.begin(), accounts_cache.end(),
                [&](const auto& pair) { return pair.second.acc_info.AuthKey == auth_key; });

            if (findit != accounts_cache.end())
                return LockedResource{ std::unique_lock(findit->second.mutex), findit->second };
            else
            {
                static thread_local std::shared_mutex null_player_mutex;
                static thread_local Player null_player;
                return LockedResource{ std::unique_lock(null_player_mutex), null_player };
            }
        }
        auto GetAccCacheUniqueByNickname(const std::string& nickname)
        {
            std::shared_lock lock(accounts_cache_mutex);

            auto findit = accounts_cache.begin();
            findit = std::find_if(accounts_cache.begin(), accounts_cache.end(), [&](const auto& pair)
            {
                return Utility::ToLowercase(pair.second.acc_info.Nickname) == Utility::ToLowercase(nickname);
            });

            if (findit != accounts_cache.end())
                return LockedResource{ std::unique_lock(findit->second.mutex), findit->second };
            else
            {
                static thread_local std::shared_mutex null_player_mutex;
                static thread_local Player null_player;
                return LockedResource{ std::unique_lock(null_player_mutex), null_player };
            }
        }
        auto GetAccCacheSize()
        {
            std::shared_lock lock(accounts_cache_mutex);
            return accounts_cache.size();
        }

        auto AddPlayerItemInventory(AccCacheResource& acc_cache, const Item& new_item)
        {
            acc_cache->inventory_items.push_back(new_item);
            acc_cache->items_added.push_back(new_item.item_info.serial_info.data);
            return true;
        }
        auto AddPlayerItemInventory(AccCacheResource& acc_cache, const std::vector<Item>& new_items)
        {
            acc_cache->inventory_items.insert(acc_cache->inventory_items.end(), new_items.begin(), new_items.end());
            std::transform(new_items.begin(), new_items.end(), std::back_inserter(acc_cache->items_added),
                [](const Item& item) { return item.item_info.serial_info.data; });

            return true;
        }
        auto RemovePlayerItemInventory(AccCacheResource& acc_cache, const ItemSerialInfo& serial_info)
        {
            auto new_end = std::remove_if(acc_cache->inventory_items.begin(), acc_cache->inventory_items.end(),
                [&serial_info](const Item& item) {
                return item.item_info.serial_info.data == serial_info.data;
            });

            bool item_removed = (new_end != acc_cache->inventory_items.end());
            acc_cache->inventory_items.erase(new_end, acc_cache->inventory_items.end());

            return item_removed;
        }
        auto GetRoomCacheShared(const std::uint32_t& room_id)
        {
            std::shared_lock lock(rooms_cache_mutex);
            auto it = rooms_cache.find(room_id);
            if (it != rooms_cache.end())
                return LockedResource{ std::shared_lock(it->second.mutex), it->second };
            else
            {
                static thread_local std::shared_mutex null_room_mutex;
                static thread_local Room null_room;
                null_room.room_id = 0;
                null_room.title = "";
                return LockedResource{ std::shared_lock(null_room_mutex), null_room };
            }
        }
        auto GetRoomCacheUnique(const std::uint32_t& room_id)
        {
            std::shared_lock lock(rooms_cache_mutex);
            auto it = rooms_cache.find(room_id);
            if (it != rooms_cache.end())
                return LockedResource{ std::unique_lock(it->second.mutex), it->second };
            else
            {
                static thread_local std::shared_mutex null_room_mutex;
                static thread_local Room null_room;
                null_room.room_id = 0;
                null_room.title = "";
                return LockedResource{ std::unique_lock(null_room_mutex), null_room };
            }
        }
        auto GetPlazaCacheShared(const std::uint32_t& plaza_id)
        {
            std::shared_lock lock(plaza_cache_mutex);
            auto it = plaza_cache.find(plaza_id);
            if (it != plaza_cache.end())
                return LockedResource{ std::shared_lock(it->second.mutex), it->second };
            else
            {
                static thread_local std::shared_mutex null_plaza_mutex;
                static thread_local Plaza null_plaza;
                return LockedResource{ std::shared_lock(null_plaza_mutex), null_plaza };
            }
        }
        auto GetPlazaCacheUnique(const std::uint32_t& plaza_id)
        {
            std::shared_lock lock(plaza_cache_mutex);
            auto it = plaza_cache.find(plaza_id);
            if (it != plaza_cache.end())
                return LockedResource{ std::unique_lock(it->second.mutex), it->second };
            else
            {
                static thread_local std::shared_mutex null_plaza_mutex;
                static thread_local Plaza null_plaza;
                return LockedResource{ std::unique_lock(null_plaza_mutex), null_plaza };
            }
        }

        auto GetClanCacheShared(const std::uint32_t& clan_id)
        {
            std::shared_lock lock(clan_cache_mutex);
            auto it = clan_cache.find(clan_id);
            if (it != clan_cache.end())
                return LockedResource{ std::shared_lock(it->second.mutex), it->second };
            else
            {
                static thread_local std::shared_mutex null_clan_mutex;
                static thread_local Clan null_clan;
                return LockedResource{ std::shared_lock(null_clan_mutex), null_clan };
            }
        }

        auto GetClanCacheUnique(const std::uint32_t& clan_id)
        {
            std::shared_lock lock(clan_cache_mutex);
            auto it = clan_cache.find(clan_id);
            if (it != clan_cache.end())
                return LockedResource{ std::unique_lock(it->second.mutex), it->second };
            else
            {
                static thread_local std::shared_mutex null_clan_mutex;
                static thread_local Clan null_clan;
                return LockedResource{ std::unique_lock(null_clan_mutex), null_clan };
            }
        }

        auto IsClanAlready(const std::uint32_t& clan_id)
        {
            std::shared_lock lock(clan_cache_mutex);
            if (auto findit = clan_cache.find(clan_id); findit != clan_cache.end())
                return true;
            else
                return false;
        }
        void AddClanCache(const std::uint32_t& clan_id, const Clan& new_clan)
        {
            if (!IsClanAlready(clan_id))
            {
                auto locked_clan_cache = LockedResource{ std::unique_lock(clan_cache_mutex), clan_cache };
                locked_clan_cache->emplace(clan_id, new_clan);
            }
        }
        void RemoveClanCache(const std::uint32_t& clan_id)
        {
            if (IsClanAlready(clan_id))
            {
                auto locked_clan_cache = LockedResource{ std::unique_lock(clan_cache_mutex), clan_cache };
                locked_clan_cache->erase(clan_id);
            }
        }

        auto GetPartyCacheShared(const std::uint32_t& party_id)
        {
            std::shared_lock lock(party_cache_mutex);
            auto it = party_cache.find(party_id);
            if (it != party_cache.end())
                return LockedResource{ std::shared_lock(it->second.mutex), it->second };
            else
            {
                static thread_local std::shared_mutex null_party_mutex;
                static thread_local Party null_party;
                return LockedResource{ std::shared_lock(null_party_mutex), null_party };
            }
        }

        auto GetPartyCacheUnique(const std::uint32_t& party_id)
        {
            std::shared_lock lock(party_cache_mutex);
            auto it = party_cache.find(party_id);
            if (it != party_cache.end())
                return LockedResource{ std::unique_lock(it->second.mutex), it->second };
            else
            {
                static thread_local std::shared_mutex null_party_mutex;
                static thread_local Party null_party;
                return LockedResource{ std::unique_lock(null_party_mutex), null_party };
            }
        }

        auto IsPartyAlready(const std::uint32_t& party_id)
        {
            std::shared_lock lock(party_cache_mutex);
            if (auto findit = party_cache.find(party_id); findit != party_cache.end())
                return true;
            else
                return false;
        }

        void AddPartyCache(const std::uint32_t& party_id, const Party& new_party)
        {
            if (!IsPartyAlready(party_id))
            {
                auto locked_party_cache = LockedResource{ std::unique_lock(party_cache_mutex), party_cache };
                locked_party_cache->emplace(party_id, new_party);
            }
        }

        void RemovePartyCache(const std::uint32_t& party_id)
        {
            if (IsPartyAlready(party_id))
            {
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "will remove party cache: ({})", party_id);
                auto party_cache_locked = LockedResource{ std::unique_lock(party_cache_mutex), party_cache };
                auto party_ids_locked = LockedResource{ std::unique_lock(party_ids_mutex), party_ids };

                party_cache_locked->erase(party_id);
                party_ids_locked->erase(std::remove(party_ids_locked->begin(), party_ids_locked->end(), party_id), party_ids_locked->end());
            }
        }

        auto IsGachaponSaleInfoAlready(const std::uint32_t& gachapon_id)
        {
            std::shared_lock lock(gachapon_sale_cache_mutex);
            if (auto findit = gachapon_sales_info.find(gachapon_id); findit != gachapon_sales_info.end())
                return true;
            else
                return false;
        }

        auto GetGachaponSaleCacheShared(const std::uint32_t& gachapon_id)
        {
            std::shared_lock lock(gachapon_sale_cache_mutex);
            auto it = gachapon_sales_info.find(gachapon_id);
            if (it != gachapon_sales_info.end())
                return LockedResource{ std::shared_lock(gachapon_sale_cache_mutex), it->second };
            else
            {
                static thread_local std::shared_mutex null_gachapon_sale_cache_mutex;
                static thread_local GachaponSaleInfo null_gachapon_sale_info;
                return LockedResource{ std::shared_lock(null_gachapon_sale_cache_mutex), null_gachapon_sale_info };
            }
        }

        auto GetGachaponSaleCacheUnique(const std::uint32_t& gachapon_id)
        {
            std::shared_lock lock(gachapon_sale_cache_mutex);
            auto it = gachapon_sales_info.find(gachapon_id);
            if (it != gachapon_sales_info.end())
                return LockedResource{ std::unique_lock(gachapon_sale_cache_mutex), it->second };
            else
            {
                static thread_local std::shared_mutex null_gachapon_sale_cache_mutex;
                static thread_local GachaponSaleInfo null_gachapon_sale_info;
                return LockedResource{ std::unique_lock(null_gachapon_sale_cache_mutex), null_gachapon_sale_info };
            }
        }

        auto GetGachaponSaleIdsCacheShared()
        {
            std::shared_lock lock(gachapon_ids_sale_cache_mutex);
            return LockedResource{ std::shared_lock(gachapon_ids_sale_cache_mutex), gachapon_ids_sale };
        }

        void AddGachaponSaleCache(const std::vector<GachaponSaleInfo>& new_gachapon_sales)
        {
           
            for (const auto& sale_info : new_gachapon_sales)
            {
                const auto& gachapon_id = sale_info.gachapon_id;
                if (!IsGachaponSaleInfoAlready(gachapon_id))
                {
                    auto locked_gachapon_sale_cache = LockedResource{ std::unique_lock(gachapon_sale_cache_mutex), gachapon_sales_info };
                    auto gacha_ids_locked = LockedResource{ std::unique_lock(gachapon_ids_sale_cache_mutex), gachapon_ids_sale };
                    
                    locked_gachapon_sale_cache->emplace(gachapon_id, sale_info);
                    gacha_ids_locked->push_back(gachapon_id);   
                }
                    
            }
        }
        void AddGachaponSaleCache(const std::uint32_t& gachapon_id, const GachaponSaleInfo& new_gachapon_sale)
        {
            if (!IsGachaponSaleInfoAlready(gachapon_id))
            {
                auto locked_gachapon_sale_cache = LockedResource{ std::unique_lock(gachapon_sale_cache_mutex), gachapon_sales_info };
                locked_gachapon_sale_cache->emplace(gachapon_id, new_gachapon_sale);
            }
        }
        void ClearGachaponSaleCache()
        {
            auto locked_gachapon_sale_cache = LockedResource{ std::unique_lock(gachapon_sale_cache_mutex), gachapon_sales_info };
            auto gacha_ids_locked = LockedResource{ std::unique_lock(gachapon_ids_sale_cache_mutex), gachapon_ids_sale };
            locked_gachapon_sale_cache->clear();
            gacha_ids_locked->clear();
        }
        void RemoveGachaponSaleCache(const std::uint32_t& gachapon_id)
        {
            if (IsGachaponSaleInfoAlready(gachapon_id))
            {
                auto locked_gachapon_sale_cache = LockedResource{ std::unique_lock(gachapon_sale_cache_mutex), gachapon_sales_info };
                auto gacha_ids_locked = LockedResource{ std::unique_lock(gachapon_ids_sale_cache_mutex), gachapon_ids_sale };
                locked_gachapon_sale_cache->erase(gachapon_id);
                gacha_ids_locked->erase(std::remove(gacha_ids_locked->begin(), gacha_ids_locked->end(), gachapon_id), gacha_ids_locked->end());
                BaseLib::Database->DeleteGachaponSaleInfo(gachapon_id);
            }
        }


        auto GetMailboxDataCacheShared(const std::uint32_t& mail_id)
        {
            std::shared_lock lock(mailbox_data_cache_mutex);
            auto it = mailbox_data_cache.find(mail_id);
            if (it != mailbox_data_cache.end())
                return LockedResource{ std::shared_lock(it->second.mutex), it->second };
            else
            {
                static thread_local std::shared_mutex null_mailbox_data_mutex;
                static thread_local MailboxData null_mailbox;
                return LockedResource{ std::shared_lock(null_mailbox_data_mutex), null_mailbox };
            }
        }

        auto GetMailboxDataCacheUnique(const std::uint32_t& mail_id)
        {
            std::shared_lock lock(mailbox_data_cache_mutex);
            auto it = mailbox_data_cache.find(mail_id);
            if (it != mailbox_data_cache.end())
                return LockedResource{ std::unique_lock(it->second.mutex), it->second };
            else
            {
                static thread_local std::shared_mutex null_mailbox_data_mutex;
                static thread_local MailboxData null_mailbox;
                return LockedResource{ std::unique_lock(null_mailbox_data_mutex), null_mailbox };
            }
        }

        auto IsMailboxDataAlready(const std::uint32_t& mail_id)
        {
            std::shared_lock lock(mailbox_data_cache_mutex);
            if (auto findit = mailbox_data_cache.find(mail_id); findit != mailbox_data_cache.end())
                return true;
            else
                return false;
        }

        void AddMailboxDataCache(const std::uint32_t& mail_id, const MailboxData& new_mailbox)
        {
            if (!IsMailboxDataAlready(mail_id))
            {
                auto locked_mailbox_data_cache = LockedResource{ std::unique_lock(mailbox_data_cache_mutex), mailbox_data_cache };
                locked_mailbox_data_cache->emplace(mail_id, new_mailbox);
            }
        }
        void RemoveMailboxDataCache(const std::uint32_t& mail_id)
        {
            if (IsMailboxDataAlready(mail_id))
            {
                auto locked_mailbox_data_cache = LockedResource{ std::unique_lock(mailbox_data_cache_mutex), mailbox_data_cache };
                locked_mailbox_data_cache->erase(mail_id);
            }
        }

        auto GetMailboxSentCacheShared(const std::uint32_t& acc_id)
        {
            std::shared_lock lock(mailbox_sent_cache_mutex);
            auto it = mailbox_sent_cache.find(acc_id);
            if (it != mailbox_sent_cache.end())
                return LockedResource{ std::shared_lock(mailbox_sent_cache_mutex), it->second };
            else
            {
                static thread_local std::shared_mutex null_mailbox_sent_mutex;
                static thread_local std::vector<std::uint32_t> null_mail_ids;
                return LockedResource{ std::shared_lock(null_mailbox_sent_mutex), null_mail_ids };
            }
        }

        auto IsMailboxSentAlready(const std::vector<std::uint32_t>& mail_ids, std::uint32_t mail_id)
        {
            auto it = std::find_if(mail_ids.begin(), mail_ids.end(),
                [&mail_id](const std::uint32_t& current_mail_id) {
                return current_mail_id == mail_id;
            });
            return it != mail_ids.end();
        }
        auto AddMailboxSentIdCache(const std::uint32_t& id, const std::uint32_t& mail_id)
        {
            auto mailbox_sent_cache_locked = LockedResource{ std::unique_lock(mailbox_sent_cache_mutex), mailbox_sent_cache };
            auto& mails_list = (*mailbox_sent_cache_locked)[id];
            mails_list.insert(mails_list.end(), mail_id);
        }
        auto AddMailboxSentIdsCache(const std::uint32_t& id, const std::vector<std::uint32_t>& mail_ids)
        {
            auto mailbox_sent_cache_locked = LockedResource{ std::unique_lock(mailbox_sent_cache_mutex), mailbox_sent_cache };
            auto& mails_list = (*mailbox_sent_cache_locked)[id];
            mails_list.insert(mails_list.end(), mail_ids.begin(), mail_ids.end());
        }
        auto RemoveMailboxSentIdCache(const std::uint32_t& id, const std::uint32_t& mail_id)
        {
            auto mailbox_sent_cache_locked = LockedResource{ std::unique_lock(mailbox_sent_cache_mutex), mailbox_sent_cache };
            auto& mails_list = (*mailbox_sent_cache_locked)[id];
            mails_list.erase(std::remove(mails_list.begin(), mails_list.end(), mail_id), mails_list.end());
        }
        auto RemoveMailboxSentCache(const std::uint32_t& id)
        {
            auto mailbox_sent_cache_locked = LockedResource{ std::unique_lock(mailbox_sent_cache_mutex), mailbox_sent_cache };
            mailbox_sent_cache_locked->erase(id);
        }


        auto GetMailboxRecvCacheShared(const std::uint32_t& acc_id)
        {
            std::shared_lock lock(mailbox_recv_cache_mutex);
            auto it = mailbox_recv_cache.find(acc_id);
            if (it != mailbox_recv_cache.end())
                return LockedResource{ std::shared_lock(mailbox_recv_cache_mutex), it->second };
            else
            {
                static thread_local std::shared_mutex null_mailbox_recv_mutex;
                static thread_local std::vector<std::uint32_t> null_mail_ids;
                return LockedResource{ std::shared_lock(null_mailbox_recv_mutex), null_mail_ids };
            }
        }
        auto GetGiftboxRecvCacheShared(const std::uint32_t& acc_id)
        {
            std::shared_lock lock(giftbox_recv_cache_mutex);
            auto it = giftbox_recv_cache.find(acc_id);
            if (it != giftbox_recv_cache.end())
                return LockedResource{ std::shared_lock(giftbox_recv_cache_mutex), it->second };
            else
            {
                static thread_local std::shared_mutex null_giftbox_recv_mutex;
                static thread_local std::vector<std::uint32_t> null_gifts_ids;
                return LockedResource{ std::shared_lock(null_giftbox_recv_mutex), null_gifts_ids };
            }
        }

        std::size_t GetMailboxRecvCount(const std::uint32_t& acc_id)
        {
            std::shared_lock lock(mailbox_recv_cache_mutex);
            auto it = mailbox_recv_cache.find(acc_id);
            if (it != mailbox_recv_cache.end())
                return it->second.size();
      
            return 0;
        }

        std::size_t GetGiftboxRecvCount(const std::uint32_t& acc_id)
        {
            std::shared_lock lock(giftbox_recv_cache_mutex);
            auto it = giftbox_recv_cache.find(acc_id);
            if (it != giftbox_recv_cache.end())
                return it->second.size();

            return 0;
        }


        std::size_t GetMailboxSentCount(const std::uint32_t& acc_id)
        {
            std::shared_lock lock(mailbox_sent_cache_mutex);
            auto it = mailbox_sent_cache.find(acc_id);
            if (it != mailbox_sent_cache.end())
                return it->second.size();

            return 0;
        }


        auto IsMailboxRecvAlready(const std::vector<std::uint32_t>& mail_ids, std::uint32_t mail_id)
        {
            auto it = std::find_if(mail_ids.begin(), mail_ids.end(),
                [&mail_id](const std::uint32_t& current_mail_id) {
                return current_mail_id == mail_id;
            });
            return it != mail_ids.end();
        }
        auto AddGiftboxRecvIdCache(const std::uint32_t& id, const std::uint32_t& mail_id)
        {
            auto giftbox_recv_cache_locked = LockedResource{ std::unique_lock(giftbox_recv_cache_mutex), giftbox_recv_cache };
            auto& gifts_list = (*giftbox_recv_cache_locked)[id];
            gifts_list.insert(gifts_list.end(), mail_id);
        }
        auto AddMailboxRecvIdCache(const std::uint32_t& id, const std::uint32_t& mail_id)
        {
            auto mailbox_recv_cache_locked = LockedResource{ std::unique_lock(mailbox_recv_cache_mutex), mailbox_recv_cache };
            auto& mails_list = (*mailbox_recv_cache_locked)[id];
            mails_list.insert(mails_list.end(), mail_id);
        }
        auto AddMailboxRecvIdsCache(const std::uint32_t& id, const std::vector<std::uint32_t>& mail_ids)
        {
            auto mailbox_recv_cache_locked = LockedResource{ std::unique_lock(mailbox_recv_cache_mutex), mailbox_recv_cache };
            auto& mails_list = (*mailbox_recv_cache_locked)[id];
            mails_list.insert(mails_list.end(), mail_ids.begin(), mail_ids.end());
        }
        auto RemoveMailboxRecvIdCache(const std::uint32_t& id, const std::uint32_t& mail_id)
        {
            auto mailbox_recv_cache_locked = LockedResource{ std::unique_lock(mailbox_recv_cache_mutex), mailbox_recv_cache };
            auto& mails_list = (*mailbox_recv_cache_locked)[id];
            mails_list.erase(std::remove(mails_list.begin(), mails_list.end(), mail_id), mails_list.end());
        }
        auto RemoveGiftboxRecvIdCache(const std::uint32_t& id, const std::uint32_t& mail_id)
        {
            auto giftbox_recv_cache_locked = LockedResource{ std::unique_lock(giftbox_recv_cache_mutex), giftbox_recv_cache };
            auto& gifts_list = (*giftbox_recv_cache_locked)[id];
            gifts_list.erase(std::remove(gifts_list.begin(), gifts_list.end(), mail_id), gifts_list.end());
        }
        auto RemoveMailboxRecvCache(const std::uint32_t& id)
        {
            auto mailbox_recv_cache_locked = LockedResource{ std::unique_lock(mailbox_recv_cache_mutex), mailbox_recv_cache };
            mailbox_recv_cache_locked->erase(id);
        }
        auto RemoveGiftboxRecvCache(const std::uint32_t& id)
        {
            auto giftbox_recv_cache_locked = LockedResource{ std::unique_lock(giftbox_recv_cache_mutex), giftbox_recv_cache };
            giftbox_recv_cache_locked->erase(id);
        }



        auto FindFirstNonFullPlaza()
        {
            auto locked_plaza_cache = LockedResource{ std::shared_lock(plaza_cache_mutex), plaza_cache };

            for (auto [plaza_id, plaza] : *locked_plaza_cache)
            {
                if (plaza.session_ids.size() < plaza.max_players)
                    return plaza_id;
            }
            return std::numeric_limits<std::uint32_t>::max();
        }
        auto IsPlazaFull(PlazaCacheSharedResource& plaza_cache)
        {
            return plaza_cache->session_ids.size() >= plaza_cache->max_players;
        }
        auto IsPlazaFull(PlazaCacheResource& plaza_cache)
        {
            return plaza_cache->session_ids.size() >= plaza_cache->max_players;
        }
        auto IsPlazaBroadcastable(PlazaCacheSharedResource& plaza_cache)
        {
            return plaza_cache->session_ids.size() >= 2;
        }
        auto IsPlazaBroadcastable(PlazaCacheResource& plaza_cache)
        {
            return plaza_cache->session_ids.size() >= 2;
        }
        auto IsPlazaAlready(const std::uint32_t& plaza_id)
        {
            std::shared_lock lock(plaza_cache_mutex);
            if (auto findit = plaza_cache.find(plaza_id); findit != plaza_cache.end())
                return true;
            else
                return false;
        }
        void AddPlazaCache(const std::uint32_t& plaza_id, const Plaza& new_plaza)
        {
            if (!IsPlazaAlready(plaza_id))
            {
                auto locked_plaza_cache = LockedResource{ std::unique_lock(plaza_cache_mutex), plaza_cache };
                locked_plaza_cache->emplace(plaza_id, new_plaza);
            }
        }
        void RemovePlazaCache(const std::uint32_t& plaza_id)
        {
            if (IsPlazaAlready(plaza_id))
            {
                auto locked_plaza_cache = LockedResource{ std::unique_lock(plaza_cache_mutex), plaza_cache };
                locked_plaza_cache->erase(plaza_id);
            }
        }

        auto IsRoomAlready(const std::uint32_t& room_id)
        {
            std::shared_lock lock(rooms_cache_mutex);
            if (auto findit = rooms_cache.find(room_id); findit != rooms_cache.end())
                return true;
            else
                return false;
        }
       
        
        std::uint32_t FindLowestAvailableItemSerialInfoId(const std::vector<Item>& inventory_items)
        {
            std::unordered_set<std::uint32_t> used_ids;

            for (const auto& item : inventory_items)
                if (item.item_info.serial_info.id >= 0 && item.item_info.serial_info.id <= 0x100000)
                    used_ids.insert(item.item_info.serial_info.id);

            for (std::uint32_t id = 0; id <= 0x100000; id++)
                if (used_ids.find(id) == used_ids.end())
                    return id;

            return -1;
        }
        auto IsModeTeamBased(const NetEngine::Room::Mode::Index& mode)
        {
            using namespace NetEngine::Room::Mode;

            return mode == TeamDeathMatch || mode == ItemMatch
                || mode == CaptureTheBattery || mode == CloseCombat
                || mode == Elimination || mode == SuperItemMatch
                || mode == Scrimmage || mode == BombBattle
                || mode == CLAN_CaptureTheBattery || mode == CLAN_Elimination
                || mode == CLAN_TeamDeathMatch;
        }
        auto GetRoomSortedPlayerSessionIds(RoomCacheSharedResource& room_cache)
        {
            std::vector<std::pair<std::uint16_t, std::uint32_t>> player_slot_pairs;

            auto addPlayerToSlotPairs = [&](const std::vector<std::uint16_t>& team_session_ids)
            {
                for (const auto& id : team_session_ids)
                {
                    auto player_cache = GetAccCacheSharedBySessionId(id);
                    if (player_cache->acc_info.Index != -1 && player_cache->in_room && player_cache->room_id == room_cache->room_id)
                        player_slot_pairs.emplace_back(id, player_cache->slot_id);

                    player_cache.unlock();
                }
            };
            if (IsModeTeamBased(room_cache->ModeIndex))
            {
                addPlayerToSlotPairs(room_cache->blueteam_session_ids);
                addPlayerToSlotPairs(room_cache->redteam_session_ids);
            }
            else
                addPlayerToSlotPairs(room_cache->neutralteam_session_ids);

            addPlayerToSlotPairs(room_cache->observers_session_ids);

            std::stable_sort(player_slot_pairs.begin(), player_slot_pairs.end(),
                [](const std::pair<std::uint16_t, std::uint32_t>& a, const std::pair<std::uint16_t, std::uint32_t>& b) {
                return a.second < b.second;
            });

            std::vector<std::uint16_t> players_ids;
            for (const auto& pair : player_slot_pairs)
                players_ids.push_back(pair.first);

            return players_ids;
        }
        auto GetRoomSortedPlayerSessionIds(RoomCacheResource& room_cache)
        {
            std::vector<std::pair<std::uint16_t, std::uint32_t>> player_slot_pairs;

            auto addPlayerToSlotPairs = [&](const std::vector<std::uint16_t>& team_session_ids) 
            {
                for (const auto& id : team_session_ids)
                {
                    auto player_cache = GetAccCacheSharedBySessionId(id);
                    if (player_cache->acc_info.Index != -1 && player_cache->in_room && player_cache->room_id == room_cache->room_id)
                        player_slot_pairs.emplace_back(id, player_cache->slot_id);

                    player_cache.unlock();
                }
            };
            if (IsModeTeamBased(room_cache->ModeIndex))
            {
                addPlayerToSlotPairs(room_cache->blueteam_session_ids);
                addPlayerToSlotPairs(room_cache->redteam_session_ids);
            }
            else
                addPlayerToSlotPairs(room_cache->neutralteam_session_ids);

            addPlayerToSlotPairs(room_cache->observers_session_ids);

            std::stable_sort(player_slot_pairs.begin(), player_slot_pairs.end(),
                [](const std::pair<std::uint16_t, std::uint32_t>& a, const std::pair<std::uint16_t, std::uint32_t>& b) {
                return a.second < b.second;
            });

            std::vector<std::uint16_t> players_ids;
            for (const auto& pair : player_slot_pairs)
                players_ids.push_back(pair.first);

            return players_ids;
        }
        auto GetRoomSortedPlayerWithoutObserverSessionIds(RoomCacheResource& room_cache)
        {
            std::vector<std::pair<std::uint16_t, std::uint32_t>> player_slot_pairs;

            auto addPlayerToSlotPairs = [&](const std::vector<std::uint16_t>& team_session_ids)
            {
                for (const auto& id : team_session_ids)
                {
                    auto player_cache = GetAccCacheSharedBySessionId(id);
                    if (player_cache->acc_info.Index != -1 && player_cache->in_room && player_cache->room_id == room_cache->room_id)
                        player_slot_pairs.emplace_back(id, player_cache->slot_id);

                    player_cache.unlock();
                }
            };
            if (IsModeTeamBased(room_cache->ModeIndex))
            {
                addPlayerToSlotPairs(room_cache->blueteam_session_ids);
                addPlayerToSlotPairs(room_cache->redteam_session_ids);
            }
            else
                addPlayerToSlotPairs(room_cache->neutralteam_session_ids);

           

            std::stable_sort(player_slot_pairs.begin(), player_slot_pairs.end(),
                [](const std::pair<std::uint16_t, std::uint32_t>& a, const std::pair<std::uint16_t, std::uint32_t>& b) {
                return a.second < b.second;
            });

            std::vector<std::uint16_t> players_ids;
            for (const auto& pair : player_slot_pairs)
                players_ids.push_back(pair.first);

            return players_ids;
        }
        auto GetRoomSortedPlayerPlayingAndObserverSessionIds(RoomCacheResource& room_cache)
        {
            std::vector<std::pair<std::uint16_t, std::uint32_t>> player_slot_pairs;

            auto addPlayerToSlotPairs = [&](const std::vector<std::uint16_t>& team_session_ids)
                {
                    for (const auto& id : team_session_ids)
                    {
                        auto player_cache = GetAccCacheSharedBySessionId(id);
                        if (player_cache->acc_info.Index != -1 && player_cache->in_room && player_cache->room_id == room_cache->room_id && player_cache->playing)
                            player_slot_pairs.emplace_back(id, player_cache->slot_id);

                        player_cache.unlock();
                    }
                };
            if (IsModeTeamBased(room_cache->ModeIndex))
            {
                addPlayerToSlotPairs(room_cache->blueteam_session_ids);
                addPlayerToSlotPairs(room_cache->redteam_session_ids);
            }
            else
                addPlayerToSlotPairs(room_cache->neutralteam_session_ids);

            addPlayerToSlotPairs(room_cache->observers_session_ids);

            std::stable_sort(player_slot_pairs.begin(), player_slot_pairs.end(),
                [](const std::pair<std::uint16_t, std::uint32_t>& a, const std::pair<std::uint16_t, std::uint32_t>& b) {
                    return a.second < b.second;
                });

            std::vector<std::uint16_t> players_ids;
            for (const auto& pair : player_slot_pairs)
                players_ids.push_back(pair.first);

            return players_ids;
        }
        auto GetRoomSortedPlayerPlayingWithoutObserverSessionIds(RoomCacheResource& room_cache)
        {
            std::vector<std::pair<std::uint16_t, std::uint32_t>> player_slot_pairs;

            auto addPlayerToSlotPairs = [&](const std::vector<std::uint16_t>& team_session_ids)
            {
                for (const auto& id : team_session_ids)
                {
                    auto player_cache = GetAccCacheSharedBySessionId(id);
                    if (player_cache->acc_info.Index != -1 && player_cache->in_room && player_cache->room_id == room_cache->room_id && player_cache->playing)
                        player_slot_pairs.emplace_back(id, player_cache->slot_id);

                    player_cache.unlock();
                }
            };
            if (IsModeTeamBased(room_cache->ModeIndex))
            {
                addPlayerToSlotPairs(room_cache->blueteam_session_ids);
                addPlayerToSlotPairs(room_cache->redteam_session_ids);
            }
            else
                addPlayerToSlotPairs(room_cache->neutralteam_session_ids);



            std::stable_sort(player_slot_pairs.begin(), player_slot_pairs.end(),
                [](const std::pair<std::uint16_t, std::uint32_t>& a, const std::pair<std::uint16_t, std::uint32_t>& b) {
                return a.second < b.second;
            });

            std::vector<std::uint16_t> players_ids;
            for (const auto& pair : player_slot_pairs)
                players_ids.push_back(pair.first);

            return players_ids;
        }
        auto GetRoomSortedPlayerPingSessionIds(RoomCacheResource& room_cache)
        {
            std::vector<std::pair<std::uint16_t, std::uint32_t>> player_ping_pairs;

            const auto& session_ids = GetRoomSortedPlayerWithoutObserverSessionIds(room_cache);// by slot



            for (const auto& id : session_ids)
            {
                auto player_cache = GetAccCacheSharedBySessionId(id);
                if (player_cache->acc_info.Index != -1 && player_cache->in_room && player_cache->room_id == room_cache->room_id)
                    player_ping_pairs.emplace_back(id, player_cache->ping);

                player_cache.unlock();
            }

            std::stable_sort(player_ping_pairs.begin(), player_ping_pairs.end(),
                [](const std::pair<std::uint16_t, std::uint32_t>& a, const std::pair<std::uint16_t, std::uint32_t>& b) {
                return a.second >= b.second;
            });

            std::vector<std::uint16_t> players_ids;
            for (const auto& pair : player_ping_pairs)
                players_ids.push_back(pair.first);

            return players_ids;
        }
        std::uint16_t GetBestPlayerPingSessionIdInMatch(RoomCacheResource& room_cache)
        {
            const auto& session_ids = GetRoomSortedPlayerWithoutObserverSessionIds(room_cache);// by slot
            std::vector<std::pair<std::uint16_t, std::uint32_t>> player_ping_pairs;
            for (const auto& id : session_ids)
            {
                auto player_cache = GetAccCacheSharedBySessionId(id);
                if (player_cache->acc_info.Index != -1 && player_cache->in_room && player_cache->room_id == room_cache->room_id && player_cache->playing)
                    player_ping_pairs.emplace_back(id, player_cache->ping);

                player_cache.unlock();
            }
            std::stable_sort(player_ping_pairs.begin(), player_ping_pairs.end(),
                [](const std::pair<std::uint16_t, std::uint32_t>& a, const std::pair<std::uint16_t, std::uint32_t>& b) {
                return a.second >= b.second;
            });

            return player_ping_pairs.size() > 0 ? player_ping_pairs[0].first : 0;
        }
        std::uint16_t GetBestPlayerPingSessionIdInRoom(RoomCacheResource& room_cache)
        {
            const auto& session_ids = GetRoomSortedPlayerWithoutObserverSessionIds(room_cache);// by slot
            std::vector<std::pair<std::uint16_t, std::uint32_t>> player_ping_pairs;
            for (const auto& id : session_ids)
            {
                auto player_cache = GetAccCacheSharedBySessionId(id);
                if (player_cache->acc_info.Index != -1 && player_cache->in_room && player_cache->room_id == room_cache->room_id)
                    player_ping_pairs.emplace_back(id, player_cache->ping);

                player_cache.unlock();
            }
            std::stable_sort(player_ping_pairs.begin(), player_ping_pairs.end(),
                [](const std::pair<std::uint16_t, std::uint32_t>& a, const std::pair<std::uint16_t, std::uint32_t>& b) {
                return a.second >= b.second;
            });

            return player_ping_pairs.size() > 0 ? player_ping_pairs[0].first : 0;
        }
       void AddRoomCache(const std::uint32_t& room_id, Room& new_room)
        {
            if (!IsRoomAlready(room_id))
            {
                auto rooms_cache_locked = LockedResource{ std::unique_lock(rooms_cache_mutex), rooms_cache };
                auto rooms_ids_locked = LockedResource{ std::unique_lock(room_ids_mutex), room_ids };

                auto [it, inserted] = rooms_cache_locked->emplace(room_id, std::move(new_room));
                rooms_ids_locked->push_back(room_id);

                if (!inserted)
                    BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "Attempted to add a room with id: ({}), but it already exists ", room_id);

            }

        }
        void RemoveRoomCache(const std::uint32_t& room_id)
        {
            if (IsRoomAlready(room_id))
            {
                auto rooms_cache_locked = LockedResource{ std::unique_lock(rooms_cache_mutex), rooms_cache };
                auto rooms_ids_locked = LockedResource{ std::unique_lock(room_ids_mutex), room_ids };

                rooms_cache_locked->erase(room_id);
                rooms_ids_locked->erase(std::remove(rooms_ids_locked->begin(), rooms_ids_locked->end(), room_id), rooms_ids_locked->end());
            }

        }
        void RemoveRoomPlayerCache(const std::uint32_t& room_id, const std::uint16_t& session_id, const std::uint8_t& team_id)
        {
            if (IsRoomAlready(room_id))
            {
                auto rooms_cache_locked = GetRoomCacheUnique(room_id);

                //if (team_id == static_cast<std::uint8_t>(NetEngine::Team::IdType::Neutral))
                {
                    auto remove_myself = std::remove(rooms_cache_locked->neutralteam_session_ids.begin(), rooms_cache_locked->neutralteam_session_ids.end(), session_id);
                    rooms_cache_locked->neutralteam_session_ids.erase(remove_myself, rooms_cache_locked->neutralteam_session_ids.end());
                }
                //else if (team_id == static_cast<std::uint8_t>(NetEngine::Team::IdType::Red))
                {
                    auto remove_myself2 = std::remove(rooms_cache_locked->redteam_session_ids.begin(), rooms_cache_locked->redteam_session_ids.end(), session_id);
                    rooms_cache_locked->redteam_session_ids.erase(remove_myself2, rooms_cache_locked->redteam_session_ids.end());
                }
                //else if (team_id == static_cast<std::uint8_t>(NetEngine::Team::IdType::Blue))
                {
                    auto remove_myself3 = std::remove(rooms_cache_locked->blueteam_session_ids.begin(), rooms_cache_locked->blueteam_session_ids.end(), session_id);
                    rooms_cache_locked->blueteam_session_ids.erase(remove_myself3, rooms_cache_locked->blueteam_session_ids.end());
                }
                //else if (team_id == static_cast<std::uint8_t>(NetEngine::Team::IdType::Observer))
                {
                    auto remove_myself4 = std::remove(rooms_cache_locked->observers_session_ids.begin(), rooms_cache_locked->observers_session_ids.end(), session_id);
                    rooms_cache_locked->observers_session_ids.erase(remove_myself4, rooms_cache_locked->observers_session_ids.end());
                }
            }
        }

        void RemoveSessionId(std::vector<std::uint16_t>& session_ids, std::uint16_t session_id) {
            if (auto it = std::find(session_ids.begin(), session_ids.end(), session_id); it != session_ids.end()) {
                std::swap(*it, session_ids.back());
                session_ids.pop_back();
            }
        }

        void RemoveRoomPlayerCache(RoomCacheResource& room_cache, const std::uint16_t& session_id, const std::uint8_t& team_id)
        {
            if (IsRoomAlready(room_cache->room_id))
            {
                if (team_id == static_cast<std::uint8_t>(NetEngine::Team::IdType::Neutral))
                {
                    for (int i = 0, j = room_cache->neutralteam_session_ids.size(); i < j; i++)
                    {
                        if (room_cache->neutralteam_session_ids[i] == session_id)
                        {
                            BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "found leaving player at position: ({}) in team", i);
                            auto last_index = room_cache->neutralteam_session_ids.size() - 1;
                            auto last_id = room_cache->neutralteam_session_ids[last_index];

                            if (room_cache->host_session_id == session_id)
                            {
                                auto remove_myself = std::remove(room_cache->neutralteam_session_ids.begin(), room_cache->neutralteam_session_ids.end(), session_id);
                                room_cache->neutralteam_session_ids.erase(remove_myself, room_cache->neutralteam_session_ids.end());
                                break;
                            }

                            if (session_id != last_id)
                            {
                                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "different position change");
                                auto shared_target_cache = this->GetAccCacheSharedBySessionId(session_id);
                                auto target_slot_id = shared_target_cache->slot_id;
                                shared_target_cache.unlock();

                                auto uni_target_cache = this->GetAccCacheUniqueBySessionId(last_id);
                                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "exchange slot id ({}) with ({})", uni_target_cache->slot_id, target_slot_id);
                                uni_target_cache->slot_id = target_slot_id;
                                uni_target_cache.unlock();

                                room_cache->neutralteam_session_ids[i] = last_id;
                            }
                            room_cache->neutralteam_session_ids.pop_back();
                            break;
                        }
                    }
                    //RemoveSessionId(room_cache->neutralteam_session_ids, session_id);
                    //auto remove_myself = std::remove(room_cache->neutralteam_session_ids.begin(), room_cache->neutralteam_session_ids.end(), session_id);
                    //room_cache->neutralteam_session_ids.erase(remove_myself, room_cache->neutralteam_session_ids.end());
                }
                else if (team_id == static_cast<std::uint8_t>(NetEngine::Team::IdType::Red))
                {
                    auto remove_myself2 = std::remove(room_cache->redteam_session_ids.begin(), room_cache->redteam_session_ids.end(), session_id);
                    room_cache->redteam_session_ids.erase(remove_myself2, room_cache->redteam_session_ids.end());
                }
                else if (team_id == static_cast<std::uint8_t>(NetEngine::Team::IdType::Blue))
                {
                    auto remove_myself3 = std::remove(room_cache->blueteam_session_ids.begin(), room_cache->blueteam_session_ids.end(), session_id);
                    room_cache->blueteam_session_ids.erase(remove_myself3, room_cache->blueteam_session_ids.end());
                }
                else if (team_id == static_cast<std::uint8_t>(NetEngine::Team::IdType::Observer))
                {
                    auto remove_myself4 = std::remove(room_cache->observers_session_ids.begin(), room_cache->observers_session_ids.end(), session_id);
                    room_cache->observers_session_ids.erase(remove_myself4, room_cache->observers_session_ids.end());
                }
            }
        }



        std::optional<Item> GetPlayerItemInventory(AccCacheResource& acc_cache, const ItemSerialInfo& serial_info)
        {

            auto it = std::find_if(acc_cache->inventory_items.begin(), acc_cache->inventory_items.end(),
                [&serial_info](const Item& item) {
                return item.item_info.serial_info.data == serial_info.data;
            });

            if (it != acc_cache->inventory_items.end())
                return *it;
            else
                return {};
        }
        std::optional<Item> GetPlayerItemInventory(AccCacheResource& acc_cache, const std::uint32_t& item_id)
        {

            auto it = std::find_if(acc_cache->inventory_items.begin(), acc_cache->inventory_items.end(),
                [&item_id](const Item& item) {
                return item.item_info.item_number.item_id == item_id && !item.is_equipped;
            });

            if (it != acc_cache->inventory_items.end())
                return *it;
            else
                return {};
        }

        
        auto AddPlayerItemsAdded(AccCacheResource& acc_cache, const Item& new_item)
        {
            acc_cache->items_added.push_back(new_item.item_info.serial_info.data);
            return true;
        }
        auto AddPlayerItemsDeleted(AccCacheResource& acc_cache, const ItemSerialInfo& new_item)
        {
            acc_cache->items_deleted.push_back(new_item.data);
            return true;
        }
        auto AddPlayerItemsDeleted(AccCacheResource& acc_cache, const std::vector<ItemSerialInfo>& new_items)
        {
            acc_cache->items_deleted.insert(acc_cache->items_deleted.end(), new_items.begin(), new_items.end());
            return true;
        }
        auto AddPlayerItemsUpdated(AccCacheResource& acc_cache, const ItemSerialInfo& new_item)
        {
            acc_cache->items_updated.push_back(new_item);
            return true;
        }
        auto RemovePlayerItemsAdded(AccCacheResource& acc_cache, const ItemSerialInfo& new_item)
        {
            auto new_end = std::remove_if(acc_cache->items_updated.begin(), acc_cache->items_updated.end(),
                [&new_item](const ItemSerialInfo& item) {
                return item.data == new_item.data;
            });

            bool item_removed = (new_end != acc_cache->items_updated.end());
            acc_cache->items_updated.erase(new_end, acc_cache->items_updated.end());

            return item_removed;
        }
        auto RemovePlayerItemsDeleted(AccCacheResource& acc_cache, const ItemSerialInfo& new_item)
        {
            auto new_end = std::remove_if(acc_cache->items_deleted.begin(), acc_cache->items_deleted.end(),
                [&new_item](const ItemSerialInfo& item) {
                return item.data == new_item.data;
            });

            bool item_removed = (new_end != acc_cache->items_deleted.end());
            acc_cache->items_deleted.erase(new_end, acc_cache->items_deleted.end());

            return item_removed;
        }
        auto RemovePlayerItemsUpdated(AccCacheResource& acc_cache, const ItemSerialInfo& new_item)
        {
            auto new_end = std::remove_if(acc_cache->items_updated.begin(), acc_cache->items_updated.end(),
                [&new_item](const ItemSerialInfo& item) {
                return item.data == new_item.data;
            });

            bool item_removed = (new_end != acc_cache->items_updated.end());
            acc_cache->items_updated.erase(new_end, acc_cache->items_updated.end());

            return item_removed;
        }

        void DisconnectPlayer(CServer* server, const std::uint16_t& session_id, const std::uint8_t& reason)
        {
            auto send_msg = [&](CSession* session, std::uint16_t order, std::uint8_t mission, std::uint8_t extra, std::uint8_t option, std::uint8_t* data = nullptr, std::uint16_t data_size = 0)
            {
                CMessage message(session->GetEncryptionKey());
                message.SetSession(session->GetSessionId());
                message.SetCommand(order, mission, extra, option);
                if (data_size > 0 && data != nullptr) message.SetData(data, data_size);
                session->Send(message);
            };
            
            if (auto player_session = server->GetSessionById(session_id))
            {
                auto acc_cache = GetAccCacheSharedBySessionId(session_id);
                auto auth_key = acc_cache->acc_info.AuthKey;
                acc_cache.unlock();
                send_msg(player_session.get(), 73, 0, reason, 0);
                player_session.get()->Disconnect();
                SendCastIpc(PacketIds::Ipc::MainToCastDisconnectPlayer, Utility::ToVector(auth_key));
                // send ipc to cast to disconnect same session id
            }
            else
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::red, "couldn't forcefully disconnect session id: ({})", session_id);

        }

        void DisconnectPlayer(CServer* server, const std::uint16_t& session_id, const std::uint64_t& auth_key, const std::uint8_t& reason)
        {
            auto send_msg = [&](CSession* session, std::uint16_t order, std::uint8_t mission, std::uint8_t extra, std::uint8_t option, std::uint8_t* data = nullptr, std::uint16_t data_size = 0)
            {
                CMessage message(session->GetEncryptionKey());
                message.SetSession(session->GetSessionId());
                message.SetCommand(order, mission, extra, option);
                if (data_size > 0 && data != nullptr) message.SetData(data, data_size);
                session->Send(message);
            };

            if (auto player_session = server->GetSessionById(session_id))
            {
                send_msg(player_session.get(), 73, 0, reason, 0);
                player_session.get()->Disconnect();
                SendCastIpc(PacketIds::Ipc::MainToCastDisconnectPlayer, Utility::ToVector(auth_key));
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "MainToCastDisconnectPlayer auth key: ({}) session id: ({})", auth_key, session_id);
                // send ipc to cast to disconnect same session id
            }

            else
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::red, "couldn't forcefully disconnect session id: ({})", session_id);

        }



        auto AddPlayerFriendsDeleted(AccCacheResource& acc_cache, const BaseLib::FriendInfo& new_friend)
        {
            acc_cache->friends_deleted.push_back(new_friend);
            return true;
        }
        auto AddPlayerFriendsAccepted(AccCacheResource& acc_cache, const BaseLib::FriendInfo& new_friend)
        {
            acc_cache->friends_accepted.push_back(new_friend);
            return true;
        }
        auto AddPlayerFriendsPendings(AccCacheResource& acc_cache, const BaseLib::FriendInfo& new_friend)
        {
            acc_cache->friends_pendings.push_back(new_friend);
            return true;
        }
        auto RemovePlayerFriendsDeleted(AccCacheResource& acc_cache, const std::uint32_t& acc_id)
        {
            auto new_end = std::remove_if(acc_cache->friends_deleted.begin(), acc_cache->friends_deleted.end(),
                [&acc_id](BaseLib::FriendInfo& friend_info) {
                return friend_info.friend_account_id == acc_id;
            });

            bool friends_removed = (new_end != acc_cache->friends_deleted.end());
            acc_cache->friends_deleted.erase(new_end, acc_cache->friends_deleted.end());

            return friends_removed;
        }
        auto RemovePlayerFriendsAccepted(AccCacheResource& acc_cache, const std::uint32_t& acc_id)
        {
            auto new_end = std::remove_if(acc_cache->friends_accepted.begin(), acc_cache->friends_accepted.end(),
                [&acc_id](BaseLib::FriendInfo& friend_info) {
                return friend_info.friend_account_id == acc_id;
            });

            bool friends_removed = (new_end != acc_cache->friends_accepted.end());
            acc_cache->friends_accepted.erase(new_end, acc_cache->friends_accepted.end());

            return friends_removed;
        }
        auto RemovePlayerFriendsPendings(AccCacheResource& acc_cache, const std::uint32_t& acc_id)
        {
            auto new_end = std::remove_if(acc_cache->friends_pendings.begin(), acc_cache->friends_pendings.end(),
                [&acc_id](BaseLib::FriendInfo& friend_info) {
                return friend_info.player_account_id == acc_id;
            });

            bool friends_removed = (new_end != acc_cache->friends_pendings.end());
            acc_cache->friends_pendings.erase(new_end, acc_cache->friends_pendings.end());

            return friends_removed;
        }
        auto AddPlayerBlockedsDeleted(AccCacheResource& acc_cache, const BaseLib::BlockedInfo& new_blocked)
        {
            acc_cache->blockeds_deleted.push_back(new_blocked);
            return true;
        }
        auto AddPlayerBlockedsAdded(AccCacheResource& acc_cache, const BaseLib::BlockedInfo& new_blocked)
        {
            acc_cache->blockeds_added.push_back(new_blocked);
            return true;
        }
        auto RemovePlayerBlockedsDeleted(AccCacheResource& acc_cache, const std::uint32_t& acc_id)
        {
            auto new_end = std::remove_if(acc_cache->blockeds_deleted.begin(), acc_cache->blockeds_deleted.end(),
                [&acc_id](BaseLib::BlockedInfo& blocked_info) {
                return blocked_info.blocked_account_id == acc_id;
            });

            bool blocked_removed = (new_end != acc_cache->blockeds_deleted.end());
            acc_cache->blockeds_deleted.erase(new_end, acc_cache->blockeds_deleted.end());

            return blocked_removed;
        }
        auto RemovePlayerBlockedsAdded(AccCacheResource& acc_cache, const std::uint32_t& acc_id)
        {
            auto new_end = std::remove_if(acc_cache->blockeds_added.begin(), acc_cache->blockeds_added.end(),
                [&acc_id](BaseLib::BlockedInfo& blocked_info) {
                return blocked_info.blocked_account_id == acc_id;
            });

            bool blocked_removed = (new_end != acc_cache->blockeds_added.end());
            acc_cache->blockeds_added.erase(new_end, acc_cache->blockeds_added.end());

            return blocked_removed;
        }
        auto GetFriendsList(const std::uint32_t& session_id)
        {
            std::shared_lock lock(friends_cache_mutex);
            auto it = friends_cache.find(session_id);
            if (it != friends_cache.end())
                return LockedResource{ std::shared_lock(friends_cache_mutex), it->second };
            else
            {
                static thread_local std::shared_mutex null_friends_mutex;
                static thread_local std::vector<BaseLib::FriendInfo> null_friends_info;
                return LockedResource{ std::shared_lock(null_friends_mutex), null_friends_info };
            }
        }

        auto IsFriendsAlready(const std::vector<PlayerFriendInfo>& friends, std::uint32_t acc_id)
        {
            auto it = std::find_if(friends.begin(), friends.end(),
                [&acc_id](const PlayerFriendInfo& friend_info) {
                return friend_info.friend_id == acc_id;
            });
            return it != friends.end();
        }
        auto AddPlayerFriends(const std::uint16_t& id, const BaseLib::FriendInfo& friend_info)
        {
            auto friends_cache_locked = LockedResource{ std::unique_lock(friends_cache_mutex), friends_cache };
            auto& friends_list = (*friends_cache_locked)[id];
            friends_list.insert(friends_list.end(), friend_info);
        }
        auto AddPlayerFriends(const std::uint16_t& id, const std::vector<BaseLib::FriendInfo>& friend_list)
        {
            auto friends_cache_locked = LockedResource{ std::unique_lock(friends_cache_mutex), friends_cache };
            auto& friends = (*friends_cache_locked)[id];
            friends.insert(friends.end(), friend_list.begin(), friend_list.end());
        }
        auto RemoveFriendsCache(const std::uint16_t& id)
        {
            auto friends_cache_locked = LockedResource{ std::unique_lock(friends_cache_mutex), friends_cache };
            friends_cache_locked->erase(id);
        }
        auto RemovePlayerFriends(const std::uint16_t& id, const std::uint32_t& acc_id)
        {
            //deadlock
            auto friends_cache_locked = LockedResource{ std::unique_lock(friends_cache_mutex), friends_cache };

            auto& friends_list = (*friends_cache_locked)[id];

            auto new_end = std::remove_if(friends_list.begin(), friends_list.end(),
                [&acc_id](const BaseLib::FriendInfo& friend_info) {
                return friend_info.friend_account_id == acc_id;
            });

            bool friend_removed = (new_end != friends_list.end());

            friends_list.erase(new_end, friends_list.end());

            return friend_removed;
        }

        auto GetBlockedsList(const std::uint32_t& session_id)
        {
            std::shared_lock lock(blockeds_cache_mutex);
            auto it = blockeds_cache.find(session_id);
            if (it != blockeds_cache.end())
                return LockedResource{ std::shared_lock(blockeds_cache_mutex), it->second };
            else
            {
                static thread_local std::shared_mutex null_blockeds_mutex;
                static thread_local std::vector<BaseLib::BlockedInfo> null_blockeds_info;
                return LockedResource{ std::shared_lock(null_blockeds_mutex), null_blockeds_info };
            }
        }
        auto IsBlockedAlready(BlockedCacheResource& blockeds, std::uint32_t acc_id)
        {
            auto it = std::find_if(blockeds->begin(), blockeds->end(),
                [&acc_id](const BlockedInfo& blocked_info) {
                return blocked_info.blocked_account_id == acc_id;
            });
            return it != blockeds->end();
        }
        auto IsBlockedAlready(const std::vector<BaseLib::BlockedInfo>& blockeds, std::uint32_t acc_id)
        {
            auto it = std::find_if(blockeds.begin(), blockeds.end(),
                [&acc_id](const BlockedInfo& blocked_info) {
                return blocked_info.blocked_account_id == acc_id;
            });
            return it != blockeds.end();
        }
        auto AddPlayerBlockeds(const std::uint16_t& id, const BaseLib::BlockedInfo& blocked_info)
        {
            auto blockeds_cache_locked = LockedResource{ std::unique_lock(blockeds_cache_mutex), blockeds_cache };
            auto& blockeds_list = (*blockeds_cache_locked)[id];
            blockeds_list.insert(blockeds_list.end(), blocked_info);
           
        }
        auto AddPlayerBlockeds(const std::uint16_t& id, const std::vector<BaseLib::BlockedInfo>& blocked_list)
        {
            auto blockeds_cache_locked = LockedResource{ std::unique_lock(blockeds_cache_mutex), blockeds_cache };
            auto& blockeds = (*blockeds_cache_locked)[id];
            blockeds.insert(blockeds.end(), blocked_list.begin(), blocked_list.end());
        }
        auto RemoveBlockedsCache(const std::uint16_t& id)
        {
            auto blockeds_cache_locked = LockedResource{ std::unique_lock(blockeds_cache_mutex), blockeds_cache };
            blockeds_cache_locked->erase(id);
        }
        auto RemovePlayerBlockeds(const std::uint16_t& id, const std::uint32_t& acc_id)
        {
            auto blockeds_cache_locked = LockedResource{ std::unique_lock(blockeds_cache_mutex), blockeds_cache };
            auto& blockeds_list = (*blockeds_cache_locked)[id];

            auto new_end = std::remove_if(blockeds_list.begin(), blockeds_list.end(),
                [&acc_id](const BaseLib::BlockedInfo& blocked_info) {
                return blocked_info.blocked_account_id == acc_id;
            });

            bool blocked_removed = (new_end != blockeds_list.end());
            blockeds_list.erase(new_end, blockeds_list.end());

            return blocked_removed;
        }
        auto IsItemInShop(const std::uint32_t& item_id)
        {
            std::shared_lock lock(vendor_item_ids_mutex);
            return std::find(vendor_item_ids.begin(), vendor_item_ids.end(), item_id) != vendor_item_ids.end();
        }

        void RehashItemsInfo()
        {
            auto items_info_locked = LockedResource{ std::unique_lock(items_info_mutex), items_info };
            items_info_locked->max_load_factor(0.7f);
            items_info_locked->rehash(0);
        }
        void AddItemInfoCache(const std::uint32_t& id, BaseLib::ItemInfo item_info)
        {
            auto items_info_locked = LockedResource{ std::unique_lock(items_info_mutex), items_info };

            auto [it, inserted] = items_info_locked->emplace(id, std::move(item_info));

            //if (!inserted)
            //    BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "Attempted to add a item info with item id: ({}), but it already exists ", item_info.Id);
        }
        void RemoveItemInfoCache(const std::uint32_t& id)
        {
            auto items_info_locked = LockedResource{ std::unique_lock(items_info_mutex), items_info };
            items_info_locked->erase(id);
        }
        auto GetItemInfoCache(const std::uint32_t& id)
        {

            std::shared_lock lock(items_info_mutex);
            auto it = items_info.find(id);
            if (it != items_info.end())
                return LockedResource{ std::shared_lock(items_info_mutex), it->second };
            else
            {
                static thread_local std::shared_mutex null_iteminfo_mutex;
                static thread_local BaseLib::ItemInfo null_item_info;
                return LockedResource{ std::shared_lock(null_iteminfo_mutex), null_item_info };
            }
        }

        std::optional<Item> GetPlayerItemInventory(AccCacheResource& acc_cache, const std::uint32_t& item_type, const std::uint8_t& char_id)
        {

            auto it = std::find_if(acc_cache->inventory_items.begin(), acc_cache->inventory_items.end(),
                [&item_type, char_id, this](const Item& item) {
                if (item.character_id == char_id && item.is_equipped)
                {
                    auto item_info = GetItemInfoCache(item.item_info.item_number.item_id);
                    if (item_info->Type == item_type)
                        return true;
                    else
                        return false;
                }
                else return false;
            });

            if (it != acc_cache->inventory_items.end())
                return *it;
            else
                return {};
        }

        auto GetItemsInfoCacheSize()
        {
            std::shared_lock lock(items_info_mutex);
            return items_info.size();
        }

        void AddEffectInfoCache(const std::uint32_t& id, BaseLib::EffectInfo new_effect_info)
        {
            auto effect_info_locked = LockedResource{ std::unique_lock(effect_info_mutex), effect_info };

            auto [it, inserted] = effect_info_locked->emplace(id, std::move(new_effect_info));
        }
        void RemoveEffectInfoCache(const std::uint32_t& id)
        {
            auto effect_info_locked = LockedResource{ std::unique_lock(effect_info_mutex), effect_info };
            effect_info_locked->erase(id);
        }
        auto GetEffectInfoCache(const std::uint32_t& id)
        {

            std::shared_lock lock(effect_info_mutex);
            auto it = effect_info.find(id);
            if (it != effect_info.end())
                return LockedResource{ std::shared_lock(effect_info_mutex), it->second };
            else
            {
                static thread_local std::shared_mutex null_effectinfo_mutex;
                static thread_local BaseLib::EffectInfo null_effect_info;
                return LockedResource{ std::shared_lock(null_effectinfo_mutex), null_effect_info };
            }
        }
        auto GetEffectInfoCacheSize()
        {
            std::shared_lock lock(effect_info_mutex);
            return effect_info.size();
        }

        void AddCollectionInfoCache(const std::uint32_t& id, BaseLib::CollectionInfo new_collection_info)
        {
            auto collection_info_locked = LockedResource{ std::unique_lock(collection_info_mutex), collection_info };

            auto [it, inserted] = collection_info_locked->emplace(id, std::move(new_collection_info));
        }
        void RemoveCollectionInfoCache(const std::uint32_t& id)
        {
            auto collection_info_locked = LockedResource{ std::unique_lock(collection_info_mutex), collection_info };
            collection_info_locked->erase(id);
        }
        auto GetCollectionInfoCache(const std::uint32_t& id)
        {

            std::shared_lock lock(collection_info_mutex);
            auto it = collection_info.find(id);
            if (it != collection_info.end())
                return LockedResource{ std::shared_lock(collection_info_mutex), it->second };
            else
            {
                static thread_local std::shared_mutex null_collectioninfo_mutex;
                static thread_local BaseLib::CollectionInfo null_collection_info;
                return LockedResource{ std::shared_lock(null_collectioninfo_mutex), null_collection_info };
            }
        }
        auto GetCollectionInfoCacheSize()
        {
            std::shared_lock lock(collection_info_mutex);
            return collection_info.size();
        }

        auto GetRandomDailyMissionIds(std::uint32_t count, std::uint32_t id1, std::uint32_t id2, std::uint32_t id3) {
            std::vector<std::uint32_t> result;
            result.reserve(count);

            std::random_device rd;
            std::mt19937 gen(rd());

            {
                std::shared_lock lock(dailymission_ids_mutex);

                if (dailymission_ids.size() < count) {
                    return result;  // Not enough IDs to fulfill the request
                }

                while (result.size() < count) {
                    auto random_it = std::next(dailymission_ids.begin(), gen() % dailymission_ids.size());
                    std::uint32_t random_id = *random_it;

                    // Exclude already used IDs and avoid duplicates
                    if (random_id != id1 && random_id != id2 && random_id != id3 &&
                        std::find(result.begin(), result.end(), random_id) == result.end()) {
                        result.push_back(random_id);
                    }
                }
            }

            return result;
        }

        void AddDailyMissionInfoCache(const std::uint32_t& id, BaseLib::DailyMissionInfo new_dailymission_info)
        {
            auto dailymission_info_locked = LockedResource{ std::unique_lock(dailymission_info_mutex), dailymission_info };
            auto dailymission_ids_locked = LockedResource{ std::unique_lock(dailymission_ids_mutex), dailymission_ids };

            dailymission_ids_locked->push_back(id);

            auto [it, inserted] = dailymission_info_locked->emplace(id, std::move(new_dailymission_info));
        }
        void RemoveDailyMissionInfoCache(const std::uint32_t& id)
        {
            auto dailymission_info_locked = LockedResource{ std::unique_lock(dailymission_info_mutex), dailymission_info };
            auto dailymission_ids_locked = LockedResource{ std::unique_lock(dailymission_ids_mutex), dailymission_ids };

            dailymission_ids_locked->erase(std::remove(dailymission_ids_locked->begin(), dailymission_ids_locked->end(), id), dailymission_ids_locked->end());

            dailymission_info_locked->erase(id);
        }
        auto GetDailyMissionInfoCache(const std::uint32_t& id)
        {

            std::shared_lock lock(dailymission_info_mutex);
            auto it = dailymission_info.find(id);
            if (it != dailymission_info.end())
                return LockedResource{ std::shared_lock(dailymission_info_mutex), it->second };
            else
            {
                static thread_local std::shared_mutex null_dailymission_mutex;
                static thread_local BaseLib::DailyMissionInfo null_dailymission_info;
                return LockedResource{ std::shared_lock(null_dailymission_mutex), null_dailymission_info };
            }
        }
        auto GetDailyMissionInfoCacheSize()
        {
            std::shared_lock lock(dailymission_info_mutex);
            return dailymission_info.size();
        }

        void AddSetItemInfoCache(const std::uint32_t& id, BaseLib::SetItemInfo& item_info)
        {
            auto setitems_info_locked = LockedResource{ std::unique_lock(setitems_info_mutex), setitems_info };

            auto [it, inserted] = setitems_info_locked->emplace(id, std::move(item_info));

            if (!inserted)
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "Attempted to add a set item info with item id: ({}), but it already exists ", item_info.Id);
        }
        void RemoveSetItemInfoCache(const std::uint32_t& id)
        {
            auto setitems_info_locked = LockedResource{ std::unique_lock(setitems_info_mutex), setitems_info };
            setitems_info_locked->erase(id);
        }
        auto GetSetItemInfoCache(const std::uint32_t& id)
        {

            std::shared_lock lock(setitems_info_mutex);
            auto it = setitems_info.find(id);
            if (it != setitems_info.end())
                return LockedResource{ std::shared_lock(setitems_info_mutex), it->second };
            else
            {
                static thread_local std::shared_mutex null_setiteminfo_mutex;
                static thread_local BaseLib::SetItemInfo null_setitem_info;
                return LockedResource{ std::shared_lock(null_setiteminfo_mutex), null_setitem_info };
            }
        }
        auto GetSetItemsInfoCacheSize()
        {
            std::shared_lock lock(setitems_info_mutex);
            return setitems_info.size();
        }


        void AddRoomOptionInfoCache(const std::uint32_t& game_mode, const BaseLib::RoomOptionInfo& room_option_info)
        {
            auto roomoptionsinfo_cache_locked = LockedResource{ std::unique_lock(roomoptionsinfo_cache_mutex), roomoptionsinfo_cache };
            auto& inner_map = (*roomoptionsinfo_cache_locked)[game_mode];
            inner_map[room_option_info.Type].push_back(room_option_info);
            //BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "Attempted to add a room option info with game mode: ({}), but it already exists ", game_mode);

        }
        auto GetRoomOptionInfosGameModeCache(const std::uint32_t& game_mode)
        {
            std::shared_lock lock(roomoptionsinfo_cache_mutex);

            auto it = roomoptionsinfo_cache.find(game_mode);
            if (it != roomoptionsinfo_cache.end())
                return LockedResource{ std::shared_lock(roomoptionsinfo_cache_mutex), it->second };
            else
            {
                static thread_local std::shared_mutex null_roomoptionsinfo_mutex;
                //static thread_local std::unordered_map<std::uint32_t, std::vector<BaseLib::RoomOptionInfo>> empty;
                static thread_local boost::unordered_flat_map<std::uint32_t, std::vector<BaseLib::RoomOptionInfo>> empty;
                return LockedResource{ std::shared_lock(null_roomoptionsinfo_mutex), empty };
            }
        }
        auto GetRoomOptionInfoByTypeCache(RoomOptionsCacheResource& infos, const std::uint32_t& type, const std::uint32_t& data)
        {
            if (auto typeIt = infos->find(type); typeIt != infos->end())
            {
                auto options = typeIt->second;
                auto it = std::find_if(options.begin(), options.end(), [data](const BaseLib::RoomOptionInfo& option_info) { return option_info.Data == data; });
                if (it != options.end())
                    return *it;
                else
                    return BaseLib::RoomOptionInfo();
            }
            else
                return BaseLib::RoomOptionInfo();
        }
        auto GetRoomOptionsInfoSize()
        {
            std::shared_lock lock(roomoptionsinfo_cache_mutex);
            return roomoptionsinfo_cache.size();
        }

        auto UpdatePlayerItemEquip(AccCacheResource& acc_cache, const ItemSerialInfo& serial_info, const std::uint8_t& character_type, const bool& is_equipped)
        {
            auto& inventory = acc_cache->inventory_items;
            bool updated = false;
            auto it = std::find_if(inventory.begin(), inventory.end(),
                [&serial_info](const Item& item) {
                return item.item_info.serial_info.data == serial_info.data;
            });
            if (it != inventory.end())
            {
                it->is_equipped = is_equipped;
                it->character_id = character_type;
                acc_cache->items_updated.push_back(serial_info);
                updated = true;
            }
            return updated;
        }
        
        auto UpdatePlayerItemEquip(AccCacheResource& acc_cache, const std::uint32_t& item_type, const std::uint8_t& character_type, const bool& is_equipped)
        {
            auto& inventory = acc_cache->inventory_items;
            std::vector<BaseLib::Item> items_updated;
            for (auto& item : inventory)
            {
                auto item_info = GetItemInfoCache(item.item_info.item_number.item_id);

                if (item.character_id == character_type && item_info->Type == item_type)
                {
                    item.is_equipped = is_equipped;
                    item.character_id = character_type;
                    acc_cache->items_updated.push_back(item.item_info.serial_info);
                    items_updated.push_back(item);
                }
            }
            return items_updated;
        }
        auto UpdatePlayerItemExpireDate(AccCacheResource& acc_cache, const ItemSerialInfo& serial_info, const std::uint32_t expire_date)
        {
            auto& inventory = acc_cache->inventory_items;
            bool updated = false;
            auto it = std::find_if(inventory.begin(), inventory.end(),
                [&serial_info](const Item& item) {
                return item.item_info.serial_info.data == serial_info.data;
            });
            if (it != inventory.end())
            {
                it->item_info.expire_date = expire_date;
                acc_cache->items_updated.push_back(serial_info);
                updated = true;
            }
            return updated;
        }
        auto UpdatePlayerItemsRepair(AccCacheResource& acc_cache, const std::vector<ItemSerialInfo>& serial_infos, const std::vector<std::uint32_t>& durabilities)
        {
            auto& inventory = acc_cache->inventory_items;
            bool updated = false;
            for (size_t i = 0; i < serial_infos.size(); ++i)
            {
                const auto& serial_info = serial_infos[i];
                auto it = std::find_if(inventory.begin(), inventory.end(),
                    [&serial_info](const Item& item) {
                    return item.item_info.serial_info.data == serial_info.data;
                });
                if (it != inventory.end())
                {
                    it->item_info.repair = durabilities[i];
                    acc_cache->items_updated.push_back(serial_info);
                    updated = true;
                }
            }
            return updated;
        }
        auto UpdatePlayerItemEnergy(AccCacheResource& acc_cache, const ItemSerialInfo& serial_info, const std::uint32_t& energy)
        {
            auto& inventory = acc_cache->inventory_items;
            bool updated = false;
            auto it = std::find_if(inventory.begin(), inventory.end(),
                [&serial_info](const Item& item) {
                return item.item_info.serial_info.data == serial_info.data;
            });
            if (it != inventory.end())
            {
                it->item_info.energy = energy;
                acc_cache->items_updated.push_back(serial_info);
                updated = true;
            }
            return updated;
        }
        auto UpdatePlayerItemUpgrade(AccCacheResource& acc_cache, const ItemSerialInfo& serial_info, const std::uint32_t& new_item_id, const std::uint32_t& repair, const std::uint32_t& energy)
        {
            auto& inventory = acc_cache->inventory_items;
            bool updated = false;
            auto it = std::find_if(inventory.begin(), inventory.end(),
                [&serial_info](const Item& item) {
                return item.item_info.serial_info.data == serial_info.data;
            });
            if (it != inventory.end())
            {
                it->item_info.item_number.item_id = new_item_id;
                it->item_info.repair = repair;
                it->item_info.energy = energy;
                acc_cache->items_updated.push_back(serial_info);
                updated = true;
            }
            return updated;
        }

        auto GetLuckyGachaponInfo()
        {
            std::shared_lock lock(gachapons_info_mutex);
            if (!gachapons_info.empty())
            {
                BaseLib::GachaponInfo last_element;
                for (auto it = gachapons_info.begin(); it != gachapons_info.end(); ++it) last_element = it->second;
                return LockedResource{ std::shared_lock(gachapons_info_mutex), last_element };
                //return LockedResource{ std::shared_lock(gachapons_info_mutex), std::prev(gachapons_info.end())->second };
            }
            else
            {
                static thread_local std::shared_mutex null_gachapons_info;
                static thread_local BaseLib::GachaponInfo empty;
                return LockedResource{ std::shared_lock(null_gachapons_info), empty };
            }
        }
        auto GetGachaponInfo(const std::uint32_t& gachapon_id)
        {
            std::shared_lock lock(gachapons_info_mutex);

            auto it = gachapons_info.find(gachapon_id);
            if (it != gachapons_info.end())
                return LockedResource{ std::shared_lock(gachapons_info_mutex), it->second };
            else
            {
                static thread_local std::shared_mutex null_gachapons_info;
                static thread_local BaseLib::GachaponInfo empty;
                return LockedResource{ std::shared_lock(null_gachapons_info), empty };
            }
        }
        std::uint32_t ExtractIndex(std::vector<std::uint32_t> probabilities)
        {
            std::uint32_t sum_of_probabilities = 0;
            std::vector<ProbabilityStruct> probability_list;

            for (std::uint32_t i = 0; i < probabilities.size(); i++)
            {
                probability_list.push_back(ProbabilityStruct(i, sum_of_probabilities + (probabilities[i] - 1)));
                sum_of_probabilities += probabilities[i];
            }
            std::uint32_t prev_probability = 0;
            std::uint32_t extracted = Utility::Random::CustomGen(0, sum_of_probabilities - 1);
            for (std::uint32_t i = 0; i < probability_list.size(); i++)
            {
                if (extracted >= prev_probability && extracted <= probability_list[i].prob)
                    return probability_list[i].id;
                else
                    prev_probability = probability_list[i].prob;
            }
            return -1;
        }
        void AddPackageItemCache(const std::uint32_t& package_id, const std::uint32_t& package_group_id, const BaseLib::PackageInfo& package_package_item)
        {
            auto packages_info_locked = LockedResource{ std::unique_lock(packages_info_mutex), packages_info };
            auto& inner_map = (*packages_info_locked)[package_id];
            inner_map[package_group_id].push_back(package_package_item);
            //BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "Attempted to add a package info with id: ({}), but it already exists ", package_id);
        }
        auto GetPackagesCacheSize()
        {
            std::shared_lock lock(packages_info_mutex);

            std::size_t count = 0;
            for (const auto& groupPair : packages_info)
                for (const auto& packagesGroups : groupPair.second)
                    count += packagesGroups.second.size();

            return count;
        }
        auto GetPackageInfo(const std::uint32_t& package_id)
        {
            std::shared_lock lock(packages_info_mutex);
            auto it = packages_info.find(package_id);
            if (it != packages_info.end())
                return LockedResource{ std::shared_lock(packages_info_mutex), it->second };
            else
            {
                static thread_local std::shared_mutex null_packages_info;
                //static thread_local std::unordered_map<std::uint32_t, std::vector<BaseLib::PackageInfo>> empty;
                static thread_local boost::unordered_flat_map<std::uint32_t, std::vector<BaseLib::PackageInfo>> empty;
                return LockedResource{ std::shared_lock(null_packages_info), empty };
            }
        }
        auto ExtractPackageItemsWon(PackageCacheResource& package_info)
        {
            std::vector<BaseLib::PackageInfo> return_items;
            for (const auto& group : *package_info)
            {
                std::vector<std::uint32_t> probability_list;
                for (const auto& item : group.second)
                    probability_list.push_back(item.Probability);

                const auto& extracted = ExtractIndex(probability_list);
                if (extracted != -1)
                    return_items.push_back(group.second[extracted]);
            }
            return return_items;
        }
        auto ExtractGachaponItemsWon(GachaponCacheResource& gachapon_info, std::vector<GachaponPackageItem>& return_items, std::uint32_t coupon_chance)
        {
            for (const auto& group : gachapon_info->Gachapons)
            {
                std::vector<std::uint32_t> probability_list;
                for (const auto& item : group.second)
                    probability_list.push_back(item.Probability);

                const auto& extracted = ExtractIndex(probability_list);
                if (extracted != -1)
                    return_items.push_back(group.second[extracted]);
            }
            return Utility::Random::CustomGen(0, 100) < coupon_chance;
        }
        auto AddGachaponInfoCache(const std::uint32_t& gachapon_id, BaseLib::GachaponInfo& gachapon_info)
        {
            auto gachapons_info_cache_locked = LockedResource{ std::unique_lock(gachapons_info_mutex), gachapons_info };
            auto [it, inserted] = gachapons_info_cache_locked->emplace(gachapon_id,  std::move(gachapon_info));
            if (!inserted)
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "Attempted to add a gachapon info with id: ({}), but it already exists ", gachapon_id);
        }
        auto AddGachaponPackageItemCache(const std::uint32_t& gachapon_id, const std::uint32_t& gachapon_group_id, const BaseLib::GachaponPackageItem& gachapon_package_item)
        {
            auto gachapons_info_cache_locked = LockedResource{ std::unique_lock(gachapons_info_mutex), gachapons_info };
            auto& inner_map = (*gachapons_info_cache_locked)[gachapon_id];
            inner_map.Gachapons[gachapon_group_id].push_back(gachapon_package_item);
            //BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan,
            //    "Added a gachapon package item to id: ({}) and group id: ({}).",
            //    gachapon_id, gachapon_group_id);
        }
        auto GetGachaponsCacheSize()
        {
            std::shared_lock lock(gachapons_info_mutex);

            std::size_t count = 0;
            for (const auto& groupPair : gachapons_info)
                for (const auto& gachaponsGroups : groupPair.second.Gachapons)
                    count += gachaponsGroups.second.size();

            return count;
        }
        void AddUpgradeInfoCache(const std::uint32_t& group_id, const Items::Upgrade::Type& upgrade_type, const BaseLib::UpgradeInfo& upgrade_info)
        {
            auto upgrades_info_cache_locked = LockedResource{ std::unique_lock(upgrades_info_mutex), upgrades_info };

            auto group_it = upgrades_info_cache_locked->find(group_id);
            if (group_it == upgrades_info_cache_locked->end()) group_it = upgrades_info_cache_locked->insert({ group_id, {} }).first;

            auto& inner_map = group_it->second;
            if (upgrade_type != Items::Upgrade::Type::NoUpgrade)
            {
                auto upgrade_vec_it = inner_map.find(upgrade_type);
                if (upgrade_vec_it == inner_map.end()) upgrade_vec_it = inner_map.insert({ upgrade_type, {} }).first;
                auto& upgrade_vector = upgrade_vec_it->second;
                if (upgrade_vector.empty())
                {
                    auto no_upgrade_it = inner_map.find(Items::Upgrade::Type::NoUpgrade);
                    if (no_upgrade_it != inner_map.end() && !no_upgrade_it->second.empty()) upgrade_vector.push_back(no_upgrade_it->second.front());
                }
                upgrade_vector.push_back(upgrade_info);
            }
            else
            {
                auto no_upgrade_it = inner_map.find(Items::Upgrade::Type::NoUpgrade);
                if (no_upgrade_it == inner_map.end())  no_upgrade_it = inner_map.insert({ Items::Upgrade::Type::NoUpgrade, {} }).first;
                no_upgrade_it->second.push_back(upgrade_info);
            }

            /*
            if (upgrade_type != Items::Upgrade::Type::NoUpgrade && upgrades_info_cache_locked->at(group_id)[upgrade_type].empty())
            {
                auto noUpgradeIt = upgrades_info_cache_locked->at(group_id).find(Items::Upgrade::Type::NoUpgrade);
                if (noUpgradeIt != upgrades_info_cache_locked->at(group_id).end() && !noUpgradeIt->second.empty())
                    upgrades_info_cache_locked->at(group_id)[upgrade_type].push_back(noUpgradeIt->second.front());
            }
            auto& inner_map = (*upgrades_info_cache_locked)[group_id];
            inner_map[upgrade_type].push_back(upgrade_info);
            */
        }

        auto GetUpgradeCollectionInfoCache(const Items::Upgrade::Type& upgrade_type, const std::uint32_t& item_id)
        {
            std::shared_lock lock(upgrades_info_mutex);

            auto groupIt = upgrades_info.find(item_id);
            if (groupIt != upgrades_info.end())
            {

                auto typeIt = groupIt->second.find(upgrade_type);
                if (typeIt != groupIt->second.end())
                {
                    auto& upgradeInfos = typeIt->second;
                    for (auto& upgradeInfo : upgradeInfos)
                        if (upgradeInfo.ItemId == item_id)
                            return LockedResource{ std::shared_lock(upgrades_info_mutex), upgradeInfos };
                }
            }

            for (auto& groupPair : upgrades_info)
            {
                auto typeIt = groupPair.second.find(upgrade_type);
                if (typeIt != groupPair.second.end())
                {
                    auto& upgradeInfos = typeIt->second;
                    for (auto& upgradeInfo : upgradeInfos)
                        if (upgradeInfo.ItemId == item_id)
                            return LockedResource{ std::shared_lock(upgrades_info_mutex), upgradeInfos };
                }
            }
            static thread_local std::shared_mutex null_upgrades_info;
            static thread_local std::vector<BaseLib::UpgradeInfo> empty;
            return LockedResource{ std::shared_lock(null_upgrades_info), empty };
        }
        std::uint32_t GetUpgradeLevel(UpgradeCacheResource& upgrade_collection, const std::uint32_t& id)
        {
            for (std::uint32_t i = 0; i < upgrade_collection->size(); ++i)
                if (upgrade_collection->at(i).ItemId == id)
                    return i;

            return 0;
        }
        auto GetUpgradeInfoNext(UpgradeCacheResource& upgrade_collection, const std::uint32_t& id)
        {
            for (std::uint32_t i = 0; i < upgrade_collection->size(); ++i)
                if (upgrade_collection->at(i).ItemId == id && i + 1 < upgrade_collection->size())
                    return upgrade_collection->at(i + 1);

            return BaseLib::UpgradeInfo();
        }
        auto GetUpgradeInfoPrev(UpgradeCacheResource& upgrade_collection, const std::uint32_t& id)
        {
            for (std::uint32_t i = 0; i < upgrade_collection->size(); ++i)
                if (upgrade_collection->at(i).ItemId == id && i > 0)
                    return upgrade_collection->at(i - 1);

            return BaseLib::UpgradeInfo();
        }
        auto GetUpgradeInfoCache(const std::uint32_t& item_id)
        {
            std::shared_lock lock(upgrades_info_mutex);

            auto groupIt = upgrades_info.find(item_id);
            if (groupIt != upgrades_info.end())
                for (auto& upgradePair : groupIt->second)
                    for (auto& upgradeInfo : upgradePair.second)
                        if (upgradeInfo.ItemId == item_id)
                            return LockedResource{ std::shared_lock(upgrades_info_mutex), upgradeInfo };

            for (auto& groupPair : upgrades_info)
                for (auto& upgradePair : groupPair.second)
                    for (auto& upgradeInfo : upgradePair.second)
                        if (upgradeInfo.ItemId == item_id)
                            return LockedResource{ std::shared_lock(upgrades_info_mutex), upgradeInfo };



            static thread_local std::shared_mutex null_upgrades_info;
            static thread_local BaseLib::UpgradeInfo empty;
            return LockedResource{ std::shared_lock(null_upgrades_info), empty };
        }
        std::size_t GetUpgradeInfoCacheSize()
        {
            std::shared_lock lock(upgrades_info_mutex);

            std::size_t count = 0;
            for (const auto& groupPair : upgrades_info)
                count += groupPair.second.size();

            return count;
        }

        
        //void TransformEquippedItems(const std::vector<Item>& items, std::unordered_map<std::uint8_t, std::vector<InventoryItemInfo>>& equipped_items)
        void TransformEquippedItems(const std::vector<Item>& items, boost::unordered_flat_map<std::uint8_t, std::vector<InventoryItemInfo>>& equipped_items)
        {
            for (const auto& item : items)
            {
                if (item.is_equipped == 1)
                {
                    auto new_item = item.item_info;
                    new_item.item_number.item_id = item.item_info.item_number.item_id;
                    new_item.item_number.stock = item.stock;
                    equipped_items[item.character_id].push_back(new_item);
                }
            }
        }
        void TransformItems(const std::vector<Item>& items, std::vector<Item>& new_items)
        {
            for (const auto& item : items)
            {
                if (item.is_equipped == 0)
                {
                    new_items.push_back(item);
                }
                else
                {
                    if (item.item_info.expire_date != ItemExpire::Unlimited && item.item_info.expire_date <= Utility::GetUtcTimeNow())
                    {
                        auto new_item = item;
                        new_item.is_equipped = 0;
                        new_items.push_back(new_item);
                    }
                }
            }
        }
        auto GetTransformStockItems(const std::vector<Item>& items, const std::uint32_t& fragment_index = 0, const std::uint32_t& fragment_max_size = 35)
        {
            std::vector<InventoryItemInfo> new_items;
            const std::uint32_t start_index = fragment_index * 35;
            const std::uint32_t end_index = std::min(start_index + 35, static_cast<std::uint32_t>(items.size()));
            for (auto i = start_index; i < end_index; i++)
            {
                auto new_item = items[i].item_info;
                new_item.item_number.item_id = items[i].item_info.item_number.item_id;
                new_item.item_number.stock = items[i].stock;
                new_items.push_back(new_item);
            }
            return new_items;
        }
        auto GetTransformEquippedItems(const std::vector<InventoryItemInfo>& items)
        {
            std::vector<EquipItemInfo> new_items;
            for (std::uint32_t i = 0; i < items.size(); i++)
            {
                EquipItemInfo new_item = EquipItemInfo(items[i]);
                auto item_info = GetItemInfoCache(new_item.item_number.item_id);
                new_item.item_number.item_type = item_info->Type;
                new_items.push_back(new_item);
            }
            return new_items;
        }
        auto GetItemByType(const std::vector<BaseLib::Item>& equipped_items, const std::uint32_t& item_type)
        {
            auto it = std::find_if(equipped_items.begin(), equipped_items.end(), [this, item_type](const Item& item)
            {
                auto item_info = GetItemInfoCache(item.item_info.item_number.item_id);

                return item_info->Type == item_type;
            });
            if (it != equipped_items.end())
                return *it;
            else
                return BaseLib::Item();
        }
        auto IsItemWeapon(const std::uint32_t& item_id)
        {
            auto item_info = GetItemInfoCache(item_id);

            return item_info->Type == Items::WeaponItems::Type::Melee ||
                item_info->Type == Items::WeaponItems::Type::Rifle ||
                item_info->Type == Items::WeaponItems::Type::Shotgun ||
                item_info->Type == Items::WeaponItems::Type::Sniper ||
                item_info->Type == Items::WeaponItems::Type::Gatling ||
                item_info->Type == Items::WeaponItems::Type::Bazooka ||
                item_info->Type == Items::WeaponItems::Type::Grenade;
        }
        auto IsItemCostume(const std::uint32_t& item_id)
        {
            auto item_info = GetItemInfoCache(item_id);

            return item_info->Type == Items::CostumeItems::Type::Hair ||
                item_info->Type == Items::CostumeItems::Type::Face ||
                item_info->Type == Items::CostumeItems::Type::Upper ||
                item_info->Type == Items::CostumeItems::Type::Under ||
                item_info->Type == Items::CostumeItems::Type::Pants ||
                item_info->Type == Items::CostumeItems::Type::Shirt ||
                item_info->Type == Items::CostumeItems::Type::Boots ||
                item_info->Type == Items::CostumeItems::Type::Glasses ||
                item_info->Type == Items::CostumeItems::Type::AccBack1 ||
                item_info->Type == Items::CostumeItems::Type::AccBack2;
        }
        auto IsItemDiorama(const std::uint32_t& item_id)
        {
            auto item_info = GetItemInfoCache(item_id);

            return item_info->Type == 22 || item_info->Type == 23;
        }
        auto IsItemSet(const std::uint32_t& item_id)
        {
            auto setitem_info = GetSetItemInfoCache(item_id);
            return setitem_info->Id != 0;
        }
        std::uint32_t AdjustItemType(const std::uint32_t& item_type)
        {
            if (item_type == static_cast<std::uint32_t>(Items::OtherItems::Type::Question) ||
                item_type == static_cast<std::uint32_t>(Items::OtherItems::Type::ShieldEnamel) ||
                item_type == static_cast<std::uint32_t>(Items::OtherItems::Type::FlagBlue) ||
                item_type == static_cast<std::uint32_t>(Items::OtherItems::Type::BombDrop) ||
                item_type == static_cast<std::uint32_t>(Items::OtherItems::Type::GatchaItem) ||
                item_type == static_cast<std::uint32_t>(Items::OtherItems::Type::Question1) ||
                item_type == static_cast<std::uint32_t>(Items::OtherItems::Type::MonsterFace) ||
                item_type == static_cast<std::uint32_t>(Items::OtherItems::Type::Undefined1) ||
                item_type == static_cast<std::uint32_t>(Items::OtherItems::Type::Undefined2))
            {
                return 17;
            }

            if (item_type == static_cast<std::uint32_t>(Items::DioramaItems::Type::Footing)) return 19;
            if (item_type == static_cast<std::uint32_t>(Items::DioramaItems::Type::Object)) return 20;

            return item_type;
        }
        std::uint32_t GetSetPiecesCount(const std::uint32_t& item_id)
        {
            auto setitem_info = GetSetItemInfoCache(item_id);
            if (setitem_info->Id)
            {
                std::uint32_t total_pieces = 0;
                if (setitem_info->Hair) total_pieces++;
                if (setitem_info->Face) total_pieces++;
                if (setitem_info->Upper) total_pieces++;
                if (setitem_info->Under) total_pieces++;
                if (setitem_info->Arms) total_pieces++;
                if (setitem_info->Pants) total_pieces++;
                if (setitem_info->Boots) total_pieces++;
                if (setitem_info->AccessoryA) total_pieces++;
                if (setitem_info->AccessoryB) total_pieces++;
                if (setitem_info->AccessoryC) total_pieces++;
                return total_pieces;
            }
            else return 0;
        }
        auto GetSetItemTypes(const std::uint32_t& item_id)
        {
            auto setitem_info = GetSetItemInfoCache(item_id);
            std::vector<std::uint32_t> types;
            if (setitem_info->Id)
            {
                if (setitem_info->Hair < UINT32_MAX) types.push_back(0);
                if (setitem_info->Face < UINT32_MAX) types.push_back(1);
                if (setitem_info->Upper < UINT32_MAX) types.push_back(2);
                if (setitem_info->Under < UINT32_MAX) types.push_back(3);
                if (setitem_info->Pants < UINT32_MAX) types.push_back(4);
                if (setitem_info->Arms < UINT32_MAX) types.push_back(5);
                if (setitem_info->Boots < UINT32_MAX) types.push_back(6);
                if (setitem_info->AccessoryA < UINT32_MAX) types.push_back(7);
                if (setitem_info->AccessoryB < UINT32_MAX) types.push_back(8);
                if (setitem_info->AccessoryC < UINT32_MAX) types.push_back(9);
            }
            BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "(uint32 max is {}) for the set ({}) was found hair {}, face {}, top {}, legs {}, arms {}, pants {}, boots {}, acca {}, accb {}, accc {}", UINT32_MAX, item_id, setitem_info->Hair, setitem_info->Face, setitem_info->Upper, setitem_info->Under, setitem_info->Arms, setitem_info->Pants, setitem_info->Boots, setitem_info->AccessoryA, setitem_info->AccessoryB, setitem_info->AccessoryC);
            return types;
        }
        std::string GetCharacterStr(const std::uint8_t& char_id)
        {
            switch (static_cast<Character::Type>(char_id))
            {
                case Character::Type::Naomi:
                    return "Naomi";
                case Character::Type::Kai:
                    return "Kai";
                case Character::Type::Pandora:
                    return "Pandora";
                case Character::Type::CHIP:
                    return "CHIP";
                case Character::Type::Knox:
                    return "Knox";
                case Character::Type::Simon:
                    return "Simon";
                case Character::Type::Amelia:
                    return "Amelia";
                case Character::Type::Sharkill:
                    return "Sharkill";
                case Character::Type::Sophitia:
                    return "Sophitia";
                default:
                    return "Unknown";
            }
        }


        void AddVendorItemId(const std::uint32_t& item_id)
        {

            auto vendor_item_ids_locked = LockedResource{ std::unique_lock(vendor_item_ids_mutex), vendor_item_ids };

            vendor_item_ids_locked->push_back(item_id);

            //BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "Attempted to add a package info with id: ({}), but it already exists ", package_id);
        }
        auto GetVendorInfosCacheSize()
        {
            std::shared_lock lock(vendor_item_ids_mutex);
            return vendor_item_ids.size();
        }
        void AddVendorInfo(const BaseLib::VendorInfo& vendor_info)
        {
            auto vendor_info_locked = LockedResource{ std::unique_lock(vendors_info_mutex), vendors_info };
            vendor_info_locked->push_back(vendor_info);
            
            AddVendorItemId(vendor_info.List01);
            AddVendorItemId(vendor_info.List01_a);
            AddVendorItemId(vendor_info.List01_b);
            AddVendorItemId(vendor_info.List01_c);
            AddVendorItemId(vendor_info.List01_d);

            AddVendorItemId(vendor_info.List02);
            AddVendorItemId(vendor_info.List02_a);
            AddVendorItemId(vendor_info.List02_b);
            AddVendorItemId(vendor_info.List02_c);
            AddVendorItemId(vendor_info.List02_d);

            AddVendorItemId(vendor_info.List03);
            AddVendorItemId(vendor_info.List03_a);
            AddVendorItemId(vendor_info.List03_b);
            AddVendorItemId(vendor_info.List03_c);
            AddVendorItemId(vendor_info.List03_d);

            AddVendorItemId(vendor_info.List04);
            AddVendorItemId(vendor_info.List04_a);
            AddVendorItemId(vendor_info.List04_b);
            AddVendorItemId(vendor_info.List04_c);
            AddVendorItemId(vendor_info.List04_d);
        }
        void AddGradeInfoCache(const std::uint32_t& grade, BaseLib::GradeInfo grade_info)
        {
            auto grades_info_locked = LockedResource{ std::unique_lock(grades_info_mutex), grades_info };

            auto [it, inserted] = grades_info_locked->emplace(grade, std::move(grade_info));

            if (!inserted)
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "Attempted to add a grade info with grade: ({}), but it already exists ", grade);
        }
        void RemoveGradeInfoCache(const std::uint32_t& grade)
        {
            auto grades_info_locked = LockedResource{ std::unique_lock(grades_info_mutex), grades_info };
            grades_info_locked->erase(grade);
        }
        auto GetGradeInfoLevelForExp(std::uint32_t current_level, std::uint32_t total_exp)
        {
            std::shared_lock lock(grades_info_mutex);

            BaseLib::GradeInfo* result = nullptr;

            for (std::uint32_t level = current_level; ; ++level)
            {
                auto it = grades_info.find(level);
                if (it == grades_info.end())
                    break;

                if (it->second.Exp >= total_exp)
                    break;

                result = &it->second;
            }

            if (result)
                return LockedResource{ std::shared_lock(grades_info_mutex), *result };

            static thread_local std::shared_mutex null_grade_mutex;
            static thread_local BaseLib::GradeInfo null_grade_info;
            return LockedResource{ std::shared_lock(null_grade_mutex), null_grade_info };
        }
        auto GetGradeInfoCache(const std::uint32_t& grade)
        {

            std::shared_lock lock(grades_info_mutex);
            auto it = grades_info.find(grade);
            if (it != grades_info.end())
                return LockedResource{ std::shared_lock(grades_info_mutex), it->second };
            else
            {
                static thread_local std::shared_mutex null_grade_mutex;
                static thread_local BaseLib::GradeInfo null_grade_info;
                return LockedResource{ std::shared_lock(null_grade_mutex), null_grade_info };
            }
        }
        auto GetGradesInfoCacheSize()
        {
            std::shared_lock lock(grades_info_mutex);
            return grades_info.size();
        }

        void AddRewardInfoCache(const std::uint32_t& gamemode, BaseLib::RewardInfo reward_info)
        {
            auto rewards_info_locked = LockedResource{ std::unique_lock(rewards_info_mutex), rewards_info };

            auto [it, inserted] = rewards_info_locked->emplace(gamemode, std::move(reward_info));

            if (!inserted)
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "Attempted to add a reward info with gamemode: ({}), but it already exists ", gamemode);
        }
        void RemoveRewardInfoCache(const std::uint32_t& gamemode)
        {
            auto rewards_info_locked = LockedResource{ std::unique_lock(rewards_info_mutex), rewards_info };
            rewards_info_locked->erase(gamemode);
        }
        auto GetRewardInfoCache(const std::uint32_t& gamemode)
        {

            std::shared_lock lock(rewards_info_mutex);
            auto it = rewards_info.find(gamemode);
            if (it != rewards_info.end())
                return LockedResource{ std::shared_lock(rewards_info_mutex), it->second };
            else
            {
                static thread_local std::shared_mutex null_reward_mutex;
                static thread_local BaseLib::RewardInfo null_reward_info;
                return LockedResource{ std::shared_lock(null_reward_mutex), null_reward_info };
            }
        }
        auto GetRewardsInfoCacheSize()
        {
            std::shared_lock lock(rewards_info_mutex);
            return rewards_info.size();
        }

        std::pair<std::vector<Item>, std::vector<Item>> TransformAddedAndDeletedItems(const std::vector<Item>& inventory_items, const std::vector<ItemSerialInfo>& items_added, const std::vector<ItemSerialInfo>& items_deleted)
        {
            std::vector<Item> valid_items;
            std::vector<Item> deleted_items;

            for (const auto& item : inventory_items)
            {
                auto it_added = std::find_if(items_added.begin(), items_added.end(),
                    [&](const ItemSerialInfo& serial) { return serial.data == item.item_info.serial_info.data; });

                auto it_deleted = std::find_if(items_deleted.begin(), items_deleted.end(),
                    [&](const ItemSerialInfo& serial) { return serial.data == item.item_info.serial_info.data; });

                if (it_added != items_added.end() && it_deleted == items_deleted.end() && item.in_database == 0)
                    valid_items.push_back(item);
                else if (it_deleted != items_deleted.end() && item.in_database == 1)
                    deleted_items.push_back(item);
            }

            return { valid_items, deleted_items };
        }
        auto TransformUpdatedItems(const std::vector<Item>& inventory_items, const std::vector<ItemSerialInfo>& items_updated, const std::vector<ItemSerialInfo>& items_deleted)
        {
            const auto& items_updated_filtered = [&items_updated, &items_deleted]() {
                std::vector<ItemSerialInfo> filtered;
                std::copy_if(items_updated.begin(), items_updated.end(), std::back_inserter(filtered),
                    [&](const ItemSerialInfo& updatedItem) {
                    return std::none_of(items_deleted.begin(), items_deleted.end(),
                        [&](const ItemSerialInfo& deletedItem) {
                        return updatedItem.data == deletedItem.data;
                    });
                });
                return filtered;
            }();

            std::vector<Item> updated_items;

            for (const auto& item : inventory_items)
            {
                auto it_updated = std::find_if(items_updated_filtered.begin(), items_updated_filtered.end(),
                    [&](const ItemSerialInfo& serial) { return serial.data == item.item_info.serial_info.data; });

                if (it_updated != items_updated_filtered.end())
                    updated_items.push_back(item);
            }
            return updated_items;
        }
        auto GetTransformRoomListInfo(const std::uint32_t& fragment_index, const std::uint32_t& max_batch_size)
        {
            std::vector<RoomListInfo> new_rooms;
            const std::uint32_t start_index = fragment_index * max_batch_size;
            const std::uint32_t end_index = std::min(start_index + max_batch_size, static_cast<std::uint32_t>(rooms_cache.size()));
            for (auto i = start_index; i < end_index; i++)
            {
                auto room = GetRoomCacheShared(i);
                if (!room->title.empty())
                {
                    auto host_cache = GetAccCacheSharedBySessionId(room->host_session_id);
                    if (host_cache->acc_info.Index != -1)
                    {
                        std::uint32_t room_size = 0;
                        if (IsModeTeamBased(room->ModeIndex))
                            room_size = static_cast<std::uint32_t>(room->redteam_session_ids.size() + room->blueteam_session_ids.size());
                        else
                            room_size = static_cast<std::uint32_t>(room->neutralteam_session_ids.size());

                        const auto& new_roomListInfo = RoomListInfo(room->title.c_str(), room->room_id, room->channel_id, room->MapIndex, room->ModeIndex, room->max_players, room_size, room->is_playing, room->has_password, room->allow_observers, room->Restriction, 1, host_cache->ping);
                        new_rooms.push_back(new_roomListInfo);
                    }
                    host_cache.unlock();
                }
            }
            return new_rooms;
        }
       
        void RoomPlayersSlotReorder(RoomCacheResource& room_cache)
        {
            //auto room_cache = GetRoomCacheShared(room_id);
            if (!room_cache->title.empty())
            {
                std::vector<std::uint32_t> players_ids;
                std::vector<std::pair<std::uint16_t, std::uint32_t>> player_slot_pairs;


                if (IsModeTeamBased(room_cache->ModeIndex))
                {
                    for (auto& id : room_cache->blueteam_session_ids)
                    {
                        auto player_cache = GetAccCacheSharedBySessionId(id);
                        if (player_cache->acc_info.Index != -1 && player_cache->in_room && player_cache->room_id == room_cache->room_id && player_cache->slot_id != 0xFF)
                            player_slot_pairs.emplace_back(id, player_cache->slot_id);

                        player_cache.unlock();
                    }
                    for (auto& id : room_cache->redteam_session_ids)
                    {
                        auto player_cache = GetAccCacheSharedBySessionId(id);
                        if(player_cache->acc_info.Index != -1 && player_cache->in_room && player_cache->room_id == room_cache->room_id && player_cache->slot_id != 0xFF)
                            player_slot_pairs.emplace_back(id, player_cache->slot_id);

                        player_cache.unlock();
                    }
                }
                else
                {
                    for (auto& id : room_cache->neutralteam_session_ids)
                    {
                        auto player_cache = GetAccCacheSharedBySessionId(id);
                        if (player_cache->acc_info.Index != -1 && player_cache->in_room && player_cache->room_id == room_cache->room_id && player_cache->slot_id != 0xFF)
                            player_slot_pairs.emplace_back(id, player_cache->slot_id);

                        player_cache.unlock();
                    }
                }
                for (auto& id : room_cache->observers_session_ids)
                {
                    auto player_cache = GetAccCacheSharedBySessionId(id);
                    if (player_cache->acc_info.Index != -1 && player_cache->in_room && player_cache->room_id == room_cache->room_id && player_cache->slot_id != 0xFF)
                        player_slot_pairs.emplace_back(id, player_cache->slot_id);

                    player_cache.unlock();
                }

                std::sort(player_slot_pairs.begin(), player_slot_pairs.end(),
                    [](const std::pair<std::uint32_t, int>& a, const std::pair<std::uint32_t, int>& b) {
                    return a.second < b.second;
                });

                for (const auto& pair : player_slot_pairs)
                    players_ids.push_back(pair.first);

                for (std::uint32_t i = 0; i < player_slot_pairs.size(); i++)
                {
                    auto room_player_cache = GetAccCacheUniqueBySessionId(player_slot_pairs[i].first);
                    if (room_player_cache->acc_info.Index != -1 && room_player_cache->in_room && room_player_cache->room_id == room_cache->room_id)
                        room_player_cache->slot_id = i;

                    room_player_cache.unlock();
                }
            }
        }

        bool SendInventoryItem(CSession* session, AccCacheResource& acc_cache, std::vector<std::uint32_t> item_ids, Items::Origin origin = Items::Origin::From_GM_Spawn)
        {
            auto send_msg = [&](CSession* session, std::uint16_t order, std::uint8_t mission, std::uint8_t extra, std::uint8_t option, std::uint8_t* data = nullptr, std::uint16_t data_size = 0)
            {
                CMessage message(session->GetEncryptionKey());
                message.SetSession(session->GetSessionId());
                message.SetCommand(order, mission, extra, option);
                if (data_size > 0 && data != nullptr) message.SetData(data, data_size);
                session->Send(message);
            };

            std::vector<ShopItem> items;
            for (auto item_id : item_ids)
            {
                auto item_info = GetItemInfoCache(item_id);
                if (item_info->Id != -1)
                {
                #if defined(RELEASE_1_0_3)
                    auto serial_index = FindLowestAvailableItemSerialInfoId(acc_cache->inventory_items);
                    ShopItem new_item = { {item_info->Id , item_info->Stock } , ItemExpire::Type::Unused, ItemSerialInfo(serial_index, 1, 1, origin, Utility::GetUtcTimeNow()) };
                    items.push_back(new_item);
                    const InventoryItemInfo& inv_item_info = { {item_info->Id , item_info->Stock } ,ItemExpire::Type::Unused,new_item.serial_info, item_info->Durability, 0 };
                    const Item& new_player_item = { inv_item_info, item_info->Stock, false , 0, false };
                    AddPlayerItemInventory(acc_cache, new_player_item);
                #else
                    auto serial_index = FindLowestAvailableItemSerialInfoId(accounts_cache[callback.session->GetSessionId()].inventory_items);
                    ShopItem new_item = { {item_info->Id , item_info->Stock } , ItemExpire::Type::Unused, ItemSerialInfo(serial_index, 1, 1, Items::Origin::From_GM_Spawn, Utility::GetUtcTimeNow()) };
                    items.push_back(new_item);
                    const InventoryItemInfo& inv_item_info = { item_info->Id ,ItemExpire::Type::Unused,new_item.serial_info, item_info->Durability, 0, 0, 0, 0, 0, AdjustItemType(item_info->Type) };
                    const Item& new_player_item = { inv_item_info, item_info->Stock, false , 0, false };
                    AddPlayerItemInventory(acc_cache, new_player_item);
                #endif
                }
                else
                    return false;
                    
            }
            send_msg(session, 99, 0, 37, static_cast<std::uint8_t>(items.size()), reinterpret_cast<uint8_t*>(items.data()), static_cast<std::uint16_t>(items.size() * sizeof(ShopItem)));
            return true;
        }
        bool SendGiftItem(CSession* session, AccCacheResource& target_acc_cache, std::uint32_t item_id, std::string msg)
        {
            auto send_msg = [&](CSession* session, std::uint16_t order, std::uint8_t mission, std::uint8_t extra, std::uint8_t option, std::uint8_t* data = nullptr, std::uint16_t data_size = 0)
            {
                CMessage message(session->GetEncryptionKey());
                message.SetSession(session->GetSessionId());
                message.SetCommand(order, mission, extra, option);
                if (data_size > 0 && data != nullptr) message.SetData(data, data_size);
                session->Send(message);
            };
            auto target_mailbox_received_count = 0;
            if (target_acc_cache->acc_info.Index == -1) //user offline
                return false;
            else
            {
                target_mailbox_received_count = static_cast<std::uint32_t>(GetGiftboxRecvCount(target_acc_cache->acc_info.Index));
            }
            if (target_mailbox_received_count >= 100)
            {
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "player acc id ({}) has gift box full", target_acc_cache->acc_info.Nickname.c_str());
                return false;
            }
                

            std::uint32_t new_mailbox_id = 0;
            MailboxInfo mailbox_info = { 0, static_cast<std::uint32_t>(target_acc_cache->acc_info.Index), "MegaVolts", static_cast<std::uint32_t>(target_acc_cache->acc_info.Index), target_acc_cache->acc_info.Nickname.c_str(), Utility::GetUtcTimeNow(), item_id, msg, true, true, false};
            if (BaseLib::Database->InsertPlayerMailbox(mailbox_info, new_mailbox_id))
            {
                mailbox_info.mail_id = new_mailbox_id;
                AddMailboxDataCache(new_mailbox_id, MailboxData(mailbox_info));
                AddGiftboxRecvIdCache(target_acc_cache->acc_info.Index, new_mailbox_id);

                
                std::uint32_t unopened_gifts = 0;
                auto mail_recv_ids = GetGiftboxRecvCacheShared(target_acc_cache->acc_info.Index);
                for (std::uint32_t i = 0; i < mail_recv_ids->size(); i++)
                {
                    auto mail_id = mail_recv_ids->at(i);
                    auto mailbox_data = GetMailboxDataCacheShared(mail_id);
                    if (mailbox_data->gift_itemid != 0) unopened_gifts++;
                }

                send_msg(session, 66, 0, 37, unopened_gifts); // remainder of unopened mails
                return true;
            }

            return false;
        }
        void SendServerMessage(CSession* session, const std::string& message)
        {
            CMessage chatMsgAck = CMessage(session->GetEncryptionKey());
            chatMsgAck.SetSession(session->GetSessionId());
            chatMsgAck.SetCommand(0x13C, 0, Chat::Type::Server, static_cast<std::uint8_t>(message.size()));
            auto msgData = MainChatAck("", message.data(), static_cast<std::uint32_t>(message.size())).Serialize(Chat::Type::Server, message.size());
            chatMsgAck.SetData(reinterpret_cast<uint8_t*>(msgData.data()), static_cast<std::uint16_t>(msgData.size()));
            session->Send(chatMsgAck);
        }
       
        constexpr std::string_view GetModeName(const std::uint32_t& index)
        {
            if (!NetEngine::Room::Mode::modeNames[index].empty())
                return NetEngine::Room::Mode::modeNames[index];

            return "Unknown Mode";
        }
        constexpr std::string_view GetMapName(const std::uint32_t& index)
        {
            if (!NetEngine::Room::Map::mapNames[index].empty())
                return NetEngine::Room::Map::mapNames[index];

            return "Unknown Map";
        }
        auto& GetItemInfoMutex()
        {
            return items_info_mutex;
        }
        auto& GetEffectInfoMutex()
        {
            return effect_info_mutex;
        }
        auto& GetCollectionInfoMutex()
        {
            return collection_info_mutex;
        }
        auto& GetSetItemInfoMutex()
        {
            return setitems_info_mutex;
        }
        auto& GetVendorInfoMutex()
        {
            return vendors_info_mutex;
        }
        auto& GetUpgradeInfoMutex()
        {
            return upgrades_info_mutex;
        }
        auto& GetGachaponInfoMutex()
        {
            return gachapons_info_mutex;
        }
        auto& GetPackageInfoMutex()
        {
            return packages_info_mutex;
        }
        auto& GetVendorItemIdsMutex()
        {
            return vendor_item_ids_mutex;
        }
        auto& GetRoomOptionsInfoMutex()
        {
            return roomoptionsinfo_cache_mutex;
        }
        auto& GetGradesInfoMutex()
        {
            return grades_info_mutex;
        }
        auto& GetRewardsInfoMutex()
        {
            return rewards_info_mutex;
        }
        auto& GetFriendsCacheMutex()
        {
            return friends_cache_mutex;
        }
        auto& GetBlockedsCacheMutex()
        {
            return blockeds_cache_mutex;
        }
        auto& GetAccountsCacheMutex()
        {
            return accounts_cache_mutex;
        }
        auto& GetRoomsCacheMutex()
        {
            return rooms_cache_mutex;
        }
        auto& GetPlazaCacheMutex()
        {
            return plaza_cache_mutex;
        }
        auto& GetRoomIdsMutex()
        {
            return room_ids_mutex;
        }
        auto& GetPartyIdsMutex()
        {
            return party_ids_mutex;
        }
        auto& GetDailyMissionIdsMutex()
        {
            return dailymission_ids_mutex;
        }
    };
    namespace Commands
    {
        using CommandFunc = std::function<void(const std::vector<std::string>& args, const SCallbackData& callback, AccCacheResource& acc_cache, CMainServer* main_server)>;
        struct Command
        {
            CommandFunc func;
            std::uint8_t required_grade;
        };
        //static std::unordered_map<std::string, Command> cmds;
        static boost::unordered_flat_map<std::string, Command> cmds;
        inline void Register(const std::string& name, CommandFunc func, const std::uint8_t& required_grade)
        {
            cmds[name] = { func, required_grade };
        }
        inline bool Execute(const std::string& name, const std::vector<std::string>& args, const SCallbackData& callback, AccCacheResource& acc_cache, CMainServer* main_server)
        {
            auto it = cmds.find(name);
            if (it != cmds.end())
            {
                const auto& command = it->second;
                if (acc_cache->acc_info.Grade >= command.required_grade)
                {
                    command.func(args, callback, acc_cache, main_server);
                    return true;
                }
            }
            return false;
        }
        inline std::vector<std::string> ListCommands(const std::uint8_t& grade)
        {
            std::vector<std::string> commands;
            std::string command_list = "[MegaVolts Online] Available commands:\n";
            commands.push_back(command_list);
            for (const auto& [name, command] : cmds)
            {
                if (grade < command.required_grade) continue;
                commands.push_back(fmt::format("/{}", name));
            }
            return commands;
        }
    }
}
