#include "CMessage.h"

namespace NetEngine
{
    CMessage::CMessage(std::int32_t crypt_key)
    {
        m_data = nullptr;
        m_crypt = crypt_key;

        generateBogus();
    }

    CMessage::CMessage(std::uint8_t* data, std::uint16_t size, std::int32_t crypt_key)
    {
        m_data = nullptr;
        m_crypt = crypt_key;
        m_encrypt_method = SendOption::EncryptionMethod::User;
        this->processMessage(data, size);
    }

    CMessage::~CMessage()
    {
        if (m_data != nullptr)
        {
            delete[] m_data;
            m_data = nullptr;
        }
    }

    void CMessage::SetCommand(std::uint16_t order, std::uint8_t mission, std::uint8_t extra, std::uint8_t option)
    {
        m_command.order = order;
        m_command.mission = mission;
        m_command.extra = extra;
        m_command.option = option;
    }

    void CMessage::SetEncryptMethod(SendOption::EncryptionMethod method)
    {
        m_encrypt_method = method;
    }

    void CMessage::SetSession(std::uint16_t session)
    {
        m_header.sessionId = session;
    }

    void CMessage::SetMission(std::uint8_t mission)
    {
        m_command.mission = mission;
    }

    void CMessage::SetOrder(std::uint16_t order)
    {
        m_command.order = order;
    }

    void CMessage::SetExtra(std::uint8_t extra)
    {
        m_command.extra = extra;
    }

    void CMessage::SetOption(std::uint8_t option)
    {
        m_command.option = option;
    }

    void CMessage::SetData(std::uint8_t* data, std::uint16_t size)
    {
        if (m_data != nullptr)
        {
            delete[] m_data;
            m_data = nullptr;
        }
        m_data_size = size;
        m_data = new std::uint8_t[size ];
        std::memcpy(m_data, data, size);
    }

    std::uint16_t CMessage::GetSession()
    {
        return m_header.sessionId;
    }

    std::uint8_t CMessage::GetMission()
    {
        return m_command.mission;
    }

    std::uint16_t CMessage::GetOrder()
    {
        return m_command.order;
    }

    std::uint8_t CMessage::GetExtra()
    {
        return m_command.extra;
    }

    std::uint8_t CMessage::GetOption()
    {
        return m_command.option;
    }

    Protocols::STcpPacketHeader CMessage::GetHeader()
    {
        return m_header;
    }

    Protocols::SCommandHeader CMessage::GetCommand()
    {
        return m_command;
    }

    uint32_t CMessage::GetDataSize()
    {
        return m_data_size;
    }

    std::uint32_t CMessage::GetFullSize()
    {
        return sizeof(Protocols::STcpPacketHeader) + sizeof(Protocols::SCommandHeader) + m_data_size;
    }

    void CMessage::processMessage(std::uint8_t* data, std::uint16_t size)
    {
        Cryptography::CCrypt crypt;

        constexpr std::size_t headerSize = sizeof(Protocols::STcpPacketHeader);
        constexpr std::size_t commandSize = sizeof(Protocols::SCommandHeader);

        if (size < headerSize)
        {
            return;
        }

        if (m_crypt < 0)
        {
            memcpy_s(&m_header, headerSize, data, headerSize);
        }
        else
        {
            crypt.KeySetup(0);
            crypt.RC5Decrypt32(data, &m_header, headerSize);
        }

        std::uint16_t messageSize = static_cast<std::uint16_t>(m_header.size - headerSize);

        if (messageSize <= 0)
        {
            return;
        }

        std::uint8_t* decryptedBytes = (std::uint8_t*)std::malloc(messageSize * sizeof(std::uint8_t));

        switch (m_header.crypt)
        {
        case (std::uint32_t)Cryptography::EncryptionType::NO_ENCRYPTION:
            memcpy_s(decryptedBytes, messageSize, data + headerSize, messageSize);
            break;

        case (std::uint32_t)Cryptography::EncryptionType::DEFAULT_ENCRYPTION:
            crypt.KeySetup(0);
            crypt.RC5Decrypt64(data + headerSize, decryptedBytes, messageSize);
            break;

        case (std::uint32_t)Cryptography::EncryptionType::USER_ENCRYPTION:
            crypt.KeySetup(m_crypt);
            crypt.RC5Decrypt64(data + headerSize, decryptedBytes, messageSize);
            break;

        case (std::uint32_t)Cryptography::EncryptionType::DEFAULT_LARGE_ENCRYPTION:
            crypt.KeySetup(0);
            crypt.RC6Decrypt128(data + headerSize, decryptedBytes, messageSize);
            break;

        case (std::uint32_t)Cryptography::EncryptionType::USER_LARGE_ENCRYPTION:
            crypt.KeySetup(m_crypt);
            crypt.RC6Decrypt128(data + headerSize, decryptedBytes, messageSize);
            break;

        default:
            // Invalid packet
            break;
        }

#pragma warning(suppress : 6385)
        memcpy_s(&m_command, headerSize, decryptedBytes, headerSize);

        if (messageSize > commandSize)
        {
            this->SetData(decryptedBytes + commandSize, messageSize - commandSize);
        }
        else
        {
            this->SetData(nullptr, 0);
        }

        std::free(decryptedBytes);
    }

