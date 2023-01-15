#pragma once
#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <stdint.h>
#include <optional>

namespace NetEngine
{
    class CSession;
    class CMessage;

    struct SCallbackData
    {
        CSession* session;
        CMessage* message;
    };

    namespace Cryptography
    {
        enum class EncryptionType
        {
            NO_ENCRYPTION = 0,
            DEFAULT_ENCRYPTION = 1,
            USER_ENCRYPTION = 2,
            DEFAULT_LARGE_ENCRYPTION = 3,
            USER_LARGE_ENCRYPTION = 4
        };
    }
}

#endif