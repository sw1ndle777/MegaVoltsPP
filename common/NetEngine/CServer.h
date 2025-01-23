#pragma once
#include <iostream>
#include <vector>
#include <map>
#include <set>
//#include <unordered_map>
#include <thread>
#include <asio.hpp>

#include "Constants.h"
#include "CSession.h"
#include "BaseLib/CLog.h"
//#include <boost/unordered/concurrent_flat_map.hpp>
//#include "../deps/unordered/boost/unordered/unordered_flat_map.hpp"
#include <boost/unordered/unordered_flat_map.hpp>
#include <boost/unordered/unordered_flat_set.hpp>
#include "BaseLib/CSettings.h"
namespace NetEngine
{
    class CSession;
    struct SCallbackData;

    struct ExecutionInfo
    {
        std::uint16_t session_id;
        std::uint16_t order;
        std::chrono::time_point<std::chrono::steady_clock> start_time;
    };
    /*
    struct IdGenerator
    {
        std::uint16_t m_min;
        std::uint16_t m_max;
        std::uint16_t m_counter;
        std::vector<std::uint16_t> m_freeList;

        IdGenerator(std::uint16_t minId = 1, std::uint16_t maxId = 65535)
            : m_min(minId), m_max(maxId), m_counter(minId)
        {
            if (minId > maxId) throw std::invalid_argument("minId must be less than or equal to maxId");
            m_freeList.reserve(static_cast<std::size_t>(maxId - minId + 1));
        }
        bool getNext(std::uint16_t& out)
        {
            if (!m_freeList.empty())
            {
                out = m_freeList.back();
                m_freeList.pop_back();
                return true;
            }
            if (m_counter < m_max)
            {
                out = m_counter;
                m_counter++;
                return true;
            }
            return false;
        }
        void free(std::uint16_t id)
        {
            if (id >= m_min && id <= m_max)
                m_freeList.emplace_back(id);
        }
    };*/
    struct IdGenerator
    {
        std::uint16_t m_min;
        std::uint16_t m_max;
        std::uint16_t m_counter;
        boost::unordered_flat_set<std::uint16_t> m_freeList;

        IdGenerator(std::uint16_t minId = 1, std::uint16_t maxId = 65535)
            : m_min(minId), m_max(maxId), m_counter(minId)
        {
            if (minId > maxId) throw std::invalid_argument("minId must be less than or equal to maxId");
        }
        bool getNext(std::uint16_t& out)
        {
            if (!m_freeList.empty())
            {
                auto it = m_freeList.begin();
                out = *it;
                m_freeList.erase(it);
                return true;
            }
            if (m_counter < m_max)
            {
                out = m_counter;
                m_counter++;
                return true;
            }
            return false;
        }
        void free(std::uint16_t id)
        {
            if (id >= m_min && id <= m_max && m_freeList.find(id) == m_freeList.end())
                m_freeList.insert(id);
        }
    };

    class CServer : public std::enable_shared_from_this<CServer>
    {
    public:
        struct SServerSettings
        {
            std::string ip;
            std::string port;
            std::string ipc_port;
            bool logPackets;
            bool useEncryption;
            bool useMultithreaded;
            std::uint32_t concurrent_threads;
            bool useWatchguard;
            //std::uint32_t pool_threads;
            SServerSettings(std::string ip, std::string port, std::string ipc_port, bool logPackets, bool useEncryption, bool useMultithreaded, bool useWatchguard, std::uint32_t concurrent_threads) : ip(ip), port(port), ipc_port(ipc_port), logPackets(logPackets),  useEncryption(useEncryption), useMultithreaded(useMultithreaded), useWatchguard(useWatchguard),  concurrent_threads(concurrent_threads) {}
        };

    public:
        

