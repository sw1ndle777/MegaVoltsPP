#include "CSession.h"

namespace NetEngine
{
    CSession::CSession(asio::ip::tcp::socket&& socket, SSessionSettings settings, std::uint16_t session_id = 0)
        : m_socket(std::move(socket))
    {
        m_verbose = settings.verbose;
        m_useEncryption = settings.useEncryption;
        m_callbacks = settings.callbacks;

        SetSessionId(session_id);
        if (m_useEncryption)
        {
            SetEncryptionKey(0);
        }

        asio::error_code errorCode;
        auto endPoint = m_socket.remote_endpoint(errorCode);

        if (errorCode)
        {
            std::printf("CSession::CSession() - Failed to get remote endpoint: %s\n", errorCode.message().c_str());
            BaseLib::EventLog->Error("CSession::CSession() - Failed to get remote endpoint: %s", errorCode.message().c_str());
            return;
        }
    }

    CSession::~CSession()
    {
        Disconnect();
    }

    void CSession::Disconnect()
    {
        if (!m_socket.is_open())
        {
            return;
        }

        asio::error_code errorCode;
        auto endPoint = m_socket.remote_endpoint(errorCode);

        std::printf("CSession::Close() - Connection closed from %s:%d\n", endPoint.address().to_string().c_str(), endPoint.port());
        BaseLib::EventLog->Info("CSession::Close() - Connection closed from  %s:%d", endPoint.address().to_string().data(), endPoint.port());

        m_socket.shutdown(asio::ip::tcp::socket::shutdown_both, errorCode);
        m_socket.close(errorCode);
        if (m_on_disconnect_callback) m_on_disconnect_callback(shared_from_this());
    }
    void CSession::SetOnDisconnectCallback(std::function<void(std::shared_ptr<CSession>)> callback)
    {
        m_on_disconnect_callback = callback;
    }
    void CSession::SetServer(std::shared_ptr<CServer> server)
    {
        m_server = server;
    }

    void CSession::Send(CMessage& message)
    {
        std::unique_lock ul(SendMtx);
        if (!m_socket.is_open())
            return;

        auto data = message.GenerateMessage();
        std::vector<uint8_t> data_vec(&data[0], &data[message.GetFullSize()]);
        m_SendQueue.push(data_vec);
        if (m_verbose)
        {
            int32_t encryptionKey = m_useEncryption ? m_encryptionKey : -1;
            CMessage packetMessage = CMessage(reinterpret_cast<uint8_t*>(data_vec.data()), data_vec.size(), encryptionKey);
            std::string data_buffer;
            for (std::int32_t i = 0; i < 4; i++)
                data_buffer += std::format("{:02X} ", (unsigned char)packetMessage.GetHeader().data >> (i * 8));

            for (std::int32_t i = 0; i < 4; i++)
                data_buffer += std::format("{:02X} ", (unsigned char)(packetMessage.GetCommand().data >> (i * 8)));

            for (std::int32_t i = 0; i < packetMessage.GetDataSize(); i++)
            {
                data_buffer += std::format("{:02X}", (unsigned char)packetMessage.GetData()[i]);
                if (i != packetMessage.GetDataSize() - 1) data_buffer += " ";
            }
            auto info = std::format("CSession()->Send() ({:d} bytes)\nOrder: 0x{:02X}\n{:s}", packetMessage.GetDataSize() + 8, packetMessage.GetOrder(), data_buffer);
            std::printf("%s\n", info.c_str());
            BaseLib::EventLog->Info(info.c_str());
        }
        ul.unlock();
        bool expected = false;
        bool desired = true;
        if (m_InSend.compare_exchange_strong(expected, desired))
            DoWrite();
    }

    void CSession::SetEncryptionKey(int32_t key)
    {
        m_encryptionKey = key;
    }

    void CSession::SetSessionId(uint16_t id)
    {
        m_sessionId = id;
    }
    int32_t CSession::GetEncryptionKey()
    {
        return m_encryptionKey;
    }

    uint16_t CSession::GetSessionId()
    {
        return m_sessionId;
    }

