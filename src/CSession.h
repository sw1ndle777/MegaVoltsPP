#pragma once
#ifndef CSESSION_H
#define CSESSION_H

#include <iostream>
#include <vector>
#include <map>
#include <atomic>
#include <queue>
#include <format>
#include <asio.hpp>

#include "CMessage.h"
#include "CClient.h"
#include "CServer.h"
#include "CCrypt.h"
#include "CLog.h"

#include "Constants.h"
#include "BaseProtocol.h"
#include "PacketStruct.h"
#include "PacketData.h"

namespace NetEngine
{
    class CClient;
    class CServer;
    class CMessage;

    struct SCallbackData;
    class CSession : public std::enable_shared_from_this<CSession>
    {
    public:
        struct SSessionSettings
        {
            bool verbose;
            bool useEncryption;

            std::map<uint16_t, std::function<void(SCallbackData&)>> callbacks;
        };

    public:
        friend class CClient;
        friend class CServer;

        CSession(asio::ip::tcp::socket&& socket, SSessionSettings settings, std::uint16_t session_id);
        ~CSession();

        void Disconnect();
        void Send(CMessage& message);

        void SetEncryptionKey(int32_t key);
        void SetSessionId(uint16_t id);
        void SetOnDisconnectCallback(std::function<void(std::shared_ptr<CSession>)> callback);
        void SetServer(std::shared_ptr<CServer> server);
        int32_t GetEncryptionKey();
        uint16_t GetSessionId();
        void DoRead();
        void DoWrite();
    private:
        
        void onPacket(Protocols::STcpPacketHeader& header, std::vector<uint8_t>& data);

    private:
        std::map<uint16_t, std::function<void(SCallbackData&)>> m_callbacks;
        std::queue<std::vector<std::uint8_t>> m_SendQueue;
        std::mutex SendMtx;
        std::atomic_bool m_InSend;
        std::array<uint8_t, 1024> m_buffer{};
        std::vector<uint8_t> m_reader{};
        asio::ip::tcp::socket m_socket;
        std::shared_ptr<CServer> m_server;
        bool m_verbose = true;
        bool m_useEncryption = false;
        int32_t m_encryptionKey = -1;
        uint16_t m_sessionId = 0;
        std::function<void(std::shared_ptr<CSession>)> m_on_disconnect_callback;
        
    };

}

#endif