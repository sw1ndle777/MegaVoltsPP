#pragma once
#include "CombatIpc.h"
namespace Game::Handlers
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Cast;
    inline void HostImpactProjectile(SCallbackData& callback, CCastServer* server)
    {
        auto session = callback.session;
        auto message = callback.message;
        if (!session || !message) return;

        auto order = magic_enum::enum_cast<EOrder>(u16_cast(message->GetOrder())).value_or(EOrder::NONE);

        auto hostSid = session->GetSessionId();
        auto host = CAccount.get<shared_t>(hostSid);
        if (!host) return;
        auto room_id = host->room_id;
        auto host_name = host->nickname;
        auto host_sid_cached = host->session_id;
        host.unlock();

        auto room = CRoom.get<shared_t>(room_id);
        if (!room) return;
        if (host_sid_cached != room->host_session_id)
        {
            auto orderName = magic_enum::enum_name(order);
            DEBUGLOG(yellow, "({}): host=({}) hostSid=({}) is not host of roomId=({})", orderName, host_name, hostSid, room_id);
            return;
        }

        auto extra = message->GetExtra();
        auto cnt = message->GetOption();
        auto req = message->GetData<ImpactProjectileReq*>();

        uint16_t attacker_sid = 0;
        uint8_t projectile_type = 0;
        {
            auto proj = CRoomProjectiles.get<shared_t>(room_id);
            if (proj)
            {
                if (auto it = proj->owner_by_id.find(req->projectile_id); it != proj->owner_by_id.end())
                    attacker_sid = it->second;
                if (auto it = proj->type_by_id.find(req->projectile_id); it != proj->type_by_id.end())
                    projectile_type = it->second;
            }
        }

        const auto weapon_kind = (projectile_type == 1)
            ? CombatWeaponKind::Bazooka
            : (projectile_type == 2)
                ? CombatWeaponKind::Grenade
                : CombatWeaponKind::Unknown;

        PACKETLOG(ACK, order, "roomId=({}) from host=({}) hostSid=({}) attackerSid=({}) projectileId=({}) projectileType=({}) victimsCount=({})",
            room_id,
            host_name,
            hostSid,
            static_cast<uint32_t>(attacker_sid),
            req->projectile_id,
            static_cast<uint32_t>(projectile_type),
            cnt);

        auto player_ids = room->players_session_id;
        room.unlock();

        for (uint8_t i = 0; i < cnt; i++)
        {
            auto data = reinterpret_cast<PlayerVictimDataReq*>(message->GetData() + sizeof(ImpactProjectileReq) + i * sizeof(PlayerVictimDataReq));
            DEBUGLOG(dark_cyan, "ImpactProjectile parsed victim idx=({}) attackerSid=({}) victimSid=({}) hp=({}) bodypart=({}) playerStatus=({}) idk2=({}) payloadSize=({})",
                static_cast<uint32_t>(i),
                static_cast<uint32_t>(attacker_sid),
                static_cast<uint32_t>(data->victim_unique_id.session),
                static_cast<uint32_t>(data->player_info.health),
                static_cast<uint32_t>(data->player_info.mode_index),
                static_cast<uint32_t>(data->player_info.player_status),
                data->idk2,
                static_cast<uint32_t>(message->GetDataSize()));
            UpdateVictimHealthAndSendCombatIpc(server,
                room_id,
                attacker_sid,
                static_cast<uint16_t>(data->victim_unique_id.session),
                data->player_info.health,
                weapon_kind,
                data->player_info.mode_index);

            auto enabled_sids = Game::g_tp_to_proj_sids.get_all(shared);
            const bool tp_to_proj_enabled = enabled_sids->find(attacker_sid) != enabled_sids->end();
            enabled_sids.unlock();

            if (tp_to_proj_enabled)
            {
                DEBUGLOG(dark_cyan, "tp_to_proj enabled for attackerSid=({}), attempting to respawn victim at projectile impact location", attacker_sid);
                auto user = CAccount.get<unique_t>(attacker_sid);
                if (user)
                {
                    const auto full_health = user->max_health ? user->max_health : kCastDefaultHealthRaw;
                    user->current_health = full_health;
                    user->health = full_health;
                    user->combat_health = full_health;
                    user->combat_health_known = true;
                    user->is_dead = false;
                    user->last_attacker_sid = 0;
                    user->kill_credited_this_death = false;
                    user.unlock();

                    RespawnRequest respawn_req{};
                    respawn_req.x = req->pos_x;
                    respawn_req.y = req->pos_y;
                    respawn_req.z = req->pos_z;
                    respawn_req.rotation = 0;
                    respawn_req.uid = data->victim_unique_id;

                    CMessage respawn_msg;
                    respawn_msg.SetCommand(HOST_RESPAWN, 0, 0, 0);
                    respawn_msg.SetData(reinterpret_cast<uint8_t*>(&respawn_req), sizeof(respawn_req));
                    server->Broadcast(player_ids, respawn_msg);

                    DEBUGLOG(dark_cyan,
                        "ImpactProjectile tptoproj attackerSid=({}) projectileId=({}) pos=({},{},{})",
                        attacker_sid,
                        req->projectile_id,
                        req->pos_x,
                        req->pos_y,
                        req->pos_z);
                }
            }
        }

        {
            auto proj = CRoomProjectiles.get<unique_t>(room_id);
            if (proj)
            {
                proj->owner_by_id.erase(req->projectile_id);
                proj->type_by_id.erase(req->projectile_id);
            }
        }

        server->Broadcast(player_ids, *message);
    }
}
