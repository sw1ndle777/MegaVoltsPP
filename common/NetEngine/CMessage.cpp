#include "CMessage.h"

namespace NetEngine
{
    CMessage::CMessage(std::int32_t crypt_key)
    {
        resizeBuffer(0);
        m_crypt = crypt_key;

        generateBogus();
    }

    CMessage::CMessage(std::uint8_t* data, std::uint16_t size, std::int32_t crypt_key)
    {
        resizeBuffer(size);
        m_crypt = crypt_key;
        m_encrypt_method = SendOption::EncryptionMethod::User;
        this->processMessage(data, size);
    }

    void CMessage::SetCommand(std::uint16_t order, std::uint8_t mission, std::uint8_t extra, std::uint8_t option)
    {
        m_command->order = order;
        m_command->mission = mission;
        m_command->extra = extra;
        m_command->option = option;
    }

    void CMessage::SetEncryptMethod(SendOption::EncryptionMethod method)
    {
        m_encrypt_method = method;
    }

    void CMessage::SetSession(std::uint16_t session)
    {
        m_header->sessionId = session;
    }

    void CMessage::SetMission(std::uint8_t mission)
    {
        m_command->mission = mission;
    }

    void CMessage::SetOrder(std::uint16_t order)
    {
        m_command->order = order;
    }

    void CMessage::SetExtra(std::uint8_t extra)
    {
        m_command->extra = extra;
    }

    void CMessage::SetOption(std::uint8_t option)
    {
        m_command->option = option;
    }

    void CMessage::SetData(std::uint8_t* data, std::uint16_t size)
    {
        resizeBuffer(size);
        std::memcpy(m_buffer.data() + dataOffset(), data, size);
    }

    std::uint16_t CMessage::GetSession()
    {
        return m_header->sessionId;
    }

    std::uint8_t CMessage::GetMission()
    {
        return m_command->mission;
    }

    std::uint16_t CMessage::GetOrder()
    {
        return m_command->order;
    }

    std::uint8_t CMessage::GetExtra()
    {
        return m_command->extra;
    }

    std::uint8_t CMessage::GetOption()
    {
        return m_command->option;
    }

    Protocols::STcpPacketHeader& CMessage::GetHeader()
    {
        return *m_header;
    }

    Protocols::SCommandHeader& CMessage::GetCommand()
    {
        return *m_command;
    }

    uint32_t CMessage::GetDataSize()
    {
        return GetFullSize() - dataOffset();
    }

    std::uint32_t CMessage::GetFullSize()
    {
        return m_buffer.size();
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

        resizeBuffer(size);

        if (m_crypt < 0)
        {
            memcpy_s(m_header, headerSize, data, headerSize);
            // m_header.data = crypt.decrypt_tcp_header(m_header.data);
        }
        else
        {
            crypt.KeySetup(0);
            crypt.RC5Decrypt32(data, m_header, headerSize);
            //m_header.data = crypt.decrypt_tcp_header(m_header.data);
        }

        std::uint16_t messageSize = static_cast<std::uint16_t>(m_header.size - headerSize);

        if (messageSize <= 0)
        {
            return;
        }

        thread_local std::vector<uint8_t> decryptBuffer;
        decryptBuffer.resize(messageSize * sizeof(std::uint8_t));

        switch (m_header.crypt)
        {
        case (std::uint32_t)Cryptography::EncryptionType::NO_ENCRYPTION:
            memcpy_s(decryptBuffer.data(), messageSize, data + headerSize, messageSize);
            break;

        case (std::uint32_t)Cryptography::EncryptionType::DEFAULT_ENCRYPTION:
            crypt.KeySetup(0);
            crypt.RC5Decrypt64(data + headerSize, decryptBuffer.data(), messageSize);
            break;

        case (std::uint32_t)Cryptography::EncryptionType::USER_ENCRYPTION:
            crypt.KeySetup(m_crypt);
            crypt.RC5Decrypt64(data + headerSize, decryptBuffer.data(), messageSize);
            break;

        case (std::uint32_t)Cryptography::EncryptionType::DEFAULT_LARGE_ENCRYPTION:
            crypt.KeySetup(0);
            crypt.RC6Decrypt128(data + headerSize, decryptBuffer.data(), messageSize);
            break;

        case (std::uint32_t)Cryptography::EncryptionType::USER_LARGE_ENCRYPTION:
            crypt.KeySetup(m_crypt);
            crypt.RC6Decrypt128(data + headerSize, decryptBuffer.data(), messageSize);
            break;

        default:
            // Invalid packet
            break;
        }

#pragma warning(suppress : 6385)
        memcpy_s(m_command, headerSize, decryptBuffer.data(), headerSize);

        if (messageSize > commandSize)
        {
            this->SetData(decryptBuffer.data() + commandSize, messageSize - commandSize);
        }
        else
        {
            this->SetData(nullptr, 0);
        }
    }

