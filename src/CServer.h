#pragma once
#ifndef CSERVER_H
#define CSERVER_H

#include <iostream>
#include <vector>
#include <map>
#include <unordered_map>

#include <asio.hpp>

#include "Constants.h"
#include "CSession.h"
#include "CLog.h"

namespace NetEngine
{
    struct SCallbackData;
    class CServer : public std::enable_shared_from_this<CServer>
    {
    public:
        struct SServerSettings
        {
            std::string ip;
            std::string port;
            bool useEncryption;
            bool useMultithreaded;
            std::uint32_t concurrent_threads;
            SServerSettings(std::string ip, std::string port, bool useEncryption, bool useMultithreaded, std::uint32_t concurrent_threads) : ip(ip), port(port), useEncryption(useEncryption), useMultithreaded(useMultithreaded), concurrent_threads(concurrent_threads) {}
        };

    public:
        CServer();
        ~CServer();

        void Setup(const SServerSettings& settings);
        void Run();
        void AcceptSessions();
        void AddSession(const std::shared_ptr<CSession>& session);
        void RemoveSession(std::uint16_t id);
        bool GetNextAvailableSessionId(std::uint16_t& outId);
        std::shared_ptr<CSession> GetSession(std::uint16_t id);
        void On(std::uint16_t id, std::function<void(SCallbackData&)> callback);
        void OnNewSession(std::function<void(std::shared_ptr<CSession>)> callback);
        void OnSessionDisconnected(std::function<void(std::shared_ptr<CSession>)> callback);
        bool IsMultiThreaded();
        std::shared_ptr<CServer> GetShared() {
            return shared_from_this();
        }
        std::shared_ptr<asio::thread_pool> GetThreadPool() {
            return m_threadPool;
        }
        std::shared_mutex& GetThreadPoolMutex(){
            return m_threadPoolMutex;
        }
        std::shared_mutex& GetSessionMutex() {
            return m_sessionMutex;
        }
        std::shared_mutex& GetAcceptorMutex() {
            return m_acceptorMutex;
        }
    private:
        std::map<std::uint16_t, std::function<void(SCallbackData&)>> m_callbacks;
        std::unordered_map<std::uint16_t, std::shared_ptr<CSession>> m_sessions;
        std::vector<std::uint16_t> m_available_session_ids;
        std::shared_ptr<asio::ip::tcp::acceptor> m_acceptor;
        asio::io_context m_ioContext;
        std::shared_ptr<asio::thread_pool> m_threadPool;
        std::shared_mutex m_sessionMutex;
        std::shared_mutex m_threadPoolMutex;
        std::shared_mutex m_acceptorMutex;
        asio::ip::tcp::endpoint m_endpoint;
        asio::ip::tcp::socket m_socket;
        std::string m_ip_address;
        std::string m_port;
        bool m_useEncryption = false;
        bool m_useMultithreaded = false;
        std::uint32_t m_concurrentThreads = 1;
        std::uint32_t m_availableConcurrentThreads = std::thread::hardware_concurrency();
        std::function<void(std::shared_ptr<CSession>)> m_OnDisconnect;
        std::function<void(std::shared_ptr<CSession>)> m_OnConnect;
    };
}

#endif