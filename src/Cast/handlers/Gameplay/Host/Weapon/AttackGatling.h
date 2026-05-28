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
        auto room = CRoom.get<shared_t>(host->room_id);

        if (!host || !room) return;
        if (host->session_id != room->host_session_id)
        {
            auto orderName = magic_enum::enum_name(order);
            DEBUGLOG(yellow, "({}): host=({}) hostSid=({}) is not host of roomId=({})", orderName, host->nickname, hostSid, room->room_id);
            return;
        }

		auto req = message->GetData<PlayerVictimWeapon2Req*>();
        auto cnt = message->GetOption();
        PACKETLOG(ACK, order, "roomId=({}) from host=({}) hostSid=({}) victimsCount=({})", host->room_id, host->nickname, hostSid, cnt);

        host.unlock();
        for (uint8_t i = 0; i < cnt; i++)
        {
            auto data = req->player_victims_data[i];
            UpdateVictimHealthAndSendCombatIpc(server,
                room->room_id,
                static_cast<uint16_t>(req->attacker_unique_id.session),
                static_cast<uint16_t>(data.victim_unique_id.session),
                data.player_info.health,
                CombatWeaponKind::Gatling);
        }

        server->Broadcast(room->players_session_id, *message);
    }
}
