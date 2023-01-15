#pragma once
#ifndef CCLIENT_H
#define CCLIENT_H

#include <iostream>
#include <vector>
#include <map>

#include <asio.hpp>

#include "Constants.h"
#include "CSession.h"
#include "CLog.h"

namespace NetEngine
{
    struct SCallbackData;
    class CClient
    {
    public:
        struct SClientSettings
        {
            std::string ip;
            std::string port;
            bool useEncryption;
        };

    public:
        CClient();
        ~CClient();

        void Setup(SClientSettings settings);
        void Connect();
        void Update();

        void On(uint16_t id, std::function<void(SCallbackData&)> callback);

    private:
        std::map<uint16_t, std::function<void(SCallbackData&)>> m_callbacks;

        asio::io_context m_ioContext;
        asio::ip::tcp::socket m_socket;

        std::string m_ip_address;
        std::string m_port;

        bool m_useEncryption = false;
    };
}

#endif