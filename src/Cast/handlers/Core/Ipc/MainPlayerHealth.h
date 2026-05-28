#pragma once
namespace Game::Handlers
{
    using namespace BaseLib;
    using namespace NetEngine;

    inline void IpcMainPlayerHealth(const std::vector<uint8_t>& payload, CCastServer* server)
    {
        if (!server || payload.size() < sizeof(NetEngine::Packets::Ipc::MainToCastPlayerHealthSync))
            return;

        const auto sync = Utility::FromVector<NetEngine::Packets::Ipc::MainToCastPlayerHealthSync>(payload);
        if (!sync.sid)
            return;

        auto player = CAccount.get<unique_t>(sync.sid);
        if (!player)
            return;

        player->max_health = sync.max_health ? sync.max_health : 1000;
        player->current_health = sync.current_health ? std::min(sync.current_health, player->max_health) : player->max_health;
        player->health = player->current_health;
        player->combat_health = player->current_health;
        player->combat_health_known = true;
        player->is_dead = player->current_health == 0;

        DEBUGLOG(dark_cyan, "MainToCastPlayerHealthSync sid=({}) max=({}) current=({})",
            sync.sid,
            player->max_health,
            player->current_health);
    }
}
