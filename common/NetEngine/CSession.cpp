#include "CSession.h"
namespace NetEngine
{
    using namespace BaseLib;
    using enum fmt::color;
    CSession::CSession(asio::ip::tcp::socket&& socket, asio::io_context& ioc, CServer* server, SSessionSettings settings, uint16_t session_id = 0)
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
        if (errorCode)
        {
            DEBUGLOG(red, "no remote endpoint: ({})", errorCode.message().c_str());
            return;
        }
    }
    CSession::~CSession() {}
    void CSession::Disconnect()
    {
        asio::dispatch(m_strand, [this]() {
            if (!m_socket.is_open()) return;

            asio::error_code errorCode;
            auto endPoint = m_socket.remote_endpoint(errorCode);
            DEBUGLOG(red, "connection closed from ({}:{})", endPoint.address().to_string().data(), endPoint.port());

            m_socket.shutdown(asio::ip::tcp::socket::shutdown_both, errorCode);
            m_socket.close(errorCode);
            if (m_on_disconnect_callback)  m_on_disconnect_callback(shared_from_this());

            });
    }
    void CSession::SetOnDisconnectCallback(std::function<void(std::shared_ptr<CSession>)> callback)
    {
        m_on_disconnect_callback = callback;
    }
    void CSession::SetOnIpcMessageCallback(std::function<void(std::shared_ptr<CSession>, const uint32_t& msg_id, const uint32_t& msg_size, const std::vector<uint8_t>&)> callback)
    {
        m_on_ipc_callback = callback;
    }
    void CSession::Send(CMessage& message)
    {
        auto order = message.GetOrder();
        //if (m_verbose && order != 281 && order != 71 && order != 322 && order != 72 && order != 257 && order != 282 && order != 77) Utility::LogPackets(std::source_location::current(), message, m_sessionId);
        if (m_verbose) Utility::LogPackets(std::source_location::current(), ACK, message, m_sessionId);
        auto data_vec = message.GenerateMessage();
        asio::dispatch(m_strand, [this, order, self = shared_from_this(), data_vec]() {
            if (!m_socket.is_open())
            {
                DEBUGLOG(red, "trying to send order ({}), but sid ({}) socket not open", order, m_sessionId);
                return;
            }
            asio::async_write(m_socket,
                asio::buffer(data_vec->data(), data_vec->size()),
                asio::bind_executor(m_strand,
                    [self, data_vec](const std::error_code& ec, size_t bytes_transferred)
                    {
                        if (ec)
                        {
                            DEBUGLOG(red, "failed to send data: ({})", ec.message().c_str());
                            self->Disconnect();
                        }
                    }));
            });
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
		std::shared_lock lock(mutex);
        return m_sessionId;
    }
    CServer* CSession::GetServer()
    {
        return m_server;
    }
    void CSession::onPacket(Protocols::STcpPacketHeader& header, std::vector<uint8_t>& data)
    {

        int32_t encryptionKey = m_useEncryption ? m_encryptionKey : -1;
        CMessage packetMessage = CMessage(reinterpret_cast<uint8_t*>(data.data()), static_cast<uint16_t>(data.size()), encryptionKey);
        
        /*auto packetMessage = std::make_shared<CMessage>(
            reinterpret_cast<uint8_t*>(data.data()),
            static_cast<uint16_t>(data.size()),
            encryptionKey
        );
        */
        auto order = packetMessage.GetOrder();
        //if (m_verbose && order != 281 && order != 71 && order != 322 && order != 72 && order != 257 && order != 282 && order != 77) Utility::LogPackets(std::source_location::current(), packetMessage, m_sessionId);
        if(m_verbose) Utility::LogPackets(std::source_location::current(), REQ, packetMessage, m_sessionId);

        if (!packetMessage.GetOrder()) return;
        if (m_callbacks.count(packetMessage.GetOrder()) == 0)
        {
            DEBUGLOG(red, "no callback for packet order: ({})", packetMessage.GetOrder());
            return;
        }

        SCallbackData callbackData;
        callbackData.session = this;
        callbackData.message = &packetMessage;
        callbackData.server = this->m_server;
        m_server->logExecution(m_sessionId, order);

        try
        {
            // Execute the callback
            m_callbacks[order](callbackData);
        }
        catch (const std::system_error& e)
        {
            // Handle system errors (e.g., mutex lock failures)
            DEBUGLOG(red,
                "System error in callback for packet order {}: {} (code: {})",
                                     order, e.what(), e.code().value());
        }
        catch (const std::runtime_error& e)
        {
            // Handle runtime errors
            DEBUGLOG(red,
                "Runtime error in callback for packet order {}: {}",
                                     order, e.what());
        }
        catch (const std::logic_error& e)
        {
            // Handle logic errors
            DEBUGLOG(red,
                "Logic error in callback for packet order {}: {}",
                                     order, e.what());
        }
        catch (const std::exception& e)
        {
            // Catch other standard exceptions
            DEBUGLOG(red,
                "Exception in callback for packet order {}: {}",
                                     order, e.what());
        }
        catch (...)
        {
            // Catch non-standard exceptions
            DEBUGLOG(red,
                "Unknown exception in callback for packet order {}", order);
        }
        m_server->clearExecution(m_sessionId, order);
    }
    void CSession::DoRead()
    {
        asio::dispatch(m_strand, [this, self = shared_from_this()]() {
            if (!m_socket.is_open())
            {
                DEBUGLOG(red, "socket not open");
                return;
            }
            asio::error_code errorCode;

            m_socket.async_read_some(asio::buffer(m_buffer.data(), m_buffer.size()), asio::bind_executor(m_strand, [this, self](const asio::error_code& errorCode, size_t bytesTransferred)
                {
                    if (errorCode || bytesTransferred < 0)
                    {
                        if (errorCode == asio::error::eof)
                            DEBUGLOG(red, "session closed unexpectedly: ({})", errorCode.message().c_str());
                        else
                            DEBUGLOG(red, "failed to read data: ({})", errorCode.message().c_str());
                        Disconnect();
                        return;
                    }

                    const constexpr int headerSize = sizeof(Protocols::STcpPacketHeader);
                    m_reader.insert(m_reader.end(), m_buffer.begin(), m_buffer.begin() + bytesTransferred);

                    Protocols::STcpPacketHeader header;

                    while (m_reader.size() > headerSize)
                    {
                        if (m_useEncryption)
                        {
							CCrypt crypt(CCrypt::CRYPT_TYPE::CRYPT_RC5, 0);
                            crypt.Decrypt(reinterpret_cast<uint32_t*>(m_reader.data()), &header, headerSize);
                        }
                        else
                        {
                            memcpy_s(&header, headerSize, m_reader.data(), headerSize);
                        }

                        if (header.size >= 2047) // Manage unencrypted packet with wrong size
                        {
                            DEBUGLOG(red, "invalid packet size: ({})", static_cast<uint32_t>(header.size));
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
                DEBUGLOG(red, "socket not open");
                return;
            }
            asio::error_code errorCode;

            m_socket.async_read_some(asio::buffer(m_buffer.data(), m_buffer.size()), asio::bind_executor(m_strand, [this, self](const asio::error_code& errorCode, size_t bytesTransferred)
                {
                    if (errorCode || bytesTransferred < 0)
                    {
                        if (errorCode == asio::error::eof)
                            DEBUGLOG(red, "session closed unexpectedly: ({})", errorCode.message().c_str());
                        else
                            DEBUGLOG(red, "failed to read data: ({})", errorCode.message().c_str());
                        Disconnect();
                        return;
                    }

                    const constexpr int headerSize = 8;
                    m_reader.insert(m_reader.end(), m_buffer.begin(), m_buffer.begin() + bytesTransferred);


                    while (m_reader.size() > headerSize)
                    {

                        auto ipc_id = *reinterpret_cast<uint32_t*>(m_reader.data());
                        auto data_size = *reinterpret_cast<uint32_t*>(m_reader.data() + sizeof(uint32_t));

                        if (m_reader.size() < headerSize + static_cast<unsigned long long>(data_size)) break;

                        std::vector<uint8_t> payload(m_reader.begin() + headerSize, m_reader.begin() + headerSize + data_size);

                        if (m_on_ipc_callback)
                            m_on_ipc_callback(self, ipc_id, data_size, payload);

                        auto newSize = m_reader.size() - (headerSize + data_size);
                        std::memmove(m_reader.data(), m_reader.data() + headerSize + data_size, newSize);
                        m_reader.resize(newSize);
                    }
                    DoReadIpc();
                }));
            });
    }
}