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
        if (acc->in_room && CRoom.contains(acc->room_id))
        {
            auto room = CRoom.get<unique_t>(acc->room_id);
            std::erase_if(room->players_session_id, [&](const uint16_t& id) { return id == sid; });
            DEBUGLOG(dark_cyan, "user=({}) sid=({}) left roomId=({})", acc->nickname, sid, acc->room_id);
            if (room->players_session_id.empty())
            {
                room.unlock();
                CRoom.erase(acc->room_id);
                CRoomId.erase_value(acc->room_id);
                DEBUGLOG(dark_cyan, "user=({}) sid=({}) removed roomId=({})", acc->nickname, sid, acc->room_id);
            }
            acc->in_room = false;
            acc->state_id = PlayerInfo::State::Disconnected;
        }
    }
}