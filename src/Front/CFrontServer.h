#pragma once
#include <string>
#include <functional>

#include "BaseLib/CLog.h"
#include "NetEngine/CServer.h"
#include "NetEngine/CSession.h"
#include "NetEngine/Constants.h"



namespace Game
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Front;
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
    struct Player
    {
        std::shared_mutex mutex;
        uint64_t auth_key = 0;
        bool forcefully_logged_out = false;

        Player(const uint64_t& authKey = 0, const bool& forcefullyLoggedOut = false) : auth_key(authKey), forcefully_logged_out(forcefullyLoggedOut){}
        Player(const Player& other)
        {
            auth_key = other.auth_key;
            forcefully_logged_out = other.forcefully_logged_out;
        }
        Player& operator=(const Player& other)
        {
            if (this == &other) return *this;
            auth_key = other.auth_key;
            forcefully_logged_out = other.forcefully_logged_out;
            return *this;
        }
    };
    extern std::shared_mutex players_cache_mutex;
    extern boost::unordered_flat_map<uint64_t, Player> players_cache;
    class CFrontServer : public NetEngine::CServer
    {
    public:
        CFrontServer();
        ~CFrontServer();

        auto IsPlayerAlready(const uint64_t& auth_key)
        {
            std::shared_lock lock(players_cache_mutex);
            if (auto findit = players_cache.find(auth_key); findit != players_cache.end())
                return true;
            else
                return false;
        }
        auto GetPlayerCacheShared(const uint64_t& auth_key)
        {
            std::shared_lock lock(players_cache_mutex);
            auto it = players_cache.find(auth_key);
            if (it != players_cache.end())
                return LockedResource{ std::shared_lock(it->second.mutex), it->second };
            else
            {
                static thread_local std::shared_mutex null_player_mutex;
                static thread_local Player null_player;

                return LockedResource{ std::shared_lock(null_player_mutex), null_player };
            }
        }
        auto GetPlayerCacheUnique(const uint64_t& auth_key)
        {
            std::shared_lock lock(players_cache_mutex);
            auto it = players_cache.find(auth_key);
            if (it != players_cache.end())
                return LockedResource{ std::unique_lock(it->second.mutex), it->second };
            else
            {
                static thread_local std::shared_mutex null_player_mutex;
                static thread_local Player null_player;

                return LockedResource{ std::unique_lock(null_player_mutex), null_player };
            }
        }
        void AddPlayerCache(const uint64_t& auth_key, const Player& new_player)
        {
            if (!IsPlayerAlready(auth_key))
            {
                auto players_cache_locked = LockedResource{ std::unique_lock(players_cache_mutex), players_cache };
                auto [it, inserted] = players_cache_locked->emplace(auth_key, std::move(new_player));
                if (!inserted)
                    BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "Attempted to add a player with auth key: ({}), but it already exists ", auth_key);
            }
        }
        bool RemovePlayerCache(const uint64_t& auth_key)
        {
            if (IsPlayerAlready(auth_key))
            {
                auto players_cache_locked = LockedResource{ std::unique_lock(players_cache_mutex), players_cache };
                players_cache_locked->erase(auth_key);
                return true;
            }
            return false;
        }
    private:
    };
}
