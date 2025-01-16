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


        BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::red, "connection closed from ({}:{})", endPoint.address().to_string().data(), endPoint.port());

        m_socket.shutdown(asio::ip::tcp::socket::shutdown_both, errorCode);
        m_socket.close(errorCode);
        if (m_on_disconnect_callback)
        {
            BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::red, "m_on_disconnect_callback(shared_from_this())");
            m_on_disconnect_callback(shared_from_this());
        }
        else
            BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::red, "m_on_disconnect_callback is null");
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
        
       // std::unique_lock ul(SendMtx);
        /*
        if (!m_socket.is_open())
        {
            BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::red, "socket not open");
            return;
        }
            

        auto data = message.GenerateMessage();
        std::vector<uint8_t> data_vec(&data[0], &data[message.GetFullSize()]);
        m_SendQueue.push(data_vec);
        if (m_verbose)
        {
            std::uint32_t encryptionKey = m_useEncryption ? m_encryptionKey : -1;
            CMessage packetMessage = CMessage(reinterpret_cast<uint8_t*>(data_vec.data()), data_vec.size(), encryptionKey);
            if (packetMessage.GetOrder() != 71 || packetMessage.GetOrder() != 72)
            {
                

                std::string data_buffer;
                data_buffer.reserve(static_cast<std::size_t>(4 + 4 + packetMessage.GetDataSize() * 3));

                for (std::uint32_t i = 0; i < 4; i++)
                    std::format_to(std::back_inserter(data_buffer), "{:02X} ", (unsigned char)(packetMessage.GetHeader().data >> (i * 8)));

                for (std::uint32_t i = 0; i < 4; i++)
                    std::format_to(std::back_inserter(data_buffer), "{:02X} ", (unsigned char)(packetMessage.GetCommand().data >> (i * 8)));

                for (std::uint32_t i = 0; i < packetMessage.GetDataSize(); i++)
                {
                    std::format_to(std::back_inserter(data_buffer), "{:02X}", (unsigned char)packetMessage.GetData()[i]);
                    if (i != packetMessage.GetDataSize() - 1)
                        data_buffer += ' ';
                }

                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "({:d} bytes) MsgSessionId: {}, CSessionId: {}, Order: ({}), Mission: ({}), Extra: ({}), Option: ({})\n{:s}", packetMessage.GetDataSize() + 8, packetMessage.GetSession(), m_sessionId, packetMessage.GetOrder(), packetMessage.GetMission(), packetMessage.GetExtra(), packetMessage.GetOption(), data_buffer);
            }
        }
        ul.unlock();
        bool expected = false;
        bool desired = true;
        if (m_InSend.compare_exchange_strong(expected, desired))
            DoWrite();
        */
            
       
        if (!m_socket.is_open())
        {
            BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::red, "socket not open");
            return;
        }
        auto data = message.GenerateMessage();
        std::vector<std::uint8_t>* data_vec = new std::vector<std::uint8_t>(&data[0], &data[message.GetFullSize()]);

        if(m_verbose)
        {
            std::int32_t encryptionKey = m_useEncryption ? m_encryptionKey : -1;
            CMessage packetMessage = CMessage(reinterpret_cast<std::uint8_t*>(data_vec->data()), data_vec->size(), encryptionKey);
            if (packetMessage.GetOrder() != 71 || packetMessage.GetOrder() != 72)
            {


                std::string data_buffer;
                data_buffer.reserve(static_cast<std::size_t>(4 + 4 + packetMessage.GetDataSize() * 3));

                for (std::uint32_t i = 0; i < 4; i++)
                    std::format_to(std::back_inserter(data_buffer), "{:02X} ", (unsigned char)(packetMessage.GetHeader().data >> (i * 8)));

                for (std::uint32_t i = 0; i < 4; i++)
                    std::format_to(std::back_inserter(data_buffer), "{:02X} ", (unsigned char)(packetMessage.GetCommand().data >> (i * 8)));

                for (std::uint32_t i = 0; i < packetMessage.GetDataSize(); i++)
                {
                    std::format_to(std::back_inserter(data_buffer), "{:02X}", (unsigned char)packetMessage.GetData()[i]);
                    if (i != packetMessage.GetDataSize() - 1)
                        data_buffer += ' ';
                }

                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "({:d} bytes) MsgSessionId: {}, CSessionId: {}, Order: ({}), Mission: ({}), Extra: ({}), Option: ({})\n{:s}", packetMessage.GetDataSize() + 8, packetMessage.GetSession(), m_sessionId, packetMessage.GetOrder(), packetMessage.GetMission(), packetMessage.GetExtra(), packetMessage.GetOption(), data_buffer);
            }
        }

        auto self = shared_from_this();

        asio::async_write(m_socket, asio::buffer(data_vec->data(), data_vec->size()), asio::bind_executor(m_strand, [self, data_vec](const std::error_code& ec, std::size_t bytes_transferred)
        {
            delete data_vec;
            if (ec)
            {
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::red, "failed to send data: ({})", ec.message().c_str());
                self->Disconnect();
            }
        }));
        
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
        CMessage packetMessage = CMessage(reinterpret_cast<std::uint8_t*>(data.data()), data.size(), encryptionKey);

        if (m_verbose)
        {
            if (packetMessage.GetOrder() != 71 || packetMessage.GetOrder() != 72)
            {
                /*
                std::string data_buffer;
                for (std::int32_t i = 0; i < 4; i++)//12 chars
                    data_buffer += std::format("{:02X} ", (unsigned char)packetMessage.GetHeader().data >> (i * 8));

                for (std::int32_t i = 0; i < 4; i++)//12 chars
                    data_buffer += std::format("{:02X} ", (unsigned char)(packetMessage.GetCommand().data >> (i * 8)));

                for (std::int32_t i = 0; i < packetMessage.GetDataSize(); i++)//3 * data size_chars
                {
                    data_buffer += std::format("{:02X}", (unsigned char)packetMessage.GetData()[i]);
                    if (i != packetMessage.GetDataSize() - 1) data_buffer += " ";
                }
                */
                std::string data_buffer;
                data_buffer.reserve(static_cast<std::size_t>(4 + 4 + packetMessage.GetDataSize() * 3));

                for (std::uint32_t i = 0; i < 4; i++)
                    std::format_to(std::back_inserter(data_buffer), "{:02X} ", (unsigned char)(packetMessage.GetHeader().data >> (i * 8)));

                for (std::uint32_t i = 0; i < 4; i++)
                    std::format_to(std::back_inserter(data_buffer), "{:02X} ", (unsigned char)(packetMessage.GetCommand().data >> (i * 8)));

                for (std::uint32_t i = 0; i < packetMessage.GetDataSize(); i++)
                {
                    std::format_to(std::back_inserter(data_buffer), "{:02X}", (unsigned char)packetMessage.GetData()[i]);
                    if (i != packetMessage.GetDataSize() - 1)
                        data_buffer += ' ';
                }
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "({:d} bytes) MsgSessionId: {}, CSessionId: {}, Order: ({}), Mission: ({}), Extra: ({}), Option: ({})\n{:s}", packetMessage.GetDataSize() + 8, packetMessage.GetSession(), m_sessionId, packetMessage.GetOrder(), packetMessage.GetMission(), packetMessage.GetExtra(), packetMessage.GetOption(), data_buffer);

            }
        }
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
        m_callbacks[packetMessage.GetOrder()](callbackData);
        m_server->clearExecution(m_sessionId, packetMessage.GetOrder());
    }

    void CSession::DoRead()
    {
        if (!m_socket.is_open())
        {
            BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::red, "socket not open");
            return;
        }
        asio::error_code errorCode;
        auto self(shared_from_this());

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
    }
    void CSession::DoReadIpc()
    {
        if (!m_socket.is_open())
        {
            BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::red, "socket not open");
            return;
        }
        asio::error_code errorCode;
        auto self(shared_from_this());

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

                if (m_on_ipc_callback) m_on_ipc_callback(shared_from_this(), ipc_id, data_size, payload);

                auto newSize = m_reader.size() - (headerSize + data_size);
                std::memmove(m_reader.data(), m_reader.data() + headerSize + data_size, newSize);
                m_reader.resize(newSize);
            }
            DoReadIpc();
        }));
    }
    void CSession::DoWrite()
    {
        //sync
        /*
        bool done = false;
        while (!done)
        {
            std::unique_lock<std::mutex> lg(SendMtx);
            if (m_SendQueue.empty())
            {
                //BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::red, "SendQueue Empty");
                m_InSend = false;
                done = true; 
            }
            else if (!m_socket.is_open())
            {
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::red, "socket not open");
                done = true; 
            }
            else
            {
                auto& next = m_SendQueue.front();
                lg.unlock();

                try
                {
                    asio::write(m_socket, asio::buffer(next.data(), next.size()));
                    lg.lock();
                    m_SendQueue.pop();
                }
                catch (const std::system_error& e)
                {
                    BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::red, "failed to send data: ({})", e.what());
                    Disconnect();
                    done = true;
                }
            }
        }
        */
        
        std::unique_lock lg(SendMtx);
        if (m_SendQueue.empty()) 
        {
            m_InSend = false;
            return;
        }
        if (!m_socket.is_open())
        {
            BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::red, "socket not open");
            return;
        }
        auto& next = m_SendQueue.front();
        lg.unlock();
        auto self = shared_from_this();
        //asio::async_write(m_socket, asio::buffer(next.data(), next.size()), [self](const std::error_code& ec, std::size_t bytes_transferred)
        asio::async_write(m_socket, asio::buffer(next.data(), next.size()), asio::bind_executor(m_strand, [self](const std::error_code& ec, std::size_t bytes_transferred)
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
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::red, "failed to send data: ({})", ec.message().c_str());
                self->Disconnect();
            }
        }));
        
    }
}