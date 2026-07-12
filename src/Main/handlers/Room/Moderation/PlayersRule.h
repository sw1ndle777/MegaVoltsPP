#pragma once
#include "RoomSettingLog.h"
namespace Game::Handlers
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Main;
    inline void PlayersRule
    (SCallbackData& callback, CMainServer* main_server)
    {
        auto session = callback.session;
        auto message = callback.message;
        if (!session || !message) return;
        //std::shared_lock lock(session->GetMutex());
        CServer* server = callback.server;
        auto session_id = session->GetSessionId();
        auto acc_cache = CAccount.get<shared_t>(session_id);
        auto acc_index = acc_cache->acc_info.Index;
        auto max_players = message->GetOption();
        if (acc_index == -1 || !acc_cache->in_room || !CRoom.contains(acc_cache->room_id)) return;
        auto room_cache = CRoom.get<unique_t>(acc_cache->room_id);
        if (room_cache->is_playing || room_cache->host_session_id != session_id) return;
        auto players_count = room_cache->blueteam_session_ids.size() + room_cache->redteam_session_ids.size() + room_cache->neutralteam_session_ids.size();
        if (players_count > max_players) return;
        const auto old_max = static_cast<int32_t>(room_cache->max_players);
        const auto server_id = acc_cache->server_id;
        room_cache->max_players = max_players;
        LogRoomSettingChange(acc_index, acc_index, server_id, room_cache->room_id,
            RoomLog::EventType::MaxPlayersChanged, old_max, static_cast<int32_t>(max_players));
        acc_cache.unlock();
        auto players_ids = main_server->GetRoomSortedPlayerSessionIds(room_cache);

        for (const auto& room_player_session_id : players_ids)
            if (auto player_session = server->GetSessionById(room_player_session_id))
                player_session->SendMsg(132, 0, 0, max_players);
    }
}