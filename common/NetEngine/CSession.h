#pragma once
#include "BaseLib/CLog.h"
#include <iostream>
#include <vector>
#include <map>
#include <atomic>
#include <queue>
#include <format>
#include <asio.hpp>

#include "CMessage.h"
#include "CServer.h"
#include "CCrypt.h"

#include "Constants.h"
#include "NetEngine/Protocols/BaseProtocol.h"
#include "NetEngine/Packets/PacketStruct.h"
#include "NetEngine/Packets/PacketData.h"
#include <boost_unordered.hpp>

namespace NetEngine
{
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
            boost::unordered_flat_map<uint16_t, std::function<void(SCallbackData&)>> callbacks;
        };

    public:
        friend class CServer;

        CSession(asio::ip::tcp::socket&& socket, asio::io_context& ioc, CServer* server, SSessionSettings settings, uint16_t session_id);
        ~CSession();

        void Disconnect();
        void Send(CMessage& message);
        void SendIpc(const uint32_t& ipc_id, const std::vector<uint8_t>& payload);

        template <typename O, typename M, typename E, typename P>
            requires (
        (std::integral<std::remove_cvref_t<O>> || std::is_enum_v<std::remove_cvref_t<O>>) &&
            (std::integral<std::remove_cvref_t<M>> || std::is_enum_v<std::remove_cvref_t<M>>) &&
            (std::integral<std::remove_cvref_t<E>> || std::is_enum_v<std::remove_cvref_t<E>>) &&
            (std::integral<std::remove_cvref_t<P>> || std::is_enum_v<std::remove_cvref_t<P>>)
            )
        __forceinline void SendMsg(O order, M mission, E extra, P option, uint8_t* data = nullptr, uint16_t data_size = 0, SendOption::EncryptionMethod enc = SendOption::EncryptionMethod::User)
        {
            CMessage message(this->GetEncryptionKey());
            message.SetSession(this->GetSessionId());
            message.SetCommand(order, mission, extra, option);
            message.SetEncryptMethod(enc);
            if (data_size > 0 && data != nullptr) message.SetData(data, data_size);
            this->Send(message);
        }
        __forceinline void SendMsg(NetEngine::Protocols::SCommandHeader cmd, uint8_t* data = nullptr, uint16_t data_size = 0, SendOption::EncryptionMethod enc = SendOption::EncryptionMethod::User)
        {
            CMessage message(this->GetEncryptionKey());
            message.SetSession(this->GetSessionId());
            message.SetCommand(cmd.order, cmd.mission, cmd.extra, cmd.option);
            message.SetEncryptMethod(enc);
            if (data_size > 0 && data != nullptr) message.SetData(data, data_size);
            this->Send(message);
        }

        template<Any16  Order, Any8  Mission, Any8  Extra, Any8  Option>
        __forceinline void ForwardMsg(uint16_t session_id, Order order, Mission mission, Extra extra, Option option, uint8_t* data = nullptr, uint16_t data_size = 0, SendOption::EncryptionMethod enc = SendOption::EncryptionMethod::User)
        {
            CMessage message(this->GetEncryptionKey());
            message.SetSession(session_id);
            message.SetCommand(to_u(order), to_u(mission), to_u(extra), to_u(option));
            message.SetEncryptMethod(enc);
            if (data_size > 0 && data != nullptr) message.SetData(data, data_size);
            this->Send(message);
        }

        void DoSend();
        void SetEncryptionKey(int32_t key);
        void SetSessionId(uint16_t id);
        void SetOnDisconnectCallback(std::function<void(std::shared_ptr<CSession>)> callback);
        void SetOnIpcMessageCallback(std::function<void(std::shared_ptr<CSession>, const uint32_t& msg_id, const uint32_t& msg_size, const std::vector<uint8_t>&)> callback);
        int32_t GetEncryptionKey();
        uint16_t GetSessionId();
        CServer* GetServer();
        bool IsOpen() const;
        void DoRead();
        void DoReadIpc();
        std::shared_mutex& GetMutex() {

            return mutex;
        }
        auto& GetStrand() {
            return m_strand;
        }
		std::string GetIpAddress() const
		{
			return m_socket.remote_endpoint().address().to_string();
		}
        static std::shared_ptr<CSession> Create(
            asio::ip::tcp::socket&& socket,
            asio::io_context& ioc,
            CServer* server,
            SSessionSettings settings,
            uint16_t session_id
        ) {
            auto self = std::make_shared<CSession>(
                std::move(socket), ioc, server, settings, session_id
            );
            return self;
        }
    private:
        
        void onPacket(Protocols::STcpPacketHeader& header, std::vector<uint8_t>& data);

    private:
        boost::unordered_flat_map<uint16_t, std::function<void(SCallbackData&)>> m_callbacks;
        std::deque<std::shared_ptr<std::vector<uint8_t>>> m_SendQueue;
        std::shared_mutex mutex;
        std::shared_mutex SendMtx;
        
        std::atomic_bool m_InSend;
        std::array<uint8_t, 1024> m_buffer{};
        std::vector<uint8_t> m_reader{};
        asio::ip::tcp::socket m_socket;
        asio::strand<asio::io_context::executor_type> m_strand;
        CServer* m_server = nullptr;
        bool m_verbose = false;
        bool m_useEncryption = false;
        int32_t m_encryptionKey = -1;
        uint16_t m_sessionId = 1;
        std::function<void(std::shared_ptr<CSession>)> m_on_disconnect_callback;
        std::function<void(std::shared_ptr<CSession>, const uint32_t& msg_id, const uint32_t& msg_size, const std::vector<uint8_t>&)> m_on_ipc_callback;
        bool m_ipc_identifier_skipped;
    };
}
