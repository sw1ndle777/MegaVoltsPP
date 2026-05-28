#pragma once
#include <string>
#include <functional>

#include "BaseLib/CLog.h"
#include "NetEngine/CServer.h"
#include "NetEngine/CSession.h"
#include "NetEngine/Constants.h"

#include "NetEngine/Packets/PacketStruct.h"
#include "NetEngine/Packets/PacketData.h"
#include "BaseLib/CDatabase.h"
#include "BaseLib/CCache.h"
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
			Fail2fa = 0x06,
			EmailNotVerified = 0x07,
			TooManyAttempts = 0x08,
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
        uint32_t sid{ 0 };
        BaseLib::PlazaAuth plazaAuth{};
        BaseLib::FrontAccount frontAccount{};
        BaseLib::ClanInfo clanInfo{};
        Player() {}
        Player(const Player& other)
        {
			sid = other.sid;
			plazaAuth = other.plazaAuth;
            frontAccount = other.frontAccount;
			clanInfo = other.clanInfo;
        }
        Player& operator=(const Player& other)
        {
            if (this == &other) return *this;
			sid = other.sid;
            plazaAuth = other.plazaAuth;
            frontAccount = other.frontAccount;
            clanInfo = other.clanInfo;
            return *this;
        }
    };
    extern CCache<boost::unordered_flat_map<int32_t, Player>> CAccount;
    extern CCache<boost::unordered_flat_set<uint64_t>> CAuthKeys;
    //extern std::shared_mutex players_cache_mutex;
    //extern boost::unordered_flat_map<int32_t, Player> players_cache;
    class CFrontServer : public NetEngine::CServer
    {
    public:
        CFrontServer();
        ~CFrontServer();
        NetEngine::RateLimit::IdentitySnapshot BuildPacketRateLimitIdentitySnapshot(const SCallbackData& callback);
        /*
        auto IsPlayerAlready(int32_t aid)
        {
            std::shared_lock lock(players_cache_mutex);
            if (auto findit = players_cache.find(aid); findit != players_cache.end())
                return true;
            else
                return false;
        }
        auto isPlayerAlreadyByAuthKey(uint64_t key)
        {
            std::shared_lock lock(players_cache_mutex);
            for (auto& [aid, player] : players_cache)
            {
                std::shared_lock player_lock(player.mutex);
                if (player.plazaAuth.AuthKey == key || player.frontAccount.AuthKey == key)
                    return true;
            }
			return false;
        }
        auto GetPlayerCacheShared(int32_t aid)
        {
            std::shared_lock lock(players_cache_mutex);
            auto it = players_cache.find(aid);
            if (it != players_cache.end())
                return LockedResource{ std::shared_lock(it->second.mutex), it->second };
            else
            {
                static thread_local std::shared_mutex null_player_mutex;
                static thread_local Player null_player;

                return LockedResource{ std::shared_lock(null_player_mutex), null_player };
            }
        }
        auto GetPlayerCacheUnique(int32_t aid)
        {
            std::shared_lock lock(players_cache_mutex);
            auto it = players_cache.find(aid);
            if (it != players_cache.end())
                return LockedResource{ std::unique_lock(it->second.mutex), it->second };
            else
            {
                static thread_local std::shared_mutex null_player_mutex;
                static thread_local Player null_player;
                return LockedResource{ std::unique_lock(null_player_mutex), null_player };
            }
        }
        void AddPlayerCache(int32_t aid, Player& new_player)
        {
            if (!IsPlayerAlready(aid))
            {
				DEBUGLOG(dark_cyan, "Adding player to cache with aid: ({}), key: ({})", aid, new_player.plazaAuth.AuthKey);
                auto players_cache_locked = LockedResource{ std::unique_lock(players_cache_mutex), players_cache };
                auto [it, inserted] = players_cache_locked->emplace(aid, new_player);
                if (!inserted)
                    DEBUGLOG(dark_cyan, "Attempted to add a player with aid: ({}), but it already exists ", aid);
            }
            else
                DEBUGLOG(dark_cyan, "Attempted to add a player with aid: ({}), but it already exists ", aid);
        }
        bool RemovePlayerCache(int32_t aid)
        {
            if (IsPlayerAlready(aid))
            {
                auto players_cache_locked = LockedResource{ std::unique_lock(players_cache_mutex), players_cache };
                players_cache_locked->erase(aid);
                return true;
            }
            return false;
        }
        bool RemovePlayerCacheBySid(uint32_t sid)
        {
            auto players_cache_locked = LockedResource{ std::unique_lock(players_cache_mutex), players_cache };
            for (auto it = players_cache_locked->begin(); it != players_cache_locked->end(); ++it)
            {
                auto& player = it->second;
                std::shared_lock player_lock(player.mutex);
                if (player.sid == sid)
                {
                    players_cache_locked->erase(it);
                    return true;
                }
            }
            return false;
		}
        */
    private:
    };
}
