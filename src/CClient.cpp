#include "CClient.h"

namespace NetEngine
{
    CClient::CClient()
        : m_socket(m_ioContext)
    {
    }

    CClient::~CClient()
    {
    }

    void CClient::Setup(SClientSettings settings)
    {
        m_ip_address = settings.ip;
        m_port = settings.port;
        m_useEncryption = settings.useEncryption;
    }

    void CClient::Connect()
    {
        asio::error_code errorCode;
        asio::ip::tcp::resolver resolver(m_ioContext);
        auto endpoints = resolver.resolve(m_ip_address, m_port, errorCode);

        if (errorCode)
        {
            std::printf("CClient::Connect() - Failed to resolve endpoint: %s\n", errorCode.message().c_str());
            BaseLib::EventLog->Error("CClient::Connect() - Failed to resolve endpoint: %s", errorCode.message().c_str());
            return;
        }

        CSession::SSessionSettings settings;

        settings.verbose = false;
        settings.useEncryption = m_useEncryption;
        settings.callbacks.insert(m_callbacks.begin(), m_callbacks.end());

        asio::connect(m_socket, endpoints, errorCode);
        std::make_shared<CSession>(std::move(m_socket), settings, 0)->Run();

        if (errorCode)
        {
            std::printf("CClient::Connect() - Failed to connect: %s\n", errorCode.message().c_str());
            BaseLib::EventLog->Error("CClient::Connect() - Failed to connect: %s", errorCode.message().c_str());
            return;
        }

        std::printf("CClient::Connect() - Connected to %s:%s\n", m_ip_address.c_str(), m_port.c_str());
        BaseLib::EventLog->Info("CClient::Connect() - Connected to %s:%s", m_ip_address.c_str(), m_port.c_str());
    }

    void CClient::Update()
    {
        asio::error_code errorCode;
        m_ioContext.poll(errorCode);
    }

    void CClient::On(uint16_t id, std::function<void(SCallbackData&)> callback)
    {
        if (m_callbacks.find(id) != m_callbacks.end())
        {
            std::printf("CClient::On() - Callback with id %d already exists\n", id);
            BaseLib::EventLog->Error("CClient::On() - Callback with id %d already exists", id);

            throw std::runtime_error("Callback already exists");
        }

        m_callbacks[id] = callback;
    }
}