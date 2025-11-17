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

        auto sid = session->GetSessionId();
        auto acc = CAccount.get<unique_t>(sid);
        if (!acc->in_room) return;
        auto roomId = acc->room_id;
        acc->state_id = PlayerInfo::State::Lobby;
        acc->in_room = false;
        acc.unlock();

        if (CRoom.contains(roomId)) return;
        auto room = CRoom.get<unique_t>(roomId);
        std::erase_if(room->players_session_id, [&](const uint16_t& id) { return id == sid; });
        DEBUGLOG(dark_cyan, "sid=({}) left roomId=({})", sid, roomId);
        if (!room->players_session_id.empty()) return;
        room.unlock();
        CRoom.erase(roomId);
        CRoomId.erase_value(roomId);
        server->SetRoomIdAvailable(roomId);
        DEBUGLOG(dark_cyan, "sid=({}) removed roomId=({})", sid, roomId);
    }
}