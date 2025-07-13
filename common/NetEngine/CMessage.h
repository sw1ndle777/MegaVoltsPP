#pragma once
#include <stdlib.h>
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

        void SetCommand(uint16_t order, uint8_t mission, uint8_t extra, uint8_t option);
        void SetEncryptMethod(SendOption::EncryptionMethod method);
        void SetSession(uint16_t session);
        void SetMission(uint8_t mission);
        void SetOrder(uint16_t order);
        void SetExtra(uint8_t extra);
        void SetOption(uint8_t option);

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
            memcpy_s(m_buffer.data() + dataOffset(), sizeof(data), &data, sizeof(data));
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
