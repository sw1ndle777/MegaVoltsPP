#pragma once
namespace Game::Handlers
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Main;
    inline void MapRule(SCallbackData& callback, CMainServer* main_server)
    {
        auto session = callback.session;
        auto message = callback.message;
        if (!session || !message) return;
        //std::shared_lock lock(session->GetMutex());
        CServer* server = callback.server;
        auto session_id = session->GetSessionId();
        auto acc_cache = CAccount.get<shared_t>(session_id);
        auto acc_index = acc_cache->acc_info.Index;
        if (acc_index == -1 || !acc_cache->in_room || !CRoom.contains(acc_cache->room_id)) return;
        auto room_cache = CRoom.get<unique_t>(acc_cache->room_id);
        if (room_cache->is_playing || room_cache->host_session_id != session_id) return;
        room_cache->MapIndex = static_cast<NetEngine::Room::Map::Index>(message->GetExtra());
        acc_cache.unlock();
        auto players_ids = main_server->GetRoomSortedPlayerSessionIds(room_cache);
        for (const auto& room_player_session_id : players_ids)
            if (auto player_session = server->GetSessionById(room_player_session_id))
                player_session->SendMsg(131, 1, room_cache->MapIndex, room_cache->ModeIndex);
    }
}