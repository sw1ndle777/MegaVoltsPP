#include "CServer.h"
#include <numeric>
namespace NetEngine
{
    CServer::CServer() : m_ioContext(), m_socket(m_ioContext) {}
    CServer::~CServer() {}
    void CServer::Setup(const SServerSettings& settings)
    {
        m_ip_address = settings.ip;
        m_port = settings.port;
        m_useEncryption = settings.useEncryption;
        m_useMultithreaded = settings.useMultithreaded;
        m_concurrentThreads = settings.concurrent_threads;

        asio::error_code errorCode;
        asio::ip::tcp::resolver resolver(m_ioContext);
        asio::ip::tcp::endpoint endpoint = *resolver.resolve(m_ip_address, m_port, errorCode).begin();

        if (errorCode)
        {
            std::printf("CServer::Setup() - Failed to resolve endpoint: %s\n", errorCode.message().c_str());
            BaseLib::EventLog->Error("CServer::Setup() - Failed to resolve endpoint: %s", errorCode.message().c_str());
            return;
        }
        else
        {
            /*
            if (m_useMultithreaded && m_concurrentThreads != 1)
            {
                auto max_concurrent_threads = std::thread::hardware_concurrency();
                if (m_concurrentThreads == 0)
                    m_concurrentThreads = max_concurrent_threads;
                else if (m_concurrentThreads >= max_concurrent_threads)
                    m_concurrentThreads = max_concurrent_threads;

                m_threadPool = std::make_shared<asio::thread_pool>(m_concurrentThreads);
            }
            */
            m_available_session_ids.resize(65536);
            std::iota(m_available_session_ids.begin(), m_available_session_ids.end(), 0);

            m_acceptor = std::make_shared<asio::ip::tcp::acceptor>(m_ioContext);
            m_acceptor->open(endpoint.protocol());
            m_acceptor->set_option(asio::ip::tcp::acceptor::reuse_address(true));
            m_acceptor->bind(endpoint);
            m_acceptor->listen();
   
        }
    }
    void CServer::Run()
    {
        std::printf("CServer::Run() - Running server on: %s:%s\n", m_ip_address.c_str(), m_port.c_str());
        std::printf("CServer::Run() - m_useEncryption: %s\n", m_useEncryption ? "true" : "false");
        //if (m_useMultithreaded)
        //    std::printf("CServer::Run() - m_useMultithreaded: true\nCServer::Run() - m_concurrentThreads: %d out of %d\n", m_concurrentThreads, std::thread::hardware_concurrency());

        BaseLib::EventLog->Error("CServer::Run() - Running server on: %s:%s", m_ip_address.c_str(), m_port.c_str());
        BaseLib::EventLog->Error("CServer::Run() - m_useEncryption: %s", m_useEncryption ? "true" : "false");
       /* if (m_useMultithreaded)
        {
            BaseLib::EventLog->Error("CServer::Run() - m_useMultithreaded: true");
            BaseLib::EventLog->Error("CServer::Run() - m_concurrentThreads: %d out of %d", m_concurrentThreads, std::thread::hardware_concurrency());
        }*/
        //m_threadPool->join();
        AcceptSessions();
        while (true)
        {
            m_ioContext.run();
        }
    }

   
    
    void CServer::AcceptSessions()
    {
        /*
        if (m_useMultithreaded)
        {
            std::scoped_lock<std::shared_mutex> lock(m_acceptorMutex);
            asio::post(*this->m_threadPool, [this]()
                {
                    
                    m_acceptor->async_accept(m_socket, [this](std::error_code ec)
                        {
                            if (!ec)
                            {
                                CSession::SSessionSettings settings;

                                settings.verbose = false;
                                settings.useEncryption = m_useEncryption;
                                settings.callbacks.insert(m_callbacks.begin(), m_callbacks.end());

                                std::uint16_t session_id = 0;
                                if (GetNextAvailableSessionId(session_id))
                                {
                                    auto session = std::make_shared<CSession>(std::move(m_socket), settings, session_id);
                                    if (m_OnDisconnect) session->SetOnDisconnectCallback(m_OnDisconnect);
                                    if (m_OnConnect)  m_OnConnect(session);
                                    session->SetServer(GetShared());
                                    AddSession(session);
                                    std::scoped_lock<std::shared_mutex> session_lock(m_sessionMutex);
                                    asio::post(*this->m_threadPool, [session]() {session->Run(); });
                                }
                                else
                                {
                                    std::printf("CServer::AcceptSessions() - There's no available session id, session pool is full!\n");
                                    BaseLib::EventLog->Error("CServer::AcceptSessions() - There's no available session id, session pool is full!");
                                }
                            }
                            else
                            {
                                std::printf("CServer::AcceptSessions() - Failed to accept session: %s\n", ec.message().c_str());
                                BaseLib::EventLog->Error("CServer::AcceptSessions() - Failed to accept session: %s", ec.message().c_str());
                            }
            AcceptSessions();
                        });
                });
        }
        else
        {
            

        }*/
        m_acceptor->async_accept(m_socket, [this](std::error_code ec) {
            if (!ec)
            {
                CSession::SSessionSettings settings;

                settings.verbose = false;
                settings.useEncryption = m_useEncryption;
                settings.callbacks.insert(m_callbacks.begin(), m_callbacks.end());

                std::uint16_t session_id = 0;
                if (GetNextAvailableSessionId(session_id))
                {
                    auto session = std::make_shared<CSession>(std::move(m_socket), settings, session_id);
                    if (m_OnDisconnect) session->SetOnDisconnectCallback(m_OnDisconnect);
                    if (m_OnConnect)  m_OnConnect(session);
                    AddSession(session);
                    session->Run();
                }
                else
                {
                    std::printf("CServer::AcceptSessions() - There's no available session id, session pool is full!\n");
                    BaseLib::EventLog->Error("CServer::AcceptSessions() - There's no available session id, session pool is full!");
                }
            }
            else
            {
                std::printf("CServer::AcceptSessions() - Failed to accept session: %s\n", ec.message().c_str());
                BaseLib::EventLog->Error("CServer::AcceptSessions() - Failed to accept session: %s", ec.message().c_str());
            }
        //AcceptSessions();
            });
       
    }
    void CServer::AddSession(const std::shared_ptr<CSession>& session)
    {
        this->m_sessions[session->GetSessionId()] = session;
        this->m_available_session_ids[session->GetSessionId()] = -1;
    }
    void CServer::RemoveSession(std::uint16_t id)
    {
        this->m_sessions.erase(id);
        this->m_available_session_ids[id] = id;
    }
    bool CServer::GetNextAvailableSessionId(std::uint16_t& outId)
    {
        for (auto id : m_available_session_ids)
        {
            if (id != -1)
            {
                outId = id;
                return true;
            }    
        }
        return false;
    }
    std::shared_ptr<CSession> CServer::GetSession(std::uint16_t id)
    {
        return this->m_sessions[id];
    }
    void CServer::On(uint16_t id, std::function<void(SCallbackData&)> callback)
    {
        if (m_callbacks.find(id) != m_callbacks.end())
        {
            std::printf("CServer::On() - Callback with id %d already exists\n", id);
            BaseLib::EventLog->Error("CServer::On() - Callback with id %d already exists", id);

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
    bool CServer::IsMultiThreaded()
    {
        return this->m_useMultithreaded;
    }
}

