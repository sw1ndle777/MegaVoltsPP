#include "CServer.h"
#include <fmt/color.h>
#include <numeric>
namespace NetEngine
{
    CServer::CServer() : m_ioContext(), m_socket(m_ioContext), m_watchdog_timer(asio::steady_timer(m_ioContext)), m_available_session_ids(65536, true), m_available_room_ids(4096, true), m_available_plaza_ids(65536, true)
    { 
        m_available_session_ids[0] = false; 
        //m_available_room_ids[0] = false;
        for (std::uint16_t i = 0; i < 2048; i++)
            m_available_room_ids[i] = false;
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
        m_watchguard = settings.useWatchguard;
        //m_poolThreads = settings.pool_threads;

        asio::error_code errorCode;
        asio::ip::tcp::resolver resolver(m_ioContext);
        asio::ip::tcp::endpoint endpoint = *resolver.resolve(m_ip_address, m_port, errorCode).begin();
        asio::ip::tcp::endpoint ipc_endpoint = *resolver.resolve(m_ip_address, m_ipc_port, errorCode).begin();

        if (errorCode)
        {
            BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::red, "failed to resolve endpoint: ({})", errorCode.message().c_str());
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

        BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "running server on: ({}:{})", m_ip_address.c_str(), m_port.c_str());
        BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "asio threads: ({}) out of ({})", m_concurrentThreads, std::jthread::hardware_concurrency());
        //BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "pool threads: ({}) out of ({})", m_poolThreads, std::jthread::hardware_concurrency());

        this->start_time = Utility::GetUtcTimeNowInMilliseconds();
        static const std::set<std::string> ipc_addresses = { "127.0.0.1", this->server_settings.front.host, this->server_settings.main.host, this->server_settings.cast.host};

        if (m_watchguard)
            startWatchdog(std::chrono::seconds(1), std::chrono::seconds(5));


        if (m_useMultithreaded)
        {
            auto work = asio::make_work_guard(m_ioContext);
            for (std::uint32_t i = 0; i < m_concurrentThreads; i++)
                threads.emplace_back(std::jthread([&] {
                while (true)
                {
                    try { m_ioContext.run(); }
                    catch (const std::exception& e)
                    {
                        BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::red, e.what());
                    }
                    catch (...)
                    {
                        BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::red, "unknown asio exception type");
                    }
                }
            }));

            AcceptSessions();
            AcceptIpcSessions(ipc_addresses);
            //for (auto& t : threads)
            //    t.join();
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
                std::uint16_t session_id = 0;

                if (GetNextAvailableSessionId(session_id))
                {
                    auto session = std::make_shared<CSession>(std::move(m_socket), m_ioContext, this, settings, session_id);
                    if (m_OnDisconnect) session->SetOnDisconnectCallback(m_OnDisconnect);
                    if (m_OnConnect)  m_OnConnect(session);
                    AddSession(session);
                    session->DoRead();
                }
                else
                    BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::red, "session pool is full");
               
            }
            else
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::red, "failed to accept session: ({})", ec.message().c_str());

            AcceptSessions();
        });
    }
    void CServer::AcceptIpcSessions(const std::set<std::string>& ipc_addresses)
    {
        m_ipc_acceptor->async_accept(m_socket, [this, ipc_addresses](std::error_code ec)
        {
            if (!ec)
            {

                asio::error_code endpoint_error;
                auto remote_endpoint = m_socket.remote_endpoint(endpoint_error);

                if (endpoint_error)
                {
                    BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::red, "Failed to retrieve remote endpoint: ({})", endpoint_error.message());
                    AcceptIpcSessions(ipc_addresses);
                    return;
                }

                auto remote_ip = remote_endpoint.address().to_string();
                bool is_ipc = ipc_addresses.find(remote_ip) != ipc_addresses.end();
                
                if (is_ipc)
                {
                    CSession::SSessionSettings settings;

                    settings.verbose = m_verbose;
                    settings.useEncryption = m_useEncryption;
                    settings.callbacks.insert(m_callbacks.begin(), m_callbacks.end());

                    std::uint16_t session_id = 0;

                    auto session = std::make_shared<CSession>(std::move(m_socket), m_ioContext, this, settings, session_id);
                    if (m_OnIpcMessage) session->SetOnIpcMessageCallback(m_OnIpcMessage);
                    session->DoReadIpc();
                }
                else
                    BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::red, "non whitelisted IPC connection from: ({})", remote_ip.c_str());
            }
            else
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::red, "failed to accept session: ({})", ec.message().c_str());

            AcceptIpcSessions(ipc_addresses);
        });
    }

    bool CServer::AddSession(const std::shared_ptr<CSession>& session)
    {
        std::unique_lock lock(m_sessions_mutex);
        const auto& id = session->GetSessionId();
        if (id == 0 || !m_available_session_ids[id]) return false;

        m_sessions[id] = session;
        m_available_session_ids[id] = false; 
        return true;
    }
    void CServer::RemoveSession(std::uint16_t id)
    {
        std::unique_lock lock(m_sessions_mutex);

        auto it = m_sessions.find(id);
        if (id == 0 || it == m_sessions.end()) return;

        m_sessions.erase(it);
        m_available_session_ids[id] = true;
    }
    bool CServer::GetNextAvailableSessionId(std::uint16_t& outId)
    {
        std::shared_lock lock(m_sessions_mutex);
        for (std::uint16_t id = 1; id < m_available_session_ids.size(); id++) 
        {
            if (m_available_session_ids[id]) 
            {
                outId = id;
                return true;
            }
        }
        return false;
    }
    bool CServer::GetNextAvailableRoomId(std::uint16_t& outId)
    {
        std::shared_lock lock(m_rooms_mutex);
        for (std::uint16_t id = 0; id < m_available_room_ids.size(); id++)
        {
            if (m_available_room_ids[id])
            {
                outId = id;
                return true;
            }
        }
        return false;
    }
    bool CServer::SetRoomIdAvailable(const std::uint16_t& room_id, bool available)
    {
        std::unique_lock lock(m_rooms_mutex);
        m_available_room_ids[room_id] = available;
        return true;
    }
    bool CServer::GetNextAvailablePlazaId(std::uint16_t& outId)
    {
        std::shared_lock lock(m_plazas_mutex);
        for (std::uint16_t id = 0; id < m_available_plaza_ids.size(); id++)
        {
            if (m_available_plaza_ids[id])
            {
                outId = id;
                return true;
            }
        }
        return false;
    }
    bool CServer::SetPlazaIdAvailable(const std::uint16_t& plaza_id, bool available)
    {
        std::unique_lock lock(m_plazas_mutex);
        m_available_plaza_ids[plaza_id] = available;
        return true;
    }

  
    void CServer::On(uint16_t id, std::function<void(SCallbackData&)> callback)
    {
        if (m_callbacks.find(id) != m_callbacks.end())
        {
            BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::red, "packet callback with order: ({}) already exists", id);
            throw std::runtime_error("Callback already exists");
        }

        m_callbacks[id] = callback;
    }
    void CServer::OnNewSession(std::function<void(std::shared_ptr<CSession>)> callback)
    {
        this->m_OnConnect = callback;
    }
    void CServer::OnSessionDisconnected(std::function<void(std::shared_ptr<CSession>)> callback)
    {
        this->m_OnDisconnect = callback;
    }
    void CServer::OnIpcMessage(std::function<void(std::shared_ptr<CSession>, const std::uint32_t& msg_id, const std::uint32_t& msg_size, const std::vector<uint8_t>&)>  callback)
    {
        this->m_OnIpcMessage = callback;
    }
    bool CServer::IsMultiThreaded()
    {
        return this->m_useMultithreaded;
    }
    void CServer::SendIpcMessage(const std::string& ip, const std::string& port, const std::uint32_t ipc_id, const std::vector<std::uint8_t>& payload)
    {
        try
        {
            auto socket = std::make_shared<asio::ip::tcp::socket>(m_ioContext);
            asio::ip::tcp::resolver resolver(m_ioContext);
            asio::ip::tcp::resolver::results_type endpoints = resolver.resolve(ip, port);

            asio::async_connect(*socket, endpoints, [socket, this, ipc_id, payload](const asio::error_code& ec, const asio::ip::tcp::endpoint&)
            {
                if (ec)
                {
                    BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::red, "Failed to connect to IPC target: {}", ec.message());
                    return;
                }
                
                std::uint32_t data_size = static_cast<std::uint32_t>(payload.size());
                std::vector<std::uint8_t> message;
                message.resize(8 + payload.size());
               
                std::memcpy(&message[0], &ipc_id, sizeof(ipc_id));
                std::memcpy(&message[4], &data_size, sizeof(data_size));
                std::copy(payload.begin(), payload.end(), message.begin() + 8);

                asio::async_write(*socket, asio::buffer(message), [socket](const asio::error_code& ec, std::size_t /*bytes_transferred*/)
                {
                    if (ec)
                    {
                        BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::red, "Failed to send IPC message: {}", ec.message());
                        return;
                    }
                    socket->shutdown(asio::ip::tcp::socket::shutdown_both);
                    socket->close();
                });
            });
        }
        catch (const std::exception& e)
        {
            BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::red, "Exception in SendIpcMessage: {}", e.what());
        }
    }
    void CServer::SendFrontIpc(const std::uint32_t ipc_id, const std::vector<std::uint8_t>& payload)
    {
        std::shared_lock lock(m_server_settings_mutex);
        std::string host = (this->server_settings.front.host == "0.0.0.0") ? "127.0.0.1" : this->server_settings.front.host;
        SendIpcMessage(host, std::to_string(this->server_settings.front.ipc_port), ipc_id, payload);
    }
    void CServer::SendMainIpc(const std::uint32_t ipc_id, const std::vector<std::uint8_t>& payload)
    {
        std::shared_lock lock(m_server_settings_mutex);
        std::string host = (this->server_settings.main.host == "0.0.0.0") ? "127.0.0.1" : this->server_settings.main.host;
        SendIpcMessage(host, std::to_string(this->server_settings.main.ipc_port), ipc_id, payload);
    }
    void CServer::SendCastIpc(const std::uint32_t ipc_id, const std::vector<std::uint8_t>& payload)
    {
        std::shared_lock lock(m_server_settings_mutex);
        std::string host = (this->server_settings.cast.host == "0.0.0.0") ? "127.0.0.1" : this->server_settings.cast.host;
        SendIpcMessage(host, std::to_string(this->server_settings.cast.ipc_port), ipc_id, payload);
    }
    void CServer::logExecution(std::uint16_t session_id, std::uint16_t order)
    {
        if (m_watchguard)
        {
            std::unique_lock lock(m_execution_guard_mutex);
            auto thread_id = std::hash<std::thread::id>{}(std::this_thread::get_id());
            auto& execution_vector = m_execution_info[thread_id];

            auto time_now = std::chrono::steady_clock::now();
            execution_vector.push_back({ session_id, order, time_now });
            BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_green,
                "Handler started: Thread ID: {}, Session ID: {}, Order: {}",
                thread_id, session_id, order, time_now.time_since_epoch());
        }
        
    }
    void CServer::clearExecution(std::uint16_t session_id, std::uint16_t order)
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
                    // Calculate elapsed time
                    auto elapsed_time = std::chrono::duration<double, std::milli>(
                        std::chrono::steady_clock::now() - it->start_time);

                    // Log the elapsed time
                    BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_green,
                        "Handler completed: Thread ID: {}, Session ID: {}, Order: {}, Elapsed Time: {:.3f}ms",
                        thread_id, session_id, order, elapsed_time.count());

                    // Remove the entry
                    execution_vector.erase(it);
                    break;
                }
            }

            // Clean up the thread entry if the vector becomes empty
            if (execution_vector.empty())
            {
                m_execution_info.erase(thread_id);
            }
        }
    }
    void CServer::watchdog(std::chrono::nanoseconds timeout)
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
                    BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::red,
                        "[Watchdog] Deadlock detected! Thread ID: {}, Session ID: {}, Order: {}, Elapsed Time: {:.3f}ms",
                        thread_id, info.session_id, info.order, elapsed_time.count());
                }
            }
        }
    }
    void CServer::startWatchdog(std::chrono::nanoseconds interval, std::chrono::nanoseconds timeout)
    {
        std::jthread([this, interval, timeout]()
        {
            while (true)
            {
                this->watchdog(timeout);
                std::this_thread::sleep_for(interval); // Sleep for the specified interval
            }
        }).detach();

        /*
        m_watchdog_timer.expires_after(interval);
        m_watchdog_timer.async_wait([this, interval, timeout](const asio::error_code& ec) 
        {
            if (!ec)
            {
                //asio::post(m_ioContext, [this, timeout]() {  });
                watchdog(timeout);
                startWatchdog(interval, timeout);
            }
            else
            {
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::red,
                    "[Watchdog] Timer error: {}", ec.message());
            }
        });
        */
    }
}

