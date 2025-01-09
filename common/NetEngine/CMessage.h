#pragma once
#include <stdlib.h>

#include "NetEngine/Protocols/BaseProtocol.h"
#include "Constants.h"
#include "CCrypt.h"

namespace NetEngine
{
    struct SendOption
    {
        enum EncryptionMethod : std::uint8_t
        {
            Default = 0x00,
            User = 0x01,
            None = 0x3
        };
    };
    class CMessage
    {
    public:
        CMessage(std::int32_t crypt_key = -1);
        CMessage(std::uint8_t* data, std::uint16_t size, std::int32_t crypt_key = -1);

        ~CMessage();

        void SetCommand(std::uint16_t order, std::uint8_t mission, std::uint8_t extra, std::uint8_t option);
        void SetEncryptMethod(SendOption::EncryptionMethod method);
        void SetSession(std::uint16_t session);
        void SetMission(std::uint8_t mission);
        void SetOrder(std::uint16_t order);
        void SetExtra(std::uint8_t extra);
        void SetOption(std::uint8_t option);

        std::uint16_t GetSession();
        std::uint8_t GetMission();
        std::uint16_t GetOrder();
        std::uint8_t GetExtra();
        std::uint8_t GetOption();
        Protocols::STcpPacketHeader GetHeader();
        Protocols::SCommandHeader GetCommand();
        std::uint32_t GetDataSize();
        std::uint32_t GetFullSize();

        void SetData(std::uint8_t* data, std::uint16_t size);

        std::uint8_t* GetData()
        {
            return m_data;
        }

        template <typename T>
        T GetData()
        {
            return reinterpret_cast<T>(m_data);
        }

        template <typename T>
        void SetData(T data)
        {
            m_data = reinterpret_cast<std::uint8_t*>(data);
            m_data_size = sizeof(data);
        }

        std::uint8_t* GenerateMessage();
    private:
        void processMessage(std::uint8_t* data, std::uint16_t size);
        void generateBogus();

    private:
        Protocols::STcpPacketHeader m_header;
        Protocols::SCommandHeader m_command;

        std::uint8_t* m_data;

        std::uint32_t m_data_size;
        std::int32_t m_crypt;
        SendOption::EncryptionMethod m_encrypt_method = SendOption::EncryptionMethod::User;
    };
}

//#endif