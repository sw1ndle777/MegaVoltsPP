#pragma once
namespace Game::Handlers
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Main;
    inline void Battery(SCallbackData& callback, CMainServer* main_server)
    {
        auto session = callback.session;
        auto message = callback.message;
        if (!session || !message) return;

        //std::shared_lock lock(session->GetMutex());
        const auto& req = reinterpret_cast<MainGetEnergyInGameReq*>(message->GetData());
        const auto uid = NetEngine::Packets::Core::UniqueId(req->uniqueId);
        auto acc = CAccount.get<unique_t>(uid.session);
        if (acc->acc_info.Index == -1) return;
        if (!acc->in_room || !acc->playing || !CRoom.contains(acc->room_id)) return;
        auto room = CRoom.get<shared_t>(acc->room_id);
        acc.unlock();
        auto player_ids = main_server->GetRoomSortedPlayerSessionIds(room);
        room.unlock();
		if (!std::ranges::contains(player_ids, uid.session)) return;

        std::array<uint32_t, 3> batteries = { 30, 50, 100 };
        uint32_t index = Utility::Random::CustomGen(0, static_cast<uint32_t>(batteries.size() - 1));
        auto battery_earnt = batteries[index];
        auto temp_earnt_battery = acc->earnt_battery;
        if (temp_earnt_battery + battery_earnt > 1000) return;
        if (temp_earnt_battery + acc->acc_info.Energy > acc->acc_info.MaximumEnergy) return;
        temp_earnt_battery += battery_earnt;

        if (auto pss = main_server->GetSessionById(uid.session))
        {
            acc->earnt_battery = temp_earnt_battery;
            pss->SendMsg(86, 0, 1, battery_earnt, reinterpret_cast<uint8_t*>(&battery_earnt), sizeof(battery_earnt));
        }
    }
}