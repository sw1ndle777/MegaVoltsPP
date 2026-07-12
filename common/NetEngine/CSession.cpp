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
        m_callbacks = settings.callbacks ? settings.callbacks : std::make_shared<const CallbackMap>();

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
            // Don't log client IPs to console/file — they're already recorded in
            // login_history on auth. Log the session id instead (non-PII, correlatable).
            DEBUGLOG(red, "connection closed sid=({})", GetSessionId());

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
                DEBUGLOG(red, "trying to send order ({}), but sid ({}) socket not open", order, GetSessionId());
                return;
            }
            const bool was_idle = m_SendQueue.empty();
            m_SendQueue.push_back(data_vec);
            if (was_idle)
                DoSend();
            });
    }
    void CSession::SendIpc(const uint32_t& ipc_id, const std::vector<uint8_t>& payload)
    {
        auto data_vec = std::make_shared<std::vector<uint8_t>>(8 + payload.size());
        const uint32_t data_size = static_cast<uint32_t>(payload.size());
        std::memcpy(data_vec->data(), &ipc_id, sizeof(ipc_id));
        std::memcpy(data_vec->data() + sizeof(ipc_id), &data_size, sizeof(data_size));
        if (!payload.empty())
            std::memcpy(data_vec->data() + 8, payload.data(), payload.size());

        asio::dispatch(m_strand, [this, self = shared_from_this(), data_vec]() {
            if (!m_socket.is_open())
            {
                DEBUGLOG(red, "trying to send ipc but sid ({}) socket not open", GetSessionId());
                return;
            }
            const bool was_idle = m_SendQueue.empty();
            m_SendQueue.push_back(data_vec);
            if (was_idle)
                DoSend();
            });
    }
    void CSession::DoSend()
    {
        if (!m_socket.is_open())
        {
            m_SendQueue.clear();
            return;
        }
        if (m_SendQueue.empty()) return;

        // One async_write per packet (1:1 on the wire). Only one write is ever in flight:
        // Send()/SendIpc() call DoSend only when the queue was empty, and the completion
        // handler chains the next packet. This keeps the simple per-packet shape while
        // avoiding the overlapping-async_write UB a direct write-in-Send would have.
        auto data = m_SendQueue.front();
        asio::async_write(m_socket, asio::buffer(data->data(), data->size()),
            asio::bind_executor(m_strand,
                [this, self = shared_from_this(), data](const std::error_code& ec, size_t)
                {
                    if (ec)
                    {
                        DEBUGLOG(red, "failed to send data: ({})", ec.message().c_str());
                        m_SendQueue.clear();
                        self->Disconnect();
                        return;
                    }
                    if (!m_SendQueue.empty()) m_SendQueue.pop_front();
                    if (!m_SendQueue.empty()) DoSend();
                }));
    }

    void CSession::SetEncryptionKey(int32_t key)
    {
        m_encryptionKey = key;
    }
    void CSession::SetSessionId(uint16_t id)
    {
        m_sessionId.store(id, std::memory_order_relaxed);
    }
    int32_t CSession::GetEncryptionKey()
    {
        return m_encryptionKey;
    }
    uint16_t CSession::GetSessionId() const
    {
        return m_sessionId.load(std::memory_order_relaxed);
    }
    CServer* CSession::GetServer()
    {
        return m_server;
    }
    bool CSession::IsOpen() const
    {
        return m_socket.is_open();
    }
    void CSession::onPacket(Protocols::STcpPacketHeader& header, std::span<uint8_t> data)
    {

        int32_t encryptionKey = m_useEncryption ? m_encryptionKey : -1;
        CMessage packetMessage = CMessage(data.data(), static_cast<uint16_t>(data.size()), encryptionKey);

        auto order = packetMessage.GetOrder();
        //if (m_verbose && order != 281 && order != 71 && order != 322 && order != 72 && order != 257 && order != 282 && order != 77) Utility::LogPackets(std::source_location::current(), packetMessage, m_sessionId);
        if(m_verbose) Utility::LogPackets(std::source_location::current(), REQ, packetMessage, m_sessionId);

        if (!order) return;
        auto callback_it = m_callbacks->find(order);
        if (callback_it == m_callbacks->end())
        {
            DEBUGLOG(red, "no callback for packet order: ({})", order);
            return;
        }

        SCallbackData callbackData;
        callbackData.session = shared_from_this();
        callbackData.message = &packetMessage;
        callbackData.server = this->m_server;
        m_server->logExecution(m_sessionId, order);

        try
        {
            // Execute the callback
            callback_it->second(callbackData);
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
                    if (errorCode)
                    {
                        if (errorCode == asio::error::eof)
                            DEBUGLOG(red, "session closed unexpectedly: ({})", errorCode.message().c_str());
                        else
                            DEBUGLOG(red, "failed to read data: ({})", errorCode.message().c_str());
                        Disconnect();
                        return;
                    }

                    const constexpr size_t headerSize = sizeof(Protocols::STcpPacketHeader);
                    // A framed packet is at least a tcp header + a command header; anything
                    // smaller can't carry an order. size==0 in particular is fatal: offset
                    // below advances by header.size, so a 0 would never advance and the while
                    // loop would spin forever on the strand, wedging the whole io_context.
                    const constexpr size_t minPacketSize = sizeof(Protocols::STcpPacketHeader) + sizeof(Protocols::SCommandHeader);
                    m_reader.insert(m_reader.end(), m_buffer.begin(), m_buffer.begin() + bytesTransferred);

                    Protocols::STcpPacketHeader header;
                    size_t offset = 0;

                    while (m_reader.size() - offset > headerSize)
                    {
                        uint8_t* cursor = m_reader.data() + offset;
                        if (m_useEncryption)
                        {
							CCrypt crypt(CCrypt::CRYPT_TYPE::CRYPT_RC5, 0);
                            crypt.Decrypt(reinterpret_cast<uint32_t*>(cursor), &header, headerSize);
                        }
                        else
                        {
                            std::memcpy(&header, cursor, headerSize);
                        }

                        if (header.size >= 2047 || header.size < minPacketSize) // reject wrong (too large / too small) sizes
                        {
                            DEBUGLOG(red, "invalid packet size: ({})", static_cast<uint32_t>(header.size));
                            Disconnect();
                            return;
                        }

                        if (m_reader.size() - offset < size_t(header.size))
                            break;

                        onPacket(header, std::span<uint8_t>(cursor, header.size));
                        offset += header.size;
                    }
                    if (offset > 0)
                        m_reader.erase(m_reader.begin(), m_reader.begin() + offset);
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
                    if (errorCode)
                    {
                        if (errorCode == asio::error::eof)
                            DEBUGLOG(red, "session closed unexpectedly: ({})", errorCode.message().c_str());
                        else
                            DEBUGLOG(red, "failed to read data: ({})", errorCode.message().c_str());
                        Disconnect();
                        return;
                    }

                    const constexpr size_t headerSize = 8;
                    m_reader.insert(m_reader.end(), m_buffer.begin(), m_buffer.begin() + bytesTransferred);

                    size_t offset = 0;

                    while (m_reader.size() - offset > headerSize)
                    {
                        uint8_t* cursor = m_reader.data() + offset;
                        auto ipc_id = *reinterpret_cast<uint32_t*>(cursor);
                        auto data_size = *reinterpret_cast<uint32_t*>(cursor + sizeof(uint32_t));

                        if (m_reader.size() - offset < headerSize + static_cast<size_t>(data_size)) break;

                        std::vector<uint8_t> payload(cursor + headerSize, cursor + headerSize + data_size);

                        if (m_on_ipc_callback)
                            m_on_ipc_callback(self, ipc_id, data_size, payload);

                        offset += headerSize + data_size;
                    }
                    if (offset > 0)
                        m_reader.erase(m_reader.begin(), m_reader.begin() + offset);
                    DoReadIpc();
                }));
            });
    }
}
