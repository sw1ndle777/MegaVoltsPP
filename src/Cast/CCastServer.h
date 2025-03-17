#pragma once

#include <string>
#include <functional>

#include "BaseLib/CLog.h"
#include "NetEngine/CServer.h"
#include "NetEngine/CSession.h"
#include "NetEngine/Constants.h"

#include "NetEngine/Packets/PacketStruct.h"
#include "NetEngine/Packets/PacketData.h"
#include <boost/unordered/unordered_flat_map.hpp>
namespace Game
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Cast;
    struct PlayerInfo
    {
        enum State : std::uint8_t
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
        std::uint16_t session_id;
        std::uint16_t host_session_id;
        std::uint16_t room_id;
        std::uint16_t plaza_id;
        std::uint16_t server_id;
        std::uint16_t state_id;
        std::uint32_t health;
        bool in_room;
        bool in_plaza;
        bool is_dead;
        std::uint64_t auth_key;

        Player(const std::uint16_t& sessionId = 0,
            const std::uint16_t& hostSessionId = 0,
            const std::uint16_t& roomId = 0,
            const std::uint16_t& plazaId = 0,
            const std::uint16_t& severId = 0,
            const std::uint16_t& stateId = 0,
            const bool& inRoom = false,
            const bool& inPlaza = false,
            const std::uint64_t& authKey = 0)
            : session_id(sessionId), host_session_id(hostSessionId), room_id(roomId), plaza_id(plazaId), server_id(severId), state_id(stateId), in_room(inRoom), in_plaza(inPlaza), auth_key(authKey) {
                is_dead = false;
                health = 0;
        }

        Player(const Player& other)
        {
            session_id = other.session_id;
            host_session_id = other.host_session_id;
            room_id = other.room_id;
            plaza_id = other.plaza_id;
            server_id = other.server_id;
            state_id = other.state_id;
            in_room = other.in_room;
            in_plaza = other.in_plaza;
            auth_key = other.auth_key;
            is_dead = other.is_dead;
            health = other.health;
        }
        Player& operator=(const Player& other)
        {
            if (this == &other) return *this;
            session_id = other.session_id;
            host_session_id = other.host_session_id;
            room_id = other.room_id;
            plaza_id = other.plaza_id;
            server_id = other.server_id;
            state_id = other.state_id;
            in_room = other.in_room;
            in_plaza = other.in_plaza;
            auth_key = other.auth_key;
            is_dead = other.is_dead;
            health = other.health;
            return *this;
        }
    };

    struct Room
    {
        std::shared_mutex mutex;
        std::uint16_t room_id;
        std::uint16_t host_session_id;
        std::vector<std::uint16_t> players_session_id;
        Room(const std::uint16_t& roomId = 0,
            const std::uint16_t& hostSessionId = 0)
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
        std::uint16_t plaza_id;
        std::vector<std::uint16_t> players_session_id;
        Plaza(const std::uint16_t& plazaId = 0) : plaza_id(plazaId)
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

    //extern asio::strand<asio::io_context::executor_type> global_strand;
    //extern asio::strand<asio::io_context::executor_type> rooms_strand;
    //extern asio::strand<asio::io_context::executor_type> plazas_strand;


    extern std::shared_mutex players_cache_mutex;
    extern std::shared_mutex rooms_cache_mutex;
    extern std::shared_mutex plaza_cache_mutex;
    extern std::shared_mutex room_ids_mutex;
    extern std::shared_mutex plaza_ids_mutex;
    /*
    extern std::unordered_map<std::uint16_t, Player> players_cache;
    extern std::unordered_map<std::uint16_t, Room> rooms_cache;
    extern std::unordered_map<std::uint16_t, Plaza> plaza_cache;
    */
    extern boost::unordered_flat_map<std::uint16_t, Player> players_cache;
    extern boost::unordered_flat_map<std::uint16_t, Room> rooms_cache;
    extern boost::unordered_flat_map<std::uint16_t, Plaza> plaza_cache;
    extern std::vector<std::uint32_t> room_ids;
    extern std::vector<std::uint32_t> plaza_ids;
    using RoomCacheResource = LockedResource<std::unique_lock<std::shared_mutex>, Room>;
    using PlazaCacheResource = LockedResource<std::unique_lock<std::shared_mutex>, Plaza>;
    class CCastServer : public NetEngine::CServer
    {
    public:
        CCastServer();
        ~CCastServer();
        auto IsSessionIdAlready(const std::uint16_t& session_id, const std::vector<std::uint16_t>& session_ids)
        {
            auto findit = std::find(session_ids.begin(), session_ids.end(), session_id);
            return findit != session_ids.end();
        }
        
        auto IsPlayerAlready(const std::uint16_t& session_id)
        {
            std::shared_lock lock(players_cache_mutex);
            if (auto findit = players_cache.find(session_id); findit != players_cache.end())
                return true;
            else
                return false;
        }
       
        auto GetPlayerCacheShared(const std::uint16_t& session_id)
        {
            std::shared_lock lock(players_cache_mutex);
            auto it = players_cache.find(session_id);
            if (it != players_cache.end())
                return LockedResource{ std::shared_lock(it->second.mutex), it->second };
            else
            {
                static thread_local std::shared_mutex null_player_mutex;
                static thread_local Player null_player;
                
                return LockedResource{ std::shared_lock(null_player_mutex), null_player };
            }
        }
        auto GetPlayerCacheUnique(const std::uint16_t& session_id)
        {
            std::shared_lock lock(players_cache_mutex);
            auto it = players_cache.find(session_id);
            if (it != players_cache.end())
                return LockedResource{ std::unique_lock(it->second.mutex), it->second };
            else
            {
                static thread_local std::shared_mutex null_player_mutex;
                static thread_local Player null_player;

                return LockedResource{ std::unique_lock(null_player_mutex), null_player };
            }
        }

        auto GetPlayerCacheSharedByAuthKey(const std::uint64_t& auth_key)
        {
            std::shared_lock lock(players_cache_mutex);

            auto findit = players_cache.begin();
            findit = std::find_if(players_cache.begin(), players_cache.end(),
                [&](const auto& pair) { return pair.second.auth_key == auth_key; });

            if (findit != players_cache.end())
                return LockedResource{ std::shared_lock(findit->second.mutex), findit->second };
            else
            {
                static thread_local std::shared_mutex null_player_mutex;
                static thread_local Player null_player;
                return LockedResource{ std::shared_lock(null_player_mutex), null_player };
            }
        }

        auto GetPlayerCacheUniqueByAuthKey(const std::uint64_t& auth_key)
        {
            std::shared_lock lock(players_cache_mutex);

            auto findit = players_cache.begin();
            findit = std::find_if(players_cache.begin(), players_cache.end(),
                [&](const auto& pair) { return pair.second.auth_key == auth_key; });

            if (findit != players_cache.end())
                return LockedResource{ std::unique_lock(findit->second.mutex), findit->second };
            else
            {
                static thread_local std::shared_mutex null_player_mutex;
                static thread_local Player null_player;
                return LockedResource{ std::unique_lock(null_player_mutex), null_player };
            }
        }

        void AddPlayerCache(const std::uint32_t& session_id, const Player& new_player)
        {
            if (!IsPlayerAlready(session_id))
            {
                auto players_cache_locked = LockedResource{ std::unique_lock(players_cache_mutex), players_cache };
                auto [it, inserted] = players_cache_locked->emplace(session_id, std::move(new_player));
                if (!inserted)
                    BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "Attempted to add a player with id: ({}), but it already exists ", session_id);
            }
        }
        

        void RemovePlayerCache(const std::uint32_t& session_id)
        {
            if (IsPlayerAlready(session_id))
            {
                auto players_cache_locked = LockedResource{ std::unique_lock(players_cache_mutex), players_cache };
                players_cache_locked->erase(session_id);
            }
        }
        
        auto IsRoomAlready(const std::uint16_t& room_id)
        {
            std::shared_lock lock(rooms_cache_mutex);
            if (auto findit = rooms_cache.find(room_id); findit != rooms_cache.end())
                return true;
            else
                return false;
        }
        auto GetRoomCacheShared(const std::uint16_t& room_id)
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
                return LockedResource{ std::shared_lock(null_room_mutex), null_room };
            }
        }
        auto GetRoomCacheUnique(const std::uint16_t& room_id)
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
                return LockedResource{ std::unique_lock(null_room_mutex), null_room };
            }
        }
        void AddRoomCache(const std::uint32_t& room_id, const Room& new_room)
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

        auto IsPlazaAlready(const std::uint16_t& plaza_id)
        {
            std::shared_lock lock(plaza_cache_mutex);
            if (auto findit = plaza_cache.find(plaza_id); findit != plaza_cache.end())
                return true;
            else
                return false;
        }
        auto GetPlazaCacheShared(const std::uint16_t& plaza_id)
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
        auto GetPlazaCacheUnique(const std::uint16_t& plaza_id)
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
        void AddPlazaCache(const std::uint16_t& plaza_id, const Plaza& new_plaza)
        {
            if (!IsPlazaAlready(plaza_id))
            {
                auto locked_plaza_cache = LockedResource{ std::unique_lock(plaza_cache_mutex), plaza_cache };
                auto plaza_ids_locked = LockedResource{ std::unique_lock(plaza_ids_mutex), plaza_ids };
                locked_plaza_cache->emplace(plaza_id, std::move(new_plaza));
                plaza_ids_locked->push_back(plaza_id);
            }
        }
        void RemovePlazaCache(const std::uint16_t& plaza_id)
        {
            if (IsPlazaAlready(plaza_id))
            {
                auto locked_plaza_cache = LockedResource{ std::unique_lock(plaza_cache_mutex), plaza_cache };
                auto plaza_ids_locked = LockedResource{ std::unique_lock(plaza_ids_mutex), plaza_ids };
                locked_plaza_cache->erase(plaza_id);
                plaza_ids_locked->erase(std::remove(plaza_ids_locked->begin(), plaza_ids_locked->end(), plaza_id), plaza_ids_locked->end());
            }
        }
    };
}
