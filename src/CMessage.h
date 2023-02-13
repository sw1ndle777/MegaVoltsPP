#pragma once
#ifndef CMESSAGE_H
#define CMESSAGE_H

#include <stdlib.h>

#include "BaseProtocol.h"
#include "Constants.h"
#include "CCrypt.h"

namespace NetEngine
{
    class CMessage
    {
    public:
        CMessage(int32_t crypt_key = -1);
        CMessage(uint8_t* data, uint16_t size, int32_t crypt_key = -1);

        ~CMessage();

        void SetCommand(uint16_t order, uint8_t mission, uint8_t extra, uint8_t option);

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
        Protocols::STcpPacketHeader GetHeader();
        Protocols::SCommandHeader GetCommand();
        uint32_t GetDataSize();
        uint32_t GetFullSize();

        void SetData(uint8_t* data, uint16_t size);

        uint8_t* GetData()
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
            m_data = reinterpret_cast<uint8_t*>(data);
            m_data_size = sizeof(data);
        }

        uint8_t* GenerateMessage();
    private:
        void processMessage(uint8_t* data, uint16_t size);
        void generateBogus();

    private:
        Protocols::STcpPacketHeader m_header;
        Protocols::SCommandHeader m_command;

        uint8_t* m_data;

        int32_t m_data_size;
        int32_t m_crypt;
    };
}

#endif