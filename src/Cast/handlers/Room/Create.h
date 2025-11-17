#pragma once
namespace Game::Handlers
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Cast;
    inline void RoomCreate(SCallbackData& callback, CCastServer* server)
    {
        auto session = callback.session;
        if (!session) return;

        auto sid = session->GetSessionId();

        uint16_t roomId = 0;
        server->GetNextAvailableRoomId(roomId);
        Game::Room room = { roomId , sid };
        room.players_session_id.push_back(sid);
        CRoom.insert(roomId, room);
        CRoomId.emplace_back(roomId);

        auto acc = CAccount.get<unique_t>(sid);
        acc->room_id = roomId;
        acc->in_room = true;
        acc->state_id = PlayerInfo::State::WaitingRoom;
        acc.unlock();
        DEBUGLOG(dark_cyan, "sid=({}) created roomId=({})", sid, roomId);
    }
}