#pragma once
namespace Game
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Main;

    namespace Handlers
    {
        inline void NextRoundUpdateMatch(SCallbackData& callback, CMainServer* main_server)
        {
            auto session = callback.session;
            auto message = callback.message;
            if (!session || !message) return;

            std::shared_lock lock(session->GetMutex());
            CServer* server = callback.server;
            auto order = message->GetOrder();
            auto session_id = session->GetSessionId();
            auto acc_cache = main_server->GetAccCacheUniqueBySessionId(session_id);
            auto acc_index = acc_cache->acc_info.Index;
            acc_cache->zombie_team = 0;

            if (acc_index == -1 || !acc_cache->in_room || !main_server->IsRoomAlready(acc_cache->room_id)) return;
            auto room_cache = main_server->GetRoomCacheShared(acc_cache->room_id);
            acc_cache.unlock();
            auto players = main_server->GetRoomSortedPlayerSessionIds(room_cache);
            auto extra = order == 108 ? 1 : message->GetExtra();
            for (const auto& id : players)
            {
                if (id == session_id) continue;
                if (auto player_session = server->GetSessionById(id))
                    player_session->SendMsg(order, 0, extra, message->GetOption(), message->GetData(), message->GetDataSize());
            }
        }
    }  
}