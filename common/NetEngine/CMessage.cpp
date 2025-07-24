#include "CMessage.h"
#include <stdexcept>
#include <format>
namespace NetEngine
{
    CMessage::CMessage(int32_t crypt_key)
    {
        m_buffer.resize(minSize());
        m_header = (Protocols::STcpPacketHeader*)m_buffer.data();
        m_command = (Protocols::SCommandHeader*)(m_header + 1);
        m_crypt = crypt_key;
        generateBogus();
    }
    CMessage::CMessage(uint8_t* data, uint16_t size, int32_t crypt_key)
    {
        m_buffer.resize(minSize() + size);
        m_header = (Protocols::STcpPacketHeader*)m_buffer.data();
        m_command = (Protocols::SCommandHeader*)(m_header + 1);
        m_crypt = crypt_key;
        m_encrypt_method = SendOption::EncryptionMethod::User;
        this->ProcessMessage(data, size);
    }
    CMessage::~CMessage()
    {
    }
    void CMessage::SetCommand(uint16_t order, uint8_t mission, uint8_t extra, uint8_t option)
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
    void CMessage::SetSession(uint16_t session)
    {
        m_header->sessionId = session;
    }
    void CMessage::SetMission(uint8_t mission)
    {
        m_command->mission = mission;
    }
    void CMessage::SetOrder(uint16_t order)
    {
        m_command->order = order;
    }
    void CMessage::SetExtra(uint8_t extra)
    {
        m_command->extra = extra;
    }
    void CMessage::SetOption(uint8_t option)
    {
        m_command->option = option;
    }
    void CMessage::SetData(uint8_t* data, uint16_t size)
    {
	   m_buffer.resize(minSize() + size);
       m_header = (Protocols::STcpPacketHeader*)m_buffer.data();
       m_command = (Protocols::SCommandHeader*)(m_header + 1);
       memcpy(m_buffer.data() + dataOffset(), data, size);
    }
    uint16_t CMessage::GetSession()
    {
        return m_header->sessionId;
    }
    uint8_t CMessage::GetMission()
    {
        return m_command->mission;
    }
    uint16_t CMessage::GetOrder()
    {
        return m_command->order;
    }
    uint8_t CMessage::GetExtra()
    {
        return m_command->extra;
    }
    uint8_t CMessage::GetOption()
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
        return GetFullSize() - static_cast<uint32_t>(minSize());
    }
    uint32_t CMessage::GetFullSize()
    {
        return static_cast<uint32_t>(m_buffer.size());
    }
    void CMessage::ProcessMessage(uint8_t* data, uint16_t size)
    {
        if (size < tcp_header_size)  return;

        Cryptography::CCrypt crypt;
        m_buffer.resize(minSize() + size);
        m_header = (Protocols::STcpPacketHeader*)m_buffer.data();
        m_command = (Protocols::SCommandHeader*)(m_header + 1);

        if (m_crypt < 0)
        {
            memcpy(m_header, data, tcp_header_size);
            //m_header.data = crypt.decrypt_tcp_header(m_header.data);
        }
        else
        {
            crypt.KeySetup(0);
            crypt.RC5Decrypt32(data, m_header, tcp_header_size);
            //m_header.data = crypt.decrypt_tcp_header(m_header.data);
        }
        const auto messageSize = static_cast<uint16_t>(m_header->size - tcp_header_size);
        if (messageSize <= 0) return;

        thread_local std::vector<std::uint8_t> decrypted_data;
		decrypted_data.resize(messageSize);
		
        switch (m_header->crypt)
        {
        case Cryptography::EncryptionType::NO_ENCRYPTION:
            memcpy(decrypted_data.data(), data + tcp_header_size, messageSize);
            break;
        case Cryptography::EncryptionType::DEFAULT_ENCRYPTION:
            crypt.KeySetup(0);
            crypt.RC5Decrypt64(data + tcp_header_size, decrypted_data.data(), messageSize);
            break;
        case Cryptography::EncryptionType::USER_ENCRYPTION:
            crypt.KeySetup(m_crypt);
            crypt.RC5Decrypt64(data + tcp_header_size, decrypted_data.data(), messageSize);
            break;
        case Cryptography::EncryptionType::DEFAULT_LARGE_ENCRYPTION:
            crypt.KeySetup(0);
            crypt.RC6Decrypt128(data + tcp_header_size, decrypted_data.data(), messageSize);
            break;
        case Cryptography::EncryptionType::USER_LARGE_ENCRYPTION:
            crypt.KeySetup(m_crypt);
            crypt.RC6Decrypt128(data + tcp_header_size, decrypted_data.data(), messageSize);
            break;
        default:
            break;
        }
#pragma warning(suppress : 6385)
        memcpy(m_command, decrypted_data.data(), tcp_header_size);

        if (messageSize > command_header_size)
        {
            SetData(decrypted_data.data() + command_header_size, messageSize - command_header_size);
        }
        else
        {
            SetData(nullptr, 0);
        }
    }
    void CMessage::generateBogus()
    {
        m_header->bogus = std::rand() % 262143 + 1;
		m_command->bogus = std::rand() % 262143 + 1;
    }
    std::shared_ptr<std::vector<uint8_t>> CMessage::GenerateMessage()
    {
        const auto data_size = dataSize();
        const auto partial_size = data_size + command_header_size;
        if (m_crypt < 0)
            m_header->crypt = Cryptography::EncryptionType::NO_ENCRYPTION;
        else
        {
            if (partial_size < 16 && m_encrypt_method == SendOption::EncryptionMethod::Default)
                m_header->crypt = Cryptography::EncryptionType::DEFAULT_ENCRYPTION;
            else if (partial_size < 16 && m_encrypt_method == SendOption::EncryptionMethod::User)
                m_header->crypt = Cryptography::EncryptionType::USER_ENCRYPTION;
            else if (partial_size >= 16 && m_encrypt_method == SendOption::EncryptionMethod::Default)
                m_header->crypt = Cryptography::EncryptionType::DEFAULT_LARGE_ENCRYPTION;
            else if (partial_size >= 16 && m_encrypt_method == SendOption::EncryptionMethod::User)
                m_header->crypt = Cryptography::EncryptionType::USER_LARGE_ENCRYPTION;
        }
        m_header->size = partial_size + tcp_header_size;

        auto* partial_data = m_buffer.data() + tcp_header_size;

        Cryptography::CCrypt crypt;

        switch (m_header->crypt)
        {
        case Cryptography::EncryptionType::NO_ENCRYPTION:
            break;
        case Cryptography::EncryptionType::DEFAULT_ENCRYPTION:
            crypt.KeySetup(0);
            crypt.RC5Encrypt64(partial_data, partial_data, partial_size);
            break;
        case Cryptography::EncryptionType::USER_ENCRYPTION:
            crypt.KeySetup(m_crypt);
            crypt.RC5Encrypt64(partial_data, partial_data, partial_size);
            break;
        case Cryptography::EncryptionType::DEFAULT_LARGE_ENCRYPTION:
            crypt.KeySetup(0);
            crypt.RC6Encrypt128(partial_data, partial_data, partial_size);
            break;
        case Cryptography::EncryptionType::USER_LARGE_ENCRYPTION:
            crypt.KeySetup(m_crypt);
            crypt.RC6Encrypt128(partial_data, partial_data, partial_size);
            break;
        default:
            break;
        }

        if (m_crypt >= 0)
        {
            crypt.KeySetup(0);
            m_header->data = crypt.encrypt_tcp_header(m_header->data);
            crypt.RC5Encrypt32(m_buffer.data(), m_buffer.data(), tcp_header_size);
        }
        
        return std::make_shared<std::vector<uint8_t>>(m_buffer);
        //return m_buffer;
    }
}