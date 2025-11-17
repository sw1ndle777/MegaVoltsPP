#include "CServer.h"
#include <fmt/color.h>
#include <numeric>

namespace NetEngine
{
	using namespace BaseLib;
    using enum fmt::color;
    CServer::CServer() : m_ioContext(), m_socket(m_ioContext), m_ipcSocket(m_ioContext)//, m_available_session_ids(65536, true), m_available_room_ids(4096, true), m_available_plaza_ids(65536, true)
    { 
        m_sessionIdGenerator = IdGenerator(1, 65535);
        m_roomIdGenerator = IdGenerator(2048, 4096);
        m_plazaIdGenerator = IdGenerator(0, 65535);
        m_queuePartyIdGenerator = IdGenerator(2048, 4096);
    }
    CServer::~CServer() {}
    void CServer::Setup(const SServerSettings& settings, const BaseLib::CSettings::ServerSettings& servers_settings)
    {
        server_settings = servers_settings;
        m_ip_address = settings.ip;
        m_port = settings.port;
        m_ipc_port = settings.ipc_port;
        m_verbose = settings.logPackets;
        m_useEncryption = settings.useEncryption;
        m_useMultithreaded = settings.useMultithreaded;
        m_concurrentThreads = settings.concurrent_threads;
        m_playtimeMinSeconds = settings.playtime_min_seconds;
        m_watchguard = settings.useWatchguard;
        m_loggerThreads = settings.logger_threads;
		m_databaseThreads = settings.database_threads;

        asio::error_code errorCode;
        asio::ip::tcp::resolver resolver(m_ioContext);
        asio::ip::tcp::endpoint endpoint = *resolver.resolve(m_ip_address, m_port, errorCode).begin();
        asio::ip::tcp::endpoint ipc_endpoint = *resolver.resolve(m_ip_address, m_ipc_port, errorCode).begin();

        if (errorCode)
        {
            DEBUGLOG(red, "failed to resolve endpoint: ({})", errorCode.message().c_str());
            return;
        }
        else
        {
            
            if (m_useMultithreaded && m_concurrentThreads != 1)
            {
                auto max_concurrent_threads = std::jthread::hardware_concurrency();
                if (m_concurrentThreads == 0)
                    m_concurrentThreads = max_concurrent_threads;
                else if (m_concurrentThreads >= max_concurrent_threads)
                    m_concurrentThreads = max_concurrent_threads;
            }
            
            

            m_acceptor = std::make_shared<asio::ip::tcp::acceptor>(m_ioContext);
            m_acceptor->open(endpoint.protocol());
            m_acceptor->set_option(asio::ip::tcp::acceptor::reuse_address(true));
            m_acceptor->set_option(asio::ip::tcp::no_delay(true));
            m_acceptor->bind(endpoint);
            m_acceptor->listen();

            m_ipc_acceptor = std::make_shared<asio::ip::tcp::acceptor>(m_ioContext);
            m_ipc_acceptor->open(ipc_endpoint.protocol());
            m_ipc_acceptor->set_option(asio::ip::tcp::acceptor::reuse_address(true));
            m_ipc_acceptor->set_option(asio::ip::tcp::no_delay(true));
            m_ipc_acceptor->bind(ipc_endpoint);
            m_ipc_acceptor->listen();
   
        }
    }
    void CServer::Run()
    {

		DEBUGLOG(dark_cyan, "running server on: ({}:{})", m_ip_address, m_port);
		DEBUGLOG(dark_cyan, "asio threads: ({}) out of ({})", m_concurrentThreads, std::jthread::hardware_concurrency());
		DEBUGLOG(dark_cyan, "logger threads: ({}) out of ({})", m_loggerThreads, std::jthread::hardware_concurrency());
		if (m_databaseThreads)
			DEBUGLOG(dark_cyan, "database threads: ({}) out of ({})", m_databaseThreads, std::jthread::hardware_concurrency());

        this->start_time = Utility::GetUtcTimeNowInMilliseconds();
        static const std::set<std::string> ipc_addresses = { "127.0.0.1", this->server_settings.front.host, this->server_settings.main.host, this->server_settings.cast.host};

        if (m_watchguard)
            startWatchdog(std::chrono::seconds(1), std::chrono::seconds(5));


        if (m_useMultithreaded)
        {
            auto work = asio::make_work_guard(m_ioContext);
            for (uint32_t i = 0; i < m_concurrentThreads; i++)
                threads.emplace_back(std::jthread([&] {
                while (true)
                {
                    try { m_ioContext.run(); }
                    catch (const std::exception& e)
                    {
                        DEBUGLOG(red, e.what());
                    }
                    catch (...)
                    {
                        DEBUGLOG(red, "unknown asio exception type");
                    }
                }
            }));

            AcceptSessions();
            AcceptIpcSessions(ipc_addresses);
        }
        else
        {
            auto work = asio::make_work_guard(m_ioContext);
            AcceptSessions();
            AcceptIpcSessions(ipc_addresses);
            while (true)
            {
                m_ioContext.run();
            }
        } 
    }
    void CServer::AcceptSessions()
    {
        m_acceptor->async_accept(m_socket, [this](std::error_code ec) 
        {
            if (!ec)
            {
                CSession::SSessionSettings settings;

                settings.verbose = m_verbose;
                settings.useEncryption = m_useEncryption;
                settings.callbacks.insert(m_callbacks.begin(), m_callbacks.end());
                uint16_t session_id = 0;

                if (GetNextAvailableSessionId(session_id))
                {
                    auto session = CSession::Create(std::move(m_socket), m_ioContext, this, settings, session_id);
                    if (m_OnDisconnect) session->SetOnDisconnectCallback(m_OnDisconnect);
                    if (m_OnConnect)  m_OnConnect(session);
                    AddSession(session);
                    session->DoRead();
                }
                else
                {
                    DEBUGLOG(red, "session pool is full");
                    m_socket.close();
                }
            }
            else
            {
                DEBUGLOG(red, "failed to accept session: ({})", ec.message().c_str());
                m_socket.close();
            }
            AcceptSessions();
        });
    }
    void CServer::AcceptIpcSessions(const std::set<std::string>& ipc_addresses)
    {
        m_ipc_acceptor->async_accept(m_ipcSocket, [this, ipc_addresses](std::error_code ec)
        {
            if (!ec)
            {

                asio::error_code endpoint_error;
                auto remote_endpoint = m_ipcSocket.remote_endpoint(endpoint_error);

                if (endpoint_error)
                {
                    DEBUGLOG(red, "Failed to retrieve remote endpoint: ({})", endpoint_error.message().c_str());
                    AcceptIpcSessions(ipc_addresses);
                }

                auto remote_ip = remote_endpoint.address().to_string();
                bool is_ipc = ipc_addresses.find(remote_ip) != ipc_addresses.end();
                
                if (is_ipc)
                {
                    CSession::SSessionSettings settings;

                    settings.verbose = m_verbose;
                    settings.useEncryption = m_useEncryption;
                    settings.callbacks.insert(m_callbacks.begin(), m_callbacks.end());

                    uint16_t session_id = 0;
                    auto session = CSession::Create(std::move(m_ipcSocket), m_ioContext, this, settings, session_id);
                    if (m_OnIpcMessage) session->SetOnIpcMessageCallback(m_OnIpcMessage);
                    session->DoReadIpc();
                }
                else
                {
                    DEBUGLOG(red, "non whitelisted IPC connection from: ({})", remote_ip.c_str());
                    m_ipcSocket.close();
                }
                   
            }
            else
            {
                DEBUGLOG(red, "failed to accept session: ({})", ec.message().c_str());
            }
            AcceptIpcSessions(ipc_addresses);
        });
    }

