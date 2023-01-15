#pragma once
#ifndef CFRONTCLIENT_H
#define CFRONTCLIENT_H

#include <string>
#include <functional>

#include "CLog.h"
#include "CClient.h"
#include "CSession.h"
#include "Constants.h"

#include "PacketStruct.h"
#include "PacketData.h"

namespace Game
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Front;

    class CFrontClient : public NetEngine::CClient
    {
    public:
        CFrontClient();
        ~CFrontClient();

    private: //Callbacks
        static void handle_FrontEngineServerConnectionAck(SCallbackData& callback);
        static void handle_FrontLoginAuthorizeAck(SCallbackData& callback);
        static void handle_FrontLoginReconnectAck(SCallbackData& callback);
        static void handle_FrontServerInfoAck(SCallbackData& callback);
    };
}

#endif