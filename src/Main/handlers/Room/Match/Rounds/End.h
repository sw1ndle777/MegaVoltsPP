#pragma once
namespace Game::Handlers
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Main;
    inline void MatchRoundsEnd(SCallbackData& callback, CMainServer* main_server)
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
                pss->SendMsg(order, 0, message->GetExtra(), message->GetOption(), message->GetData(), message->GetDataSize());
        }

        // Match-timeline: record the round end (round-based modes only). The round result lives
        // in the first dword of the packet data (client IDA of the per-mod round-end parser):
        // byte0 (->0xB4) = unknown/result, byte1 (->0xB8) and byte3 (->0xBC) = the team scores
        // (byte2 is unused by elimination). Little-endian, so d[0]/d[1]/d[3] = those bytes.
        // NOTE: byte->team (blue vs red) mapping and the GetData()==client a1+4 offset still need
        // confirming against a real round; per-mode layouts differ (each CExMod has its own parser).
        const auto mode = room->ModeIndex;
        const bool round_based = main_server->IsRoundBasedMode(mode);
        // CTB shares the same team-score round-end layout (IDA: CExModCTF parser) but isn't
        // "round-based" — its captures update the score without an inter-round revive. Record
        // its scores too, but never close combat for it (no dead window).
        const bool is_ctb = mode == NetEngine::Room::Mode::Index::CaptureTheBattery
            || mode == NetEngine::Room::Mode::Index::CLAN_CaptureTheBattery;
        const bool record_scores = round_based || is_ctb;
        const auto ev_extra = static_cast<uint8_t>(message->GetExtra());
        const auto ev_option = static_cast<uint8_t>(message->GetOption());
        const auto room_id_cached = room->room_id;
        room.unlock();

        // Round OUTCOME from the first dword (confirmed live, elimination 1v1; client IDA: every
        // team-mode parser stores byte0->0xB4, byte1->0xB8, byte3->0xBC, byte2 = trailing record count):
        //   byte0 = result : 1 = a team won, 2 = draw (only elimination can draw)
        //   byte1 = round index (0-based; tracks team_rounds_started)
        //   byte2 = count of the per-player scoreboard records that follow (== player count)
        //   byte3 = WINNING TEAM this round: 0 = draw/none, 1 = Red, 2 = Blue
        // These are NOT cumulative team scores — the website tallies the score from the winner
        // sequence (draws only occur in elimination). dsz = 4 + count*20 (e.g. 1v1 => 44).
        const uint8_t* d = message->GetData();
        const uint32_t dsz = message->GetDataSize();
        const uint8_t r_result    = dsz >= 1 ? d[0] : 0; // 1 win, 2 draw
        const uint8_t r_round_idx = dsz >= 2 ? d[1] : 0; // round index (0-based)
        const uint8_t r_count     = dsz >= 3 ? d[2] : 0; // player-record count
        const uint8_t r_winner    = dsz >= 4 ? d[3] : 0; // 0 draw, 1 red, 2 blue

        if (record_scores)
        {
            uint32_t round_no = 0;
            if (auto room_evt = CRoom.get<unique_t>(room_id_cached))
            {
                round_no = ++room_evt->round_seq; // 1-based, mode-independent
                if (round_based)
                {
                    // Close the combat window: no damage is credited until the next round revives
                    // players (MatchRoundsStart). A short grace (stamped here) still lets the deciding
                    // kill through, since its combat IPC can land just after this round-end packet.
                    room_evt->combat_open = false;
                    room_evt->combat_closed_at = Utility::GetUtcTimeNowInMilliseconds();
                }
                room_evt->match_events.push_back(PendingMatchEvent{
                    .aid = aid,
                    .event_type = 4 /* RoundEnd */,
                    .sub_a = r_winner,   // 0 draw, 1 red, 2 blue
                    .sub_b = r_result,   // round-end reason (mode-specific)
                    .value = round_no,   // 1-based round number
                    .event_ms = Utility::GetUtcTimeNow64() });
            }
            DEBUGLOG(green, "[Match Event] Room {}, round {} ended winner={} result={} (roundIdx={} count={} extra={} option={} dsz={}) - combat {}",
                room_id_cached, round_no,
                r_winner == 1 ? "red" : r_winner == 2 ? "blue" : "draw",
                static_cast<uint32_t>(r_result), static_cast<uint32_t>(r_round_idx), static_cast<uint32_t>(r_count),
                static_cast<uint32_t>(ev_extra), static_cast<uint32_t>(ev_option), dsz,
                round_based ? "closed" : "kept open (ctb)");
        }
    }
}