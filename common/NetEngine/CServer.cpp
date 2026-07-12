#include "CServer.h"
#include "BaseLib/Utility.h"
#include <fmt/color.h>
#include <numeric>
#include <utility>
#include <asio/ssl.hpp>
#include <openssl/ssl.h>

namespace NetEngine
{
	using namespace BaseLib;
    using enum fmt::color;

    namespace
    {
        [[nodiscard]] std::string MakePacketRateLimitOrderKey(const uint16_t order, const RateLimit::IdentityScope scope, const std::string_view identity)
        {
            return std::format("{}|{}|{}", order, std::to_underlying(scope), identity);
        }

        [[nodiscard]] std::string MakePacketRateLimitIdentityKey(const RateLimit::IdentityScope scope, const std::string_view identity)
        {
            return std::format("{}|{}", static_cast<uint32_t>(scope), identity);
        }

        [[nodiscard]] std::chrono::milliseconds RemainingPacketRateLimitDuration(const CServer::PacketRateLimitClock::time_point expires_at,
            const CServer::PacketRateLimitClock::time_point now)
        {
            if (expires_at <= now)
                return {};

            return std::chrono::duration_cast<std::chrono::milliseconds>(expires_at - now);
        }

        void PrunePacketRateLimitWindow(std::deque<CServer::PacketRateLimitClock::time_point>& timestamps,
            const CServer::PacketRateLimitClock::time_point now,
            const std::chrono::milliseconds window)
        {
            while (!timestamps.empty() && now - timestamps.front() >= window)
                timestamps.pop_front();
        }

        [[nodiscard]] bool IsPacketRateLimitSessionOrderKey(const std::string_view key, const std::string_view session_identity)
        {
            const auto first = key.find('|');
            if (first == std::string_view::npos) return false;
            const auto second = key.find('|', first + 1);
            if (second == std::string_view::npos) return false;

            return key.substr(first + 1, second - first - 1) == std::to_string(std::to_underlying(RateLimit::IdentityScope::Session)) &&
                key.substr(second + 1) == session_identity;
        }

        [[nodiscard]] bool IsPacketRateLimitSessionIdentityKey(const std::string_view key, const std::string_view session_identity)
        {
            const auto first = key.find('|');
            if (first == std::string_view::npos) return false;

            return key.substr(0, first) == std::to_string(std::to_underlying(RateLimit::IdentityScope::Session)) &&
                key.substr(first + 1) == session_identity;
        }
    }

    bool RateLimit::ActionContext::HasIdentity(const IdentityScope scope) const
    {
        return !IdentityValue(scope).empty();
    }

    std::string RateLimit::ActionContext::OrderName() const
    {
        if (const auto order_enum = magic_enum::enum_cast<EOrder>(order); order_enum.has_value())
        {
            const auto name = magic_enum::enum_name(*order_enum);
            if (!name.empty())
                return std::string(name);
        }

        return std::to_string(order);
    }

    std::string RateLimit::ActionContext::IdentityValue(const IdentityScope scope) const
    {
        switch (scope)
        {
        case IdentityScope::Session:
            return identity.sid ? std::to_string(identity.sid) : std::string{};
        case IdentityScope::Ip:
            return identity.ip;
        case IdentityScope::Hwid:
            return identity.hwid;
        case IdentityScope::Aid:
            return identity.aid > 0 ? std::to_string(identity.aid) : std::string{};
        default:
            return {};
        }
    }

    void RateLimit::ActionContext::ApplyOrderCooldown(const IdentityScope scope, const std::chrono::milliseconds duration) const
    {
        if (!server || duration.count() <= 0)
            return;

        if (auto value = IdentityValue(scope); !value.empty())
            server->ApplyPacketRateLimitCooldown(order, scope, std::move(value), duration);
    }

    void RateLimit::ActionContext::Blacklist(const IdentityScope scope, const std::chrono::milliseconds duration) const
    {
        if (!server || duration.count() <= 0)
            return;

        if (auto value = IdentityValue(scope); !value.empty())
            server->ApplyPacketRateLimitBlacklist(scope, std::move(value), duration);
    }

    uint32_t RateLimit::ActionContext::AddStrike(const IdentityScope scope, const std::chrono::milliseconds window) const
    {
        if (!server || window.count() <= 0)
            return 0;

        if (auto value = IdentityValue(scope); !value.empty())
            return server->AddPacketRateLimitStrike(order, scope, std::move(value), window);

        return 0;
    }

