#ifndef BASEPROTOCOL_H
#define BASEPROTOCOL_H

#include <stdint.h>
#include <string.h>

namespace NetEngine
{
    namespace Protocols
    {
#pragma pack(push, 1)

        union STcpPacketHeader
        {
            struct
            {
                uint32_t bogus : 4;
                uint32_t sessionId : 14;
                uint32_t size : 11;
                uint32_t crypt : 3;
            };
            uint32_t data;

            STcpPacketHeader()
            {
                memset(this, 0, sizeof(STcpPacketHeader));
            }

            STcpPacketHeader(uint32_t data)
            {
                memset(this, 0, sizeof(STcpPacketHeader));
                this->data = data;
            }
        };

        union SCommandHeader
        {
            struct
            {
                uint32_t bugus : 4;
                uint32_t mission : 2;
                uint32_t order : 10;
                uint32_t extra : 8;
                uint32_t option : 8;
            };
            uint32_t data;

            SCommandHeader()
            {
                memset(this, 0, sizeof(SCommandHeader));
            }

            SCommandHeader(uint32_t data)
            {
                memset(this, 0, sizeof(SCommandHeader));
                this->data = data;
            }
        };

#pragma pack(pop)
    }
}

#endif