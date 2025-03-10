#include "CSession.h"
namespace NetEngine
{
    CSession::CSession(asio::ip::tcp::socket&& socket, asio::io_context& ioc, CServer* server, SSessionSettings settings, std::uint16_t session_id = 0)
        : m_socket(std::move(socket)), m_server(server), m_strand(asio::make_strand(ioc))
    {
        m_ipc_identifier_skipped = false;
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
            BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::red, "no remote endpoint: ({})", errorCode.message().c_str());
            return;
        }
    }
    CSession::~CSession() {}
    void CSession::Disconnect()
    {
        asio::dispatch(m_strand, [this, self = shared_from_this()]() {
            if (!m_socket.is_open()) return;

            asio::error_code errorCode;
            auto endPoint = m_socket.remote_endpoint(errorCode);
            BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::red, "connection closed from ({}:{})", endPoint.address().to_string().data(), endPoint.port());

            m_socket.shutdown(asio::ip::tcp::socket::shutdown_both, errorCode);
            m_socket.close(errorCode);
            if (m_on_disconnect_callback)  m_on_disconnect_callback(shared_from_this());
        }); 
    }
    void CSession::SetOnDisconnectCallback(std::function<void(std::shared_ptr<CSession>)> callback)
    {
        m_on_disconnect_callback = callback;
    }
    void CSession::SetOnIpcMessageCallback(std::function<void(std::shared_ptr<CSession>, const std::uint32_t& msg_id, const std::uint32_t& msg_size, const std::vector<uint8_t>&)> callback)
    {
        m_on_ipc_callback = callback;
    }
    void CSession::Send(CMessage& message)
    {
        if (m_verbose) Utility::LogPackets(std::source_location::current(), message, m_sessionId);
        auto data = message.GenerateMessage();
        auto data_vec = std::make_shared<std::vector<std::uint8_t>>(
            &data[0], &data[message.GetFullSize()]);

        asio::dispatch(m_strand, [this, self = shared_from_this(), data_vec]() {
            if (!m_socket.is_open())
            {
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::red, "socket not open");
                return;
            }
            
            asio::async_write(m_socket, 
                asio::buffer(data_vec->data(), data_vec->size()),
                asio::bind_executor(m_strand, 
                    [self, data_vec](const std::error_code& ec, std::size_t bytes_transferred)
            {
                if (ec)
                {
                    BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::red, "failed to send data: ({})", ec.message().c_str());
                    self->Disconnect();
                }
            }));
            
            /*
            std::error_code ec;
            asio::write(m_socket, asio::buffer(data_vec->data(), data_vec->size()), ec);
            if (ec)
            {
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::red, "failed to send data: ({})", ec.message().c_str());
                self->Disconnect();
            }
            */
        });
    }
    void CSession::SetEncryptionKey(std::int32_t key)
    {
        m_encryptionKey = key;
    }
    void CSession::SetSessionId(std::uint16_t id)
    {
        m_sessionId = id;
    }
    std::int32_t CSession::GetEncryptionKey() 
    {
        return m_encryptionKey;
    }
    std::uint16_t CSession::GetSessionId() 
    {
        return m_sessionId;
    }
    CServer* CSession::GetServer()
    {
        return m_server;
    }
    void CSession::onPacket(Protocols::STcpPacketHeader& header, std::vector<std::uint8_t>& data)
    {

        std::int32_t encryptionKey = m_useEncryption ? m_encryptionKey : -1;
        CMessage packetMessage = CMessage(reinterpret_cast<std::uint8_t*>(data.data()), static_cast<std::uint16_t>(data.size()), encryptionKey);

        if (m_verbose) Utility::LogPackets(std::source_location::current(), packetMessage, m_sessionId);
        
        if (!packetMessage.GetOrder()) return;
        if (m_callbacks.count(packetMessage.GetOrder()) == 0)
        {
            BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::red, "no callback for packet order: ({})", packetMessage.GetOrder());
            return;
        }

        SCallbackData callbackData;

        callbackData.session = this;
        callbackData.message = &packetMessage;
        callbackData.server = this->m_server;
        m_server->logExecution(m_sessionId, packetMessage.GetOrder());

        try
        {
            // Execute the callback
            m_callbacks[packetMessage.GetOrder()](callbackData);
        }
        catch (const std::system_error& e)
        {
            // Handle system errors (e.g., mutex lock failures)
            BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::red,
                "System error in callback for packet order {}: {} (code: {})",
                packetMessage.GetOrder(), e.what(), e.code().value());
        }
        catch (const std::runtime_error& e)
        {
            // Handle runtime errors
            BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::red,
                "Runtime error in callback for packet order {}: {}",
                packetMessage.GetOrder(), e.what());
        }
        catch (const std::logic_error& e)
        {
            // Handle logic errors
            BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::red,
                "Logic error in callback for packet order {}: {}",
                packetMessage.GetOrder(), e.what());
        }
        catch (const std::exception& e)
        {
            // Catch other standard exceptions
            BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::red,
                "Exception in callback for packet order {}: {}",
                packetMessage.GetOrder(), e.what());
        }
        catch (...)
        {
            // Catch non-standard exceptions
            BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::red,
                "Unknown exception in callback for packet order {}", packetMessage.GetOrder());
        }
        m_server->clearExecution(m_sessionId, packetMessage.GetOrder());
    }
    void CSession::DoRead()
    {
        asio::dispatch(m_strand, [this, self = shared_from_this()]() {
            if (!m_socket.is_open())
            {
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::red, "socket not open");
                return;
            }
            asio::error_code errorCode;

            m_socket.async_read_some(asio::buffer(m_buffer.data(), m_buffer.size()), asio::bind_executor(m_strand, [this, self](const asio::error_code& errorCode, size_t bytesTransferred)
            {
                if (errorCode || bytesTransferred < 0)
                {
                    if (errorCode == asio::error::eof)
                        BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::red, "session closed unexpectedly: ({})", errorCode.message().c_str());
                    else
                        BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::red, "failed to read data: ({})", errorCode.message().c_str());
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
                        cryptography.RC5Decrypt32(reinterpret_cast<std::uint32_t*>(m_reader.data()), &header, headerSize);
                    }
                    else
                    {
                        memcpy_s(&header, headerSize, m_reader.data(), headerSize);
                    }

                    if (header.size >= 2047) // Manage unencrypted packet with wrong size
                    {
                        BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::red, "invalid packet size: ({})", static_cast<std::uint32_t>(header.size));
                        Disconnect();
                        return;
                    }

                    if (m_reader.size() >= std::size_t(header.size))
                    {
                        std::vector<std::uint8_t> data(m_reader.begin(), m_reader.begin() + header.size);
                        onPacket(header, data);

                        auto newSize = m_reader.size() - header.size;

                        std::memmove(m_reader.data(), m_reader.data() + header.size, newSize);
                        m_reader.resize(newSize);
                    }
                    else
                    {
                        break;
                    }
                }
                DoRead();
            }));
        });
    }
    void CSession::DoReadIpc()
    {
        asio::dispatch(m_strand, [this, self = shared_from_this()]() {
            if (!m_socket.is_open())
            {
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::red, "socket not open");
                return;
            }
            asio::error_code errorCode;

            m_socket.async_read_some(asio::buffer(m_buffer.data(), m_buffer.size()), asio::bind_executor(m_strand, [this, self](const asio::error_code& errorCode, size_t bytesTransferred)
            {
                if (errorCode || bytesTransferred < 0)
                {
                    if (errorCode == asio::error::eof)
                        BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::red, "session closed unexpectedly: ({})", errorCode.message().c_str());
                    else
                        BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::red, "failed to read data: ({})", errorCode.message().c_str());
                    Disconnect();
                    return;
                }

                const constexpr int headerSize = 8;
                m_reader.insert(m_reader.end(), m_buffer.begin(), m_buffer.begin() + bytesTransferred);


                while (m_reader.size() > headerSize)
                {

                    auto ipc_id = *reinterpret_cast<std::uint32_t*>(m_reader.data());
                    auto data_size = *reinterpret_cast<std::uint32_t*>(m_reader.data() + sizeof(std::uint32_t));

                    if (m_reader.size() < headerSize + static_cast<unsigned long long>(data_size)) break;

                    std::vector<std::uint8_t> payload(m_reader.begin() + headerSize, m_reader.begin() + headerSize + data_size);

                    if (m_on_ipc_callback) 
                        m_on_ipc_callback(shared_from_this(), ipc_id, data_size, payload);

                    auto newSize = m_reader.size() - (headerSize + data_size);
                    std::memmove(m_reader.data(), m_reader.data() + headerSize + data_size, newSize);
                    m_reader.resize(newSize);
                }
                DoReadIpc();
            }));
        });
    }
}