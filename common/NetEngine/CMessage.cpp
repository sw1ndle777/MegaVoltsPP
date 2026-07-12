#include "CMessage.h"
#include <stdexcept>
#include <format>
#include "../BaseLib/Utility.h"
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
    CMessage::~CMessage(){}

    void CMessage::SetEncryptMethod(SendOption::EncryptionMethod method)
    {
        m_encrypt_method = method;
    }
    void CMessage::SetSession(uint16_t session)
    {
        m_header->sessionId = session;
    }
    void CMessage::SetData(uint8_t* data, uint16_t size)
    {
	   m_buffer.resize(minSize() + size);
       m_header = (Protocols::STcpPacketHeader*)m_buffer.data();
       m_command = (Protocols::SCommandHeader*)(m_header + 1);
       std::memcpy(m_buffer.data() + dataOffset(), data, size);
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
        m_buffer.resize(minSize() + size);
        m_header = (Protocols::STcpPacketHeader*)m_buffer.data();
        m_command = (Protocols::SCommandHeader*)(m_header + 1);
		using enum CCrypt::CRYPT_TYPE;
        if (m_crypt < 0) std::memcpy(m_header, data, tcp_header_size);
        else
        {
			CCrypt crypt(CRYPT_RC5, 0);
            crypt.Decrypt(data, m_header, tcp_header_size);
        }
        const auto messageSize = static_cast<uint16_t>(m_header->size - tcp_header_size);
        if (messageSize <= 0) return;

        thread_local std::vector<std::uint8_t> decrypted_data;
		decrypted_data.resize(messageSize);
        const auto crypt_type = static_cast<CCrypt::CRYPT_TYPE>(m_header->crypt);

        // The header crypt field is 3 bits (0..7) but only CRYPT_NONE..CRYPT_RC6_SERIAL
        // are valid ciphers. A malformed, framing-desynced or crafted packet can carry
        // 5..7; drop it here rather than constructing CCrypt with an out-of-range type.
        if (to_u(crypt_type) > to_u(CRYPT_RC6_SERIAL))
            return;

        if (crypt_type != CRYPT_NONE)
        {
            const auto key = (crypt_type == CRYPT_RC5_SERIAL || crypt_type == CRYPT_RC6_SERIAL) ? m_crypt : 0;
            CCrypt crypt(crypt_type, key);
            crypt.Decrypt(data + tcp_header_size, decrypted_data.data(), messageSize);
        }
        else
            std::memcpy(decrypted_data.data(), data + tcp_header_size, messageSize);

        std::memcpy(m_command, decrypted_data.data(), tcp_header_size);
        if (messageSize > command_header_size)
            SetData(decrypted_data.data() + command_header_size, messageSize - command_header_size);
        else
            SetData(nullptr, 0);
    }
    void CMessage::generateBogus()
    {
        m_header->bogus = Utility::Random::CustomGen(1, 255) & 0xF;
		m_command->bogus = Utility::Random::CustomGen(1, 255) & 0xF;
    }
    std::shared_ptr<std::vector<uint8_t>> CMessage::GenerateMessage()
    {
        const auto data_size = dataSize();
        const auto partial_size = data_size + command_header_size;
        using enum CCrypt::CRYPT_TYPE;
        using enum SendOption::EncryptionMethod;
        if (m_crypt < 0)
            m_header->crypt = to_u(CRYPT_NONE);
        else
        {
            if (partial_size < 16 && m_encrypt_method == Default)
                m_header->crypt = to_u(CRYPT_RC5);
            else if (partial_size < 16 && m_encrypt_method == User)
                m_header->crypt = to_u(CRYPT_RC5_SERIAL);
            else if (partial_size >= 16 && m_encrypt_method == Default)
                m_header->crypt = to_u(CRYPT_RC6);
            else if (partial_size >= 16 && m_encrypt_method == User)
                m_header->crypt = to_u(CRYPT_RC6_SERIAL); 
        }
        m_header->size = partial_size + tcp_header_size;

        auto* partial_data = m_buffer.data() + tcp_header_size;
		const auto crypt_type = static_cast<CCrypt::CRYPT_TYPE>(m_header->crypt);
        if (crypt_type != CRYPT_NONE)
        {
            const auto key = (crypt_type == CRYPT_RC5_SERIAL || crypt_type == CRYPT_RC6_SERIAL) ? m_crypt : 0;
            CCrypt crypt(crypt_type, key);
            crypt.Encrypt(partial_data, partial_data, partial_size);
        }
        if (m_crypt >= 0)
        {
            m_header->data = encrypt_tcp_header(m_header->data);
            CCrypt crypt(CRYPT_RC5, 0);
            crypt.Encrypt(m_buffer.data(), m_buffer.data(), tcp_header_size);
        }
        return std::make_shared<std::vector<uint8_t>>(m_buffer);
        //return m_buffer;
    }
}