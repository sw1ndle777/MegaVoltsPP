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
        auto room = CRoom.get<unique_t>(acc->room_id);
        acc.unlock();
        if (room->host_session_id != sid) return;
        if (main_server->IsModeTeamBased(room->ModeIndex) && room->is_playing)
            ++room->team_rounds_started;
        const bool revive_round = room->is_playing && main_server->IsRoundBasedMode(room->ModeIndex);
        // A new round is live again: reopen combat tracking (closed at the previous round end).
        if (revive_round)
            room->combat_open = true;
        auto players = main_server->GetRoomSortedPlayerSessionIds(room);
        // Release the room lock before taking per-account locks below: every other path locks
        // accounts first, so holding the room here while locking accounts would invert the order.
        room.unlock();
        for (const auto& id : players)
        {
            if (id == sid) continue;
            if (auto pss = main_server->GetSessionById(id))
                pss->SendMsg(order, 0, 1, message->GetOption(), message->GetData(), message->GetDataSize());
        }

        // Authoritative revival: a new round starts everyone alive at full health, which brings
        // back mid-round joiners that were spawned as dead spectators (see Room/Core/Join.h).
        if (revive_round)
        {
            for (const auto& id : players)
            {
                auto p = CAccount.get<unique_t>(id);
                if (!p || p->acc_info.Index == -1) { p.unlock(); continue; }
                main_server->RefreshPlayerHealthCache(p, true);
                main_server->SendCastPlayerHealthSync(p);
                p.unlock();
            }
        }
    }
}
