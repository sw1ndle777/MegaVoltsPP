#pragma once
#ifndef CFRONTSERVER_H
#define CFRONTSERVER_H

#include <string>
#include <functional>

#include "CLog.h"
#include "CServer.h"
#include "CSession.h"
#include "Constants.h"

#include "PacketStruct.h"
#include "PacketData.h"

namespace Game
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Front;
    
    class CFrontServer : public NetEngine::CServer
    {
    public:
        CFrontServer();
        ~CFrontServer();

    private: //Callbacks
        static void handle_FrontEngineServerConnection(std::shared_ptr<CSession> session);
        static void handle_FrontEngineServerDisconnection(std::shared_ptr<CSession> session);
        static void handle_FrontLoginAuthorizeReq(SCallbackData& callback);
        static void handle_FrontLoginReconnectReq(SCallbackData& callback);
        static void handle_FrontServerInfoReq(SCallbackData& callback);
    };
}

#endif