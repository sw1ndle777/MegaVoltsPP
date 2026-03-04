#pragma once
#include <stdlib.h>
#include <cstring>
#include <vector>
#include "NetEngine/Protocols/BaseProtocol.h"
#include "Constants.h"
#include "CCrypt.h"
#include <memory>
namespace NetEngine
{
    struct SendOption
    {
        enum EncryptionMethod : uint8_t
        {
            Default = 0x00,
            User = 0x01,
            None = 0x3
        };
    };
    class CMessage
    {
    public:
        CMessage(int32_t crypt_key = -1);
        CMessage(uint8_t* data, uint16_t size, int32_t crypt_key = -1);

        ~CMessage();

       
        void SetEncryptMethod(SendOption::EncryptionMethod method);
        void SetSession(uint16_t session);

        template <typename T>
            requires (std::integral<std::remove_cvref_t<T>> || std::is_enum_v<std::remove_cvref_t<T>>)
        void SetMission(T mission)
        {
            m_command->mission = u32_cast(mission);
        }

        template <typename T>
            requires (std::integral<std::remove_cvref_t<T>> || std::is_enum_v<std::remove_cvref_t<T>>)
		void SetOrder(T order)
        {
            m_command->order = u32_cast(order);
        }

        template <typename T>
            requires (std::integral<std::remove_cvref_t<T>> || std::is_enum_v<std::remove_cvref_t<T>>)
        void SetExtra(T extra)
        {
            m_command->extra = u32_cast(extra);
        }

        template <typename T>
            requires (std::integral<std::remove_cvref_t<T>> || std::is_enum_v<std::remove_cvref_t<T>>)
        void SetOption(T option)
        {
            m_command->option = u32_cast(option);
        }

        template <typename O, typename M, typename E, typename P>
            requires (
            (std::integral<std::remove_cvref_t<O>> || std::is_enum_v<std::remove_cvref_t<O>>) &&
            (std::integral<std::remove_cvref_t<M>> || std::is_enum_v<std::remove_cvref_t<M>>) &&
            (std::integral<std::remove_cvref_t<E>> || std::is_enum_v<std::remove_cvref_t<E>>) &&
            (std::integral<std::remove_cvref_t<P>> || std::is_enum_v<std::remove_cvref_t<P>>)
                )
        void SetCommand(O order, M mission, E extra, P option)
        {
            SetOrder(order);
            SetMission(mission);
            SetExtra(extra);
            SetOption(option);
        }

        uint16_t GetSession();
        uint8_t GetMission();
        uint16_t GetOrder();
        uint8_t GetExtra();
        uint8_t GetOption();
        Protocols::STcpPacketHeader& GetHeader();
        Protocols::SCommandHeader& GetCommand();
        uint32_t GetDataSize();
        uint32_t GetFullSize();

        void SetData(uint8_t* data, uint16_t size);

        uint8_t* GetData()
        {
            return m_buffer.data() + dataOffset();
        }

        template <typename T>
        T GetData()
        {
            if constexpr (std::is_pointer_v<T>)
            {
                return reinterpret_cast<T>(m_buffer.data() + dataOffset());
            }
            else
            {
                static_assert(std::is_trivially_copyable_v<T>, "CMessage::GetData<T>() requires trivially copyable T for value return.");
                T value{};
                const auto available = m_buffer.size() > dataOffset() ? (m_buffer.size() - dataOffset()) : 0;
                const auto copy_size = std::min(available, sizeof(T));
                if (copy_size > 0)
                    std::memcpy(&value, m_buffer.data() + dataOffset(), copy_size);
                return value;
            }
        }
        template <typename T>
        void SetData(T data)
        {
			static_assert(!std::is_pointer_v<T>, "CMessage::SetData(T) does not accept pointer types. Use SetData(uint8_t*, uint16_t) instead.");
			m_buffer.resize(minSize() + sizeof(data));
			m_header = reinterpret_cast<Protocols::STcpPacketHeader*>(m_buffer.data());
			m_command = reinterpret_cast<Protocols::SCommandHeader*>(m_header + 1);
			std::memcpy(m_buffer.data() + dataOffset(), &data, sizeof(data));
			m_header->size = static_cast<uint32_t>(minSize() + sizeof(data));
        }
        std::shared_ptr<std::vector<uint8_t>> GenerateMessage();
        void ProcessMessage(uint8_t* data, uint16_t size);
    private:
        void generateBogus();
		static constexpr size_t tcp_header_size = sizeof(Protocols::STcpPacketHeader);
		static constexpr size_t command_header_size = sizeof(Protocols::SCommandHeader);
		static size_t minSize() { return tcp_header_size + command_header_size; }
        static size_t dataOffset()  { return minSize(); }
        size_t buffersize() { return m_buffer.size(); }
        size_t dataSize() { return buffersize() - minSize(); };
    private:
        std::vector<uint8_t> m_buffer;
        Protocols::STcpPacketHeader* m_header;
        Protocols::SCommandHeader* m_command;
        int32_t m_crypt;
        SendOption::EncryptionMethod m_encrypt_method = SendOption::EncryptionMethod::User;
    };
}
