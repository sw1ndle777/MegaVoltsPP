#pragma once
namespace Game::Handlers
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Cast;
    inline void NodeCastQuit(SCallbackData& callback, CCastServer* server)
    {
        auto session = callback.session;
        auto message = callback.message;
        if (!session || !message) return;

        auto hostSid = message->GetSession();
        auto sid = session->GetSessionId();
        server->Forward(hostSid, sid, *message);
        auto acc = CAccount.get<unique_t>(sid);
        if (!acc) return;
        if (acc->in_room)
            DEBUGLOG(dark_cyan, "user=({}) sid=({}) requested cast quit in roomId=({}), waiting for main room lifecycle sync", acc->nickname, sid, acc->room_id);
        acc->state_id = PlayerInfo::State::Disconnected;
    }
}