    bool CServer::AddSession(const std::shared_ptr<CSession>& session)
    {
        std::unique_lock lock(m_sessions_mutex);
        const auto& id = session->GetSessionId();
        DEBUGLOG(dark_cyan, "added sid=({})", id);
        if (id == 0 || m_sessions.count(id))
        {
            DEBUGLOG(red, "can't add sid=({})", id);
            return false;
        }

        m_sessions[id] = session;
        return true;
    }
    void CServer::RemoveSession(uint16_t id)
    {
        std::unique_lock lock(m_sessions_mutex);

        auto it = m_sessions.find(id);
        if (id == 0 || !m_sessions.count(id))
        {
            DEBUGLOG(red, "could not find sid=({})", id);
            return;
        }

        m_sessions.erase(it);
        m_sessionIdGenerator.free(id);
        DEBUGLOG(dark_cyan, "removed sid=({}) and now it's available", id);
    }
    
    bool CServer::GetNextAvailableSessionId(uint16_t& outId)
    {
        std::unique_lock lock(m_sessions_mutex);
        auto ret_status = m_sessionIdGenerator.getNext(outId);
        DEBUGLOG(dark_cyan, "assigned new sid=({})", outId);
        return ret_status;
    }
    
    bool CServer::GetNextAvailableRoomId(uint16_t& outId)
    {
        std::unique_lock lock(m_rooms_mutex);
        return m_roomIdGenerator.getNext(outId);
    }
    bool CServer::SetRoomIdAvailable(const uint16_t& room_id)
    {
        std::unique_lock lock(m_rooms_mutex);
        m_roomIdGenerator.free(room_id);
        return true;
    }
    bool CServer::GetNextAvailablePlazaId(uint16_t& outId)
    {
        std::unique_lock lock(m_plazas_mutex);
        return m_plazaIdGenerator.getNext(outId);
    }
    bool CServer::SetPlazaIdAvailable(const uint16_t& plaza_id)
    {
        std::unique_lock lock(m_plazas_mutex);
        m_plazaIdGenerator.free(plaza_id);
        return true;
    }

