#pragma once
namespace Game::Handlers
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Cast;
    inline void RoomLeave(SCallbackData& callback, CCastServer* server)
    {
        auto session = callback.session;
        if (!session) return;

        DEBUGLOG(dark_cyan,
            "sid=({}) ignored cast ROOM_LEAVE_CAST because room lifecycle is synchronized from main IPC",
            session->GetSessionId());
    }
}
