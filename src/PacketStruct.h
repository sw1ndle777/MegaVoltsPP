#pragma once
#ifndef PACKETSTRUCT_H
#define PACKETSTRUCT_H

#include <stdint.h>
#include <string.h>
#include <vector>

#include "PacketData.h"

namespace NetEngine
{
    namespace Packets
    {
        namespace Front
        {
#pragma pack(push, 1)

            // C2S

            struct FrontLoginAuthorizeReq
            {
                uint32_t  cryptoKey;
                char      password[40];
                uint32_t  serverTime;
                char      username[68];

                FrontLoginAuthorizeReq()
                    : cryptoKey(0), serverTime(0)
                {
                    memset(password, 0, sizeof(password));
                    memset(username, 0, sizeof(username));
                }

                FrontLoginAuthorizeReq(uint32_t cryptoKey, char password[], uint32_t serverTime, char username[])
                    : cryptoKey(cryptoKey), serverTime(serverTime)
                {
                    memcpy(this->password, password, sizeof(this->password));
                    memcpy(this->username, username, sizeof(this->username));
                }

                FrontLoginAuthorizeReq(uint32_t cryptoKey, const char* password, uint32_t serverTime, const char* username)
                    : cryptoKey(cryptoKey), serverTime(serverTime)
                {
                    memcpy(this->password, password, sizeof(this->password));
                    memcpy(this->username, username, sizeof(this->username));
                }
            };

            struct FrontLoginReconnectReq
            {
                uint64_t authKey;

                FrontLoginReconnectReq(uint64_t authKey)
                    : authKey(authKey)
                {
                }
            };

            struct FrontServerInfoReq
            {
            };

            // S2C

            struct FrontEngineServerConnectionAck
            {
                uint32_t cryptoKey;
                uint32_t serverTime;

                FrontEngineServerConnectionAck(uint32_t cryptoKey, uint32_t serverTime)
                    : cryptoKey(cryptoKey), serverTime(serverTime)
                {
                }
            };

            struct FrontLoginAuthorizeAck
            {
                uint64_t              authKey;
                FrontUserAccountInfo  accountInfo;

                FrontLoginAuthorizeAck(uint64_t authKey, FrontUserAccountInfo accountInfo)
                    : authKey(authKey), accountInfo(accountInfo)
                {
                }
            };

            struct FrontLoginReconnectAck
            {
                uint64_t authKey;

                FrontLoginReconnectAck(uint64_t authKey)
                    : authKey(authKey)
                {
                }
            };

            struct FrontServerInfoAck
            {
                FrontServerInfo serverInfos[];

                FrontServerInfoAck(std::vector<FrontServerInfo> serverInfos)
                {
                    memset(this, 0, sizeof(FrontServerInfoAck));
                    std::copy(serverInfos.begin(), serverInfos.end(), this->serverInfos);
                }

                FrontServerInfoAck(uint8_t* data, size_t size)
                {
                    memset(this, 0, sizeof(FrontServerInfoAck));

                    size_t structSize = sizeof(FrontServerInfo);
                    size_t elementCount = size / sizeof(FrontServerInfo);

                    for (size_t i = 0; i < elementCount; i++)
                    {
                        this->serverInfos[i] = *(FrontServerInfo*)(data + i * structSize);
                    }
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

#pragma pack(pop)
        }
    }
}

#endif