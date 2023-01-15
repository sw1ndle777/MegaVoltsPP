#pragma once
#ifndef PACKETDATA_H
#define PACKETDATA_H

#include <stdint.h>
#include <string.h>

namespace NetEngine
{
    namespace Packets
    {
        namespace Front
        {
#pragma pack(push, 1)

            union FrontServerInfo
            {
                struct
                {
                    uint8_t serverId : 8;
                    uint8_t channel1 : 2;
                    uint8_t channel2 : 2;
                    uint8_t channel3 : 2;
                    uint8_t channel4 : 2;
                    uint8_t channel5 : 2;
                    uint8_t channel6 : 2;
                    uint8_t channel7 : 2;
                    uint8_t channel8 : 2;
                    uint8_t channel9 : 2;
                    uint8_t channel10 : 2;
                };
                uint32_t data;

                FrontServerInfo()
                {
                    memset(this, 0, sizeof(FrontServerInfo));
                }

                FrontServerInfo(uint32_t data)
                {
                    memset(this, 0, sizeof(FrontServerInfo));
                    this->data = data;
                }
            };

            struct FrontUserAccountInfo
            {
                uint32_t  level;
                uint32_t  experience;
                uint32_t  kills;
                uint32_t  deaths;
                uint32_t  assists;
                uint32_t  wins;
                uint32_t  losses;
                uint32_t  draws;
                char      nickname[16];
                uint16_t  clanLogoFront;
                uint16_t  clanLogoBack;
                char      clanName[16];
                uint32_t  unknown;

                FrontUserAccountInfo()
                {
                    memset(this, 0, sizeof(FrontUserAccountInfo));
                }
            };

#pragma pack(pop)
        }

        namespace Main
        {
#pragma pack(push, 1)

#pragma pack(pop)
        }

        namespace Cast
        {
#pragma pack(push, 1)

#pragma pack(pop)
        }

        namespace Core
        {
#pragma pack(push, 1)

            union UniqueId
            {
                struct
                {
                    uint16_t session : 16;
                    uint16_t server : 15;
                    uint8_t  unknown : 1;
                };
                uint32_t data;

                UniqueId()
                {
                    memset(this, 0, sizeof(UniqueId));
                }

                UniqueId(uint32_t data)
                {
                    memset(this, 0, sizeof(UniqueId));
                    this->data = data;
                }
            };

#pragma pack(pop)
        }
    }
}

#endif