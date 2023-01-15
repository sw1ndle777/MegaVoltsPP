#include "CMessage.h"

namespace NetEngine
{
    CMessage::CMessage(int32_t crypt_key)
    {
        m_data = nullptr;
        m_crypt = crypt_key;

        generateBogus();
    }

    CMessage::CMessage(uint8_t* data, uint16_t size, int32_t crypt_key)
    {
        m_data = nullptr;
        m_crypt = crypt_key;

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

    void CMessage::SetCommand(uint16_t order, uint8_t mission, uint8_t extra, uint8_t option)
    {
        m_command.order = order;
        m_command.mission = mission;
        m_command.extra = extra;
        m_command.option = option;
    }

    void CMessage::SetSession(uint16_t session)
    {
        m_header.sessionId = session;
    }

    void CMessage::SetMission(uint8_t mission)
    {
        m_command.mission = mission;
    }

    void CMessage::SetOrder(uint16_t order)
    {
        m_command.order = order;
    }

    void CMessage::SetExtra(uint8_t extra)
    {
        m_command.extra = extra;
    }

    void CMessage::SetOption(uint8_t option)
    {
        m_command.option = option;
    }

    void CMessage::SetData(uint8_t* data, uint16_t size)
    {
        if (m_data != nullptr)
        {
            delete[] m_data;
            m_data = nullptr;
        }

        m_data_size = size;
        m_data = new uint8_t[size];
        memcpy(m_data, data, size);
    }

    uint16_t CMessage::GetSession()
    {
        return m_header.sessionId;
    }

    uint8_t CMessage::GetMission()
    {
        return m_command.mission;
    }

    uint16_t CMessage::GetOrder()
    {
        return m_command.order;
    }

    uint8_t CMessage::GetExtra()
    {
        return m_command.extra;
    }

    uint8_t CMessage::GetOption()
    {
        return m_command.option;
    }

    uint32_t CMessage::GetDataSize()
    {
        return m_data_size;
    }

    uint32_t CMessage::GetFullSize()
    {
        return sizeof(Protocols::STcpPacketHeader) + sizeof(Protocols::SCommandHeader) + m_data_size;
    }

    void CMessage::processMessage(uint8_t* data, uint16_t size)
    {
        Cryptography::CCrypt crypt;

        size_t headerSize = sizeof(Protocols::STcpPacketHeader);
        size_t commandSize = sizeof(Protocols::SCommandHeader);

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

        uint16_t messageSize = m_header.size - headerSize;

        if (messageSize <= 0)
        {
            return;
        }

        uint8_t* decryptedBytes = (uint8_t*)malloc(messageSize * sizeof(uint8_t));

        switch (m_header.crypt)
        {
        case (uint32_t)Cryptography::EncryptionType::NO_ENCRYPTION:
            memcpy_s(decryptedBytes, messageSize, data + headerSize, messageSize);
            break;

        case (uint32_t)Cryptography::EncryptionType::DEFAULT_ENCRYPTION:
            crypt.KeySetup(0);
            crypt.RC5Decrypt64(data + headerSize, decryptedBytes, messageSize);
            break;

        case (uint32_t)Cryptography::EncryptionType::USER_ENCRYPTION:
            crypt.KeySetup(m_crypt);
            crypt.RC5Decrypt64(data + headerSize, decryptedBytes, messageSize);
            break;

        case (uint32_t)Cryptography::EncryptionType::DEFAULT_LARGE_ENCRYPTION:
            crypt.KeySetup(0);
            crypt.RC6Decrypt128(data + headerSize, decryptedBytes, messageSize);
            break;

        case (uint32_t)Cryptography::EncryptionType::USER_LARGE_ENCRYPTION:
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

        free(decryptedBytes);
    }

    void CMessage::generateBogus()
    {
        m_header.bogus = rand() % 262143 + 1;
    }

    uint8_t* CMessage::GenerateMessage()
    {
        if (m_crypt < 0)
        {
            m_header.crypt = (uint32_t)Cryptography::EncryptionType::NO_ENCRYPTION;
        }
        else
        {
            if (m_data_size + sizeof(Protocols::SCommandHeader) < 16 && m_crypt == 0)
            {
                m_header.crypt = (uint32_t)Cryptography::EncryptionType::DEFAULT_ENCRYPTION;
            }
            else if (m_data_size + sizeof(Protocols::SCommandHeader) < 16 && m_crypt > 0)
            {
                m_header.crypt = (uint32_t)Cryptography::EncryptionType::USER_ENCRYPTION;
            }
            else if (m_data_size + sizeof(Protocols::SCommandHeader) >= 16 && m_crypt == 0)
            {
                m_header.crypt = (uint32_t)Cryptography::EncryptionType::DEFAULT_LARGE_ENCRYPTION;
            }
            else if (m_data_size + sizeof(Protocols::SCommandHeader) >= 16 && m_crypt > 0)
            {
                m_header.crypt = (uint32_t)Cryptography::EncryptionType::USER_LARGE_ENCRYPTION;
            }
            else
            {
                //Invalid crypto
            }
        }

        m_header.size = sizeof(Protocols::STcpPacketHeader) + sizeof(Protocols::SCommandHeader) + m_data_size;

        Cryptography::CCrypt crypt;

        size_t headerSize = sizeof(Protocols::STcpPacketHeader);
        size_t commandSize = sizeof(Protocols::SCommandHeader);

        size_t partialSize = commandSize + m_data_size;
        size_t completeSize = headerSize + commandSize + m_data_size;

        uint8_t* completeData = (uint8_t*)calloc(completeSize, sizeof(uint8_t));
        uint8_t* partialData = (uint8_t*)calloc(partialSize, sizeof(uint8_t));

        memcpy_s(partialData, commandSize, &m_command, commandSize);
        memcpy_s(partialData + headerSize, m_data_size, m_data, m_data_size);

        switch (m_header.crypt)
        {
        case (uint32_t)Cryptography::EncryptionType::NO_ENCRYPTION:
            memcpy_s(completeData + headerSize, partialSize, partialData, partialSize);
            break;

        case (uint32_t)Cryptography::EncryptionType::DEFAULT_ENCRYPTION:
            crypt.KeySetup(0);
            crypt.RC5Encrypt64(partialData, completeData + headerSize, partialSize);
            break;

        case (uint32_t)Cryptography::EncryptionType::USER_ENCRYPTION:
            crypt.KeySetup(m_crypt);
            crypt.RC5Encrypt64(partialData, completeData + headerSize, partialSize);
            break;

        case (uint32_t)Cryptography::EncryptionType::DEFAULT_LARGE_ENCRYPTION:
            crypt.KeySetup(0);
            crypt.RC6Encrypt128(partialData, completeData + headerSize, partialSize);
            break;

        case (uint32_t)Cryptography::EncryptionType::USER_LARGE_ENCRYPTION:
            crypt.KeySetup(m_crypt);
            crypt.RC6Encrypt128(partialData, completeData + headerSize, partialSize);
            break;

        default:
            // Invalid packet
            break;
        }

        memcpy_s(completeData, headerSize, &m_header, headerSize);

        if (m_crypt >= 0)
        {
            crypt.KeySetup(0);
            crypt.RC5Encrypt32(completeData, completeData, headerSize);
        }

        free(partialData);
        return completeData;
    }
}