    void RateLimit::ActionContext::Disconnect() const
    {
        if (session)
            session->Disconnect();
    }

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
        m_ipcRole = DetectIpcRole(settings, servers_settings);
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
            
            
            try {


                m_acceptor = std::make_shared<asio::ip::tcp::acceptor>(m_ioContext);
                m_acceptor->open(endpoint.protocol());
                m_acceptor->set_option(asio::ip::tcp::acceptor::reuse_address(true));
                m_acceptor->bind(endpoint);
                m_acceptor->listen();

                m_ipc_acceptor = std::make_shared<asio::ip::tcp::acceptor>(m_ioContext);
                m_ipc_acceptor->open(ipc_endpoint.protocol());
                m_ipc_acceptor->set_option(asio::ip::tcp::acceptor::reuse_address(true));
                m_ipc_acceptor->bind(ipc_endpoint);
                m_ipc_acceptor->listen();
            }
            catch (const std::system_error& e)
            {
                // Handle system errors (e.g., mutex lock failures)
                DEBUGLOG(red,
                    "System error in callback for server::setup: {} (code: {})",
                    e.what(), e.code().value());
            }
            catch (const std::runtime_error& e)
            {
                // Handle runtime errors
                DEBUGLOG(red,
                    "Runtime error in callback for server::setup: {}", e.what());
            }
            catch (const std::logic_error& e)
            {
                // Handle logic errors
                DEBUGLOG(red,
                    "Logic error in callback for server::setup: {}", e.what());
            }
            catch (const std::exception& e)
            {
                // Catch other standard exceptions
                DEBUGLOG(red,
                    "Exception in callback for server::setup: {}", e.what());
            }
            catch (...)
            {
                // Catch non-standard exceptions
                DEBUGLOG(red,
                    "Unknown exception in callback for server::setup");
            }
   
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

        // Started unconditionally: the io_context-stall detector inside must run even when
        // watchguard (the per-handler deadlock scan) is disabled. StartHeartbeat() below
        // arms the liveness stamp the stall detector reads.
        startWatchdog(std::chrono::seconds(1), std::chrono::seconds(5));

