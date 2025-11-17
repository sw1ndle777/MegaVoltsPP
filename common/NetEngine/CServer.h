#pragma once
#include "BaseLib/CLog.h"
#include <iostream>
#include <vector>
#include <map>
#include <set>
//#include <unordered_map>
#include <thread>
#include <asio.hpp>

#include "Constants.h"
#include "CSession.h"

#include <boost_unordered.hpp>
#include "BaseLib/CSettings.h"
#include <BaseLib/CThreadPool.h>
namespace NetEngine
{
    using namespace BaseLib;
    using enum fmt::color;
    class CSession;
    struct SCallbackData;

    struct ExecutionInfo
    {
        uint16_t session_id;
        uint16_t order;
        std::chrono::time_point<std::chrono::steady_clock> start_time;
    };
    
    struct IdGenerator
    {
        uint16_t m_min;
        uint16_t m_max;
        uint16_t m_counter;
        boost::unordered_flat_set<uint16_t> m_freeList;

        IdGenerator(uint16_t minId = 1, uint16_t maxId = 65535)
            : m_min(minId), m_max(maxId), m_counter(minId)
        {
            if (minId > maxId) throw std::invalid_argument("minId must be less than or equal to maxId");
        }
        bool getNext(uint16_t& out)
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
        void free(uint16_t id)
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
            uint32_t concurrent_threads;
            uint32_t database_threads;
			uint32_t logger_threads;
            uint32_t playtime_min_seconds;
            bool useWatchguard;
            SServerSettings(std::string ip, std::string port, std::string ipc_port, bool logPackets, bool useEncryption, bool useMultithreaded, bool useWatchguard, uint32_t concurrent_threads, uint32_t database_threads, uint32_t logger_threads) : ip(ip), port(port), ipc_port(ipc_port), logPackets(logPackets),  useEncryption(useEncryption), useMultithreaded(useMultithreaded),   concurrent_threads(concurrent_threads), database_threads(database_threads), logger_threads(logger_threads), playtime_min_seconds(60), useWatchguard(useWatchguard) {}
        };

    public:
        

        CServer();
        ~CServer();
        void Setup(const SServerSettings& settings, const BaseLib::CSettings::ServerSettings& servers_settings);
        void Run();
        void AcceptSessions(); 
        void AcceptIpcSessions(const std::set<std::string>& ipc_addresses);
        bool AddSession(const std::shared_ptr<CSession>& session);
        void RemoveSession(uint16_t id);
        bool GetNextAvailableSessionId(uint16_t& outId);
        bool GetNextAvailableRoomId(uint16_t& outId);
        bool SetRoomIdAvailable(const uint16_t& plaza_id);
        bool GetNextAvailablePlazaId(uint16_t& outId);
        bool SetPlazaIdAvailable(const uint16_t& plaza_id);
        bool GetNextAvailableQueuePartyId(uint16_t& outId);
        bool SetQueuePartyIdAvailable(const uint16_t& queue_party_id);
        std::shared_ptr<CServer> GetShared() { return shared_from_this(); }
        template <typename T>
            requires (std::integral<std::remove_cvref_t<T>> || std::is_enum_v<std::remove_cvref_t<T>>)
        void On(T order, std::function<void(SCallbackData&)> callback)
        {
			if (m_callbacks.contains(u16_cast(order)))
            {
				EOrder o = magic_enum::enum_cast<EOrder>(u16_cast(order)).value_or(EOrder::NONE);
				auto oName = magic_enum::enum_name(o);
                DEBUGLOG(red, "packet callback with order: ({}) already exists", oName);
                throw std::runtime_error("Callback already exists");
            }
            m_callbacks[u16_cast(order)] = callback;
        }
        //void On(uint16_t id, std::function<void(SCallbackData&)> callback);
        void OnNewSession(std::function<void(std::shared_ptr<CSession>)> callback);
        void OnSessionDisconnected(std::function<void(std::shared_ptr<CSession>)> callback);
        void OnIpcMessage(std::function<void(std::shared_ptr<CSession>, const uint32_t& msg_id, const uint32_t& msg_size, const std::vector<uint8_t>&)>  callback);
        bool IsMultiThreaded();
        uint64_t GetStartTime() const { return this->start_time; }
        uint32_t GetPlaytimeMinSeconds() const { return this->m_playtimeMinSeconds; }
        void SendIpcMessage(const std::string& ip, const std::string& port, const uint32_t ipc_id, std::vector<uint8_t> payload);
        void SendFrontIpc(const uint32_t ipc_id, const std::vector<uint8_t>& payload);
        void SendMainIpc(const uint32_t ipc_id, const std::vector<uint8_t>& payload);
        void SendCastIpc(const uint32_t ipc_id, const std::vector<uint8_t>& payload);
        void WebsitePost(const std::string& path, const std::string& data);
		bool AdoptSid(uint16_t old_sid, uint16_t new_sid, bool evictExisting = false);
      
