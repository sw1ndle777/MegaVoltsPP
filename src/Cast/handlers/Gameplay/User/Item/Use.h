#pragma once
namespace Game::Handlers
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Cast;
    inline void UserItemUse(SCallbackData& callback, CCastServer* server)
    {
        auto session = callback.session;
        auto message = callback.message;
        if (!session || !message) return;

        auto hostSid = message->GetSession();
        auto sid = session->GetSessionId();

        PACKETLOG(REQ, ITEM_USE, "sid=({}) hostSid=({})", sid, hostSid);

        server->Forward(hostSid, sid, *message);
    }
}