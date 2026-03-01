#pragma once
#include <stdlib.h>
#include <vector>
#include <cstring>
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
            return reinterpret_cast<T>(m_buffer.data() + dataOffset());
        }
        template <typename T>
        void SetData(T data)
        {
			m_buffer.resize(minSize() + sizeof(data));
#ifdef _WIN32
            memcpy_s(m_buffer.data() + dataOffset(), sizeof(data), &data, sizeof(data));
#else
            std::memcpy(m_buffer.data() + dataOffset(), &data, sizeof(data));
#endif
			m_header->size = minSize() + sizeof(data);
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
