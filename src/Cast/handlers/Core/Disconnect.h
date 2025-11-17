#pragma once
namespace Game::Handlers
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Cast;
    inline void ServerDisconnect(std::shared_ptr<CSession> session, CCastServer* cast_server)
    {
        if (!session) return;
        auto sid = session->GetSessionId();
        DEBUGLOG(dark_cyan, "sid=({}) disconnected", sid);
        cast_server->RemoveSession(sid);
        auto acc = CAccount.get<shared_t>(sid);
        if (acc->in_plaza)
        {
            if (CPlaza.contains(acc->plaza_id))
            {
                auto plaza = CPlaza.get<unique_t>(acc->plaza_id);
                std::erase_if(plaza->players_session_id, [&](const auto& id) { return id == sid; });
                DEBUGLOG(dark_cyan, "sid=({}) left plaza id: ({})", sid, acc->plaza_id);
                if (!plaza->players_session_id.size())
                {
                    plaza.unlock();
                    CPlaza.erase(acc->plaza_id);
                    DEBUGLOG(dark_cyan, "sid=({}) removed plaza id: ({})", sid, acc->plaza_id);
                }
            }
            acc->in_plaza = false;
        }
        if (acc->in_room)
        {
            if (CRoom.contains(acc->room_id))
            {
                auto room = CRoom.get<unique_t>(acc->room_id);
                std::erase_if(room->players_session_id, [&](const auto& id) { return id == sid; });
                DEBUGLOG(dark_cyan, "sid=({}) left roomId=({})", sid, acc->room_id);
                if (!room->players_session_id.size())
                {
                    room.unlock();
                    CRoom.erase(acc->room_id);
                    cast_server->SetRoomIdAvailable(acc->room_id);
                    DEBUGLOG(dark_cyan, "sid=({}) removed roomId=({})", sid, acc->room_id);
                }
            }
            acc->in_room = false;
        }
        acc->state_id = PlayerInfo::State::Disconnected;
        acc.unlock();
        CAccount.erase(sid);
    }
}