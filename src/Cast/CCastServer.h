#pragma once
#include <ranges>
#include <string>
#include <functional>

#include "BaseLib/CLog.h"
#include "NetEngine/CServer.h"
#include "NetEngine/CSession.h"
#include "NetEngine/Constants.h"

#include "NetEngine/Packets/PacketStruct.h"
#include "NetEngine/Packets/PacketData.h"
#include <boost_unordered.hpp>

#include "BaseLib/CCache.h"

namespace Game
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Cast;
    extern CCache<boost::unordered_flat_set<uint16_t>> g_tp_to_proj_sids;
    struct PlayerInfo
    {
        enum State : uint8_t
        {
            Disconnected = 0,
            Connected = 1,
            Lobby = 2,
            Plaza = 3,
            WaitingRoom = 4,
            StartMatch = 5,
            Normal = 6,
            Dying = 8
        };
    };
    
    struct Player
    {
        std::shared_mutex mutex;
        uint16_t session_id;
        uint16_t room_id;
        uint16_t plaza_id;
        uint16_t server_id;
        uint16_t state_id;
        uint32_t health;
        uint32_t combat_health;
        bool combat_health_known;
        uint32_t max_health;
        uint32_t current_health;
        uint16_t current_kill_streak;
        uint16_t highest_kill_streak;
        int32_t account_id;
        bool in_room;
        bool in_plaza;
        bool is_dead;
        uint64_t auth_key;
		std::string nickname;
		std::string hwid;

        Player(const uint16_t& sessionId = 0,
            const uint16_t& hostSessionId = 0,
            const uint16_t& roomId = 0,
            const uint16_t& plazaId = 0,
            const uint16_t& severId = 0,
            const uint16_t& stateId = 0,
            const bool& inRoom = false,
            const bool& inPlaza = false,
            const uint64_t& authKey = 0,
            const std::string& nickname = "")
            : session_id(sessionId), room_id(roomId), plaza_id(plazaId), server_id(severId), state_id(stateId), in_room(inRoom), in_plaza(inPlaza), auth_key(authKey), nickname(nickname) {
                is_dead = false;
                health = 1000;
                combat_health = 1000;
                combat_health_known = true;
                max_health = 1000;
                current_health = 1000;
                current_kill_streak = 0;
                highest_kill_streak = 0;
                account_id = -1;
                hwid.clear();
        }
        Player(const Player& other)
            : session_id(other.session_id),
            room_id(other.room_id),
            plaza_id(other.plaza_id),
            server_id(other.server_id),
            state_id(other.state_id),
            health(other.health),
            combat_health(other.combat_health),
            combat_health_known(other.combat_health_known),
            max_health(other.max_health),
            current_health(other.current_health),
            current_kill_streak(other.current_kill_streak),
            highest_kill_streak(other.highest_kill_streak),
            account_id(other.account_id),
            in_room(other.in_room),
            in_plaza(other.in_plaza),
            is_dead(other.is_dead),
            auth_key(other.auth_key),
            nickname(other.nickname),
            hwid(other.hwid)
        {
        }

        Player& operator=(const Player& other)
        {
            if (this == &other) return *this;
            session_id = other.session_id;
            room_id = other.room_id;
            plaza_id = other.plaza_id;
            server_id = other.server_id;
            state_id = other.state_id;
            health = other.health;
            combat_health = other.combat_health;
            combat_health_known = other.combat_health_known;
            max_health = other.max_health;
            current_health = other.current_health;
            current_kill_streak = other.current_kill_streak;
            highest_kill_streak = other.highest_kill_streak;
            account_id = other.account_id;
            in_room = other.in_room;
            in_plaza = other.in_plaza;
            is_dead = other.is_dead;
            auth_key = other.auth_key;
            nickname = other.nickname;
            hwid = other.hwid;
            return *this;
        }

        Player(Player&& other) noexcept
            : session_id(other.session_id),
            room_id(other.room_id),
            plaza_id(other.plaza_id),
            server_id(other.server_id),
            state_id(other.state_id),
            health(other.health),
            combat_health(other.combat_health),
            combat_health_known(other.combat_health_known),
            max_health(other.max_health),
            current_health(other.current_health),
            current_kill_streak(other.current_kill_streak),
            highest_kill_streak(other.highest_kill_streak),
            account_id(other.account_id),
            in_room(other.in_room),
            in_plaza(other.in_plaza),
            is_dead(other.is_dead),
            auth_key(other.auth_key),
            nickname(std::move(other.nickname)),
            hwid(std::move(other.hwid))
        {
        }

        Player& operator=(Player&& other) noexcept
        {
            if (this == &other) return *this;
            session_id = other.session_id;
            room_id = other.room_id;
            plaza_id = other.plaza_id;
            server_id = other.server_id;
            state_id = other.state_id;
            health = other.health;
            combat_health = other.combat_health;
            combat_health_known = other.combat_health_known;
            max_health = other.max_health;
            current_health = other.current_health;
            current_kill_streak = other.current_kill_streak;
            highest_kill_streak = other.highest_kill_streak;
            account_id = other.account_id;
            in_room = other.in_room;
            in_plaza = other.in_plaza;
            is_dead = other.is_dead;
            auth_key = other.auth_key;
            nickname = std::move(other.nickname);
            hwid = std::move(other.hwid);
            return *this;
        }
    };

    struct Room
    {
        std::shared_mutex mutex;
        uint16_t room_id;
        uint16_t host_session_id;
        bool is_playing;
        std::vector<uint16_t> players_session_id;
        boost::unordered_flat_map<uint32_t, uint16_t> projectile_owner_by_id;
        boost::unordered_flat_map<uint32_t, uint8_t> projectile_type_by_id;
        std::vector<std::vector<uint8_t>> pending_positions;
        uint32_t room_tick{};
        Room(const uint16_t& roomId = 0,
            const uint16_t& hostSessionId = 0)
            : room_id(roomId), host_session_id(hostSessionId), is_playing(false)
        {
            players_session_id.clear();
            projectile_owner_by_id.clear();
            projectile_type_by_id.clear();
        }
        Room(const Room& other)
        {
            room_id = other.room_id;
            host_session_id = other.host_session_id;
            is_playing = other.is_playing;
            players_session_id = other.players_session_id;
            projectile_owner_by_id = other.projectile_owner_by_id;
            projectile_type_by_id = other.projectile_type_by_id;
            room_tick = other.room_tick;
        }
        Room& operator=(const Room& other)
        {
            if (this == &other) return *this;
            room_id = other.room_id;
            host_session_id = other.host_session_id;
            is_playing = other.is_playing;
            players_session_id = other.players_session_id;
            projectile_owner_by_id = other.projectile_owner_by_id;
            projectile_type_by_id = other.projectile_type_by_id;
            room_tick = other.room_tick;
            return *this;
        }
    };

    struct Plaza
    {
        std::shared_mutex mutex;
        uint16_t plaza_id;
        std::vector<uint16_t> players_session_id;
        Plaza(const uint16_t& plazaId = 0) : plaza_id(plazaId)
        {
            players_session_id.clear();
        }
        Plaza(const Plaza& other)
        {
            plaza_id = other.plaza_id;
            players_session_id = other.players_session_id;
        }
        Plaza& operator=(const Plaza& other)
        {
            if (this == &other) return *this;
            plaza_id = other.plaza_id;
            players_session_id = other.players_session_id;
            return *this;
        }
    };

    extern CCache<boost::unordered_flat_map<uint16_t, Player>> CAccount;
    extern CCache<boost::unordered_flat_map<uint16_t, Room>> CRoom;
    extern CCache<boost::unordered_flat_map<uint16_t, Plaza>> CPlaza;
    extern CCache<boost::unordered_flat_map<uint64_t, uint16_t>> CAuthKey;

    extern CCache<std::vector<uint16_t>> CRoomId;
    extern CCache<std::vector<uint16_t>> CPartyId;

    using RoomCacheResource = LockedResource<std::unique_lock<std::shared_mutex>, Room>;
    using PlazaCacheResource = LockedResource<std::unique_lock<std::shared_mutex>, Plaza>;
    class CCastServer : public NetEngine::CServer
    {
    public:
        CCastServer();
        ~CCastServer();
        NetEngine::RateLimit::IdentitySnapshot BuildPacketRateLimitIdentitySnapshot(const SCallbackData& callback);
        using enum fmt::color;
        auto Broadcast(const std::vector<uint16_t>& ids, 
            CMessage& msg, 
            std::optional<uint16_t> exclude_sid = std::nullopt,
            SendOption::EncryptionMethod enc = SendOption::EncryptionMethod::None,
            std::source_location loc = std::source_location::current())
        {
            auto filtered = ids | std::views::filter([&](uint16_t id) { return !exclude_sid || id != *exclude_sid; });
            std::ranges::for_each(filtered, [&](uint16_t id) {
                msg.SetEncryptMethod(enc);
                msg.SetSession(id);
                if (auto pss = this->GetSessionById(id))
                    pss->Send(msg);
                else
                {
                    EOrder o = magic_enum::enum_cast<EOrder>(u16_cast(msg.GetOrder())).value_or(EOrder::NONE);
                    BaseLib::EventLog->Debug(loc, ACK, o, red,
                        "couldn't broadcast packet to sid=({})", id);
                }
                });
        }
        auto Forward(uint16_t to_sid,
            uint16_t from_sid,
            CMessage& msg,
            SendOption::EncryptionMethod enc = SendOption::EncryptionMethod::None,
            std::source_location loc = std::source_location::current())
        {
            msg.SetEncryptMethod(enc);
            msg.SetSession(from_sid);

            if (auto pss = this->GetSessionById(to_sid))
            {
                pss->Send(msg);
                return true;
            }

            EOrder o = magic_enum::enum_cast<EOrder>(u16_cast(msg.GetOrder())).value_or(EOrder::NONE);
            BaseLib::EventLog->Debug(loc, ACK, o, red,
                "couldn't forward packet from sid=({}) to sid=({})", from_sid, to_sid);
            return false;
        }

        bool IsBatchPositionsEnabled() const { return m_batchPositions; }
        void SetBatchPositions(bool enabled) { m_batchPositions = enabled; }

        void FlushPendingPositions();
        void StartPositionFlushTimer();

    private:
        bool m_batchPositions = false;
        std::shared_ptr<asio::steady_timer> m_positionFlushTimer;
    };
}