    void CMessage::generateBogus()
    {
        m_header.bogus = std::rand() % 262143 + 1;
    }

    std::vector<std::uint8_t> CMessage::GenerateMessage()
    {
        if (m_crypt < 0)
        {
            m_header->crypt = (uint32_t)Cryptography::EncryptionType::NO_ENCRYPTION;
        }
        else
        {
            if (GetDataSize() + sizeof(Protocols::SCommandHeader) < 16 && m_encrypt_method == SendOption::EncryptionMethod::Default)
            {
                m_header->crypt = (std::uint32_t)Cryptography::EncryptionType::DEFAULT_ENCRYPTION;
            }
            else if (GetDataSize() + sizeof(Protocols::SCommandHeader) < 16 && m_encrypt_method == SendOption::EncryptionMethod::User)
            {
                m_header->crypt = (std::uint32_t)Cryptography::EncryptionType::USER_ENCRYPTION;
            }
            else if (GetDataSize() + sizeof(Protocols::SCommandHeader) >= 16 && m_encrypt_method == SendOption::EncryptionMethod::Default)
            {
                m_header->crypt = (std::uint32_t)Cryptography::EncryptionType::DEFAULT_LARGE_ENCRYPTION;
            }
            else if (GetDataSize() + sizeof(Protocols::SCommandHeader) >= 16 && m_encrypt_method == SendOption::EncryptionMethod::User)
            {
                m_header->crypt = (std::uint32_t)Cryptography::EncryptionType::USER_LARGE_ENCRYPTION;
            }
        }
        

        Cryptography::CCrypt crypt;

        constexpr std::size_t headerSize = sizeof(Protocols::STcpPacketHeader);
        constexpr std::size_t commandSize = sizeof(Protocols::SCommandHeader);

        std::size_t partialSize = commandSize + GetDataSize();
        std::size_t completeSize = GetFullSize();

        // todo: "inline" encrypt m_buffer, instead of temporary buffers

        thread_local std::vector<uint8_t> completeVec;
        thread_local std::vector<uint8_t> partialVec;
        completeVec.resize(completeSize);
        partialVec.resize(partialSize);

        memcpy_s(partialVec.data(), commandSize, &m_command, commandSize);
        memcpy_s(partialVec.data() + headerSize, GetDataSize(), m_buffer.data() + dataOffset(), GetDataSize());

        switch (m_header.crypt)
        {
        case (std::uint32_t)Cryptography::EncryptionType::NO_ENCRYPTION:
            memcpy_s(completeVec.data() + headerSize, partialSize, partialVec.data(), partialSize);
            break;

        case (std::uint32_t)Cryptography::EncryptionType::DEFAULT_ENCRYPTION:
            crypt.KeySetup(0);
            crypt.RC5Encrypt64(partialVec.data(), completeVec.data() + headerSize, static_cast<std::int32_t>(partialSize));
            break;

        case (std::uint32_t)Cryptography::EncryptionType::USER_ENCRYPTION:
            crypt.KeySetup(m_crypt);
            crypt.RC5Encrypt64(partialVec.data(), completeVec.data() + headerSize, static_cast<std::int32_t>(partialSize));
            break;

        case (std::uint32_t)Cryptography::EncryptionType::DEFAULT_LARGE_ENCRYPTION:
            crypt.KeySetup(0);
            crypt.RC6Encrypt128(partialVec.data(), completeVec.data() + headerSize, static_cast<std::int32_t>(partialSize));
            break;

        case (std::uint32_t)Cryptography::EncryptionType::USER_LARGE_ENCRYPTION:
            crypt.KeySetup(m_crypt);
            crypt.RC6Encrypt128(partialVec.data(), completeVec.data() + headerSize, static_cast<std::int32_t>(partialSize));
            break;

        default:
            // Invalid packet
            break;
        }

        //m_header.data = crypt.encrypt_tcp_header(m_header.data);
        memcpy_s(completeVec.data(), headerSize, &m_header, headerSize);

        if (m_crypt >= 0)
        {
            crypt.KeySetup(0);
            auto new_data = crypt.encrypt_tcp_header(m_header.data);
            memcpy_s(completeVec.data(), headerSize, &new_data, headerSize);
            crypt.RC5Encrypt32(completeVec.data(), completeVec.data(), static_cast<std::int32_t>(headerSize));
        }

        return std::move(completeVec);
    }
}