        std::shared_ptr<CSession> GetSessionById(uint16_t id)
        {
            std::shared_lock lock(m_sessions_mutex);
            auto it = m_sessions.find(id);
            if (it != m_sessions.end()) return it->second;
            return nullptr;
        }
        std::shared_ptr<CSession> GetSessionByIdNoLock(uint16_t id)
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
        void logExecution(uint16_t session_id, uint16_t order);
        void clearExecution(uint16_t session_id, uint16_t order);
    private:

        std::shared_mutex m_sessions_mutex;
        std::shared_mutex m_rooms_mutex;
        std::shared_mutex m_plazas_mutex;
        std::shared_mutex m_queue_party_mutex;
        std::shared_mutex m_server_settings_mutex;
        BaseLib::CSettings::ServerSettings server_settings;
        boost::unordered_flat_map<uint16_t, std::function<void(SCallbackData&)>> m_callbacks;
        boost::unordered_flat_map<uint16_t, std::shared_ptr<CSession>> m_sessions;
        IdGenerator m_sessionIdGenerator;
        IdGenerator m_roomIdGenerator;
        IdGenerator m_plazaIdGenerator;
        IdGenerator m_queuePartyIdGenerator;
        std::shared_ptr<asio::ip::tcp::acceptor> m_acceptor;
        std::shared_ptr<asio::ip::tcp::acceptor> m_ipc_acceptor;
        asio::io_context m_ioContext;
        std::vector<std::jthread> threads;
        asio::ip::tcp::endpoint m_endpoint;
        asio::ip::tcp::socket m_socket;
        asio::ip::tcp::socket m_ipcSocket;
        std::string m_ip_address;
        std::string m_port;
        std::string m_ipc_port;
        bool m_verbose = false;
        bool m_useEncryption = false;
        bool m_useMultithreaded = false;
        bool m_watchguard = false;
        std::shared_ptr<asio::steady_timer> m_watchdogTimer;
        uint32_t m_concurrentThreads = 1;
        uint32_t m_playtimeMinSeconds = 90;
        uint32_t m_loggerThreads = 1;
		uint32_t m_databaseThreads = 0;
        uint32_t m_availableConcurrentThreads = std::jthread::hardware_concurrency();
        std::function<void(std::shared_ptr<CSession>)> m_OnDisconnect;
        std::function<void(std::shared_ptr<CSession>)> m_OnConnect;
        std::function<void(std::shared_ptr<CSession>, const uint32_t& msg_id, const uint32_t& msg_size, const std::vector<uint8_t>&)>  m_OnIpcMessage;
        uint64_t start_time = 0;
        boost::unordered_flat_map<size_t, std::vector<ExecutionInfo>> m_execution_info;
        std::shared_mutex m_execution_guard_mutex;
        void watchdog(std::chrono::nanoseconds interval, std::chrono::nanoseconds timeout);
        void startWatchdog(std::chrono::nanoseconds interval, std::chrono::nanoseconds timeout);

    };
}

//#endif