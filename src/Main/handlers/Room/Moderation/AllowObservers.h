#pragma once
namespace Game::Handlers
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Main;
    inline void AllowObservers(SCallbackData& callback, CMainServer* main_server)
    {
        auto session = callback.session;
        auto message = callback.message;
        if (!session || !message) return;
        //std::shared_lock lock(session->GetMutex());
        CServer* server = callback.server;
        auto session_id = session->GetSessionId();
        auto acc_cache = CAccount.get<shared_t>(session_id);
        auto acc_index = acc_cache->acc_info.Index;
        auto state = message->GetOption();
        if (acc_index == -1 || !acc_cache->in_room || !CRoom.contains(acc_cache->room_id)) return;
        auto room_cache = CRoom.get<unique_t>(acc_cache->room_id);
        if (room_cache->is_playing || room_cache->host_session_id != session_id) return;
        room_cache->allow_observers = static_cast<bool>(state);
        const auto& observer_ids = room_cache->observers_session_ids;
        acc_cache.unlock();
        auto players_ids = main_server->GetRoomSortedPlayerSessionIds(room_cache);
        for (const auto& room_player_session_id : players_ids)
            if (auto player_session = server->GetSessionById(room_player_session_id))
                player_session->SendMsg(133, 0, 0, state);

        if (observer_ids.empty() || room_cache->allow_observers) return;
        for (const auto& observer_id : observer_ids)
        {
            auto observer_cache = CAccount.get<unique_t>(observer_id);
            if (observer_cache->acc_info.Index == -1) continue;
            if (!observer_cache->in_room || observer_cache->room_id != room_cache->room_id) continue;
            auto player_team_id = observer_cache->team_id;
            observer_cache.unlock();
            main_server->NewRemoveRoomPlayer(room_cache, observer_id, player_team_id, NetEngine::Room::Leave::Ack::Result::Leave, true);
        }
    }
}