    void CMessage::generateBogus()
    {
        m_header.bogus = std::rand() % 262143 + 1;
    }

    std::uint8_t* CMessage::GenerateMessage()
    {
        if (m_crypt < 0)
        {
            m_header.crypt = (uint32_t)Cryptography::EncryptionType::NO_ENCRYPTION;
        }
        else
        {
            if (m_data_size + sizeof(Protocols::SCommandHeader) < 16 && m_encrypt_method == SendOption::EncryptionMethod::Default)
            {
                m_header.crypt = (std::uint32_t)Cryptography::EncryptionType::DEFAULT_ENCRYPTION;
            }
            else if (m_data_size + sizeof(Protocols::SCommandHeader) < 16 && m_encrypt_method == SendOption::EncryptionMethod::User)
            {
                m_header.crypt = (std::uint32_t)Cryptography::EncryptionType::USER_ENCRYPTION;
            }
            else if (m_data_size + sizeof(Protocols::SCommandHeader) >= 16 && m_encrypt_method == SendOption::EncryptionMethod::Default)
            {
                m_header.crypt = (std::uint32_t)Cryptography::EncryptionType::DEFAULT_LARGE_ENCRYPTION;
            }
            else if (m_data_size + sizeof(Protocols::SCommandHeader) >= 16 && m_encrypt_method == SendOption::EncryptionMethod::User)
            {
                m_header.crypt = (std::uint32_t)Cryptography::EncryptionType::USER_LARGE_ENCRYPTION;
            }
        }
        m_header.size = sizeof(Protocols::STcpPacketHeader) + sizeof(Protocols::SCommandHeader) + m_data_size;

        Cryptography::CCrypt crypt;

        constexpr std::size_t headerSize = sizeof(Protocols::STcpPacketHeader);
        constexpr std::size_t commandSize = sizeof(Protocols::SCommandHeader);

        std::size_t partialSize = commandSize + m_data_size;
        std::size_t completeSize = headerSize + commandSize + m_data_size;

        std::uint8_t* completeData = (std::uint8_t*)calloc(completeSize, sizeof(std::uint8_t));
        std::uint8_t* partialData = (std::uint8_t*)calloc(partialSize, sizeof(std::uint8_t));

        memcpy_s(partialData, commandSize, &m_command, commandSize);
        memcpy_s(partialData + headerSize, m_data_size, m_data, m_data_size);

        switch (m_header.crypt)
        {
        case (std::uint32_t)Cryptography::EncryptionType::NO_ENCRYPTION:
            memcpy_s(completeData + headerSize, partialSize, partialData, partialSize);
            break;

        case (std::uint32_t)Cryptography::EncryptionType::DEFAULT_ENCRYPTION:
            crypt.KeySetup(0);
            crypt.RC5Encrypt64(partialData, completeData + headerSize, static_cast<std::int32_t>(partialSize));
            break;

        case (std::uint32_t)Cryptography::EncryptionType::USER_ENCRYPTION:
            crypt.KeySetup(m_crypt);
            crypt.RC5Encrypt64(partialData, completeData + headerSize, static_cast<std::int32_t>(partialSize));
            break;

        case (std::uint32_t)Cryptography::EncryptionType::DEFAULT_LARGE_ENCRYPTION:
            crypt.KeySetup(0);
            crypt.RC6Encrypt128(partialData, completeData + headerSize, static_cast<std::int32_t>(partialSize));
            break;

        case (std::uint32_t)Cryptography::EncryptionType::USER_LARGE_ENCRYPTION:
            crypt.KeySetup(m_crypt);
            crypt.RC6Encrypt128(partialData, completeData + headerSize, static_cast<std::int32_t>(partialSize));
            break;

        default:
            // Invalid packet
            break;
        }

        memcpy_s(completeData, headerSize, &m_header, headerSize);

        if (m_crypt >= 0)
        {
            crypt.KeySetup(0);
            crypt.RC5Encrypt32(completeData, completeData, static_cast<std::int32_t>(headerSize));
        }

        std::free(partialData);
        return completeData;
    }
}