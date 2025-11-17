#pragma once
namespace Game::Handlers
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Main;
    inline void PveRespawn(SCallbackData& callback, CMainServer* main_server)
    {
        auto session = callback.session;
        auto message = callback.message;
        if (!session || !message) return;

        //std::shared_lock lock(session->GetMutex());
        auto session_id = session->GetSessionId();
        auto acc_cache = CAccount.get<unique_t>(session_id);
        auto acc_index = acc_cache->acc_info.Index;
        auto can_respawn = acc_cache->boss_respawn_remaining > 0;
        if (acc_index == -1 || !acc_cache->in_room || !CRoom.contains(acc_cache->room_id)) return;
        session->SendMsg(message->GetOrder(), 0, can_respawn ? 1 : 2, 0, message->GetData(), message->GetDataSize());// 1 = success, 2 NoEnergy
        if (can_respawn) acc_cache->boss_respawn_remaining--;
    }
}