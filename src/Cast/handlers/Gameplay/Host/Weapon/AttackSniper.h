#pragma once
#include "CombatIpc.h"
namespace Game::Handlers
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Cast;
    inline void HostAttackSniper(SCallbackData& callback, CCastServer* server)
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

        auto req = message->GetData<PlayerVictimWeaponReq*>();
        PACKETLOG(ACK, order, "roomId=({}) from host=({}) hostSid=({}) attackerSid=({}) victimSid=({}) hp=({}) bodypart=({}) playerStatus=({}) idk2=({})", room_id, host_name, hostSid, static_cast<uint32_t>(req->attacker_unique_id.session), static_cast<uint32_t>(req->victim_unique_id.session), static_cast<uint32_t>(req->player_info.health), static_cast<uint32_t>(req->player_info.mode_index), static_cast<uint32_t>(req->player_info.player_status), req->idk2);
        // --- DIAGNOSTIC: dump every unknown field so we can identify which one
        // flags a NOSCOPE hit. Watch which value flips between scoped/no-scope shots. ---
        DEBUGLOG(green, "[SNIPER-HIT] attackerSid=({}) victimSid=({}) hp=({}) bodypart=({}) player_status=({}) | option=({}) extra=({}) mission=({}) idk=({}) idk2=({}) <- noscope candidate?",
            static_cast<uint32_t>(req->attacker_unique_id.session),
            static_cast<uint32_t>(req->victim_unique_id.session),
            static_cast<uint32_t>(req->player_info.health),
            static_cast<uint32_t>(req->player_info.mode_index),
            static_cast<uint32_t>(req->player_info.player_status),
            static_cast<uint32_t>(message->GetOption()),
            static_cast<uint32_t>(message->GetExtra()),
            static_cast<uint32_t>(message->GetMission()),
            req->idk, req->idk2);
        auto player_ids = room->players_session_id;
        room.unlock();

        UpdateVictimHealthAndSendCombatIpc(server,
            room_id,
            static_cast<uint16_t>(req->attacker_unique_id.session),
            static_cast<uint16_t>(req->victim_unique_id.session),
            req->player_info.health,
            CombatWeaponKind::Sniper,
            req->player_info.mode_index);

        server->Broadcast(player_ids, *message);
    }
}
