#pragma once
namespace Game::Handlers
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Main;
    inline void MatchRoundsStart(SCallbackData& callback, CMainServer* main_server)
    {
        auto session = callback.session;
        auto message = callback.message;
        if (!session || !message) return;

        auto order = message->GetOrder();
        auto sid = session->GetSessionId();
        auto acc = CAccount.get<unique_t>(sid);
        auto aid = acc->acc_info.Index;
        acc->zombie_team = 0;

        if (!aid || !acc->in_room || !CRoom.contains(acc->room_id)) return;
        auto room = CRoom.get<shared_t>(acc->room_id);
        acc.unlock();
        if (room->host_session_id != sid) return;
        auto players = main_server->GetRoomSortedPlayerSessionIds(room);
        for (const auto& id : players)
        {
            if (id == sid) continue;
            if (auto pss = main_server->GetSessionById(id))
                pss->SendMsg(order, 0, 1, message->GetOption(), message->GetData(), message->GetDataSize());
        }
    }
}