    void CSession::onPacket(Protocols::STcpPacketHeader& header, std::vector<uint8_t>& data)
    {
        if (m_verbose)
        {
            std::printf("CSession::OnPacket() - Received packet of size %llu\n", data.size());
            BaseLib::EventLog->Info("CSession::OnPacket() - Received packet of size %llu", data.size());
        }

        int32_t encryptionKey = m_useEncryption ? m_encryptionKey : -1;
        CMessage packetMessage = CMessage(reinterpret_cast<uint8_t*>(data.data()), data.size(), encryptionKey);

        if (m_verbose)
        {
            std::string data_buffer;
            for (std::int32_t i = 0; i < 4; i++)
                data_buffer += std::format("{:02X} ", (unsigned char)packetMessage.GetHeader().data >> (i * 8));

            for (std::int32_t i = 0; i < 4; i++)
                data_buffer += std::format("{:02X} ", (unsigned char)(packetMessage.GetCommand().data >> (i * 8)));

            for (std::int32_t i = 0; i < packetMessage.GetDataSize(); i++)
            {
                data_buffer += std::format("{:02X}", (unsigned char)packetMessage.GetData()[i]);
                if (i != packetMessage.GetDataSize() - 1) data_buffer += " ";
            }
            auto info = std::format("CSession()->onPacket() ({:d} bytes)\nOrder: 0x{:02X}\n{:s}", packetMessage.GetDataSize() + 8, packetMessage.GetOrder(), data_buffer);
            std::printf("%s\n", info.c_str());
            BaseLib::EventLog->Info(info.c_str());
        }

        if (m_callbacks.count(packetMessage.GetOrder()) == 0)
        {
            std::printf("CSession::OnPacket() - No callback for packet with order %d\n", packetMessage.GetOrder());
            BaseLib::EventLog->Info("CSession::OnPacket() - No callback for packet with order %d", packetMessage.GetOrder());
            return;
        }

        SCallbackData callbackData;

        callbackData.session = this;
        callbackData.message = &packetMessage;

        m_callbacks[packetMessage.GetOrder()](callbackData);
    }

    void CSession::DoRead()
    {
        if (!m_socket.is_open())
        {
            std::printf("CSession::Run() - Socket not open\n");
            BaseLib::EventLog->Error("CSession::Run() - Socket not open");
            return;

        }
        asio::error_code errorCode;
        auto self(shared_from_this());

        m_socket.async_read_some(asio::buffer(m_buffer.data(), m_buffer.size()), [this, self](const asio::error_code& errorCode, size_t bytesTransferred)
            {
                if (errorCode || bytesTransferred < 0)
                {
                    if (errorCode == asio::error::eof)
                    {
                        std::printf("CSession::Run() - The peer closed the connection unexpectedly: %s", errorCode.message().c_str());
                        BaseLib::EventLog->Error("CSession::Run() - The peer closed the connection unexpectedly: %s", errorCode.message().c_str());
                    }
                    else
                    {
                        std::printf("CSession::Run() - Failed to read data: %s", errorCode.message().c_str());
                        BaseLib::EventLog->Error("CSession::Run() - Failed to read data: %s", errorCode.message().c_str());
                    }
                    Disconnect();
                    return;
                }

        const constexpr int headerSize = sizeof(Protocols::STcpPacketHeader);
        m_reader.insert(m_reader.end(), m_buffer.begin(), m_buffer.begin() + bytesTransferred);

        Cryptography::CCrypt cryptography;
        Protocols::STcpPacketHeader header;

        while (m_reader.size() > headerSize)
        {
            if (m_useEncryption)
            {
                cryptography.RC5Decrypt32(reinterpret_cast<int32_t*>(m_reader.data()), &header, headerSize);
            }
            else
            {
                memcpy_s(&header, headerSize, m_reader.data(), headerSize);
            }

            if (header.size >= 2047) // Manage unencrypted packet with wrong size
            {
                std::printf("CSession::Run() - Invalid packet size: %d\n", header.size);
                BaseLib::EventLog->Error("CSession::Run() - Invalid packet size: %d", header.size);

                Disconnect();
                return;
            }

            if (m_reader.size() >= size_t(header.size))
            {
                std::vector<uint8_t> data(m_reader.begin(), m_reader.begin() + header.size);
                onPacket(header, data);

                auto newSize = m_reader.size() - header.size;

                std::memmove(m_reader.data(), m_reader.data() + header.size, newSize);
                m_reader.resize(newSize);
            }
        }
        DoRead();
            });
    }
    void CSession::DoWrite()
    {
        std::unique_lock lg(SendMtx);
        if (m_SendQueue.empty()) 
        {
            m_InSend = false;
            return;
        }
        if (!m_socket.is_open())
        {
            return;
        }
        auto& next = m_SendQueue.front();
        lg.unlock();

        auto self = shared_from_this();
        asio::async_write(m_socket, asio::buffer(next.data(), next.size()), [self](const std::error_code& ec, std::size_t bytes_transferred)
            {
            if (!ec) 
            {
                std::unique_lock lg(self->SendMtx);
                self->m_SendQueue.pop();
                lg.unlock();
                self->DoWrite();
            }
            else
            {
                std::printf("CSession::Send() - Failed to send data: %s\n", ec.message().c_str());
                BaseLib::EventLog->Error("CSession::Send() - Failed to send data: %s", ec.message().c_str());
                self->Disconnect();
            }
            });
    }
}