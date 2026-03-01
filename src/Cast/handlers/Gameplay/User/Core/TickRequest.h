#pragma once
namespace Game::Handlers
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Cast;
    inline void UserTickRequest(SCallbackData& callback, CCastServer* server) // INFO_REQUEST_NEW
    {
        auto session = callback.session;
        auto message = callback.message;
        if (!session || !message) return;

        auto sid = session->GetSessionId();
        auto hostSid = message->GetSession();
		//PACKETLOG(REQ, USER_SERVER_TICK, "sid=({}) hostSid=({})", sid, hostSid);
        server->Forward(hostSid, sid, *message); 
    }
}