        CServer();
        ~CServer();
        void Setup(const SServerSettings& settings, const BaseLib::CSettings::ServerSettings& servers_settings);
        void Run();
        void AcceptSessions(); 
        void AcceptIpcSessions(const std::set<std::string>& ipc_addresses);
        bool AddSession(const std::shared_ptr<CSession>& session);
        void RemoveSession(std::uint16_t id);
        bool GetNextAvailableSessionId(std::uint16_t& outId);
        bool GetNextAvailableRoomId(std::uint16_t& outId);
        bool SetRoomIdAvailable(const std::uint16_t& plaza_id);
        bool GetNextAvailablePlazaId(std::uint16_t& outId);
        bool SetPlazaIdAvailable(const std::uint16_t& plaza_id);
        std::shared_ptr<CServer> GetShared() { return shared_from_this(); }
        void On(std::uint16_t id, std::function<void(SCallbackData&)> callback);
        void OnNewSession(std::function<void(std::shared_ptr<CSession>)> callback);
        void OnSessionDisconnected(std::function<void(std::shared_ptr<CSession>)> callback);
        void OnIpcMessage(std::function<void(std::shared_ptr<CSession>, const std::uint32_t& msg_id, const std::uint32_t& msg_size, const std::vector<uint8_t>&)>  callback);
        bool IsMultiThreaded();
        std::uint64_t GetStartTime() const { return this->start_time; }
        void SendIpcMessage(const std::string& ip, const std::string& port, const std::uint32_t ipc_id, std::vector<std::uint8_t> payload);
        void SendFrontIpc(const std::uint32_t ipc_id, const std::vector<std::uint8_t>& payload);
        void SendMainIpc(const std::uint32_t ipc_id, const std::vector<std::uint8_t>& payload);
        void SendCastIpc(const std::uint32_t ipc_id, const std::vector<std::uint8_t>& payload);
        
      
        std::shared_ptr<CSession> GetSessionById(std::uint16_t id)
        {
            std::shared_lock lock(m_sessions_mutex);
            auto it = m_sessions.find(id);
            if (it != m_sessions.end()) return it->second;
            return nullptr;
        }
        std::shared_ptr<CSession> GetSessionByIdNoLock(std::uint16_t id)
        {
            std::shared_lock lock(m_sessions_mutex);
            auto it = m_sessions.find(id);
            if (it != m_sessions.end()) return it->second;
            return nullptr;
        }
        auto GetSessions()
        {
            return LockedResource{ std::shared_lock(m_sessions_mutex), m_sessions };
        }
        auto& GetIoContext()
        {
            return m_ioContext;
        }
        auto IsVerbose() const { return m_verbose; }
        void logExecution(std::uint16_t session_id, std::uint16_t order);
        void clearExecution(std::uint16_t session_id, std::uint16_t order);
    private:

       


        std::shared_mutex m_sessions_mutex;
        std::shared_mutex m_rooms_mutex;
        std::shared_mutex m_plazas_mutex;
        std::shared_mutex m_server_settings_mutex;
        BaseLib::CSettings::ServerSettings server_settings;
        //std::map<std::uint16_t, std::function<void(SCallbackData&)>> m_callbacks;
        boost::unordered_flat_map<std::uint16_t, std::function<void(SCallbackData&)>> m_callbacks;
        //std::unordered_map<std::uint16_t, std::shared_ptr<CSession>> m_sessions;
        boost::unordered_flat_map<std::uint16_t, std::shared_ptr<CSession>> m_sessions;
        IdGenerator m_sessionIdGenerator;
        IdGenerator m_roomIdGenerator;
        IdGenerator m_plazaIdGenerator;

        //std::vector<bool> m_available_session_ids;
        //std::vector<bool> m_available_room_ids;
        //std::vector<bool> m_available_plaza_ids;
        std::shared_ptr<asio::ip::tcp::acceptor> m_acceptor;
        std::shared_ptr<asio::ip::tcp::acceptor> m_ipc_acceptor;
        asio::io_context m_ioContext;
        //std::shared_ptr<asio::thread_pool> m_threadPool;
        std::vector<std::jthread> threads;
        asio::ip::tcp::endpoint m_endpoint;
        asio::ip::tcp::socket m_socket;
        std::string m_ip_address;
        std::string m_port;
        std::string m_ipc_port;
        bool m_verbose = false;
        bool m_useEncryption = false;
        bool m_useMultithreaded = false;
        bool m_watchguard = false;
        std::uint32_t m_concurrentThreads = 1;
        //std::uint32_t m_poolThreads = 1;
        std::uint32_t m_availableConcurrentThreads = std::jthread::hardware_concurrency();
        std::function<void(std::shared_ptr<CSession>)> m_OnDisconnect;
        std::function<void(std::shared_ptr<CSession>)> m_OnConnect;
        std::function<void(std::shared_ptr<CSession>, const std::uint32_t& msg_id, const std::uint32_t& msg_size, const std::vector<uint8_t>&)>  m_OnIpcMessage;
        std::uint64_t start_time = 0;
        boost::unordered_flat_map<std::size_t, std::vector<ExecutionInfo>> m_execution_info;
        std::shared_mutex m_execution_guard_mutex;
        asio::steady_timer m_watchdog_timer;
        void watchdog(std::chrono::nanoseconds timeout);
        void startWatchdog(std::chrono::nanoseconds interval, std::chrono::nanoseconds timeout);

    };
}

//#endif