        StartHeartbeat();


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
            StartPersistentIpcClients();
        }
        else
        {
            auto work = asio::make_work_guard(m_ioContext);
            AcceptSessions();
            AcceptIpcSessions(ipc_addresses);
            StartPersistentIpcClients();
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
                asio::error_code opt_ec;
                m_socket.set_option(asio::ip::tcp::no_delay(true), opt_ec);

                CSession::SSessionSettings settings;

                settings.verbose = m_verbose;
                settings.useEncryption = m_useEncryption;
                settings.callbacks = GetSharedCallbacks();
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
                bool is_ipc = ipc_addresses.contains(remote_ip);
                
                if (is_ipc)
                {
                    asio::error_code opt_ec;
                    m_ipcSocket.set_option(asio::ip::tcp::no_delay(true), opt_ec);

                    CSession::SSessionSettings settings;

                    settings.verbose = m_verbose;
                    settings.useEncryption = m_useEncryption;
                    settings.callbacks = GetSharedCallbacks();

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
    CServer::IpcRole CServer::DetectIpcRole(const SServerSettings& settings, const BaseLib::CSettings::ServerSettings& servers_settings) const
    {
        if (settings.ipc_port == std::to_string(servers_settings.front.ipc_port)) return IpcRole::Front;
        if (settings.ipc_port == std::to_string(servers_settings.main.ipc_port)) return IpcRole::Main;
        if (settings.ipc_port == std::to_string(servers_settings.cast.ipc_port)) return IpcRole::Cast;
        return IpcRole::Unknown;
    }
    CServer::PersistentIpcState& CServer::GetPersistentIpcState(PersistentIpcTarget target)
    {
        switch (target)
        {
        case PersistentIpcTarget::Front: return m_frontIpcState;
        case PersistentIpcTarget::Main: return m_mainIpcState;
        case PersistentIpcTarget::Cast: return m_castIpcState;
        }
        return m_mainIpcState;
    }
    std::pair<std::string, std::string> CServer::GetPersistentIpcEndpoint(PersistentIpcTarget target)
    {
        std::shared_lock lock(m_server_settings_mutex);
        auto normalize_host = [](const std::string& host)
        {
            return host == "0.0.0.0" ? std::string("127.0.0.1") : host;
        };

        switch (target)
        {
        case PersistentIpcTarget::Front:
            return { normalize_host(server_settings.front.host), std::to_string(server_settings.front.ipc_port) };
        case PersistentIpcTarget::Main:
            return { normalize_host(server_settings.main.host), std::to_string(server_settings.main.ipc_port) };
        case PersistentIpcTarget::Cast:
            return { normalize_host(server_settings.cast.host), std::to_string(server_settings.cast.ipc_port) };
        }

        return { "127.0.0.1", "0" };
    }
    const char* CServer::GetPersistentIpcTargetName(PersistentIpcTarget target) const
    {
        switch (target)
        {
        case PersistentIpcTarget::Front: return "front";
        case PersistentIpcTarget::Main: return "main";
        case PersistentIpcTarget::Cast: return "cast";
        }
        return "unknown";
    }
    bool CServer::TrySendPersistentIpc(PersistentIpcTarget target, const uint32_t ipc_id, const std::vector<uint8_t>& payload)
    {
        std::shared_ptr<CSession> session;
        {
            std::shared_lock lock(m_persistent_ipc_mutex);
            auto& state = GetPersistentIpcState(target);
            session = state.session;
        }

        if (!session || !session->IsOpen())
            return false;

        session->SendIpc(ipc_id, payload);
        return true;
    }
    void CServer::StartPersistentIpcClients()
    {
        switch (m_ipcRole)
        {
        case IpcRole::Front:
            EnsurePersistentIpcConnection(PersistentIpcTarget::Main);
            break;
        case IpcRole::Main:
            EnsurePersistentIpcConnection(PersistentIpcTarget::Front);
            EnsurePersistentIpcConnection(PersistentIpcTarget::Cast);
            break;
        case IpcRole::Cast:
            EnsurePersistentIpcConnection(PersistentIpcTarget::Main);
            break;
        case IpcRole::Unknown:
        default:
            break;
        }
    }
    void CServer::EnsurePersistentIpcConnection(PersistentIpcTarget target)
    {
        {
            std::unique_lock lock(m_persistent_ipc_mutex);
            auto& state = GetPersistentIpcState(target);
            if ((state.session && state.session->IsOpen()) || state.connecting)
                return;
            state.connecting = true;
            if (state.retry_timer)
                state.retry_timer->cancel();
        }

        auto [host, port] = GetPersistentIpcEndpoint(target);
        auto socket = std::make_shared<asio::ip::tcp::socket>(m_ioContext);
        auto resolver = std::make_shared<asio::ip::tcp::resolver>(m_ioContext);

        resolver->async_resolve(host, port,
            [this, target, host, port, socket, resolver](const asio::error_code& ec, asio::ip::tcp::resolver::results_type endpoints)
            {
                if (ec)
                {
                    {
                        std::unique_lock lock(m_persistent_ipc_mutex);
                        GetPersistentIpcState(target).connecting = false;
                    }
                    DEBUGLOG(yellow, "persistent ipc resolve to {} failed: {}", GetPersistentIpcTargetName(target), ec.message().c_str());
                    SchedulePersistentIpcReconnect(target);
                    return;
                }

                asio::async_connect(*socket, endpoints,
                    [this, target, host, port, socket](const asio::error_code& ec, const asio::ip::tcp::endpoint&)
                    {
                        if (ec)
                        {
                            {
                                std::unique_lock lock(m_persistent_ipc_mutex);
                                GetPersistentIpcState(target).connecting = false;
                            }
                            DEBUGLOG(yellow, "persistent ipc connect to {} ({}:{}) failed: {}", GetPersistentIpcTargetName(target), host, port, ec.message().c_str());
                            SchedulePersistentIpcReconnect(target);
                            return;
                        }

                        asio::error_code opt_ec;
                        socket->set_option(asio::ip::tcp::no_delay(true), opt_ec);

                        CSession::SSessionSettings settings{};
                        settings.verbose = m_verbose;
                        settings.useEncryption = false;
                        settings.callbacks = GetSharedCallbacks();

                        auto session = CSession::Create(std::move(*socket), m_ioContext, this, settings, 0);
                        if (m_OnIpcMessage)
                            session->SetOnIpcMessageCallback(m_OnIpcMessage);

                        std::weak_ptr<CSession> weak_session = session;
                        session->SetOnDisconnectCallback([this, target, weak_session](std::shared_ptr<CSession>)
                        {
                            {
                                std::unique_lock lock(m_persistent_ipc_mutex);
                                auto& state = GetPersistentIpcState(target);
                                if (state.session == weak_session.lock())
                                    state.session.reset();
                                state.connecting = false;
                            }
                            DEBUGLOG(yellow, "persistent ipc to {} disconnected", GetPersistentIpcTargetName(target));
                            SchedulePersistentIpcReconnect(target);
                        });

                        {
                            std::unique_lock lock(m_persistent_ipc_mutex);
                            auto& state = GetPersistentIpcState(target);
                            state.session = session;
                            state.connecting = false;
                            if (state.retry_timer)
                                state.retry_timer->cancel();
                        }

                        session->DoReadIpc();
                        DEBUGLOG(dark_cyan, "persistent ipc connected to {} ({}:{})", GetPersistentIpcTargetName(target), host, port);
                    });
            });
    }
    void CServer::SchedulePersistentIpcReconnect(PersistentIpcTarget target, std::chrono::milliseconds delay)
    {
        std::shared_ptr<asio::steady_timer> timer;
        {
            std::unique_lock lock(m_persistent_ipc_mutex);
            auto& state = GetPersistentIpcState(target);
            if (!state.retry_timer)
                state.retry_timer = std::make_shared<asio::steady_timer>(m_ioContext);
            timer = state.retry_timer;
            timer->expires_after(delay);
        }

        timer->async_wait([this, target, timer](const asio::error_code& ec)
        {
            if (!ec)
                EnsurePersistentIpcConnection(target);
        });
    }

    bool CServer::AddSession(const std::shared_ptr<CSession>& session)
    {
        std::unique_lock lock(m_sessions_mutex);
        const auto& id = session->GetSessionId();
        DEBUGLOG(dark_cyan, "added sid=({})", id);
        if (id == 0 || m_sessions.contains(id))
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
        if (id == 0 || it == m_sessions.end())
        {
            DEBUGLOG(red, "could not find sid=({})", id);
            return;
        }

        m_sessions.erase(it);
        m_sessionIdGenerator.free(id);
        lock.unlock();
        ClearPacketRateLimitSessionState(id);
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
    void CServer::OnIpcMessage(std::function<void(std::shared_ptr<CSession>, const uint32_t& msg_id, const uint32_t& msg_size, const std::vector<uint8_t>&)> callback)
    {
        this->m_OnIpcMessage = callback;
    }
    std::shared_ptr<const CallbackMap> CServer::GetSharedCallbacks()
    {
        auto snap = m_shared_callbacks.load(std::memory_order_acquire);
        if (!snap)
        {
            snap = std::make_shared<const CallbackMap>(m_callbacks);
            m_shared_callbacks.store(snap, std::memory_order_release);
        }
        return snap;
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
        if (TrySendPersistentIpc(PersistentIpcTarget::Front, ipc_id, payload))
            return;
        EnsurePersistentIpcConnection(PersistentIpcTarget::Front);
        auto [host, port] = GetPersistentIpcEndpoint(PersistentIpcTarget::Front);
        SendIpcMessage(host, port, ipc_id, payload);
    }
    void CServer::SendMainIpc(const uint32_t ipc_id, const std::vector<uint8_t>& payload)
    {
        if (TrySendPersistentIpc(PersistentIpcTarget::Main, ipc_id, payload))
            return;
        EnsurePersistentIpcConnection(PersistentIpcTarget::Main);
        auto [host, port] = GetPersistentIpcEndpoint(PersistentIpcTarget::Main);
        SendIpcMessage(host, port, ipc_id, payload);
    }
    void CServer::SendCastIpc(const uint32_t ipc_id, const std::vector<uint8_t>& payload)
    {
        if (TrySendPersistentIpc(PersistentIpcTarget::Cast, ipc_id, payload))
            return;
        EnsurePersistentIpcConnection(PersistentIpcTarget::Cast);
        auto [host, port] = GetPersistentIpcEndpoint(PersistentIpcTarget::Cast);
        SendIpcMessage(host, port, ipc_id, payload);
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
    void CServer::StartHeartbeat()
    {
        {
            std::shared_lock server_settings_lock(m_server_settings_mutex);
            // Prefer this server's own heartbeat block (front/main/cast); an empty
            // per-server url falls back to the global "heartbeat" block.
            const BaseLib::CSettings::HeartbeatSettings* hb = nullptr;
            switch (m_ipcRole)
            {
            case IpcRole::Front: hb = &this->server_settings.front.heartbeat; break;
            case IpcRole::Main:  hb = &this->server_settings.main.heartbeat;  break;
            case IpcRole::Cast:  hb = &this->server_settings.cast.heartbeat;  break;
            default:             break;
            }
            const auto& src = (hb && !hb->url.empty()) ? *hb : this->server_settings.heartbeat;
            m_heartbeatUrl = src.url;
            m_heartbeatIntervalSec = src.interval_sec ? src.interval_sec : 30;
        }

        // The liveness probe runs regardless so StampAlive() is meaningful even when the
        // beacon is disabled, but there is no point spending cycles if nothing reads it.
        if (m_heartbeatUrl.empty())
        {
            DEBUGLOG(dark_cyan, "heartbeat disabled (empty url)");
            return;
        }

        const auto parts = Utility::ParseUrl(m_heartbeatUrl);
        if (!parts || !parts->https)
        {
            DEBUGLOG(red, "heartbeat url must be a valid https:// url, got: ({}) - disabling", m_heartbeatUrl.c_str());
            m_heartbeatUrl.clear();
            return;
        }

        m_heartbeatSsl = std::make_shared<asio::ssl::context>(asio::ssl::context::tls_client);
        m_heartbeatSsl->set_verify_mode(asio::ssl::verify_none); // see HttpsGetSync note

        StampAlive(); // seed so the first beat isn't suppressed as "stale"
        ArmLivenessProbe();
        ScheduleHeartbeat();
        DEBUGLOG(dark_cyan, "heartbeat enabled: every ({}s) to host ({})", m_heartbeatIntervalSec, parts->host.c_str());
    }

    void CServer::ArmLivenessProbe()
    {
        // A self-rearming 1s job ON the io_context. If every worker is wedged (deadlock,
        // runaway handler, etc.) this callback never runs, the stamp goes stale, and the
        // heartbeat sender stops beating - which is exactly what we want a hang to do.
        if (!m_livenessTimer)
            m_livenessTimer = std::make_shared<asio::steady_timer>(m_ioContext);

        m_livenessTimer->expires_after(std::chrono::seconds(1));
        m_livenessTimer->async_wait([this](const asio::error_code& ec)
        {
            if (ec) return;
            StampAlive();
            ArmLivenessProbe();
        });
    }

    void CServer::ScheduleHeartbeat()
    {
        if (!m_heartbeatTimer)
            m_heartbeatTimer = std::make_shared<asio::steady_timer>(m_ioContext);

        m_heartbeatTimer->expires_after(std::chrono::seconds(m_heartbeatIntervalSec));
        m_heartbeatTimer->async_wait([this](const asio::error_code& ec)
        {
            if (ec) return;

            const uint64_t now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now().time_since_epoch()).count();
            const uint64_t last = m_lastAliveMs.load(std::memory_order_acquire);

            // Only beat if the loop ticked recently. If it's stale we stay silent and let
            // BetterStack's grace period flip us down - a blind timer would lie "alive".
            if (last != 0 && now_ms - last <= kHeartbeatStaleMs)
                SendHeartbeatBeat();
            else
                DEBUGLOG(red, "heartbeat suppressed: loop stale ({}ms since last tick)", last ? (now_ms - last) : 0);

            ScheduleHeartbeat();
        });
    }

    void CServer::SendHeartbeatBeat()
    {
        // Async HTTPS GET, same fire-and-forget shape as WebsitePost (response ignored).
        try
        {
            const auto parts = Utility::ParseUrl(m_heartbeatUrl);
            if (!parts || !m_heartbeatSsl) return;

            const std::string host = parts->host;
            const std::string port = parts->port;
            const std::string target = parts->target;

            auto stream = std::make_shared<asio::ssl::stream<asio::ip::tcp::socket>>(m_ioContext, *m_heartbeatSsl);
            if (!SSL_set_tlsext_host_name(stream->native_handle(), host.c_str())) // SNI
            {
                DEBUGLOG(red, "heartbeat: failed to set SNI host");
                return;
            }

            auto request = std::make_shared<std::string>(
                "GET " + target + " HTTP/1.1\r\n"
                "Host: " + host + "\r\n"
                "User-Agent: MegaVoltsPP-heartbeat\r\n"
                "Connection: close\r\n\r\n");

            // Every step of the beat is fire-and-forget ON the io_context, so every step must
            // be asynchronous. A synchronous resolver.resolve() here would block a worker
            // thread on DNS; and because ScheduleHeartbeat() only re-arms AFTER this function
            // returns, a hung resolve would silently kill the heartbeat chain (no further
            // beats, BetterStack flips down) without crashing. async_resolve avoids that.
            auto resolver = std::make_shared<asio::ip::tcp::resolver>(m_ioContext);
            auto timer = std::make_shared<asio::steady_timer>(m_ioContext, std::chrono::seconds(5));
            timer->async_wait([stream, timer, resolver](const asio::error_code& ec)
            {
                if (!ec)
                {
                    DEBUGLOG(red, "heartbeat request timed out");
                    asio::error_code ig;
                    stream->lowest_layer().close(ig);
                    resolver->cancel();
                }
            });

            resolver->async_resolve(host, port,
                [stream, request, timer, resolver, host](const asio::error_code& ec, asio::ip::tcp::resolver::results_type endpoints)
            {
                if (ec)
                {
                    DEBUGLOG(red, "heartbeat resolve failed: {}", ec.message().c_str());
                    timer->cancel();
                    return;
                }
                asio::async_connect(stream->lowest_layer(), endpoints,
                    [stream, request, timer, host](const asio::error_code& ec, const asio::ip::tcp::endpoint&)
                {
                if (ec)
                {
                    DEBUGLOG(red, "heartbeat connect failed: {}", ec.message().c_str());
                    timer->cancel();
                    return;
                }
                stream->async_handshake(asio::ssl::stream_base::client,
                    [stream, request, timer](const asio::error_code& ec)
                {
                    if (ec)
                    {
                        DEBUGLOG(red, "heartbeat tls handshake failed: {}", ec.message().c_str());
                        timer->cancel();
                        return;
                    }
                    asio::async_write(*stream, asio::buffer(*request),
                        [stream, request, timer](const asio::error_code& ec, size_t /*n*/)
                    {
                        if (ec)
                            DEBUGLOG(red, "heartbeat write failed: {}", ec.message().c_str());
                        timer->cancel();
                        stream->async_shutdown([stream](const asio::error_code&) {});
                    });
                });
            });
            });
        }
        catch (const std::exception& e)
        {
            DEBUGLOG(red, "Exception in SendHeartbeatBeat: {}", e.what());
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
        ClearPacketRateLimitSessionState(old_sid);
        ClearPacketRateLimitSessionState(new_sid);
        return true;
    }
    bool CServer::ShouldProcessPacket(const uint16_t order,
        SCallbackData& callback,
        const RateLimit::IdentitySnapshot& identity,
        const std::optional<RateLimit::Rule>& rule)
    {
        if (!rule.has_value() || !rule->enabled)
            return true;

        const auto bucket_scope = rule->bucket_scope_resolver ? rule->bucket_scope_resolver(callback, identity) : rule->bucket_scope;
        const auto max_packets = rule->max_packets_resolver ? rule->max_packets_resolver(callback, identity) : rule->max_packets;
        const auto window = rule->window_resolver ? rule->window_resolver(callback, identity) : rule->window;

        RateLimit::ActionContext ctx{
            .server = this,
            .callback = &callback,
            .session = callback.session.get(),
            .event = RateLimit::Event::LimitExceeded,
            .order = order,
            .identity = identity,
            .retry_after = {},
            .packet_count = 0,
        };

        const auto now = PacketRateLimitClock::now();
        std::optional<std::pair<RateLimit::Event, std::chrono::milliseconds>> rejection{};
        bool limit_triggered = false;
        uint32_t packet_count = 0;
        std::chrono::milliseconds retry_after{};

        {
            std::scoped_lock lock(m_packet_rate_limit_mutex);

            const auto try_reject = [&](const RateLimit::Event event, auto& entries, const std::string& key)
            {
                auto it = entries.find(key);
                if (it == entries.end())
                    return false;

                const auto remaining = RemainingPacketRateLimitDuration(it->second, now);
                if (remaining.count() <= 0)
                {
                    entries.erase(it);
                    return false;
                }

                rejection = std::make_pair(event, remaining);
                return true;
            };

            constexpr RateLimit::IdentityScope scopes[] = {
                RateLimit::IdentityScope::Session,
                RateLimit::IdentityScope::Ip,
                RateLimit::IdentityScope::Hwid,
                RateLimit::IdentityScope::Aid,
            };

            for (const auto scope : scopes)
            {
                const auto value = ctx.IdentityValue(scope);
                if (value.empty())
                    continue;

                if (try_reject(RateLimit::Event::BlacklistActive, m_packet_identity_blacklists, MakePacketRateLimitIdentityKey(scope, value)))
                    break;
            }

            if (!rejection.has_value())
            {
                for (const auto scope : scopes)
                {
                    const auto value = ctx.IdentityValue(scope);
                    if (value.empty())
                        continue;

                    if (try_reject(RateLimit::Event::CooldownActive, m_packet_order_cooldowns, MakePacketRateLimitOrderKey(order, scope, value)))
                        break;
                }
            }

            if (!rejection.has_value() && max_packets > 0 && window.count() > 0)
            {
                const auto bucket_value = ctx.IdentityValue(bucket_scope);
                if (!bucket_value.empty())
                {
                    const auto window_key = MakePacketRateLimitOrderKey(order, bucket_scope, bucket_value);
                    auto& timestamps = m_packet_rate_limit_windows[window_key];
                    PrunePacketRateLimitWindow(timestamps, now, window);
                    timestamps.push_back(now);
                    packet_count = static_cast<uint32_t>(timestamps.size());
                    if (packet_count > max_packets)
                    {
                        limit_triggered = true;
                        retry_after = RemainingPacketRateLimitDuration(timestamps.front() + window, now);
                    }
                    if (timestamps.size() == 1 && bucket_scope == RateLimit::IdentityScope::Session && identity.sid)
                        m_session_rate_limit_keys[identity.sid].push_back(window_key);
                }
            }
        }

        if (rejection.has_value())
        {
            ctx.event = rejection->first;
            ctx.retry_after = rejection->second;
            if (rule->on_rejected)
                rule->on_rejected(ctx);
            return false;
        }

        if (!limit_triggered)
            return true;

        ctx.event = RateLimit::Event::LimitExceeded;
        ctx.retry_after = retry_after;
        ctx.packet_count = packet_count;
        if (rule->on_limit)
            rule->on_limit(ctx);
        return false;
    }
    void CServer::ApplyPacketRateLimitCooldown(const uint16_t order,
        const RateLimit::IdentityScope scope,
        std::string identity,
        const std::chrono::milliseconds duration)
    {
        if (identity.empty() || duration.count() <= 0)
            return;

        const auto key = MakePacketRateLimitOrderKey(order, scope, identity);
        const auto expires_at = PacketRateLimitClock::now() + duration;

        std::scoped_lock lock(m_packet_rate_limit_mutex);
        const bool is_new = !m_packet_order_cooldowns.contains(key);
        auto& current = m_packet_order_cooldowns[key];
        if (current < expires_at)
            current = expires_at;
        if (is_new && scope == RateLimit::IdentityScope::Session && !identity.empty())
            m_session_rate_limit_keys[static_cast<uint16_t>(std::stoul(identity))].push_back(key);
    }
    void CServer::ApplyPacketRateLimitBlacklist(const RateLimit::IdentityScope scope,
        std::string identity,
        const std::chrono::milliseconds duration)
    {
        if (identity.empty() || duration.count() <= 0)
            return;

        const auto key = MakePacketRateLimitIdentityKey(scope, identity);
        const auto expires_at = PacketRateLimitClock::now() + duration;

        std::scoped_lock lock(m_packet_rate_limit_mutex);
        const bool is_new = !m_packet_identity_blacklists.contains(key);
        auto& current = m_packet_identity_blacklists[key];
        if (current < expires_at)
            current = expires_at;
        if (is_new && scope == RateLimit::IdentityScope::Session && !identity.empty())
            m_session_rate_limit_keys[static_cast<uint16_t>(std::stoul(identity))].push_back(key);
    }
    uint32_t CServer::AddPacketRateLimitStrike(const uint16_t order,
        const RateLimit::IdentityScope scope,
        std::string identity,
        const std::chrono::milliseconds window)
    {
        if (identity.empty() || window.count() <= 0)
            return 0;

        const auto key = MakePacketRateLimitOrderKey(order, scope, identity);
        const auto now = PacketRateLimitClock::now();

        std::scoped_lock lock(m_packet_rate_limit_mutex);
        auto& strikes = m_packet_rate_limit_strikes[key];
        const bool was_empty = strikes.empty();
        PrunePacketRateLimitWindow(strikes, now, window);
        strikes.push_back(now);
        if (was_empty && scope == RateLimit::IdentityScope::Session && !identity.empty())
            m_session_rate_limit_keys[static_cast<uint16_t>(std::stoul(identity))].push_back(key);
        return static_cast<uint32_t>(strikes.size());
    }
    void CServer::ClearPacketRateLimitSessionState(const uint16_t sid)
    {
        if (!sid)
            return;

        std::scoped_lock lock(m_packet_rate_limit_mutex);
        auto it = m_session_rate_limit_keys.find(sid);
        if (it == m_session_rate_limit_keys.end())
            return;

        for (const auto& key : it->second)
        {
            m_packet_rate_limit_windows.erase(key);
            m_packet_rate_limit_strikes.erase(key);
            m_packet_order_cooldowns.erase(key);
            m_packet_identity_blacklists.erase(key);
        }
        m_session_rate_limit_keys.erase(it);
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
            // TEMP: silenced to isolate combat logs (restore by uncommenting).
            //if (order != 281 && order != 71 && order != 322 && order != 72 && order != 257 && order != 282 && order != 77) DEBUGLOG(dark_green,
            //    "Handler started: Thread ID: {}, sid={}, Order: {}",
            //    thread_id, session_id, order);
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

                    // TEMP: silenced to isolate combat logs (restore by uncommenting).
                    //if (order != 281 && order != 71 && order != 322 && order != 72 && order != 257 && order != 282 && order != 77) DEBUGLOG(dark_green,
                    //    "Handler completed: Thread ID: {}, sid={}, Order: {}, Elapsed Time: {:.3f}ms",
                    //    thread_id, session_id, order, elapsed_time.count());

                    execution_vector.erase(it);
                    break;
                }
            }

            if (execution_vector.empty())
                m_execution_info.erase(thread_id);
        }
    }
    void CServer::watchdog(std::stop_token stoken, std::chrono::nanoseconds interval, std::chrono::nanoseconds timeout)
    {
        // Runs on a dedicated thread, independent of the io_context and the
        // logger/database pools, so it still fires when every IO/worker thread
        // is blocked by a deadlock (the case we actually need it for).
        std::unique_lock cv_lock(m_watchdog_mutex);

        // Edge-triggered so a wedge logs once (on entry) and once on recovery,
        // instead of spamming every interval while the loop stays stuck.
        bool io_stalled = false;
        const uint64_t stall_threshold_ms = std::max<uint64_t>(
            kHeartbeatStaleMs,
            static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(timeout).count()));

        while (!stoken.stop_requested())
        {
            // Sleep for `interval`, waking immediately on a stop request.
            m_watchdog_cv.wait_for(cv_lock, stoken, interval, [] { return false; });
            if (stoken.stop_requested())
                break;

            cv_lock.unlock();

            // (1) io_context liveness. The per-second probe (ArmLivenessProbe) stamps
            // m_lastAliveMs from ON the loop; if every worker is wedged (deadlock, a
            // blocking handler, etc.) the stamp stops advancing. This is the failure that
            // silently stops the heartbeat and takes BetterStack down without a crash, so
            // we surface it loudly here on this independent thread. Skipped while the stamp
            // is still 0 (heartbeat/liveness disabled -> nothing maintaining it).
            {
                const uint64_t last = m_lastAliveMs.load(std::memory_order_acquire);
                if (last != 0)
                {
                    const uint64_t now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::steady_clock::now().time_since_epoch()).count();
                    const uint64_t age = now_ms > last ? now_ms - last : 0;
                    if (!io_stalled && age > stall_threshold_ms)
                    {
                        io_stalled = true;
                        DEBUGLOG(red,
                            "[Watchdog] io_context STALLED - no loop tick for ({}ms). Event loop "
                            "wedged (deadlock or blocking handler); heartbeat will stop beating.", age);
                    }
                    else if (io_stalled && age <= stall_threshold_ms)
                    {
                        io_stalled = false;
                        DEBUGLOG(dark_cyan, "[Watchdog] io_context recovered - loop ticking again");
                    }
                }
            }

            // (2) per-handler deadlock scan. Only populated/meaningful when watchguard is on
            // (logExecution/clearExecution are no-ops otherwise), so gate the scan to match.
            if (m_watchguard)
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
            }
            cv_lock.lock();
        }
    }
    void CServer::startWatchdog(std::chrono::nanoseconds interval, std::chrono::nanoseconds timeout)
    {
        // Always run the watchdog thread: even with watchguard off (so the per-handler
        // deadlock scan is inert), it still performs the io_context-stall check, which is
        // the detector for a full event-loop wedge. That check is nearly free (one atomic
        // load per interval) and is exactly what was missing when the loop hung silently.
        m_watchdog_thread = std::jthread([this, interval, timeout](std::stop_token stoken)
        {
            this->watchdog(stoken, interval, timeout);
        });
    }
}

