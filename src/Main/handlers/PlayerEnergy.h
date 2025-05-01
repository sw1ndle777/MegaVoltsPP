#pragma once
namespace Game
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Main;

    namespace Handlers
    {
        inline void PlayerEnergy(SCallbackData& callback, CMainServer* main_server)
        {
            auto session = callback.session;
            auto message = callback.message;
            if (!session || !message) return;

            std::shared_lock lock(session->GetMutex());
            CServer* server = callback.server;
            
            const auto& getEnergyReq = reinterpret_cast<MainGetEnergyInGameReq*>(message->GetData());
            const auto& unique_id = NetEngine::Packets::Core::UniqueId(getEnergyReq->uniqueId);

            auto acc_cache = main_server->GetAccCacheUniqueBySessionId(unique_id.session);
            //add check for unique id for what player to get battery
            if (acc_cache->acc_info.Index == -1) return;
            if (!acc_cache->in_room || !acc_cache->playing || !main_server->IsRoomAlready(acc_cache->room_id)) return;

            auto room = main_server->GetRoomCacheShared(acc_cache->room_id);   
            acc_cache.unlock();
            auto player_ids = main_server->GetRoomSortedPlayerSessionIds(room);
            room.unlock();
            if (!main_server->IsSessionIdAlready(unique_id.session, player_ids)) return;

            std::array<uint32_t, 3> batteries = { 30, 50, 100 };
            uint32_t index = Utility::Random::CustomGen(0, static_cast<uint32_t>(batteries.size() - 1));
            auto battery_earnt = batteries[index];
            if (acc_cache->earnt_battery + battery_earnt > 1000) return;
            if(acc_cache->earnt_battery + acc_cache->acc_info.Energy > acc_cache->acc_info.MaximumEnergy) return;
            acc_cache->earnt_battery += battery_earnt;
            BaseLib::Database->UpdateEnergy(static_cast<uint32_t>(acc_cache->earnt_battery), acc_cache->acc_info.AuthKey);
            if (auto player_session = server->GetSessionById(unique_id.session))
                player_session->SendMsg(86, 0, 1, battery_earnt, reinterpret_cast<uint8_t*>(&battery_earnt), sizeof(battery_earnt));
        }
    }
}