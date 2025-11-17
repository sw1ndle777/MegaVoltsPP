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
        bool in_room;
        bool in_plaza;
        bool is_dead;
        uint64_t auth_key;
		std::string nickname;

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
                health = 0;
        }
        Player(const Player& other)
            : session_id(other.session_id),
            room_id(other.room_id),
            plaza_id(other.plaza_id),
            server_id(other.server_id),
            state_id(other.state_id),
            health(other.health),
            in_room(other.in_room),
            in_plaza(other.in_plaza),
            is_dead(other.is_dead),
            auth_key(other.auth_key),
            nickname(other.nickname)
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
            in_room = other.in_room;
            in_plaza = other.in_plaza;
            is_dead = other.is_dead;
            auth_key = other.auth_key;
            nickname = other.nickname;
            return *this;
        }

        Player(Player&& other) noexcept
            : session_id(other.session_id),
            room_id(other.room_id),
            plaza_id(other.plaza_id),
            server_id(other.server_id),
            state_id(other.state_id),
            health(other.health),
            in_room(other.in_room),
            in_plaza(other.in_plaza),
            is_dead(other.is_dead),
            auth_key(other.auth_key),
            nickname(std::move(other.nickname))
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
            in_room = other.in_room;
            in_plaza = other.in_plaza;
            is_dead = other.is_dead;
            auth_key = other.auth_key;
            nickname = std::move(other.nickname);
            return *this;
        }
    };

    struct Room
    {
        std::shared_mutex mutex;
        uint16_t room_id;
        uint16_t host_session_id;
        std::vector<uint16_t> players_session_id;
        Room(const uint16_t& roomId = 0,
            const uint16_t& hostSessionId = 0)
            : room_id(roomId), host_session_id(hostSessionId)
        {
            players_session_id.clear();
        }
        Room(const Room& other)
        {
            room_id = other.room_id;
            host_session_id = other.host_session_id;
            players_session_id = other.players_session_id;
        }
        Room& operator=(const Room& other)
        {
            if (this == &other) return *this;
            room_id = other.room_id;
            host_session_id = other.host_session_id;
            players_session_id = other.players_session_id;
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
    };
}
