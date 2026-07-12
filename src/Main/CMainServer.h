#pragma once
#include <string>
#include <chrono>
#include <deque>
#include <functional>
#include <mutex>
#include <unordered_set>
#include <optional>
#include <numeric>
#include <random>
#include <ranges>
#include <expected>
#include "BaseLib/CLog.h"

#include "NetEngine/CServer.h"
#include "NetEngine/CSession.h"
#include "NetEngine/Constants.h"

#include "NetEngine/Packets/PacketStruct.h"
#include "NetEngine/Packets/PacketData.h"
#include <boost_unordered.hpp>
#include "BaseLib/CDatabase.h"
#include "BaseLib/CDBData.h"
#include "BaseLib/CCache.h"

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
        enum Reason : uint8_t
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
        enum Type : uint32_t
        {
            Unlimited = 0,
            Unused = 1,
            Destroyed = 2,
            Expired = 3
        };
    };
    struct EPlazaJoin
    {
        enum Result : uint8_t
        {
            Success = 0x01,
            Full = 0x07,
            NoPlaza = 0x23
        };
    };
    struct FrontAuthorize
    {
        enum Type : uint8_t
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
        enum Status : uint8_t
        {
            Normal = 0x00,
            Busy = 0x01,
            VeryBusy = 0x02,
            Offline = 0x03
        };
    };
    struct VersionCheckInfo
    {
        enum Result : uint8_t
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
        enum Result : uint8_t
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
        enum Result : uint8_t
        {
            Ok = 0x01,
            NotExist = 0x0D,
            Deny = 0x23,
            Fail
        };
    };

    struct PlayerInfo
    {
#if defined(RELEASE_1_0_3)
        enum State : uint8_t
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
#else
        enum State : uint8_t
        {
            Standby = 0,
            CharacterSelection = 1,
            SceneLoading = 2,
            SceneLoaded = 3,
            LobbyAgora = 4,
            Shop = 5,
            Inventory = 6,
            GachaponMachine = 7,
            Waiting = 8,
            PlayerReady = 9,
            Loading = 10,
            Loaded = 11,
            Normal = 12,
            Dying = 13
        };
#endif
    };
    struct ProbabilityStruct
    {
        uint32_t id;
        uint32_t prob;
        ProbabilityStruct(const uint32_t& id, const uint32_t& prob) : id(id), prob(prob) {}
    };

    inline bool g_gacha_pity_enabled = true;
    extern CCache<boost::unordered_flat_set<uint16_t>> g_tp_to_proj_sids;

    struct MatchCombatPlayerStats
    {
        uint64_t damage_dealt_raw{ 0 };
        uint32_t packet_kills{ 0 };
        uint32_t highest_kill_streak{ 0 };
    };

    struct Player
    {
        std::shared_mutex mutex;
        uint16_t session_id;
        uint16_t server_id;
        NetEngine::Packets::Core::UniqueId uid;
        uint32_t ping;
        uint8_t fps_limit;
        uint32_t room_id;
        uint32_t plaza_id;
        uint32_t party_id;
        bool playing;
        uint32_t slot_id;
        uint8_t team_id;
        uint8_t state;
        uint64_t server_time;
        bool in_room;
        bool in_plaza;
        bool in_party;
        bool sent_ping_once;
        uint32_t boss_respawn_remaining{3};
        uint8_t voice_id;
        uint32_t max_health;
        uint32_t current_health;
        uint64_t match_loaded_time;
        uint32_t earnt_battery;
        uint32_t zombie_team;
        PlayerDailyMission daily_mission_info;
        BaseLib::FrontAccount acc_info;
        std::vector<Item> inventory_items;
		bool multiple_accs_logged_in;
		uint32_t front_sid{ 0 };
		std::vector<GachaPityEntry> gacha_pity;
		std::string hwid;
		bool is_invisible{false};
        Player(const uint16_t& sessionId, const uint64_t& serverTime, const BaseLib::FrontAccount& accountInfo, const std::vector<Item>& inventoryItems)
            : session_id(sessionId), server_time(serverTime), acc_info(accountInfo), inventory_items(inventoryItems)
        {
            boss_respawn_remaining = 3;
            plaza_id = 0;
            server_id = 0;
			uid = NetEngine::Packets::Core::UniqueId(0, 0);
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
            max_health = 1000;
            current_health = 1000;
            match_loaded_time = 0;
            earnt_battery = 0;
            zombie_team = 0;
            multiple_accs_logged_in = false;
            front_sid = 0;
            gacha_pity.clear();
            hwid.clear();
        }
        Player(const Player& other)
        {
			boss_respawn_remaining = other.boss_respawn_remaining;
            session_id = other.session_id;
			server_id = other.server_id;
			uid = other.uid;
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
            max_health = other.max_health;
            current_health = other.current_health;
            match_loaded_time = other.match_loaded_time;
            earnt_battery = other.earnt_battery;
            acc_info = other.acc_info;
            inventory_items = other.inventory_items;
            daily_mission_info = other.daily_mission_info;
			multiple_accs_logged_in = other.multiple_accs_logged_in;
			front_sid = other.front_sid;
			gacha_pity = other.gacha_pity;
			hwid = other.hwid;
		}
		Player& operator=(const Player& other)
        {
            if (this == &other) return *this;
			boss_respawn_remaining = other.boss_respawn_remaining;
            session_id = other.session_id;
			server_id = other.server_id;
			uid = other.uid;
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
            max_health = other.max_health;
            current_health = other.current_health;
            match_loaded_time = other.match_loaded_time;
            earnt_battery = other.earnt_battery;
            acc_info = other.acc_info;
            inventory_items = other.inventory_items;
            daily_mission_info = other.daily_mission_info;
			multiple_accs_logged_in = other.multiple_accs_logged_in;
			front_sid = other.front_sid;
			gacha_pity = other.gacha_pity;
			hwid = other.hwid;
			return *this;
		}
        Player()
        {
			boss_respawn_remaining = 3;
            plaza_id = 0;
            party_id = 0;
            acc_info = BaseLib::FrontAccount();
            session_id = 0;
            uid = NetEngine::Packets::Core::UniqueId(0, 0);
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
            max_health = 1000;
            current_health = 1000;
            match_loaded_time = 0;
            inventory_items.clear();
			multiple_accs_logged_in = false;
			front_sid = 0;
			gacha_pity.clear();
			hwid.clear();
		}
	};
    // A mid-match leaver's session span, stashed on the room until match end (when
    // the MatchUniqueId is computed) so it can be written to player_match_sessions.
    struct PendingLeftSession
    {
        int32_t aid{ 0 };
        uint8_t team_id{ 0 };
        uint64_t joined_ms{ 0 };
        uint64_t left_ms{ 0 };
        uint8_t reason{ 1 }; // 1=Leave, 2=Kicked, 3=Disconnect
    };

    // One damaging hit buffered during the match; the match unique id is stamped
    // (and the batch persisted) at match end, same as left_sessions.
    struct PendingCombatEvent
    {
        int32_t attacker_aid{ 0 };
        int32_t victim_aid{ 0 };
        uint8_t weapon{ 0 };
        uint8_t bodypart{ 0 };
        uint8_t hit_variant{ 0xFF };
        uint32_t damage{ 0 };
        uint32_t victim_hp_after{ 0 };
        uint8_t is_kill{ 0 };
        uint64_t event_ms{ 0 };
    };

    // One non-combat match-timeline event (respawn, bomb plant/defuse, item pickup),
    // buffered during the match and flushed (with the match unique id) at match end.
    struct PendingMatchEvent
    {
        int32_t aid{ 0 };
        uint8_t event_type{ 0 }; // CastMatchEventType: 1 Respawn, 2 Bomb, 3 ItemPickup
        uint8_t sub_a{ 0 };      // Bomb: role (0 defuser, 1 planter)
        uint8_t sub_b{ 0 };      // Bomb: phase (0 start, 1 stop, 2 finish)
        uint32_t value{ 0 };     // ItemPickup: item id
        uint64_t event_ms{ 0 };
    };

    struct Room
    {
        std::shared_mutex mutex;
        uint16_t room_id, channel_id, host_session_id;
        std::string title, password;
        NetEngine::Room::Map::Index MapIndex;
        NetEngine::Room::Map::Index RandomMapIndex;
        NetEngine::Room::Mode::Index ModeIndex;
        NetEngine::Room::Restriction::Type Restriction;
        NetEngine::Room::Balance::State TeamBalance;
        uint32_t max_players, score_rule, time_rule;
        bool allow_intruders, allow_drops, allow_observers, is_playing, has_password;
        uint64_t start_time{ 0 };
        uint64_t match_instance_id{ 0 };
        std::vector<uint16_t> neutralteam_session_ids, blueteam_session_ids, redteam_session_ids, observers_session_ids;
        boost::unordered_flat_set<int32_t> kicked;
		boost::unordered_flat_set<int32_t> voters;
		boost::unordered_flat_set<int32_t> voteKickers;
        uint16_t vote_kick_target_session_id;
        bool is_kick_vote_running = false;
        bool is_clan_room;
        uint32_t clan_id_1;
        uint32_t clan_id_2;
        uint32_t team_rounds_started{ 0 };
        boost::unordered_flat_map<uint16_t, MatchCombatPlayerStats> match_combat_stats;
        std::vector<PendingLeftSession> left_sessions; // mid-match leavers, flushed at match end
        std::vector<PendingCombatEvent> combat_events; // per-hit combat log, flushed at match end
        std::vector<PendingMatchEvent> match_events;   // respawn/bomb/pickup timeline, flushed at match end
        // Round-based modes only: false during the dead window between a round ending and the
        // next round's revival, so combat hosts can't keep crediting damage/kills then. Always
        // true for non-round modes (no MatchRoundsEnd ever closes it).
        bool combat_open{ true };
        // UTC ms when combat last closed (round end). Used for a short post-close grace so the
        // deciding kill — whose combat IPC can arrive just after the round-end packet — still counts.
        uint64_t combat_closed_at{ 0 };
        // 1-based count of rounds ended this match. team_rounds_started is unreliable (0-based,
        // and never increments for non-team modes like Zombie), so RoundEnd events number rounds
        // from this instead. Reset at match start.
        uint32_t round_seq{ 0 };
        Room(const uint16_t& roomId = 0, const uint16_t& channelId = 0, const std::string& title = "", const std::string& password = "",
            const NetEngine::Room::Map::Index& mapIndex = NetEngine::Room::Map::Index::Chess, const NetEngine::Room::Mode::Index& modeIndex = NetEngine::Room::Mode::Index::TeamDeathMatch,
            const NetEngine::Room::Restriction::Type& restriction = NetEngine::Room::Restriction::AllWeapons, const NetEngine::Room::Balance::State& teamBalance = NetEngine::Room::Balance::State::Disabled,
            const uint32_t& maxPlayers = 0, const uint32_t& scoreRule = 0, const uint32_t& timeRule = 0,
            const bool& allowIntruders = true, const bool& allowDrops = true, const bool& allowObservers = true, const bool& isPlaying = false, const bool& hasPassword = false,
            const uint16_t& hostSessionId = 0)
            : room_id(roomId), channel_id(channelId), title(title), password(password), MapIndex(mapIndex), ModeIndex(modeIndex), Restriction(restriction), TeamBalance(teamBalance), 
            max_players(maxPlayers), score_rule(scoreRule), time_rule(timeRule), allow_intruders(allowIntruders), allow_drops(allowDrops), allow_observers(allowObservers),
            is_playing(isPlaying), has_password(hasPassword), host_session_id(hostSessionId), is_kick_vote_running(false), vote_kick_target_session_id(0)
        {
            neutralteam_session_ids.clear();
            blueteam_session_ids.clear();
            redteam_session_ids.clear();
            observers_session_ids.clear();
            kicked.clear();
            voters.clear();
			voteKickers.clear();
            match_combat_stats.clear();
            match_instance_id = 0;
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
			start_time = other.start_time;
            match_instance_id = other.match_instance_id;
            host_session_id = other.host_session_id;
            neutralteam_session_ids = other.neutralteam_session_ids;
            blueteam_session_ids = other.blueteam_session_ids;
            redteam_session_ids = other.redteam_session_ids;
            observers_session_ids = other.observers_session_ids;
            kicked = other.kicked;
            voters = other.voters;
			voteKickers = other.voteKickers;
            is_kick_vote_running = other.is_kick_vote_running;
            vote_kick_target_session_id = other.vote_kick_target_session_id;
            is_clan_room = other.is_clan_room;
            clan_id_1 = other.clan_id_1;
            clan_id_2 = other.clan_id_2;
            team_rounds_started = other.team_rounds_started;
            match_combat_stats = other.match_combat_stats;
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
			start_time = other.start_time;
            match_instance_id = other.match_instance_id;
            neutralteam_session_ids = other.neutralteam_session_ids;
            blueteam_session_ids = other.blueteam_session_ids;
            redteam_session_ids = other.redteam_session_ids;
            observers_session_ids = other.observers_session_ids;
            kicked = other.kicked;
            voters = other.voters;
			voteKickers = other.voteKickers;
            is_kick_vote_running = other.is_kick_vote_running;
            vote_kick_target_session_id = other.vote_kick_target_session_id;
            is_clan_room = other.is_clan_room;
            clan_id_1 = other.clan_id_1;
            clan_id_2 = other.clan_id_2;
            team_rounds_started = other.team_rounds_started;
            match_combat_stats = other.match_combat_stats;
            return *this;
        }
    };
    struct Plaza
    {
        std::shared_mutex mutex;
        uint16_t plaza_id, max_players;
        std::vector<uint16_t> session_ids;
        Plaza(const uint16_t& plazaId = 0, const uint16_t& maxSize = 2) : plaza_id(plazaId), max_players(maxSize) { session_ids.clear(); }
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
        uint32_t room_id;
        uint16_t host_session_id;
        std::vector<uint16_t> team_sessions;
        ClanMatch(const bool& is_playing = false, const uint32_t& room_id = 0, const uint16_t& host_session_id = 0) : is_playing(is_playing), room_id(room_id), host_session_id(host_session_id) { team_sessions.clear(); }
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
        uint32_t clan_id;
        std::string clan_name;
        uint32_t logo_front;
        uint32_t logo_back;
        std::vector<uint16_t> online_members;
        std::vector<ClanMatch> clan_matches;
        Clan(const uint32_t& clan_id = 0, const std::string& clan_name = "", const uint32_t& logo_front = 0, const uint32_t& logo_back = 0) : clan_id(clan_id), logo_front(logo_front), logo_back(logo_back) { online_members.clear(); clan_matches.clear(); }
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
        uint32_t party_id;
        bool is_playing;
        bool is_queueing;
        bool is_registered;
        bool is_clan;
        bool has_password;
        std::string password;
        uint8_t max_members;
        uint16_t clan_id;
        uint16_t party_host_session_id;
        std::vector<uint16_t> members;
        std::vector<uint16_t> kicked_members;
        uint16_t mod_id;
        uint16_t map_id;
        Party(const bool& is_playing = false, const bool& is_queueing = false, const bool& is_clan = false, const uint8_t& max_members = 4, const uint16_t& party_host_session_id = 0, const uint16_t& clan_id = 0, const uint16_t& mod_id = 0, const uint16_t& map_id = 0) : is_playing(is_playing), is_queueing(is_queueing), is_clan(is_clan), max_members(max_members), party_host_session_id(party_host_session_id), clan_id(clan_id), mod_id(mod_id), map_id(map_id), is_registered(is_registered) {
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
        uint32_t mail_id{};
        int32_t sender_account_id{};
        std::string sender_nickname{};
        int32_t receiver_account_id{};
        std::string receiver_nickname{};
        uint32_t time{};
        uint32_t gift_itemid{};
        std::string message{};
        bool is_new{};
        bool deleted_from_sender{};
        bool deleted_from_receiver{};
        MailboxData(const uint32_t& mailId = 0, const int32_t& senderAccountId = 0, const std::string& senderNickname = "", const int32_t& receiverAccountId = 0, const std::string& receiverNickname = "",
            const uint32_t& time = 0, const uint32_t& giftItemid = 0, const std::string& message = "", const bool& isNew = false, const bool& deletedFromSender = false, const bool& deletedFromReceiver = false)
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

    extern CCache<boost::unordered_flat_map<uint32_t, std::vector<uint32_t>>> CItemsType;
	extern CCache<boost::unordered_flat_map<uint32_t, BaseLib::ItemInfo>> CItemsInfo;
    extern CCache<boost::unordered_flat_map<uint32_t, BaseLib::SetItemInfo>> CSetItemsInfo;
    extern CCache<boost::unordered_flat_map<uint32_t, BaseLib::EffectInfo>> CEffectInfo;
    extern CCache<boost::unordered_flat_map<uint32_t, BaseLib::BaseUnitInfo>> CBaseUnitInfo;
    extern CCache<boost::unordered_flat_map<uint32_t, boost::unordered_flat_map<Items::Upgrade::Type, std::vector<BaseLib::UpgradeInfo>>>> CUpgradesInfo;

    extern CCache<boost::unordered_flat_map<uint32_t, BaseLib::CollectionInfo>> CCollectionInfo;
    extern CCache<boost::unordered_flat_map<uint32_t, BaseLib::DailyMissionInfo>> CDailyMissionInfo;
   
    extern CCache<boost::unordered_flat_map<uint32_t, BaseLib::GachaponInfo>> CGachaponsInfo;
    extern CCache<boost::unordered_flat_map<uint32_t, boost::unordered_flat_map<uint32_t, std::vector<BaseLib::PackageInfo>>>> CPackagesInfo;
    extern CCache<boost::unordered_flat_set<uint32_t>> CVendorItems;

    extern CCache<std::vector<uint32_t>> CDailyMissions;
    extern CCache<boost::unordered_flat_map<uint32_t, BaseLib::MapInfo>> CMapsInfo;
    extern CCache<boost::unordered_flat_map<uint32_t, boost::unordered_flat_map<uint32_t, std::vector<BaseLib::RoomOptionInfo>>>> CRoomOptionsInfo;
    extern CCache<boost::unordered_flat_map<uint32_t, BaseLib::GradeInfo>> CGradesInfo;
    extern CCache<boost::unordered_flat_map<uint32_t, BaseLib::RewardInfo>> CRewardsInfo;
    extern CCache<boost::unordered_flat_map<uint16_t, std::vector<BaseLib::SocialInfo>>> CSocial;

	extern CCache<boost::unordered_flat_map<uint16_t, Player>> CAccount;
	extern CCache<boost::unordered_flat_map<uint32_t, uint16_t>> CAidSid;
	extern CCache<boost::unordered_flat_map<uint64_t, uint16_t>> CAuthKey;
    extern CCache<std::vector<uint16_t>> CSid;

	extern CCache<boost::unordered_flat_map<uint32_t, Room>> CRoom;
	extern CCache<boost::unordered_flat_map<uint32_t, Plaza>> CPlaza;
	extern CCache<boost::unordered_flat_map<uint32_t, Clan>> CClan;
	extern CCache<boost::unordered_flat_map<uint16_t, Party>> CParty;
	extern CCache<std::vector<uint32_t>> CRoomId;
	extern CCache<std::vector<uint32_t>> CPartyId;

	extern CCache<boost::unordered_flat_map<uint32_t, MailboxData>> CMailboxData;
	extern CCache<boost::unordered_flat_map<uint32_t, std::vector<uint32_t>>> CMailSent;
	extern CCache<boost::unordered_flat_map<uint32_t, std::vector<uint32_t>>> CMailRecv;
	extern CCache<boost::unordered_flat_map<uint32_t, std::vector<uint32_t>>> CGiftSent;
	extern CCache<boost::unordered_flat_map<uint32_t, std::vector<uint32_t>>> CGiftRecv;

	extern CCache<boost::unordered_flat_map<uint32_t, BaseLib::GachaponSaleInfo>> CGachaponSaleInfo;
	extern CCache<std::vector<uint32_t>> CGachaponSale;

    using AccCacheResource = CLocker<std::unique_lock<std::shared_mutex>, Player>;
    using AccCacheSharedResource = CLocker<std::shared_lock<std::shared_mutex>, Player>;
    using RoomCacheResource = CLocker<std::unique_lock<std::shared_mutex>, Room>;
    using RoomCacheSharedResource = CLocker<std::shared_lock<std::shared_mutex>, Room>;
    using PlazaCacheResource = CLocker<std::unique_lock<std::shared_mutex>, Plaza>;
    using PlazaCacheSharedResource = CLocker<std::shared_lock<std::shared_mutex>, Plaza>;
    using ClanCacheResource = CLocker<std::unique_lock<std::shared_mutex>, Clan>;
    using ClanCacheSharedResource = CLocker<std::shared_lock<std::shared_mutex>, Clan>;
    using RoomOptionsCacheResource = CLocker<std::shared_lock<std::shared_mutex>, boost::unordered_flat_map<uint32_t, std::vector<BaseLib::RoomOptionInfo>>>;
    using GachaponCacheResource = CLocker<std::shared_lock<std::shared_mutex>, BaseLib::GachaponInfo>;
    using PackageCacheResource = CLocker<std::shared_lock<std::shared_mutex>, boost::unordered_flat_map<uint32_t, std::vector<BaseLib::PackageInfo>>>;
    using UpgradeCacheResource = CLocker<std::shared_lock<std::shared_mutex>, std::vector<BaseLib::UpgradeInfo>>;
    using BlockedCacheResource = CLocker<std::shared_lock<std::shared_mutex>, std::vector<BaseLib::BlockedInfo>>;
    using FriendCacheResource = CLocker<std::shared_lock<std::shared_mutex>, std::vector<BaseLib::FriendInfo>>;
	using SocialCacheResource = CLocker<std::shared_lock<std::shared_mutex>, std::vector<BaseLib::SocialInfo>>;
    
    class CMainServer : public NetEngine::CServer
    {
    public:
        CMainServer();
        ~CMainServer();

        template <auto HandlerFn, typename T>
            requires (std::integral<std::remove_cvref_t<T>> || std::is_enum_v<std::remove_cvref_t<T>>)
        void BindMainHandler(T order)
        {
            this->BindPacketHandler<HandlerFn>(this, order);
        }

        NetEngine::RateLimit::IdentitySnapshot BuildPacketRateLimitIdentitySnapshot(const SCallbackData& callback);
        void SendSessionAnnouncement(CSession* session, std::string_view message);

        
        MainAccountInfoAck CraftAccInfoAck(AccCacheResource& acc_cache, uint32_t server_id, std::string clan_name = "", uint32_t logo_front = 0,  uint32_t logo_back = 0)
        {
            MainAccountInfoAck msg{};

            msg.Diorama = 0;
            msg.Kills = acc_cache->acc_info.Kills;
            msg.Deaths = acc_cache->acc_info.Deaths;
            msg.Assists = acc_cache->acc_info.Assists;
            msg.Wins = acc_cache->acc_info.Wins;
            msg.Loses = acc_cache->acc_info.Loses;
            msg.Draws = acc_cache->acc_info.Draws;
            msg.Melee = acc_cache->acc_info.MeleeKills;
            msg.Rifle = acc_cache->acc_info.RifleKills;
            msg.Shotgun = acc_cache->acc_info.ShotgunKills;
            msg.Sniper = acc_cache->acc_info.SniperKills;
            msg.Gatling = acc_cache->acc_info.GatlingKills;
            msg.Bazooka = acc_cache->acc_info.BazookaKills;
            msg.Grenade = acc_cache->acc_info.GrenadeKills;
            msg.Headshots = acc_cache->acc_info.Headshots;
            msg.HighestKillStreak = acc_cache->acc_info.HighestKillStreak;
            msg.Unknown2 = 0;
            msg.PlayTime = static_cast<uint32_t>(acc_cache->acc_info.PlayTime);
            msg.ClanId = acc_cache->acc_info.ClanId;
            msg.ZombieKillPoints = acc_cache->acc_info.ZombieKills * 3;
            msg.Infections = acc_cache->acc_info.Infections;
            msg.UniqueId = NetEngine::Packets::Core::UniqueId(acc_cache->session_id, server_id).data;
            msg.Unknown3 = 210;
            msg.ServerTime = acc_cache->server_time;
            msg.Grade = acc_cache->acc_info.Grade;
            msg.SelectedCharacter = acc_cache->acc_info.SelectedCharacter;
            msg.OwnedCharacters = 511; 
            msg.Level = acc_cache->acc_info.Level + 1;

            msg.GoldenMode = acc_cache->acc_info.PCRoom; //PCROOM PC BANG PC ROOM
        #if defined(RELEASE_1_0_3)
            msg.Energy     = 50;
            msg.Energy2    = acc_cache->acc_info.Energy;
        #else
            msg.Coins   = acc_cache->acc_info.Coins;
            msg.Energy  = acc_cache->acc_info.Energy;
        #endif

            msg.LuckyPoints    = acc_cache->acc_info.LuckyPoints;
            msg.Experience     = acc_cache->acc_info.Experience;
            msg.MicroPoints    = acc_cache->acc_info.MicroPoints;
            msg.RockTokens     = acc_cache->acc_info.RockTokens;
            msg.Tutorial       = acc_cache->acc_info.Tutorial;
            msg.MaximumItems   = acc_cache->acc_info.MaximumItems;
            msg.MaximumEnergy  = acc_cache->acc_info.MaximumEnergy;
            msg.DailyAttempts  = acc_cache->acc_info.SingleWaveDailyAttempts;
            msg.HighestWave    = acc_cache->acc_info.SingleWaveHighestWave;
            msg.SinglewaveHighscore = acc_cache->acc_info.SingleWaveHighScore;
            msg.Unknown4 = 24;
            msg.Story          = acc_cache->acc_info.Story;
            msg.Achievements[0] = acc_cache->acc_info.Achievement;

        #if defined(RELEASE_1_1_1)
            msg.VIPLevel = acc_cache->acc_info.VIPExperience;
        #endif

            msg.AccountAuthkey = acc_cache->acc_info.AuthKey;
            std::strcpy(msg.Unused, "");

            std::strcpy(msg.Nickname, acc_cache->acc_info.Nickname.c_str());
            std::strcpy(msg.ClanName, clan_name.c_str());
            msg.ClanId = acc_cache->acc_info.ClanId;
            msg.ClanContribution = acc_cache->acc_info.ClanContribution;
            msg.ClanWins = acc_cache->acc_info.ClanWins;
            msg.ClanLoses = acc_cache->acc_info.ClanLoses;
            msg.ClanDraws = acc_cache->acc_info.ClanDraws;
            msg.ClanKills = acc_cache->acc_info.ClanKills;
            msg.ClanDeaths = acc_cache->acc_info.ClanDeaths;
            msg.ClanAssists = acc_cache->acc_info.ClanAssists;
            msg.ClanLogoBack = logo_back;
            msg.ClanLogoFront = logo_front;
            return msg;
        }
        
        auto FindFirstNonFullPlaza()
        {
            auto plazas = CPlaza.get_all(shared);
            for(auto& [id, plaza] : *plazas)
            {
                if (plaza.session_ids.size() < plaza.max_players)
                    return id;
			}
            return std::numeric_limits<uint32_t>::max();
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
        auto IsPlazaAlready(const uint32_t& plaza_id)
        {
			return CPlaza.contains(plaza_id);
        }
        uint32_t FindLowestAvailableItemSerialInfoId(const std::vector<Item>& inventory_items)
        {
            std::unordered_set<uint32_t> used_ids;

            for (const auto& item : inventory_items)
                if (item.item_info.serial_info.id >= 0 && item.item_info.serial_info.id <= 0x100000)
                    used_ids.insert(item.item_info.serial_info.id);

            for (uint32_t id = 0; id <= 0x100000; id++)
                if (used_ids.find(id) == used_ids.end())
                    return id;

            return -1;
        }
        std::vector<uint32_t> FindLowestAvailableSerialIds(const std::vector<Item>& inventory_items, size_t count)
        {
			boost::unordered_flat_set<uint32_t> used_ids;

            for (const auto& item : inventory_items)
                if (item.item_info.serial_info.id >= 0 && item.item_info.serial_info.id <= 0x100000)
                    used_ids.insert(item.item_info.serial_info.id);

            std::vector<uint32_t> reserved;
            reserved.reserve(count);

            for (uint32_t id = 0; id <= 0x100000 && reserved.size() < count; id++)
            {
                if (!used_ids.contains(id))
                {
                    reserved.push_back(id);
                    used_ids.insert(id);
                }
            }
            return reserved;
        }
        auto IsModeTeamBased(const NetEngine::Room::Mode::Index& mode)
        {
            using namespace NetEngine::Room::Mode;

            return mode == TeamDeathMatch || mode == ItemMatch
                || mode == CaptureTheBattery || mode == CloseCombat
                || mode == Elimination || mode == SuperItemMatch
                || mode == Scrimmage || mode == BombBattle 
                || mode == CLAN_CaptureTheBattery || mode == CLAN_Elimination
                || mode == CLAN_TeamDeathMatch || mode == CLAN_BombBattle;
        }
        // Round-based modes: a mid-match joiner spawns as a dead spectator until the next
        // round revives everyone (see Room/Match/Rounds/Start.h). Static so non-member
        // helpers (e.g. End.h's IsRoundBasedAdrMode) can share the single definition.
        static bool IsRoundBasedMode(const NetEngine::Room::Mode::Index& mode)
        {
            using namespace NetEngine::Room::Mode;

            return mode == Elimination || mode == ZombieMode
                || mode == BombBattle
                || mode == CLAN_Elimination || mode == CLAN_BombBattle;
        }
        auto GetRoomSortedPlayerSessionIds(RoomCacheSharedResource& room_cache)
        {
            std::vector<std::pair<uint16_t, uint32_t>> slots;

            auto addSlots = [&](std::vector<uint16_t>& team_session_ids)
            {
                for (auto& id : team_session_ids)
                {
                    auto acc = CAccount.get<shared_t>(id);
                    if (acc &&
                        acc->acc_info.Index != -1 && 
                        acc->in_room && 
                        acc->room_id == room_cache->room_id)
                        slots.emplace_back(id, acc->slot_id);

                    acc.unlock();
                }
            };
            if (IsModeTeamBased(room_cache->ModeIndex))
            {
                addSlots(room_cache->blueteam_session_ids);
                addSlots(room_cache->redteam_session_ids);
            }
            else
                addSlots(room_cache->neutralteam_session_ids);

            addSlots(room_cache->observers_session_ids);

            std::stable_sort(slots.begin(), slots.end(),
                [](const std::pair<uint16_t, uint32_t>& a, const std::pair<uint16_t, uint32_t>& b) {
                return a.second < b.second;
            });

            std::vector<uint16_t> idx;
            for (const auto& pair : slots)
                idx.push_back(pair.first);

            return idx;
        }
        auto GetRoomSortedObserversSessionIds(RoomCacheResource& room_cache)
        {
            std::vector<std::pair<uint16_t, uint32_t>> slots;

            auto addSlots = [&](std::vector<uint16_t>& team_session_ids)
                {
                    for (auto& id : team_session_ids)
                    {
                        auto acc = CAccount.get<shared_t>(id);
                        if (acc &&
                            acc->acc_info.Index != -1 &&
                            acc->in_room &&
                            acc->room_id == room_cache->room_id)
                            slots.emplace_back(id, acc->slot_id);

                        acc.unlock();
                    }
                };

            addSlots(room_cache->observers_session_ids);

            std::stable_sort(slots.begin(), slots.end(),
                [](const std::pair<uint16_t, uint32_t>& a, const std::pair<uint16_t, uint32_t>& b) {
                    return a.second < b.second;
                });

            std::vector<uint16_t> idx;
            for (const auto& pair : slots)
                idx.push_back(pair.first);

            return idx;
        }

        auto GetRoomSortedPlayerSessionIds(RoomCacheResource& room_cache)
        {
            std::vector<std::pair<uint16_t, uint32_t>> slots;

            auto addSlots = [&](std::vector<uint16_t>& team_session_ids)
                {
                    for (auto& id : team_session_ids)
                    {
                        auto acc = CAccount.get<shared_t>(id);
                        if (acc &&
                            acc->acc_info.Index != -1 &&
                            acc->in_room &&
                            acc->room_id == room_cache->room_id)
                            slots.emplace_back(id, acc->slot_id);

                        acc.unlock();
                    }
                };
            if (IsModeTeamBased(room_cache->ModeIndex))
            {
                addSlots(room_cache->blueteam_session_ids);
                addSlots(room_cache->redteam_session_ids);
            }
            else
                addSlots(room_cache->neutralteam_session_ids);

            addSlots(room_cache->observers_session_ids);

            std::stable_sort(slots.begin(), slots.end(),
                [](const std::pair<uint16_t, uint32_t>& a, const std::pair<uint16_t, uint32_t>& b) {
                return a.second < b.second;
            });

            std::vector<uint16_t> idx;
            for (const auto& pair : slots)
                idx.push_back(pair.first);

            return idx;
        }
        auto GetRoomSortedPlayerWithoutObserverSessionIds(RoomCacheResource& room_cache)
        {
            std::vector<std::pair<uint16_t, uint32_t>> slots;

            auto addSlots = [&](std::vector<uint16_t>& team_session_ids)
                {
                    for (auto& id : team_session_ids)
                    {
                        auto acc = CAccount.get<shared_t>(id);
                        if (acc &&
                            acc->acc_info.Index != -1 &&
                            acc->in_room &&
                            acc->room_id == room_cache->room_id)
                            slots.emplace_back(id, acc->slot_id);

                        acc.unlock();
                    }
                };
            if (IsModeTeamBased(room_cache->ModeIndex))
            {
                addSlots(room_cache->blueteam_session_ids);
                addSlots(room_cache->redteam_session_ids);
            }
            else
                addSlots(room_cache->neutralteam_session_ids);

           

            std::stable_sort(slots.begin(), slots.end(),
                [](const std::pair<uint16_t, uint32_t>& a, const std::pair<uint16_t, uint32_t>& b) {
                return a.second < b.second;
            });

            std::vector<uint16_t> idx;
            for (const auto& pair : slots)
                idx.push_back(pair.first);

            return idx;
        }
        auto GetRoomSortedPlayerPlayingAndObserverSessionIds(RoomCacheResource& room_cache)
        {
            std::vector<std::pair<uint16_t, uint32_t>> slots;

            auto addSlots = [&](std::vector<uint16_t>& team_session_ids)
                {
                    for (auto& id : team_session_ids)
                    {
                        auto acc = CAccount.get<shared_t>(id);
                        if (acc &&
                            acc->acc_info.Index != -1 &&
                            acc->in_room &&
                            acc->room_id == room_cache->room_id)
                            slots.emplace_back(id, acc->slot_id);

                        acc.unlock();
                    }
                };
            if (IsModeTeamBased(room_cache->ModeIndex))
            {
                addSlots(room_cache->blueteam_session_ids);
                addSlots(room_cache->redteam_session_ids);
            }
            else
                addSlots(room_cache->neutralteam_session_ids);

            addSlots(room_cache->observers_session_ids);

            std::stable_sort(slots.begin(), slots.end(),
                [](const std::pair<uint16_t, uint32_t>& a, const std::pair<uint16_t, uint32_t>& b) {
                    return a.second < b.second;
                });

            std::vector<uint16_t> idx;
            for (const auto& pair : slots)
                idx.push_back(pair.first);

            return idx;
        }
        auto GetRoomSortedPlayerPlayingWithoutObserverSessionIds(RoomCacheResource& room_cache)
        {
            std::vector<std::pair<uint16_t, uint32_t>> slots;

            auto addSlots = [&](std::vector<uint16_t>& team_session_ids)
                {
                    for (auto& id : team_session_ids)
                    {
                        auto acc = CAccount.get<shared_t>(id);
                        if (acc &&
                            acc->acc_info.Index != -1 &&
                            acc->in_room &&
                            acc->room_id == room_cache->room_id)
                            slots.emplace_back(id, acc->slot_id);

                        acc.unlock();
                    }
                };
            if (IsModeTeamBased(room_cache->ModeIndex))
            {
                addSlots(room_cache->blueteam_session_ids);
                addSlots(room_cache->redteam_session_ids);
            }
            else
                addSlots(room_cache->neutralteam_session_ids);



            std::stable_sort(slots.begin(), slots.end(),
                [](const std::pair<uint16_t, uint32_t>& a, const std::pair<uint16_t, uint32_t>& b) {
                return a.second < b.second;
            });

            std::vector<uint16_t> idx;
            for (const auto& pair : slots)
                idx.push_back(pair.first);

            return idx;
        }
        inline void TryRemoveObservers(RoomCacheResource& room)
        {
            for (const auto& id : room->observers_session_ids)
            {
                auto acc = CAccount.get<unique_t>(id);
                if (!acc ||
                    !acc->acc_info.Index ||
                    !acc->in_room ||
                    acc->room_id != room->room_id) continue;

                acc->in_room = false;
                acc->slot_id = 0;
                acc->playing = false;
                acc->state = PlayerInfo::State::Waiting;
                acc.unlock();

                if (auto pss = this->GetSessionById(id))
                    pss->SendMsg(141, 0, NetEngine::Room::Leave::Ack::Result::Leave, 0);
            }
        }
        uint16_t PickAutoHost(RoomCacheResource& room_cache, uint16_t leaving_hostSid)
        {
            const auto& sids = GetRoomSortedPlayerWithoutObserverSessionIds(room_cache);
            auto scan = [&](bool requirePlaying) ->uint16_t
                {
                    uint16_t best_sid = 0;
                    uint32_t best_ping = std::numeric_limits<uint32_t>::max();
                    for (auto sid : sids)
                    {
                        if (sid == leaving_hostSid) continue;

                        auto acc = CAccount.get<shared_t>(sid);
                        if (!acc ||
                            !acc->acc_info.Index ||
                            !acc->in_room ||
                            acc->room_id != room_cache->room_id)
                        {
                            continue;
                        }

                        bool isPlaying = acc->playing;
                        if (requirePlaying && !isPlaying) { acc.unlock(); continue; }
                        if (!requirePlaying && isPlaying) { acc.unlock(); continue; }

                        if (acc->ping < best_ping)
                        {
                            best_ping = acc->ping;
                            best_sid = sid;
                        }
                        acc.unlock();
                    }
                    return best_sid;
                };

            if (auto inMatchCandidate = scan(true); inMatchCandidate != 0)
                return inMatchCandidate;

            return scan(false);
        }
        [[nodiscard]] inline auto& GetTeamList(RoomCacheResource& room, const uint8_t team_id) noexcept 
        {
            using enum NetEngine::Team::IdType;
            const auto id = static_cast<NetEngine::Team::IdType>(team_id);
            switch (id) 
            {
                case Neutral: return room->neutralteam_session_ids;
                case Red: return room->redteam_session_ids;
                case Blue: return room->blueteam_session_ids;
                default: return room->observers_session_ids;
            }
        }
		[[nodiscard]] inline void ReorderTeamList(std::vector<uint16_t>& team_list, uint16_t sid) noexcept
		{
            auto get_slot = [&](uint16_t s) -> uint32_t {
                auto acc = CAccount.get<shared_t>(s);
                auto slot = acc->slot_id;
                acc.unlock();
                return slot;
                };

            if (auto it = std::find(team_list.begin(), team_list.end(), sid); it != team_list.end())
                team_list.erase(it);

            const uint32_t sid_slot = get_slot(sid);
            auto itIns = std::lower_bound(team_list.begin(), team_list.end(), sid_slot,
                [&](uint16_t a, uint32_t slotKey) {
                    return get_slot(a) < slotKey;
                });

            team_list.insert(itIns, sid);
		}
		const char* GetTeamNameById(const uint8_t team_id)
		{
			using enum NetEngine::Team::IdType;
			const auto id = static_cast<NetEngine::Team::IdType>(team_id);
			switch (id)
			{
				case Neutral: return "Neutral";
				case Red: return "Red";
				case Blue: return "Blue";
				default: return "Observer";
			}
		}
        void NewRemoveRoomPlayer(RoomCacheResource& room, const uint16_t sid, const uint8_t team_id, NetEngine::Room::Leave::Ack::Result leave_type, bool return_state)
        {
            const auto room_id = room->room_id;
            if (!CRoom.contains(room_id))
            {
                DEBUGLOG(dark_cyan, "dont exist room ({})", room_id);
                return;
            }
            DEBUGLOG(dark_cyan, "player ({}) leave room (observer-aware swap-with-last)", sid);

            auto acc = CAccount.get<unique_t>(sid);
            if (!acc ||
                !acc->acc_info.Index ||
                !acc->in_room ||
                acc->room_id != room_id)
            {
                return;
            }

            auto removed_uid = acc->uid;
            auto removed_teamid = acc->team_id;
            auto my_slot_id = acc->slot_id;
            acc.unlock();

            const bool is_observer = static_cast<NetEngine::Team::IdType>(removed_teamid) == NetEngine::Team::IdType::Observer;
            constexpr uint32_t observer_slot_base = 16;
            bool host_changed = false;
            uint32_t previousSlotIdNewHost = 0;
            if (!is_observer && room->host_session_id == sid)
            {
                auto new_hostSid = PickAutoHost(room, sid);
                if (new_hostSid != 0)
                {
                    room->host_session_id = new_hostSid;

                    struct RoomAuthData { uint16_t room_id; uint64_t auth_key; };

                    auto [first_sid, second_sid] = std::minmax(sid, new_hostSid);
                    auto first_lock = CAccount.get<unique_t>(first_sid);
                    auto second_lock = CAccount.get<unique_t>(second_sid);
                    if (!first_lock || !second_lock) return;
                    auto& leaving_host = (sid == first_sid) ? first_lock : second_lock;
                    auto& new_host = (new_hostSid == first_sid) ? first_lock : second_lock;

                    RoomAuthData new_host_data{ room->room_id, new_host->acc_info.AuthKey };
                    SendCastIpc(PacketIds::Ipc::MainToCastHostChange, Utility::ToVector(new_host_data));

                    previousSlotIdNewHost = new_host->slot_id;
                    new_host->slot_id = 0;
                    leaving_host->slot_id = previousSlotIdNewHost;
                    my_slot_id = previousSlotIdNewHost;

                    auto& nh_team = GetTeamList(room, new_host->team_id);
                    auto new_host_nickname = new_host->acc_info.Nickname;
                    auto leaving_host_nickname = leaving_host->acc_info.Nickname;
                    first_lock.unlock();
                    second_lock.unlock();
                    ReorderTeamList(nh_team, new_hostSid);

                    DEBUGLOG(dark_cyan,
                        "player host ({}) left room ({}), new host is ({})",
                        leaving_host_nickname.c_str(), room->room_id, new_host_nickname.c_str());
                    host_changed = true;
                }
            }

            uint32_t slot_to_erase = my_slot_id;

            auto ids = is_observer ? this->GetRoomSortedObserversSessionIds(room) : this->GetRoomSortedPlayerWithoutObserverSessionIds(room);
            if (!ids.empty())
            {
                auto last_sid = ids.back();
                auto last_acc = CAccount.get<shared_t>(last_sid);
                auto last_slot_id = last_acc ? last_acc->slot_id : 0u;
                if (last_acc) last_acc.unlock();

                if (last_sid != sid)
                {
                    auto last_acc_u = CAccount.get<unique_t>(last_sid);
                    if (last_acc_u)
                    {
                        const auto last_team_id = last_acc_u->team_id;
                        last_acc_u->slot_id = my_slot_id;
                        last_acc_u.unlock();
                        auto removed_acc2 = CAccount.get<unique_t>(sid);
                        if (removed_acc2)
                        {
                            removed_acc2->slot_id = last_slot_id;
                            removed_acc2.unlock();
                        }
                        auto& last_team = GetTeamList(room, last_team_id);
                        ReorderTeamList(last_team, last_sid);
                    }
                    slot_to_erase = last_slot_id;
                }
                else
                    slot_to_erase = last_slot_id;

                auto& team_list = GetTeamList(room, removed_teamid);
                std::erase(team_list, sid);
            }

            auto all_ids = this->GetRoomSortedPlayerSessionIds(room);
            for (const auto& id : all_ids)
            {
                if (id == sid) continue;
                if (auto pss = this->GetSessionById(id))
                {
                    if (host_changed) pss->SendMsg(128, 0, 1, static_cast<uint8_t>(previousSlotIdNewHost));
                    pss->SendMsg(422, 0, 0, static_cast<uint8_t>(slot_to_erase), reinterpret_cast<uint8_t*>(&removed_uid), static_cast<uint16_t>(sizeof(removed_uid)));
                }
            }

            auto acc_final = CAccount.get<unique_t>(sid);
            if (!acc_final) return;
            acc_final->zombie_team = 0;
            acc_final->in_room = false;
            acc_final->slot_id = 0;
            acc_final->playing = false;
            acc_final->room_id = 0;
            acc_final->state = PlayerInfo::State::Waiting;
            const auto target_aid = acc_final->acc_info.Index;
            acc_final.unlock();

            SendCastRoomLeaveSync(room_id, sid, room->host_session_id, room->is_playing);

            using enum NetEngine::Room::Leave::Ack::Result;
            switch (leave_type)
            {
            case KickedByKickVote:
            case KickedByHost:
            case KickedByGm:
            {
                if (!room->kicked.contains(target_aid))
                    room->kicked.emplace(target_aid);
                else
                    DEBUGLOG(dark_cyan, "player acc ({}) was already kicked previously", target_aid);
                break;
            }
            default: break;
            }

            if (return_state)
                if (auto left_session = this->GetSessionById(sid))
                    left_session->SendMsg(141, 0, leave_type, 0);

            if (!room->neutralteam_session_ids.empty() || !room->redteam_session_ids.empty() || !room->blueteam_session_ids.empty())
                return;

            if (!room->observers_session_ids.empty())
                TryRemoveObservers(room);

            if (CRoom.contains(room_id))
            {
                SendCastRoomRemoveSync(room_id);
                CRoom.erase(room_id);
                CRoomId.erase_value(room_id);
                DEBUGLOG(dark_cyan, "room ({}) deleted from CRoom map", room_id);
            }
            else
                DEBUGLOG(dark_cyan, "room ({}) not found in CRoom map on deletion attempt", room_id);
            this->SetRoomIdAvailable(room_id);
        }

        std::optional<Item> GetPlayerItemInventory(AccCacheResource& acc_cache, const ItemSerialInfo& serial_info)
        {
            auto it = std::ranges::find_if(acc_cache->inventory_items,
                [&serial_info](const Item& item) {
                    return item.item_info.serial_info.data == serial_info.data;
                });
            if (it != acc_cache->inventory_items.end())
                return *it;
            else
                return {};
        }
        std::optional<Item> GetPlayerItemInventory(AccCacheResource& acc_cache, const uint32_t& item_id)
        {
            auto it = std::ranges::find_if(acc_cache->inventory_items,
                [&item_id](const Item& item) {
                return item.item_info.item_number.item_id == item_id && !item.is_equipped;
				});
            if (it != acc_cache->inventory_items.end())
                return *it;
            else
                return {};
        }

        std::optional<Item> GetPlayerItemInventory(AccCacheResource& acc_cache, const uint32_t& item_type, const uint8_t& char_id)
        {

            auto it = std::ranges::find_if(acc_cache->inventory_items,
                [&item_type, char_id, this](const Item& item) {
                    if (item.character_id == char_id && !item.is_equipped)
                    {
                        auto item_info = CItemsInfo.get<shared_t>(item.item_info.item_number.item_id);
                        return (item_info->Type == item_type);
                    }
                    else return false;
                });

            if (it != acc_cache->inventory_items.end())
                return *it;
            else
                return {};
        }
        std::optional<Item> GetPlayerEquippedItem(AccCacheResource& acc_cache, const uint32_t& item_type, const uint8_t& char_id)
        {
            auto it = std::ranges::find_if(acc_cache->inventory_items,
                [&item_type, char_id, this](const Item& item)
                {
                    if (item.character_id == char_id && item.is_equipped)
                    {
                        auto item_info = CItemsInfo.get<shared_t>(item.item_info.item_number.item_id);
                        return item_info->Type == item_type;
                    }
                    return false;
                });

            if (it != acc_cache->inventory_items.end())
                return *it;
            else
                return {};
        }

        void DisconnectPlayer(const uint16_t& session_id, const uint8_t& reason)
        {
            if (auto player_session = GetSessionById(session_id))
            {
                player_session->SendMsg(73, 0, reason, 0);
                player_session.get()->Disconnect();
                DEBUGLOG(dark_cyan, "MainToCastDisconnectPlayer sid=({})", session_id);
            }
            else
                DEBUGLOG(red, "couldn't forcefully disconnect sid=({})", session_id);

        }
        auto IsBlockedAlready(SocialCacheResource& socials, int32_t aid)
        {
            auto it = std::ranges::find_if(*socials,
                [&aid](const BaseLib::SocialInfo& social_info) {
                    return social_info.targetAid == aid && social_info.State == Socials::State::Blocked;
				});
            return it != socials->end();
        }
        
        [[nodiscard]] std::optional<std::reference_wrapper<const BaseLib::SocialInfo>>
            GetPlayerSocial(SocialCacheResource& s, int32_t aid)
        {
            if (auto it = std::ranges::find(*s, aid, &BaseLib::SocialInfo::targetAid); it != s->end())
                return std::cref(*it);
            return std::nullopt;
        }

        auto GetRandomDailyMissionIds(uint32_t count, uint32_t id1, uint32_t id2, uint32_t id3) 
        {
            auto ids = CDailyMissions.get_all(shared);
            std::vector<uint32_t> pool;
            pool.reserve(ids->size());
            for (auto& id : *ids) 
                if (id != id1 && id != id2 && id != id3)
                    pool.push_back(id);


            std::vector<uint32_t> result;
            result.reserve(count);
            static thread_local std::mt19937 gen{ std::random_device{}() };
            std::sample(pool.begin(), pool.end(), std::back_inserter(result), count, gen);
            return result;
        } 

        [[nodiscard]] static bool IsMapAvailableForMode(const BaseLib::MapInfo& map_info,
                                                        const NetEngine::Room::Mode::Index mode)
        {
            using enum NetEngine::Room::Mode::Index;

            switch (mode)
            {
            case TeamDeathMatch:        return map_info.tdm;
            case FreeForAll:            return map_info.ffa;
            case ItemMatch:             return map_info.itm;
            case CaptureTheBattery:     return map_info.ctm;
            case CloseCombat:           return map_info.cim;
            case Elimination:           return map_info.sab;
            case SuperItemMatch:        return map_info.itm || map_info.mock;
            case ZombieMode:            return map_info.zsm;
            case ArmsRace:              return map_info.grm;
            case Scrimmage:             return map_info.mock;
            case BombBattle:            return map_info.bmb;
            case BossBattle:            return map_info.pve;
            case CLAN_CaptureTheBattery:return map_info.clan_ctf;
            case CLAN_Elimination:      return map_info.clan_sab;
            case CLAN_TeamDeathMatch:   return map_info.clan_tdm;
            case CLAN_BombBattle:       return map_info.clan_bmb;
#if defined(RELEASE_1_0_3)
            case CLAN_Random:
                return map_info.clan_ctf || map_info.clan_sab || map_info.clan_tdm || map_info.clan_bmb;
#endif
            default:
                return false;
            }
        }

        [[nodiscard]] std::optional<NetEngine::Room::Map::Index>
            GetRandomMapIndexForMode(const NetEngine::Room::Mode::Index mode, const uint32_t max_players)
        {
            auto maps = CMapsInfo.get_all(shared);
            std::vector<NetEngine::Room::Map::Index> pool;
            pool.reserve(maps->size());

            for (const auto& [map_id, map_info] : *maps)
            {
                if (map_id == static_cast<uint32_t>(NetEngine::Room::Map::Index::Random))
                    continue;

                if (!IsMapAvailableForMode(map_info, mode))
                    continue;

                if (max_players != 0 && map_info.MaxUsers != 0 && map_info.MaxUsers < max_players)
                    continue;

                pool.push_back(static_cast<NetEngine::Room::Map::Index>(map_id));
            }

            maps.unlock();

            if (pool.empty())
                return std::nullopt;

            static thread_local std::mt19937 gen{ std::random_device{}() };
            std::uniform_int_distribution<std::size_t> dist(0, pool.size() - 1);
            return pool[dist(gen)];
        }

        BaseLib::RoomOptionInfo GetRoomOptionInfoByTypeCache(RoomOptionsCacheResource& infos, const uint32_t& type, const uint32_t& data)
        {

            if (auto it = infos->find(type); it != infos->end())
                if (auto r = std::ranges::find(it->second, data, &BaseLib::RoomOptionInfo::Data);
                    r != it->second.end())
                    return *r;

            return {};
        }

        //   Weighted random selection (classic "raffle ticket" algorithm).
        //   This method is very common in Asian online games (Korean, Japanese, Chinese).
        //   In "gacha" or loot-box systems, each item is assigned a weight ("probability")
        //   instead of a direct percentage. The game server treats these weights like 
        //   a bag of raffle tickets: an item with weight 50,000 has 50,000 tickets,
        //   while one with weight 500 has only 500. The server draws one random ticket,
        //   and whichever item "owns" it is the winner. 

        template <class It, class Proj>
        [[nodiscard]] inline std::optional<std::size_t> ExtractIndex(It first, It last, Proj proj) noexcept
        {
            if (first == last) return std::nullopt;
            uint32_t total = 0;
            for (auto it = first; it != last; it++) // compute total weight
                total += static_cast<uint32_t>(proj(*it));
            if (total == 0) return std::nullopt;
			const uint32_t r = Utility::Random::CustomGen(0, total - 1);
            uint32_t acc = 0;
            std::size_t idx = 0;
            for (auto it = first; it != last; it++, idx++)
            {
				acc += static_cast<uint32_t>(proj(*it)); // accumulate weight
                if (r < acc) return idx; // found item 
            }
            return std::nullopt;
        }
        template <class Container, class Proj>
        [[nodiscard]] inline std::optional<std::size_t> ExtractIndex(const Container& c, Proj proj) noexcept
        {
            return ExtractIndex(std::begin(c), std::end(c), proj);
        }
        auto ExtractPackageItemsWon(PackageCacheResource& package_info)
        {
            
            std::vector<BaseLib::PackageInfo> out;

            for (const auto& [groupId, itemVec] : *package_info) 
                if (auto idx = ExtractIndex(itemVec, [](const PackageInfo& e){ return e.Probability; }))
                    out.push_back(itemVec[*idx]);

            return out;      
        }
        auto ExtractGachaponItemsWon(GachaponCacheResource& gachapon_info, std::vector<GachaponPackageItem>& out, uint32_t coupon_chance)
        {
            for (const auto& [groupId, itemVec] : gachapon_info->Gachapons) 
                if (auto idx = ExtractIndex(itemVec, [](const GachaponPackageItem& e){ return e.Probability; }))
                    out.push_back(itemVec[*idx]);
            return Utility::Random::CustomGen(0, 100) < coupon_chance;
        }    
        std::optional<GachaponPackageItem> ExtractGachaponRareItem(GachaponCacheResource& gachapon_info)
        {
            std::vector<GachaponPackageItem> rare_items;
            for (const auto& [groupId, itemVec] : gachapon_info->Gachapons)
                for (const auto& item : itemVec)
                    if (item.ItemType == Items::Gachapon::Rarity::Rare)
                        rare_items.push_back(item);
            if (rare_items.empty()) return std::nullopt;
            if (auto idx = ExtractIndex(rare_items, [](const GachaponPackageItem& e) { return e.Probability; }))
                return rare_items[*idx];
            return rare_items[0];
        }
        auto GetUpgradeCollectionInfoCache(const Items::Upgrade::Type& upgrade_type, const uint32_t& item_id)
        {
            if (auto vecLock = CUpgradesInfo.get<BaseLib::shared_t>(item_id, upgrade_type);
                vecLock && !vecLock.is_null())
            {
                for (const auto& ui : *vecLock)
                    if (ui.ItemId == item_id)
                        return vecLock;
            }

            auto all = CUpgradesInfo.get_all(BaseLib::shared);
            for (const auto& [groupKey, /*byType*/ _] : *all) {
                if (groupKey == item_id) continue;
                auto vecLock = CUpgradesInfo.get<BaseLib::shared_t>(groupKey, upgrade_type);
                if (vecLock && !vecLock.is_null()) {
                    for (const auto& ui : *vecLock)
                        if (ui.ItemId == item_id)
                            return vecLock;
                }
            }
            using CacheT = decltype(CUpgradesInfo);
            using MutexT = typename CacheT::mutex_type;
            using VectorT = std::vector<BaseLib::UpgradeInfo>;
            static thread_local MutexT null_mtx;
            static thread_local VectorT empty{};

            return CLocker{ std::shared_lock<MutexT>(null_mtx), empty, true };
        }
        uint32_t GetUpgradeLevel(UpgradeCacheResource& upgrade_collection, const uint32_t& id)
        {
            for (uint32_t i = 0; i < upgrade_collection->size(); ++i)
                if (upgrade_collection->at(i).ItemId == id)
                    return i;

            return 0;
        }
        auto GetUpgradeInfoNext(UpgradeCacheResource& upgrade_collection, const uint32_t& id)
        {
            for (uint32_t i = 0; i < upgrade_collection->size(); ++i)
                if (upgrade_collection->at(i).ItemId == id && i + 1 < upgrade_collection->size())
                    return upgrade_collection->at(i + 1);

            return BaseLib::UpgradeInfo();
        }
        auto GetUpgradeInfoPrev(UpgradeCacheResource& upgrade_collection, const uint32_t& id)
        {
            for (uint32_t i = 0; i < upgrade_collection->size(); ++i)
                if (upgrade_collection->at(i).ItemId == id && i > 0)
                    return upgrade_collection->at(i - 1);

            return BaseLib::UpgradeInfo();
        }
        
        auto GetUpgradeInfoCache(const uint32_t& item_id)
        {
            using CacheT = decltype(CUpgradesInfo);
            using MutexT = typename CacheT::mutex_type;
            using Outer = typename CacheT::container_type;
            using GroupMap = typename Outer::mapped_type;
            using Vec = typename GroupMap::mapped_type;
            using Info = typename Vec::value_type;

            std::shared_lock<MutexT> lk(CUpgradesInfo.mutex());
            auto& cont = CUpgradesInfo.unsafe_ref();

            if (auto g = cont.find(item_id); g != cont.end())
            {
                for (auto& [_, vec] : g->second)
                {
                    if (auto it = std::find_if(vec.begin(), vec.end(),
                        [&](const Info& ui) { return ui.ItemId == item_id; });
                        it != vec.end())
                    {
                        return CLocker{ std::move(lk), *it };
                    }
                }
            }
            for (auto& [groupKey, buckets] : cont)
            {
                if (groupKey == item_id) continue; 
                for (auto& [_, vec] : buckets)
                {
                    if (auto it = std::find_if(vec.begin(), vec.end(),
                        [&](const Info& ui) { return ui.ItemId == item_id; });
                        it != vec.end())
                    {
                        return CLocker{ std::move(lk), *it };
                    }
                }
            }

            static thread_local MutexT null_mtx;
            static thread_local Info   empty{};
            return CLocker{ std::shared_lock<MutexT>(null_mtx), empty, true };
        }
        void TransformEquippedItems(const std::vector<Item>& items, boost::unordered_flat_map<uint8_t, std::vector<InventoryItemInfo>>& equipped_items)
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
        auto GetTransformStockItems(const std::vector<Item>& items, const uint32_t& fragment_index = 0, const uint32_t& fragment_max_size = 35)
        {
            std::vector<InventoryItemInfo> new_items;
            const uint32_t start_index = fragment_index * fragment_max_size;
            const uint32_t end_index = std::min(start_index + fragment_max_size, static_cast<uint32_t>(items.size()));
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
            for (uint32_t i = 0; i < items.size(); i++)
            {
                EquipItemInfo new_item = EquipItemInfo(items[i]);
                auto item_info = CItemsInfo.get<shared_t>(new_item.item_number.item_id);
                new_item.item_number.item_type = item_info->Type;
                new_items.push_back(new_item);
            }
            return new_items;
        }
        auto GetItemByType(const std::vector<BaseLib::Item>& equipped_items, const uint32_t& item_type)
        {
            auto it = std::ranges::find_if(equipped_items,
                [this, item_type](const Item& item)
                {
                    auto item_info = CItemsInfo.get<shared_t>(item.item_info.item_number.item_id);
                    return item_info->Type == item_type;
				});
            if (it != equipped_items.end())
                return *it;
            else
                return BaseLib::Item();
        }
        std::vector<BaseLib::Item> GetEquippedInventoryItems(const std::vector<BaseLib::Item>& inventory_items, const uint8_t& selected_character)
        {
            std::vector<BaseLib::Item> equipped;
            equipped.reserve(18);
            std::copy_if(inventory_items.begin(), inventory_items.end(), std::back_inserter(equipped), [&](const BaseLib::Item& it)
                {
                    return it.is_equipped == 1 && it.character_id == selected_character;
                });
            return equipped;
        }
        uint32_t GetCharacterBaseHealth(const uint8_t& character_type)
        {
            auto base_info = CBaseUnitInfo.get<shared_t>(character_type);
            if (base_info->Type == character_type && base_info->Life)
                return base_info->Life;
            return 1000;
        }
        uint32_t GetEffectHealthBonus(const uint32_t& effect_id)
        {
            if (!effect_id || effect_id == UINT32_MAX)
                return 0;

            auto effect_info = CEffectInfo.get<shared_t>(effect_id);
            if (!effect_info->id || effect_info->key != 3)
                return 0;

            return effect_info->valueA;
        }
        uint32_t GetItemHealthBonus(const uint32_t& item_id)
        {
            if (!item_id)
                return 0;

            auto item_info = CItemsInfo.get<shared_t>(item_id);
            if (!item_info->Id)
                return 0;

            return GetEffectHealthBonus(item_info->EffectId1) +
                GetEffectHealthBonus(item_info->EffectId2) +
                GetEffectHealthBonus(item_info->EffectId3);
        }
        uint32_t GetSetSlotEffectId(const BaseLib::SetItemInfo& set_info, const uint8_t& item_type)
        {
            switch (item_type)
            {
            case 0: return set_info.Hair;
            case 1: return set_info.Face;
            case 2: return set_info.Upper;
            case 3: return set_info.Under;
            case 4: return set_info.Pants;
            case 5: return set_info.Arms;
            case 6: return set_info.Boots;
            case 7: return set_info.AccessoryA;
            case 8: return set_info.AccessoryB;
            case 9: return set_info.AccessoryC;
            default: return 0;
            }
        }
        bool DoesSetOccupySlot(const uint32_t& set_field_value)
        {
            return set_field_value != UINT32_MAX;
        }
        bool DoesSetOccupySlot(const BaseLib::SetItemInfo& set_info, const uint8_t& item_type)
        {
            return DoesSetOccupySlot(GetSetSlotEffectId(set_info, item_type));
        }
        uint32_t ResolveEquippedCostumeItemId(const uint32_t& direct_item_id, const uint32_t& set_field_value, const uint32_t& set_item_id)
        {
            if (direct_item_id)
                return direct_item_id;
            return (set_item_id && DoesSetOccupySlot(set_field_value)) ? set_item_id : 0;
        }
        uint32_t GetEquippedHealthBonus(const std::vector<BaseLib::Item>& inventory_items, const uint8_t& selected_character)
        {
            const auto equipped = GetEquippedInventoryItems(inventory_items, selected_character);
            const auto set_item_id = GetItemByType(equipped, 25).item_info.item_number.item_id;
            auto set_info = CSetItemsInfo.get<shared_t>(set_item_id);

            uint32_t bonus = 0;
            for (uint8_t item_type = 0; item_type <= 9; ++item_type)
            {
                const auto item_id = GetItemByType(equipped, item_type).item_info.item_number.item_id;
                if (item_id)
                    bonus += GetItemHealthBonus(item_id);
                else if (set_info->Id && DoesSetOccupySlot(*set_info, item_type))
                    bonus += GetEffectHealthBonus(GetSetSlotEffectId(*set_info, item_type));
            }

            return bonus;
        }
        uint32_t GetPlayerMaxHealth(const std::vector<BaseLib::Item>& inventory_items, const uint8_t& selected_character)
        {
            return std::max<uint32_t>(1u, GetCharacterBaseHealth(selected_character) + GetEquippedHealthBonus(inventory_items, selected_character));
        }
        uint32_t GetPlayerMaxHealth(const BaseLib::FrontAccount& acc_info, const std::vector<BaseLib::Item>& inventory_items)
        {
            return GetPlayerMaxHealth(inventory_items, static_cast<uint8_t>(acc_info.SelectedCharacter));
        }
        void RefreshPlayerHealthCache(Player& player, const bool reset_current = false)
        {
            player.max_health = GetPlayerMaxHealth(player.acc_info, player.inventory_items);
            if (reset_current || !player.current_health || player.current_health > player.max_health)
                player.current_health = player.max_health;
        }
        void RefreshPlayerHealthCache(AccCacheResource& acc, const bool reset_current = false)
        {
            RefreshPlayerHealthCache(*acc, reset_current);
        }
        void SendCastPlayerHealthSync(const uint16_t& session_id, const uint32_t& max_health, const uint32_t& current_health)
        {
            NetEngine::Packets::Ipc::MainToCastPlayerHealthSync sync{ session_id, max_health, current_health };
            SendCastIpc(PacketIds::Ipc::MainToCastPlayerHealthSync, Utility::ToVector(sync));
        }
        void SendCastPlayerHealthSync(AccCacheResource& acc)
        {
            SendCastPlayerHealthSync(acc->session_id, acc->max_health, acc->current_health);
        }
        void SendCastRoomLifecycleSync(const NetEngine::Packets::Ipc::MainToCastRoomLifecycleAction action,
            const uint16_t room_id,
            const uint16_t sid = 0,
            const uint16_t host_session_id = 0,
            const bool is_playing = false)
        {
            NetEngine::Packets::Ipc::MainToCastRoomLifecycleSync sync
            {
                action,
                room_id,
                sid,
                host_session_id,
                static_cast<uint8_t>(is_playing ? 1 : 0)
            };
            SendCastIpc(PacketIds::Ipc::MainToCastRoomLifecycleSync, Utility::ToVector(sync));
        }
        void SendCastRoomCreateSync(const uint16_t room_id, const uint16_t host_session_id)
        {
            SendCastRoomLifecycleSync(NetEngine::Packets::Ipc::MainToCastRoomLifecycleAction::Create,
                room_id,
                host_session_id,
                host_session_id,
                false);
        }
        void SendCastRoomJoinSync(const uint16_t room_id, const uint16_t sid, const uint16_t host_session_id, const bool is_playing)
        {
            SendCastRoomLifecycleSync(NetEngine::Packets::Ipc::MainToCastRoomLifecycleAction::Join,
                room_id,
                sid,
                host_session_id,
                is_playing);
        }
        void SendCastRoomLeaveSync(const uint16_t room_id, const uint16_t sid, const uint16_t host_session_id, const bool is_playing)
        {
            SendCastRoomLifecycleSync(NetEngine::Packets::Ipc::MainToCastRoomLifecycleAction::Leave,
                room_id,
                sid,
                host_session_id,
                is_playing);
        }
        void SendCastRoomRemoveSync(const uint16_t room_id)
        {
            SendCastRoomLifecycleSync(NetEngine::Packets::Ipc::MainToCastRoomLifecycleAction::Remove,
                room_id,
                0,
                0,
                false);
        }
        void SendCastRoomMatchStateSync(const uint16_t room_id, const uint16_t host_session_id, const bool is_playing)
        {
            SendCastRoomLifecycleSync(NetEngine::Packets::Ipc::MainToCastRoomLifecycleAction::MatchState,
                room_id,
                0,
                host_session_id,
                is_playing);
        }
        auto IsItemWeapon(const uint32_t& item_id)
        {
            auto item_info = CItemsInfo.get<shared_t>(item_id);
            return item_info->Type == Items::WeaponItems::Type::Melee ||
                item_info->Type == Items::WeaponItems::Type::Rifle ||
                item_info->Type == Items::WeaponItems::Type::Shotgun ||
                item_info->Type == Items::WeaponItems::Type::Sniper ||
                item_info->Type == Items::WeaponItems::Type::Gatling ||
                item_info->Type == Items::WeaponItems::Type::Bazooka ||
                item_info->Type == Items::WeaponItems::Type::Grenade;
        }
        auto IsItemCostume(const uint32_t& item_id)
        {
            auto item_info = CItemsInfo.get<shared_t>(item_id);

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
        auto IsItemDiorama(const uint32_t& item_id)
        {
            auto item_info = CItemsInfo.get<shared_t>(item_id);

            return item_info->Type == 22 || item_info->Type == 23;
        }
        auto IsItemSet(const uint32_t& item_id)
        {
            auto setitem_info = CSetItemsInfo.get<shared_t>(item_id);
            return setitem_info->Id != 0;
        }
        uint32_t AdjustItemType(const uint32_t& item_type)
        {
            if (item_type == static_cast<uint32_t>(Items::OtherItems::Type::Question) ||
                item_type == static_cast<uint32_t>(Items::OtherItems::Type::ShieldEnamel) ||
                item_type == static_cast<uint32_t>(Items::OtherItems::Type::FlagBlue) ||
                item_type == static_cast<uint32_t>(Items::OtherItems::Type::BombDrop) ||
                item_type == static_cast<uint32_t>(Items::OtherItems::Type::GatchaItem) ||
                item_type == static_cast<uint32_t>(Items::OtherItems::Type::Question1) ||
                item_type == static_cast<uint32_t>(Items::OtherItems::Type::MonsterFace) ||
                item_type == static_cast<uint32_t>(Items::OtherItems::Type::Undefined1) ||
                item_type == static_cast<uint32_t>(Items::OtherItems::Type::Undefined2))
            {
                return 17;
            }

            if (item_type == static_cast<uint32_t>(Items::DioramaItems::Type::Footing)) return 19;
            if (item_type == static_cast<uint32_t>(Items::DioramaItems::Type::Object)) return 20;

            return item_type;
        }

        auto GetSetItemTypes(const uint32_t& item_id)
        {
            auto setitem_info = CSetItemsInfo.get<shared_t>(item_id);
            std::vector<uint32_t> types;
            if (setitem_info->Id)
            {
                if (DoesSetOccupySlot(setitem_info->Hair)) types.push_back(0);
                if (DoesSetOccupySlot(setitem_info->Face)) types.push_back(1);
                if (DoesSetOccupySlot(setitem_info->Upper)) types.push_back(2);
                if (DoesSetOccupySlot(setitem_info->Under)) types.push_back(3);
                if (DoesSetOccupySlot(setitem_info->Pants)) types.push_back(4);
                if (DoesSetOccupySlot(setitem_info->Arms)) types.push_back(5);
                if (DoesSetOccupySlot(setitem_info->Boots)) types.push_back(6);
                if (DoesSetOccupySlot(setitem_info->AccessoryA)) types.push_back(7);
                if (DoesSetOccupySlot(setitem_info->AccessoryB)) types.push_back(8);
                if (DoesSetOccupySlot(setitem_info->AccessoryC)) types.push_back(9);
            }
            return types;
        }
        std::string GetCharacterStr(const uint8_t& char_id)
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
        auto GetRoomUserPlayerInfo1(AccCacheResource& acc)
        {
            RoomUserPlayerInfo1 info{};
            info.grade = acc->acc_info.Grade;
            info.vip_level = acc->acc_info.PCRoom;
            info.character = acc->acc_info.SelectedCharacter;
            info.team = acc->team_id;
            info.level = acc->acc_info.Level + 1;
            info.ping = acc->ping;
            return info;
		}
        auto GetRoomUserPlayerInfo1(AccCacheSharedResource& acc)
        {
            RoomUserPlayerInfo1 info{};
            info.grade = acc->acc_info.Grade;
            info.vip_level = acc->acc_info.PCRoom;
            info.character = acc->acc_info.SelectedCharacter;
            info.team = acc->team_id;
            info.level = acc->acc_info.Level + 1;
            info.ping = acc->ping;
            return info;
        }
        auto GetRoomUserPlayerInfo2(AccCacheResource& acc)
        {
            RoomUserPlayerInfo2 info{};
            info.player_state = acc->state;
            info.ping = acc->ping;
            info.fps_limit = acc->fps_limit;
            return info;
        }
        auto GetRoomUserPlayerInfo2(AccCacheSharedResource& acc)
        {
            RoomUserPlayerInfo2 info{};
            info.player_state = acc->state;
            info.ping = acc->ping;
            info.fps_limit = acc->fps_limit;
            return info;
        }

        std::vector<EquipItemNumber> GetEquippedItems(AccCacheSharedResource& acc)
        {
            static constexpr std::size_t kMaxEquippedSlots = 17;
            std::vector<EquipItemNumber> equipped_items;
            equipped_items.reserve(kMaxEquippedSlots);

            const auto sel_char = static_cast<uint8_t>(acc->acc_info.SelectedCharacter);
            std::vector<BaseLib::Item> equipped;
            equipped.reserve(kMaxEquippedSlots);
            std::copy_if(acc->inventory_items.begin(), acc->inventory_items.end(), std::back_inserter(equipped), [&](const BaseLib::Item& it)
                {
                    return it.is_equipped == 1 && it.character_id == sel_char;
                });
            auto item_id_of = [&](uint8_t type) { return GetItemByType(equipped, type).item_info.item_number.item_id; };
            const uint32_t set_item_id = item_id_of(25);
            auto setinfo = CSetItemsInfo.get<shared_t>(set_item_id);//GetSetItemInfoCache(set_item_id);
            auto fallback = [&](uint32_t direct, uint32_t set_field_value)
                {
                    return ResolveEquippedCostumeItemId(direct, set_field_value, setinfo->Id);
                };

            const auto hair = item_id_of(0);
            const auto face = item_id_of(1);
            const auto upper = item_id_of(2);
            const auto under = item_id_of(3);
            const auto skirt = item_id_of(4);
            const auto gloves = item_id_of(5);
            const auto boots = item_id_of(6);
            const auto accH = item_id_of(7);
            const auto accW = item_id_of(8);
            const auto accB = item_id_of(9);

            equipped_items.emplace_back(EquipItemNumber(fallback(hair, setinfo->Hair), 0)); //hair
            equipped_items.emplace_back(EquipItemNumber(fallback(face, setinfo->Face), 1)); //face
            equipped_items.emplace_back(EquipItemNumber(fallback(upper, setinfo->Upper), 2)); //upper
            equipped_items.emplace_back(EquipItemNumber(fallback(under, setinfo->Under), 3)); //under
            equipped_items.emplace_back(EquipItemNumber(fallback(skirt, setinfo->Pants), 4)); //skirt
            equipped_items.emplace_back(EquipItemNumber(fallback(gloves, setinfo->Arms), 5)); //gloves
            equipped_items.emplace_back(EquipItemNumber(fallback(boots, setinfo->Boots), 6)); //boots
            equipped_items.emplace_back(EquipItemNumber(fallback(accH, setinfo->AccessoryA), 7)); //head acc
            equipped_items.emplace_back(EquipItemNumber(fallback(accW, setinfo->AccessoryB), 8)); //waist acc
            equipped_items.emplace_back(EquipItemNumber(fallback(accB, setinfo->AccessoryC), 9)); //back acc
            equipped_items.emplace_back(EquipItemNumber(item_id_of(10), 10)); //melee
            equipped_items.emplace_back(EquipItemNumber(item_id_of(11), 11)); //rifle
            equipped_items.emplace_back(EquipItemNumber(item_id_of(12), 12)); //shotgun
            equipped_items.emplace_back(EquipItemNumber(item_id_of(13), 13)); //sniper
            equipped_items.emplace_back(EquipItemNumber(item_id_of(14), 14)); //gatling
            equipped_items.emplace_back(EquipItemNumber(item_id_of(15), 15)); //bazooka
            equipped_items.emplace_back(EquipItemNumber(item_id_of(16), 16)); //grenade

            return equipped_items;
        }

        std::vector<EquipItemNumber> GetEquippedItems(AccCacheResource& acc)
        {
            static constexpr std::size_t kMaxEquippedSlots = 17;
            std::vector<EquipItemNumber> equipped_items;
            equipped_items.reserve(kMaxEquippedSlots);

            const auto sel_char = static_cast<uint8_t>(acc->acc_info.SelectedCharacter);
            std::vector<BaseLib::Item> equipped;
            equipped.reserve(kMaxEquippedSlots);
            std::copy_if(acc->inventory_items.begin(), acc->inventory_items.end(), std::back_inserter(equipped), [&](const BaseLib::Item& it)
                {
                    return it.is_equipped == 1 && it.character_id == sel_char;
                });
            auto item_id_of = [&](uint8_t type) { return GetItemByType(equipped, type).item_info.item_number.item_id; };
            const uint32_t set_item_id = item_id_of(25);
            auto setinfo = CSetItemsInfo.get<shared_t>(set_item_id);//GetSetItemInfoCache(set_item_id);
            auto fallback = [&](uint32_t direct, uint32_t set_field_value)
                {
                    return ResolveEquippedCostumeItemId(direct, set_field_value, setinfo->Id);
                };

            const auto hair = item_id_of(0);
            const auto face = item_id_of(1);
            const auto upper = item_id_of(2);
            const auto under = item_id_of(3);
            const auto skirt = item_id_of(4);
            const auto gloves = item_id_of(5);
            const auto boots = item_id_of(6);
            const auto accH = item_id_of(7);
            const auto accW = item_id_of(8);
            const auto accB = item_id_of(9);

            equipped_items.emplace_back(EquipItemNumber(fallback(hair, setinfo->Hair), 0)); //hair
            equipped_items.emplace_back(EquipItemNumber(fallback(face, setinfo->Face), 1)); //face
            equipped_items.emplace_back(EquipItemNumber(fallback(upper, setinfo->Upper), 2)); //upper
            equipped_items.emplace_back(EquipItemNumber(fallback(under, setinfo->Under), 3)); //under
            equipped_items.emplace_back(EquipItemNumber(fallback(skirt, setinfo->Pants), 4)); //skirt
            equipped_items.emplace_back(EquipItemNumber(fallback(gloves, setinfo->Arms), 5)); //gloves
            equipped_items.emplace_back(EquipItemNumber(fallback(boots, setinfo->Boots), 6)); //boots
            equipped_items.emplace_back(EquipItemNumber(fallback(accH, setinfo->AccessoryA), 7)); //head acc
            equipped_items.emplace_back(EquipItemNumber(fallback(accW, setinfo->AccessoryB), 8)); //waist acc
            equipped_items.emplace_back(EquipItemNumber(fallback(accB, setinfo->AccessoryC), 9)); //back acc
            equipped_items.emplace_back(EquipItemNumber(item_id_of(10), 10)); //melee
            equipped_items.emplace_back(EquipItemNumber(item_id_of(11), 11)); //rifle
            equipped_items.emplace_back(EquipItemNumber(item_id_of(12), 12)); //shotgun
            equipped_items.emplace_back(EquipItemNumber(item_id_of(13), 13)); //sniper
            equipped_items.emplace_back(EquipItemNumber(item_id_of(14), 14)); //gatling
            equipped_items.emplace_back(EquipItemNumber(item_id_of(15), 15)); //bazooka
            equipped_items.emplace_back(EquipItemNumber(item_id_of(16), 16)); //grenade

            return equipped_items;
        }

        // Convenience overload: SCallbackData::session is now an owning shared_ptr,
        // so handlers pass it directly. Forwards to the raw-pointer implementation.
        void SendServerMessage(const std::shared_ptr<CSession>& session, const std::string& message)
        {
            if (session) SendServerMessage(session.get(), message);
        }
        void SendServerMessage(CSession* session, const std::string& message)
        {
            auto msgData = MainChatAck("", message.data(), static_cast<uint32_t>(message.size())).Serialize(Chat::Type::Server, message.size());
			session->SendMsg(316, 0, Chat::Type::Server, static_cast<uint8_t>(message.size()), reinterpret_cast<uint8_t*>(msgData.data()), static_cast<uint16_t>(msgData.size()));
        }

        // Lock-free identity/state snapshot of an online player resolved by nickname.
        // Reads only under the map-level shared lock (NEVER a per-entry lock), so it is
        // safe to call while already holding ANOTHER account's lock. This avoids the
        // ABBA deadlock that CAccount.get_by_filter<>(nickname) causes: a handler holds
        // the caller's account lock and then locks the matched target's entry, so two
        // players acting on each other by name lock A->B and B->A simultaneously and hang.
        // `aid == -1` means not-found/offline (matches the cache's null-entry Index), so
        // every "!= -1" / truthiness check downstream behaves exactly as the old code did.
        struct OnlineNickIdentity
        {
            uint16_t                            sid{};
            int32_t                             aid{ -1 };
            NetEngine::Packets::Core::UniqueId  uid{};
            std::string                         nickname;
            uint32_t                            room_id{};
            uint32_t                            party_id{};
            bool                                in_room{};
            bool                                in_party{};
            bool                                playing{};
            explicit operator bool() const { return aid > 0; }
        };

        static OnlineNickIdentity ResolveOnlineByNickname(std::string_view nickname)
        {
            OnlineNickIdentity out{};
            const auto wanted = Utility::ToLowercase(nickname);
            auto accounts = CAccount.get_all(BaseLib::shared);
            for (const auto& entry : *accounts)
            {
                const auto& acc = entry.second;
                if (acc.acc_info.Index > 0 && Utility::ToLowercase(acc.acc_info.Nickname) == wanted)
                {
                    out.sid      = acc.session_id;
                    out.aid      = acc.acc_info.Index;
                    out.uid      = acc.uid;
                    out.nickname = acc.acc_info.Nickname;
                    out.room_id  = acc.room_id;
                    out.party_id = acc.party_id;
                    out.in_room  = acc.in_room;
                    out.in_party = acc.in_party;
                    out.playing  = acc.playing;
                    break;
                }
            }
            return out;
        }

        std::expected<ItemAddCtx, DbUpdateError> CraftInventoryItems(AccCacheResource& acc_cache, std::vector<uint32_t> item_ids, Items::Origin origin = Items::Origin::From_GM_Spawn)
        {
            ItemAddCtx item_add{};
            auto available_serials = FindLowestAvailableSerialIds(acc_cache->inventory_items, item_ids.size());
            if (available_serials.size() < item_ids.size()) return std::unexpected(DbUpdateError::NotEnoughSerialInfos);
            for (uint32_t i = 0; i < item_ids.size(); i++)
            {
                auto item_info = CItemsInfo.get<shared_t>(item_ids[i]);//GetItemInfoCache(item_ids[i]);
                if (!item_info->Id) return std::unexpected(DbUpdateError::ItemNotFound);
                ShopItem new_item = { {item_info->Id , item_info->Stock } , ItemExpire::Type::Unused, ItemSerialInfo(available_serials[i], 1, 1, origin, Utility::GetUtcTimeNow())};
            #if defined(RELEASE_1_0_3)
                const InventoryItemInfo& inv_item_info = { {item_info->Id , item_info->Stock } ,ItemExpire::Type::Unused,new_item.serial_info, item_info->Durability, 0 };
            #else
                const InventoryItemInfo& inv_item_info = { item_info->Id ,ItemExpire::Type::Unused,new_item.serial_info, item_info->Durability, 0, 0, 0, 0, 0, AdjustItemType(item_info->Type) };
            #endif
                const Item& new_player_item = { inv_item_info, item_info->Stock, false , 0, false };
                item_add.items.push_back(new_player_item);
            }
            return item_add;
        }

        std::expected<ResultLevelUpInfo, DbUpdateError> ProcessLevelUp(AccCacheResource& acc_cache, const uint32_t bonus_exp, DatabaseUpdateCtx& out_ctx)
        {
            ResultLevelUpInfo result_info{.sid = out_ctx.sid};
            auto old_level = acc_cache->acc_info.Level;
            DEBUGLOG(dark_cyan, "will check if level up, current level: ({})", old_level);
            auto gi = CGradesInfo.get<shared_t>(old_level + 2);
            auto new_exp = acc_cache->acc_info.Experience + bonus_exp;
            if (!gi->Grade || new_exp < gi->Exp)
            {
                out_ctx.ops.emplace_back(AccountInfoPatch{ .experience = new_exp});
				return result_info; // no level up available, just update experience
            }
            result_info.level_up = true;
			result_info.new_level = old_level + 1;
            out_ctx.ops.emplace_back(AccountInfoPatch{ .experience = new_exp, .level = result_info.new_level});
           
            DEBUGLOG(dark_cyan, "will level up: ({})", gi->Grade - 1);
            if (gi->RewardPoint)
            {
                using enum CurrencyType;
                out_ctx.ops.emplace_back(AccountCurrencyDelta{ .type = MP, .value = gi->RewardPoint, .is_reward = true });
				result_info.reward_mp.emplace(gi->RewardPoint);
                DEBUGLOG(dark_cyan, "will get point reward: ({})", gi->RewardPoint);
            }
            if (gi->RewardItem)
            {
                DEBUGLOG(dark_cyan, "will get item reward: ({})", gi->RewardItem);
                auto crafted_item = CraftInventoryItems(acc_cache, { gi->RewardItem }, NetEngine::Items::Origin::From_Game);
                if(!crafted_item.has_value())
                {
                    DEBUGLOG(red, "CraftInventoryItems failed for player [{}] [{}]: {}", acc_cache->acc_info.Index, acc_cache->acc_info.Nickname.c_str(), static_cast<int>(crafted_item.error()));
                    return std::unexpected(crafted_item.error());
                }
				auto& item = crafted_item->items[0];
                auto item_info = CItemsInfo.get<shared_t>(item.item_info.item_number.item_id);
                result_info.reward_item.emplace(ShopItem{ {item.item_info.item_number.item_id , item_info->Stock} , ItemExpire::Type::Unused,  item.item_info.serial_info });
                out_ctx.ops.push_back(crafted_item.value());
            }
            return result_info;
        }

        std::expected<ValidatedDbUpdates, DbUpdateError> ValidateDatabaseUpdates(AccCacheResource& acc_cache, const DatabaseUpdateCtx& ctx, bool bypass_inv_limit = false, bool bypass_currency_limits = false)
        {
            ValidatedDbUpdates out{ .sid = ctx.sid, .aid = ctx.aid };
            auto& inv = acc_cache->inventory_items;
            struct Accum { uint32_t rewards{0}, costs{0}; };
            Accum mp{}, rt{}, cp{}, en{};
            using enum CurrencyType;
            for (const auto& op : ctx.ops) 
            {
                if (auto p = std::get_if<ItemPatchCtx>(&op)) 
                {
                    std::optional<uint64_t> serial;
                    if (p->sel.serial.has_value()) 
                        serial = p->sel.serial->data;
                    else if (p->sel.item_type.has_value() && p->sel.character_id.has_value())
                    {
                        for (const auto& it : inv)
                        {
                            auto info = CItemsInfo.get<shared_t>(it.item_info.item_number.item_id);
                            if (!info->Id || info->Type != p->sel.item_type.value() || it.character_id != p->sel.character_id.value())
                                continue;

                            if (p->is_equipped.has_value() && it.is_equipped != *p->is_equipped)
                            {
                                serial = it.item_info.serial_info.data;
                                break;
                            }
                        }

                        if (!serial.has_value())
                            for (const auto& it : inv) 
                            {
                                auto info = CItemsInfo.get<shared_t>(it.item_info.item_number.item_id);
                                if (info->Id && info->Type == p->sel.item_type.value() && it.character_id == p->sel.character_id.value())
                                {
                                    serial = it.item_info.serial_info.data;
                                    break;
                                }
                            }
                    }
                    if (!serial.has_value()) return std::unexpected(DbUpdateError::ItemNotFound);
                    out.items_patches.push_back(ResolvedItemPatch{serial.value(), *p});
                }
                else if (auto d = std::get_if<ItemDeleteCtx>(&op)) 
                {
                    for (const auto& s : d->serials) 
                    {
                        auto it = std::find_if(inv.begin(), inv.end(), [&](const auto& item){ return item.item_info.serial_info.data == s.data; });
                        if (it == inv.end()) return std::unexpected(DbUpdateError::ItemNotFound);
                        out.items_deleted.push_back(s);
                    }
                }
                else if (auto a = std::get_if<ItemAddCtx>(&op))
                {
                    if(!bypass_inv_limit)
                        if (inv.size() + out.items_added.size() > acc_cache->acc_info.MaximumItems)
                            return std::unexpected(DbUpdateError::InventoryFull);

                    out.items_added.insert(out.items_added.end(), a->items.begin(), a->items.end());
                    
                    
                }                 
                else if (auto cur = std::get_if<AccountCurrencyDelta>(&op))
                {
                    Accum* bucket = nullptr;
                    switch (cur->type) 
                    {
                        case MP: bucket = &mp; break;
                        case RT: bucket = &rt; break;
                        case COUPONS: bucket = &cp; break;
                        case ENERGY: bucket = &en; break;
                    }
                    if (bucket) 
                        (cur->is_reward) ? bucket->rewards += cur->value : bucket->costs += cur->value;
                }
                else if (auto aip = std::get_if<AccountInfoPatch>(&op))
                    out.acc_info_patches.push_back(*aip);
                else if (auto pmp = std::get_if<PlayerMissionsPatch>(&op))
                    out.player_missions_patches.push_back(*pmp);
                else if (auto pmrp = std::get_if<PlayerMonthlyRewardPatch>(&op))
                    out.player_monthly_reward_patches.push_back(*pmrp);
                else if (auto pwrp = std::get_if<PlayerWeeklyRewardPatch>(&op))
                    out.player_weekly_reward_patches.push_back(*pwrp);
                else if (auto pptp = std::get_if<PlayerPlaytimePatch>(&op))
                    out.player_playtime_patches.push_back(*pptp);
                else if (auto pbpp = std::get_if<PlayerBattlePassPatch>(&op))
                    out.player_battlepass_patches.push_back(*pbpp);
				else if (auto mbp = std::get_if<MailboxPatch>(&op))
					out.mailbox_patches.push_back(*mbp);     
				else if (auto mha = std::get_if<MatchInfoHistoryAdd>(&op))
					out.match_history_adds.push_back(*mha);
				else if (auto psp = std::get_if<PlayerSessionsPatch>(&op))
					out.player_sessions_patches.push_back(*psp);
				else if (auto pslp = std::get_if<PlayerSocialPatch>(&op))
					out.player_social_patches.push_back(*pslp);
				else if (auto gpp = std::get_if<GachaPityPatch>(&op))
					out.gacha_pity_patches.push_back(*gpp);
			}

            auto finalize = [&](Accum a, CurrencyType type) 
            {
                if (a.rewards == 0 && a.costs == 0) return;
                if (a.rewards >= a.costs) 
                {
                    auto v = a.rewards - a.costs;
                    if (v > 0) out.currency_updates.push_back(AccountCurrencyDelta{type, v, true});
                } else 
                {
                    auto v = a.costs - a.rewards;
                    if (v > 0) out.currency_updates.push_back(AccountCurrencyDelta{type, v, false});
                }
            };

            finalize(mp, MP);
            finalize(rt, RT);
            finalize(cp, COUPONS);
            finalize(en, ENERGY);

            constexpr uint32_t MAX_COUPONS = 250;
            constexpr uint32_t MAX_UINT32 = std::numeric_limits<uint32_t>::max();
            using enum DbUpdateError;
            if (!bypass_currency_limits)
            for (const auto& cu : out.currency_updates)
            {
                switch (cu.type)
                {
                    case MP:
                        if (!cu.is_reward) 
                        {
                            if (acc_cache->acc_info.MicroPoints < cu.value)
                                return std::unexpected(InsufficientMP);
                        } else 
                        {
                            if (acc_cache->acc_info.MicroPoints > MAX_UINT32 - cu.value)
                                return std::unexpected(MpFull);
                        }
                        break;
                    case RT:
                        if (!cu.is_reward) 
                        {
                            if (acc_cache->acc_info.RockTokens < cu.value)
                                return std::unexpected(InsufficientRT);
                        } else 
                        {
                            if (acc_cache->acc_info.RockTokens > MAX_UINT32 - cu.value)
                                return std::unexpected(RtFull); // or Overflow
                        }
                        break;

                    case COUPONS:
                        if (!cu.is_reward) 
                        {
                            if (acc_cache->acc_info.Coupons < cu.value)
                                return std::unexpected(InsufficientCOUPONS);
                        } else 
                        {
                            if (acc_cache->acc_info.Coupons > MAX_COUPONS - cu.value)
                                return std::unexpected(CouponsFull); 
                        }
                        break;
                    case ENERGY:
                        if (!cu.is_reward) 
                        {
                            if (acc_cache->acc_info.Energy < cu.value)
                                return std::unexpected(InsufficientENERGY);
                        } else 
                        {
                            if (acc_cache->acc_info.Energy > acc_cache->acc_info.MaximumEnergy - cu.value)
                                return std::unexpected(EnergyFull);
                        }
                        break;
                }
            }

            for (const auto& aip : out.acc_info_patches)
            {
				if (aip.maximum_energy > 5000)
					return std::unexpected(MaxEnergyReachedAlready);

                if (aip.maximum_items > 1000)
					return std::unexpected(MaxInventoryItemsReachedAlready);

                if (aip.nickname)
				{
					if (aip.nickname->size() < 3)
						return std::unexpected(AVA_CREATE_SHORTNAME);
					if (!Utility::IsValidNickname(*aip.nickname))
						return std::unexpected(AVA_CREATE_BANNAME);
				}
            }
			for (const auto& mbp : out.mailbox_patches)
			{

                if (mbp.op == MailboxPatch::Op::Delete || mbp.op == MailboxPatch::Op::MarkRead)
                {
                    if (!mbp.mail_id) return std::unexpected(MEMO_MAIL_NOT_FOUND);   
                    auto data = CMailboxData.get<shared_t>(mbp.mail_id);
                    if(!data->mail_id) return std::unexpected(MEMO_MAIL_NOT_FOUND);     
                }	
				if (mbp.op == MailboxPatch::Op::Insert)
				{
                    auto sent = CMailSent.get<shared_t>(acc_cache->acc_info.Index)->size();
					if (sent >= 100)
						return std::unexpected(MEMO_MAIL_FULL_SENDER);

                    if (mbp.insert->receiver_nickname)
                    {
                        if (mbp.insert->receiver_nickname == acc_cache->acc_info.Nickname)
                            return std::unexpected(MEMO_MAIL_SEND_MYSELF);

                        // lock-free resolve (we already hold acc_cache's lock) — see ResolveOnlineByNickname
                        const auto receiver = ResolveOnlineByNickname(mbp.insert->receiver_nickname.value());

                        if (!receiver.aid) continue; //user offline


                        auto my_socials = CSocial.get<shared_t>(acc_cache->session_id);
                        if (IsBlockedAlready(my_socials, receiver.aid))
                            return std::unexpected(MEMO_MAIL_BLOCKEDBY_SENDER);
                        auto target_socials = CSocial.get<shared_t>(receiver.sid);
                        if (IsBlockedAlready(target_socials, acc_cache->acc_info.Index))
                            return std::unexpected(MEMO_MAIL_BLOCKEDBY_RECEIVER);
                        auto received = CMailRecv.get<shared_t>(receiver.aid)->size();
                        if (received >= 100)
                            return std::unexpected(MEMO_MAIL_FULL_RECEIVER);

                        if (mbp.insert->gift_item_id)
                        {
							auto received_gifts = CGiftRecv.get<shared_t>(receiver.aid)->size();
							if (received_gifts >= 100)
								return std::unexpected(MEMO_GIFT_FULL_RECEIVER);
                        }
                    }
				}
			}

            return out;
        }

        std::expected<ApplyCacheUpdateResult, DbUpdateError> ApplyDatabaseUpdates(AccCacheResource& acc_cache, const ValidatedDbUpdates& v)
        {
            ApplyCacheUpdateResult r;
            //update currency cache
            for (const auto& cu : v.currency_updates) 
            {
                switch (cu.type) 
                {
                    case CurrencyType::MP:
                        if(cu.is_reward)
                            acc_cache->acc_info.MicroPoints += cu.value;
                        else
                            acc_cache->acc_info.MicroPoints -= cu.value;
                        r.new_mp = acc_cache->acc_info.MicroPoints;
                        break;

                    case CurrencyType::RT:
                        if(cu.is_reward)
                            acc_cache->acc_info.RockTokens += cu.value;
                        else
                            acc_cache->acc_info.RockTokens -= cu.value;
                        r.new_rt = acc_cache->acc_info.RockTokens;
                        break;

                    case CurrencyType::COUPONS:
                        if(cu.is_reward)
                            acc_cache->acc_info.Coupons += cu.value;
                        else
                            acc_cache->acc_info.Coupons -= cu.value;
                        r.new_coupons = acc_cache->acc_info.Coupons;
                        break;

                    case CurrencyType::ENERGY:
                        if(cu.is_reward)
                            acc_cache->acc_info.Energy += cu.value;
                        else
                            acc_cache->acc_info.Energy -= cu.value;
                        r.new_energy = acc_cache->acc_info.Energy;
                        break;
                }
            }
            auto& inv = acc_cache->inventory_items;

            for (const auto& ser : v.items_deleted)  //delete items from inv cache
            {
                std::erase_if(inv, [&](const auto& item){ return item.item_info.serial_info.data == ser.data; });
                r.deleted_serials.push_back(ser.data);
            }

            for (const auto& rp : v.items_patches)  //update items from inv cache
            {
                auto it = std::find_if(inv.begin(), inv.end(), [&](auto& item){ return item.item_info.serial_info.data == rp.serial; });
                if (it == inv.end()) return std::unexpected(DbUpdateError::ItemNotFound);
                if (rp.patch.is_equipped) it->is_equipped = *rp.patch.is_equipped;
                if (rp.patch.character_id) it->character_id = *rp.patch.character_id;
                if (rp.patch.repair) it->item_info.repair = *rp.patch.repair;
                if (rp.patch.energy) it->item_info.energy = *rp.patch.energy;
                if (rp.patch.new_item_id) it->item_info.item_number.item_id = *rp.patch.new_item_id;
                if (rp.patch.expire_date) it->item_info.expire_date = *rp.patch.expire_date;
                r.patched_serials.push_back(rp.serial);
            }
            inv.insert(inv.end(), v.items_added.begin(), v.items_added.end()); //add items to inv cache
            for (const auto& it : v.items_added) 
                r.added_serials.push_back(it.item_info.serial_info.data);

            for (const auto& aip : v.acc_info_patches) //update account info
            {
                if (aip.sw_daily_attempts) acc_cache->acc_info.SingleWaveDailyAttempts = *aip.sw_daily_attempts;
				if (aip.sw_high_score) acc_cache->acc_info.SingleWaveHighScore = *aip.sw_high_score;
				if (aip.sw_highest_wave) acc_cache->acc_info.SingleWaveHighestWave = *aip.sw_highest_wave;
				if (aip.sw_last_update) acc_cache->acc_info.SingleWaveLastUpdate = *aip.sw_last_update;
				if (aip.clan_kills) acc_cache->acc_info.ClanKills = *aip.clan_kills;
				if (aip.clan_deaths) acc_cache->acc_info.ClanDeaths = *aip.clan_deaths;
				if (aip.clan_assists) acc_cache->acc_info.ClanAssists = *aip.clan_assists;
				if (aip.clan_contribution) acc_cache->acc_info.ClanContribution = *aip.clan_contribution;
				if (aip.clan_wins) acc_cache->acc_info.ClanWins = *aip.clan_wins;
				if (aip.clan_loses) acc_cache->acc_info.ClanLoses = *aip.clan_loses;
				if (aip.clan_draws) acc_cache->acc_info.ClanDraws = *aip.clan_draws;
				if (aip.selected_character) acc_cache->acc_info.SelectedCharacter = *aip.selected_character;
				if (aip.play_time) acc_cache->acc_info.PlayTime = *aip.play_time;
				if (aip.story) acc_cache->acc_info.Story = *aip.story;
                if (aip.achievement_tier1) acc_cache->acc_info.Achievement = *aip.achievement_tier1;
                if (aip.guide_mission) acc_cache->acc_info.GuideMission = *aip.guide_mission;
                if (aip.voice_type) acc_cache->acc_info.VoiceType = *aip.voice_type;
                if (aip.bTutorial) acc_cache->acc_info.Tutorial = *aip.bTutorial;
				if (aip.maximum_energy) acc_cache->acc_info.MaximumEnergy = *aip.maximum_energy;
				if (aip.maximum_items) acc_cache->acc_info.MaximumItems = *aip.maximum_items;
                if (aip.experience) acc_cache->acc_info.Experience = *aip.experience;
                if (aip.level) acc_cache->acc_info.Level = *aip.level;
				if (aip.lucky_points) acc_cache->acc_info.LuckyPoints = *aip.lucky_points;
				if (aip.wins) acc_cache->acc_info.Wins = *aip.wins;
				if (aip.loses) acc_cache->acc_info.Loses = *aip.loses;
                if (aip.draws) acc_cache->acc_info.Draws = *aip.draws;
                if (aip.kills) acc_cache->acc_info.Kills = *aip.kills;
                if (aip.deaths) acc_cache->acc_info.Deaths = *aip.deaths;
                if (aip.assists) acc_cache->acc_info.Assists = *aip.assists;
                if (aip.headshots) acc_cache->acc_info.Headshots = *aip.headshots;
                if (aip.highest_kill_streak) acc_cache->acc_info.HighestKillStreak = *aip.highest_kill_streak;
                if (aip.melee_kills) acc_cache->acc_info.MeleeKills = *aip.melee_kills;
                if (aip.rifle_kills) acc_cache->acc_info.RifleKills = *aip.rifle_kills;
				if (aip.shotgun_kills) acc_cache->acc_info.ShotgunKills = *aip.shotgun_kills;
				if (aip.sniper_kills) acc_cache->acc_info.SniperKills = *aip.sniper_kills;
				if (aip.gatling_kills) acc_cache->acc_info.GatlingKills = *aip.gatling_kills;
				if (aip.bazooka_kills) acc_cache->acc_info.BazookaKills = *aip.bazooka_kills;
				if (aip.grenade_kills) acc_cache->acc_info.GrenadeKills = *aip.grenade_kills;
				if (aip.zombie_kills) acc_cache->acc_info.ZombieKills = *aip.zombie_kills;
				if (aip.infections) acc_cache->acc_info.Infections = *aip.infections;   
				if (aip.nickname) acc_cache->acc_info.Nickname = *aip.nickname;
            }

            for (const auto& pmp : v.player_missions_patches) //update player missions
            {
                if (pmp.update_time) acc_cache->daily_mission_info.update_time = *pmp.update_time;
                if (pmp.mission1) acc_cache->daily_mission_info.mission1 = *pmp.mission1;
                if (pmp.mission2) acc_cache->daily_mission_info.mission2 = *pmp.mission2;
                if (pmp.mission3) acc_cache->daily_mission_info.mission3 = *pmp.mission3;
                if (pmp.goal1) acc_cache->daily_mission_info.goal_mission1 = *pmp.goal1;
                if (pmp.goal2) acc_cache->daily_mission_info.goal_mission2 = *pmp.goal2;
                if (pmp.goal3) acc_cache->daily_mission_info.goal_mission3 = *pmp.goal3;
            }

            for (const auto& mbp : v.mailbox_patches) //update mailbox
            {
                using enum MailboxPatch::Op;
                switch (mbp.op)
                {
                    case MarkRead:
                    case Delete:
                    {
                        if (!mbp.mail_id) return std::unexpected(DbUpdateError::InvalidMailId);
                        auto data = CMailboxData.get<unique_t>(mbp.mail_id);
                        if (!data->mail_id) return std::unexpected(DbUpdateError::InvalidMailId);
                        if (mbp.op == MarkRead) data->is_new = false;
                        if (mbp.op == Delete)
                        {
                            if (data->receiver_account_id == acc_cache->acc_info.Index)
                            {
                                data->deleted_from_receiver = true;
								if (data->gift_itemid)
									CGiftRecv.erase_value(acc_cache->acc_info.Index, data->mail_id);
								else
                                    CMailRecv.erase_value(acc_cache->acc_info.Index, data->mail_id);
							}
                            else if (data->sender_account_id == acc_cache->acc_info.Index)
                            {
                                data->deleted_from_sender = true;
                                if (data->gift_itemid)
                                    CGiftSent.erase_value(acc_cache->acc_info.Index, data->mail_id);
                                else
                                    CMailSent.erase_value(acc_cache->acc_info.Index, data->mail_id);
                            }
                            if (data->deleted_from_sender && data->deleted_from_receiver)
                            {
                                data.unlock();
								CMailboxData.erase(mbp.mail_id);
                            }
                        }
                        break;
                    }
                    case Insert:
                    {
                        if (!mbp.insert.has_value()) return std::unexpected(DbUpdateError::InvalidMailInserts);
                        const auto& in = *mbp.insert;
                        if (!in.sender_nickname || in.sender_nickname->empty() || !in.receiver_id ||
                            !in.receiver_nickname || in.receiver_nickname->empty() || in.message.empty())
                            return std::unexpected(DbUpdateError::InvalidMailInserts);

                        MailboxInfo mailbox_info = 
                        {
                            mbp.mail_id,
                            static_cast<uint32_t>(acc_cache->acc_info.Index),
                            *in.sender_nickname,
                            *in.receiver_id,
                            *in.receiver_nickname,
                            Utility::GetUtcTimeNow(),
                            in.gift_item_id,
                            in.message,
                            true, false, false
                        };
                        if (in.gift_item_id)
                        {
							CGiftSent.emplace_back(mailbox_info.sender_account_id, mailbox_info.mail_id);
							CGiftRecv.emplace_back(mailbox_info.receiver_account_id, mailbox_info.mail_id);
						}
						else
						{
							CMailSent.emplace_back(mailbox_info.sender_account_id, mailbox_info.mail_id);
							CMailRecv.emplace_back(mailbox_info.receiver_account_id, mailbox_info.mail_id);
						}
						CMailboxData.insert(mailbox_info.mail_id, MailboxData(mailbox_info));
                        break;
                    }
					default:
						return std::unexpected(DbUpdateError::InvalidOp);
                }
            }
           // auto social_list = GetSocialsList(acc_cache->session_id);
            for (const auto& psp : v.player_social_patches)
            {
                using enum PlayerSocialPatch::Op;
                if (!psp.aid) return std::unexpected(DbUpdateError::InvalidAid);
                if (!psp.targetAid) return std::unexpected(DbUpdateError::InvalidTargetAid);
                switch (psp.op)
                {
                case Delete:
                {
					auto socials = CSocial.get<unique_t>(acc_cache->session_id);
					std::erase_if(*socials, [&](const BaseLib::SocialInfo& si) { return si.targetAid == psp.targetAid; });
                    socials.unlock();
                    break;
                }
                case InsertOrUpdate:
                {
					BaseLib::SocialInfo info = { psp.aid, psp.targetAid, psp.State.value_or(0), psp.TargetNickname.value_or("")};
                    CSocial.upsert_in_vector_assign(
                        acc_cache->session_id,
                        info,
                        [](const BaseLib::SocialInfo& a, const BaseLib::SocialInfo& b) { return a.targetAid == b.targetAid; }
                    );
                    break;
                }
                default:
                    return std::unexpected(DbUpdateError::InvalidOp);
                }
            }
            for (const auto& gpp : v.gacha_pity_patches)
            {
                auto it = std::find_if(acc_cache->gacha_pity.begin(), acc_cache->gacha_pity.end(),
                    [&](const GachaPityEntry& e) { return e.gacha_type == gpp.gacha_type; });
                if (it != acc_cache->gacha_pity.end())
                    it->lucky_points = gpp.lucky_points;
                else
                    acc_cache->gacha_pity.push_back({ gpp.gacha_type, gpp.lucky_points });
            }
            return r;
        }

        constexpr std::string_view GetModeName(const uint32_t& index)
        {
            if (!NetEngine::Room::Mode::modeNames[index].empty())
                return NetEngine::Room::Mode::modeNames[index];

            return "Unknown Mode";
        }
        constexpr std::string_view GetMapName(const uint32_t& index)
        {
            if (!NetEngine::Room::Map::mapNames[index].empty())
                return NetEngine::Room::Map::mapNames[index];

            return "Unknown Map";
        }
    };
    /*
    namespace Commands
    {
        using CommandFunc = std::function<void(const std::vector<std::string>& args, const SCallbackData& callback, AccCacheResource& acc_cache, CMainServer* main_server)>;
        struct Command
        {
            CommandFunc func;
            uint8_t required_grade;
        };
        static boost::unordered_flat_map<std::string, Command> cmds;
        inline void Register(const std::string& name, CommandFunc func, const uint8_t& required_grade)
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
        inline std::vector<std::string> ListCommands(const uint8_t& grade)
        {
            std::vector<std::string> commands;
            std::string command_list = "[MegaVolts Online] Available commands:\n";
            commands.push_back(command_list);
            for (const auto& [name, command] : cmds)
            {
                if (grade < command.required_grade) continue;
                commands.push_back(std::format("/{}", name));
            }
            return commands;
        }
    }
    */
}