    bool CServer::GetNextAvailableQueuePartyId(uint16_t& outId)
    {
        std::unique_lock lock(m_queue_party_mutex);
        return m_queuePartyIdGenerator.getNext(outId);
    }

    bool CServer::SetQueuePartyIdAvailable(const uint16_t& queue_party_id)
    {
        std::unique_lock lock(m_queue_party_mutex);
        m_queuePartyIdGenerator.free(queue_party_id);
        return true;
    }
    void CServer::OnNewSession(std::function<void(std::shared_ptr<CSession>)> callback)
    {
        this->m_OnConnect = callback;
    }
    void CServer::OnSessionDisconnected(std::function<void(std::shared_ptr<CSession>)> callback)
    {
        this->m_OnDisconnect = callback;
    }
    void CServer::OnIpcMessage(std::function<void(std::shared_ptr<CSession>, const uint32_t& msg_id, const uint32_t& msg_size, const std::vector<uint8_t>&)>  callback)
    {
        this->m_OnIpcMessage = callback;
    }
    bool CServer::IsMultiThreaded()
    {
        return this->m_useMultithreaded;
    }
    void CServer::SendIpcMessage(const std::string& ip, const std::string& port, const uint32_t ipc_id, std::vector<uint8_t> payload)
    {
        try
        {
            auto socket = std::make_shared<asio::ip::tcp::socket>(m_ioContext);
            asio::ip::tcp::resolver resolver(m_ioContext);
            asio::ip::tcp::resolver::results_type endpoints = resolver.resolve(ip, port);

            asio::async_connect(*socket, endpoints, [socket, ipc_id, payload = std::move(payload)](const asio::error_code& ec, const asio::ip::tcp::endpoint&)
            {
                if (ec)
                {
                    DEBUGLOG(red, "Failed to connect to IPC target: {}", ec.message().c_str());
                    return;
                }
                
                asio::error_code optEc;
                socket->set_option(asio::ip::tcp::no_delay(true), optEc);

                optEc = {};
                socket->set_option(asio::socket_base::linger(true, 0), optEc);

                uint32_t data_size = static_cast<uint32_t>(payload.size());
                std::vector<uint8_t>* message = new std::vector<uint8_t>(8 + payload.size());
                std::memcpy(&(*message)[0], &ipc_id, sizeof(ipc_id));
                std::memcpy(&(*message)[4], &data_size, sizeof(data_size));
                std::copy(payload.begin(), payload.end(), message->begin() + 8);

                asio::async_write(*socket, asio::buffer(*message), [socket, message](const asio::error_code& ec, size_t /*bytes_transferred*/)
                {
                    delete message;
                    if (ec)
                    {
                        DEBUGLOG(red, "Failed to send IPC message: {}", ec.message().c_str());
                    }
                    socket->shutdown(asio::ip::tcp::socket::shutdown_both);
                    socket->close();
                });
            });
        }
        catch (const std::exception& e)
        {
            DEBUGLOG(red, "Exception in SendIpcMessage: {}", e.what());
        }
    }
    void CServer::SendFrontIpc(const uint32_t ipc_id, const std::vector<uint8_t>& payload)
    {
        std::shared_lock lock(m_server_settings_mutex);
        std::string host = (this->server_settings.front.host == "0.0.0.0") ? "127.0.0.1" : this->server_settings.front.host;
        SendIpcMessage(host, std::to_string(this->server_settings.front.ipc_port), ipc_id, payload);
    }
    void CServer::SendMainIpc(const uint32_t ipc_id, const std::vector<uint8_t>& payload)
    {
        std::shared_lock lock(m_server_settings_mutex);
        std::string host = (this->server_settings.main.host == "0.0.0.0") ? "127.0.0.1" : this->server_settings.main.host;
        SendIpcMessage(host, std::to_string(this->server_settings.main.ipc_port), ipc_id, payload);
    }
    void CServer::SendCastIpc(const uint32_t ipc_id, const std::vector<uint8_t>& payload)
    {
        std::shared_lock lock(m_server_settings_mutex);
        std::string host = (this->server_settings.cast.host == "0.0.0.0") ? "127.0.0.1" : this->server_settings.cast.host;
        SendIpcMessage(host, std::to_string(this->server_settings.cast.ipc_port), ipc_id, payload);
    }
    void CServer::WebsitePost(const std::string& path, const std::string& payload)
    {
        try
        {
            std::shared_lock server_settings_lock(m_server_settings_mutex);
            auto &host = this->server_settings.website.host;
            auto port = std::to_string(this->server_settings.website.port);
            auto timeout = this->server_settings.website.timeout;


            auto socket = std::make_shared<asio::ip::tcp::socket>(m_ioContext);
            asio::ip::tcp::resolver resolver(m_ioContext);
            auto endpoints = resolver.resolve(host, port);

            auto request = std::make_shared<std::string>(
                "POST " + path + " HTTP/1.1\r\n" +
                "Host: " + host + "\r\n" +
                "Content-Type: application/json\r\n" +
                "Content-Length: " + std::to_string(payload.size()) + "\r\n" +
                "Connection: close\r\n\r\n" +
                payload
            );

            auto timer = std::make_shared<asio::steady_timer>(m_ioContext, std::chrono::milliseconds(timeout));

            timer->async_wait([socket, timer](const asio::error_code& ec) {
                if (!ec)
                {
                    DEBUGLOG(red, "Failed to send post website request due to timeout");
                    socket->close();
                }
            });

            asio::async_connect(*socket, endpoints, [socket, host, path, request, timer](const asio::error_code& ec, const asio::ip::tcp::endpoint&)
            {
                if (ec)
                {
                    DEBUGLOG(red, "Failed to connect to website: {}", ec.message().c_str());
                    timer->cancel();
                    return;
                }
                asio::async_write(*socket, asio::buffer(*request), [socket, request, timer](const asio::error_code& ec, size_t /*bytes_transferred*/)
                {
                    if (ec)
                    {
                        DEBUGLOG(red, "Failed to send post website request: {}", ec.message().c_str());
                    }
                    timer->cancel();
                    socket->shutdown(asio::ip::tcp::socket::shutdown_both);
                    socket->close();
                });
            });
        }
        catch (const std::exception& e)
        {
            DEBUGLOG(red, "Exception in WebsitePost: {}", e.what());
        }
    }
    bool CServer::AdoptSid(uint16_t old_sid, uint16_t new_sid, bool evictExisting)
    {
        if (new_sid == 0 || old_sid == 0)
        {
            DEBUGLOG(red,
				"AdoptSessionId: cannot rebind session from ({}) to ({})", old_sid, new_sid);
            return false;
        }
            
        if (new_sid == old_sid)
        {
            DEBUGLOG(dark_cyan,
				"AdoptSessionId: no need to rebind session from ({}) to ({})", old_sid, new_sid);
            return true;
        }

        std::shared_ptr<CSession> moving;

        {
            std::unique_lock lk(m_sessions_mutex);
            auto itOld = m_sessions.find(old_sid);
            if (itOld == m_sessions.end())
            {
				DEBUGLOG(red, "AdoptSessionId: cannot find session with id ({}) to rebind to ({})", old_sid, new_sid);
                return false;
            }
            auto itNew = m_sessions.find(new_sid);
            if (itNew != m_sessions.end())
            {
                if (!evictExisting)
                {
					DEBUGLOG(red, "AdoptSessionId: session with id ({}) already exists, cannot rebind session from ({}) to ({})", new_sid, old_sid, new_sid);
                    return false;
                }
                auto victim = itNew->second;
                m_sessions.erase(itNew);
                lk.unlock();
                if (victim) victim->Disconnect();
                lk.lock();
                itOld = m_sessions.find(old_sid);
                if (itOld == m_sessions.end())
                {
					DEBUGLOG(red, "AdoptSessionId: cannot find session with id ({}) to rebind to ({})", old_sid, new_sid);
                    return false;
                }
            }

            moving = itOld->second;
            m_sessions.erase(itOld);
            m_sessions[new_sid] = moving;
            if (moving) moving->SetSessionId(new_sid);
            m_sessionIdGenerator.free(old_sid);
        }

        DEBUGLOG(dark_cyan,
            "AdoptSessionId: rebound session from ({}) to ({})", old_sid, new_sid);
        return true;
    }
    void CServer::logExecution(uint16_t session_id, uint16_t order)
    {
        if (m_watchguard)
        {
            std::unique_lock lock(m_execution_guard_mutex);
            auto thread_id = std::hash<std::thread::id>{}(std::this_thread::get_id());
            auto& execution_vector = m_execution_info[thread_id];

            auto time_now = std::chrono::steady_clock::now();
            execution_vector.push_back({ session_id, order, time_now });
            if (order != 281 && order != 71 && order != 322 && order != 72 && order != 257 && order != 282 && order != 77) DEBUGLOG(dark_green,
                "Handler started: Thread ID: {}, sid={}, Order: {}",
                thread_id, session_id, order);
        }
        
    }
    void CServer::clearExecution(uint16_t session_id, uint16_t order)
    {
        if (m_watchguard)
        {
            std::unique_lock lock(m_execution_guard_mutex);

            auto thread_id = std::hash<std::thread::id>{}(std::this_thread::get_id());
            auto& execution_vector = m_execution_info[thread_id];

            for (auto it = execution_vector.begin(); it != execution_vector.end(); ++it)
            {
                if (it->session_id == session_id && it->order == order)
                {
                    auto elapsed_time = std::chrono::duration<double, std::milli>(
                        std::chrono::steady_clock::now() - it->start_time);

                    if (order != 281 && order != 71 && order != 322 && order != 72 && order != 257 && order != 282 && order != 77) DEBUGLOG(dark_green,
                        "Handler completed: Thread ID: {}, sid={}, Order: {}, Elapsed Time: {:.3f}ms",
                        thread_id, session_id, order, elapsed_time.count());

                    execution_vector.erase(it);
                    break;
                }
            }

            if (execution_vector.empty())
                m_execution_info.erase(thread_id);
        }
    }
    void CServer::watchdog(std::chrono::nanoseconds interval, std::chrono::nanoseconds timeout)
    {
        std::shared_lock lock(m_execution_guard_mutex);

        auto now = std::chrono::steady_clock::now();
        for (const auto& [thread_id, execution_vector] : m_execution_info)
        {
            for (const auto& info : execution_vector)
            {
                auto elapsed_time = std::chrono::duration<double, std::milli>(now - info.start_time);
                if (elapsed_time > timeout)
                {
                    DEBUGLOG(red,
                        "[Watchdog] Deadlock detected! Thread ID: {}, sid={}, Order: {}, Elapsed Time: {:.3f}ms",
                        thread_id, info.session_id, info.order, elapsed_time.count());
                }
            }
        }
		if(m_watchdogTimer)
		{
			m_watchdogTimer->expires_after(interval);
			m_watchdogTimer->async_wait([this, interval, timeout](const std::error_code& ec)
			{
				if (!ec && !m_watchguard)
				{
					[[maybe_unused]] auto ignored_result = BaseLib::LogPool->submit_task([this, interval, timeout]()
					{
						this->watchdog(interval, timeout);
					}, BS::pr::lowest);
				}
			});
		}	
    }
    void CServer::startWatchdog(std::chrono::nanoseconds interval, std::chrono::nanoseconds timeout)
    {
		if (!m_watchguard)
		{
			if (!m_watchdogTimer)
				m_watchdogTimer = std::make_shared<asio::steady_timer>(GetIoContext());

			m_watchdogTimer->expires_after(interval);
			m_watchdogTimer->async_wait([this, interval, timeout](const std::error_code& ec)
			{
				if (!ec && !m_watchguard)
				{
                    [[maybe_unused]] auto ignored_result = BaseLib::LogPool->submit_task([this, interval, timeout]()
					{
						this->watchdog(interval, timeout);
					}, BS::pr::lowest);
				}
			});
		}
    }
}

