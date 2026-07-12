#pragma once
#include "CombatIpc.h"
namespace Game::Handlers
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Cast;
    inline void HostAttackGatling(SCallbackData& callback, CCastServer* server)
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

		auto req = message->GetData<PlayerVictimWeapon2Req*>();
        auto cnt = message->GetOption();
        PACKETLOG(ACK, order, "roomId=({}) from host=({}) hostSid=({}) victimsCount=({})", room_id, host_name, hostSid, cnt);
        auto player_ids = room->players_session_id;
        room.unlock();

        for (uint8_t i = 0; i < cnt; i++)
        {
            auto data = req->player_victims_data[i];
            DEBUGLOG(green, "WEAPON_HIT weapon=GATLING attackerSid=({}) victimSid=({}) hp=({}) bodypart=({}) playerStatus=({}) idk2=({})",
                static_cast<uint32_t>(req->attacker_unique_id.session),
                static_cast<uint32_t>(data.victim_unique_id.session),
                static_cast<uint32_t>(data.player_info.health),
                static_cast<uint32_t>(data.player_info.mode_index),
                static_cast<uint32_t>(data.player_info.player_status),
                data.idk2);
            UpdateVictimHealthAndSendCombatIpc(server,
                room_id,
                static_cast<uint16_t>(req->attacker_unique_id.session),
                static_cast<uint16_t>(data.victim_unique_id.session),
                data.player_info.health,
                CombatWeaponKind::Gatling,
                data.player_info.mode_index);
        }

        server->Broadcast(player_ids, *message);
    }
}
