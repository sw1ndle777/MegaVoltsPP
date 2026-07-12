#pragma once
#include "../Weapon/CombatIpc.h"
#include "../MatchEventIpc.h"
namespace Game::Handlers
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Cast;
 
    inline void HostRespawn(SCallbackData& callback, CCastServer* server)
    {
        auto session = callback.session;
        auto message = callback.message;
        if (!session || !message) return;

        auto order = magic_enum::enum_cast<EOrder>(u16_cast(message->GetOrder())).value_or(EOrder::NONE);

        auto req = message->GetData<RespawnRequest*>();
		auto userSid = static_cast<uint16_t>(req->uid.session);
        auto hostSid = session->GetSessionId();
        auto host = CAccount.get<shared_t>(hostSid);
        if (!host) return;
        auto hostName = host->nickname;
        auto roomId = host->room_id;
        host.unlock();

        auto room = CRoom.get<shared_t>(roomId);
        if (!room) return;
        if (hostSid != room->host_session_id)
        {
            auto orderName = magic_enum::enum_name(order);
            DEBUGLOG(yellow, "({}): host=({}) hostSid=({}) is not host of roomId=({})", orderName, hostName, hostSid, roomId);
            return;
        }

        auto user = CAccount.get<unique_t>(userSid);
        if (!user) return;
        const auto full_health = user->max_health ? user->max_health : kCastDefaultHealthRaw;
        user->current_health = full_health;
        user->health = full_health;
        user->combat_health = full_health;
        user->combat_health_known = true;
        user->is_dead = false;
        user->last_attacker_sid = 0;
        user->kill_credited_this_death = false;
        // A respawn starts a fresh life, so the live kill streak resets here. This also
        // covers round-based modes (Elimination/Zombie/Bomb) since each round respawns
        // players. The match-peak (highest_kill_streak) is preserved for the best-streak stat.
        user->current_kill_streak = 0;

        PACKETLOG(ACK, order, "roomId=({}) user=({}) sid=({}) from host=({}) hostSid=({}) pos=({} {} {}) rot=({})",
            roomId, user->nickname, userSid, hostName, hostSid,
            Utility::XMConvertHalfToFloat(req->x), Utility::XMConvertHalfToFloat(req->y), Utility::XMConvertHalfToFloat(req->z), Utility::XMConvertHalfToFloat(req->rotation));

        user.unlock();
        auto player_ids = room->players_session_id;
        room.unlock();
        server->Broadcast(player_ids, *message);

        // Match-timeline: record this respawn (Main resolves the aid and buffers it).
        SendMatchTimelineEventIpc(server, static_cast<uint16_t>(roomId), userSid, MatchEventType::Respawn);
    }
}
