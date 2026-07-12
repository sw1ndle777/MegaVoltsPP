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
#include "MovementBatcher.h"

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
        // Cast-side kill attribution (anti-cheat). Death STATE stays host-authoritative
        // (Status.h m_bIsDead); these only attribute a kill to the last damager + dedupe.
        uint16_t last_attacker_sid{};        // sid of whoever last damaged this player
        uint64_t last_damage_time{};         // unix seconds of that last damage (attribution window)
        bool kill_credited_this_death{};     // ensures exactly one kill is credited per death
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
        // Maps a kit dropId (KitDropInfo.id, the instance the host assigns at spawn) to the
        // packed item word (itemId:23 | itemType:5 | flag:1). Lets the pickup handler resolve
        // what was actually grabbed (ammo / health / bomb / radar), since the pickup packet
        // only carries the dropId, not the catalog item.
        boost::unordered_flat_map<uint32_t, uint32_t> kit_item_by_drop;
        Room(const uint16_t& roomId = 0,
            const uint16_t& hostSessionId = 0)
            : room_id(roomId), host_session_id(hostSessionId), is_playing(false)
        {
        }
        Room(const Room& other)
            : room_id(other.room_id),
              host_session_id(other.host_session_id),
              is_playing(other.is_playing),
              players_session_id(other.players_session_id),
              kit_item_by_drop(other.kit_item_by_drop)
        {
        }
        Room& operator=(const Room& other)
        {
            if (this == &other) return *this;
            room_id = other.room_id;
            host_session_id = other.host_session_id;
            is_playing = other.is_playing;
            players_session_id = other.players_session_id;
            kit_item_by_drop = other.kit_item_by_drop;
            return *this;
        }
    };

    struct RoomProjectiles
    {
        std::shared_mutex mutex;
        boost::unordered_flat_map<uint32_t, uint16_t> owner_by_id;
        boost::unordered_flat_map<uint32_t, uint8_t> type_by_id;
        RoomProjectiles() = default;
        RoomProjectiles(const RoomProjectiles& other)
            : owner_by_id(other.owner_by_id), type_by_id(other.type_by_id)
        {
        }
        RoomProjectiles& operator=(const RoomProjectiles& other)
        {
            if (this == &other) return *this;
            owner_by_id = other.owner_by_id;
            type_by_id = other.type_by_id;
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
    extern CCache<boost::unordered_flat_map<uint16_t, RoomProjectiles>> CRoomProjectiles;
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
            msg.SetEncryptMethod(enc);
            auto filtered = ids | std::views::filter([&](uint16_t id) { return !exclude_sid || id != *exclude_sid; });
            std::ranges::for_each(filtered, [&](uint16_t id) {
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

        // --- 128-tick movement batching (per-room fixed-rate flush). ---
        // See MovementBatcher.h for the client constraints this satisfies.
        bool IsMoveBatchEnabled() const { return m_moveBatch; }
        void SetMoveBatch(bool enabled, uint32_t hz = 128)
        {
            m_moveBatch = enabled;
            m_moveBatchHz = hz ? hz : 128;
            if (enabled) StartMovementBatcher(m_moveBatchHz);
            else         StopMovementBatcher();
        }
        // Forwarded from the USER_MOVE handler: queue this player's latest entry
        // (cmd-322 response body without its leading 4-byte tick) for the next flush.
        void SubmitMovement(uint16_t room_id, uint16_t sid, const uint8_t* entry, uint8_t len, uint32_t matchTick)
        {
            m_moveBatcher.Submit(room_id, sid, entry, len, matchTick);
        }

    private:
        void StartMovementBatcher(uint32_t hz);
        void StopMovementBatcher() { m_moveBatcher.Stop(); }

        bool m_moveBatch = false;
        uint32_t m_moveBatchHz = 128;
        MovementBatcher m_moveBatcher;
    };
}
