#pragma once
namespace Game::Handlers
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Cast;
    inline void RoomJoin(SCallbackData& callback, CCastServer* server)
    {
        auto session = callback.session;
        auto message = callback.message;
        if (!session || !message) return;

        DEBUGLOG(dark_cyan,
            "sid=({}) ignored cast ROOM_JOIN hostSid=({}) because room lifecycle is synchronized from main IPC",
            session->GetSessionId(),
            message->GetSession());
    }
}
