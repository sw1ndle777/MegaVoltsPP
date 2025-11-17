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

        auto hostSid = message->GetSession();
        auto sid = session->GetSessionId();

        DEBUGLOG(dark_cyan, "sid=({}) attempt to join hostSid=({})'s room", sid, hostSid);

        auto host_acc = CAccount.get<shared_t>(hostSid);
        uint16_t roomId = host_acc->room_id;
        host_acc.unlock();
        auto room = CRoom.get<unique_t>(roomId);

        if (std::ranges::contains(room->players_session_id, sid))
        {
            DEBUGLOG(red, "sid=({}) already in roomId=({})", sid, roomId);
            std::erase_if(room->players_session_id, [&](const uint16_t& val) { return val == sid; });
            return;
        }
        room->players_session_id.push_back(sid);
        room.unlock();
        DEBUGLOG(dark_cyan, "sid=({}) joined roomId=({})", sid, roomId);
        auto acc = CAccount.get<unique_t>(sid);
        acc->room_id = roomId;
        acc->in_room = true;
        acc->state_id = PlayerInfo::State::WaitingRoom;
    }
}