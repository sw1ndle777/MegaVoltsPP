#pragma once
namespace Game::Handlers
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Cast;
    inline void HostAttackProjectile(SCallbackData& callback, CCastServer* server)
    {
        auto session = callback.session;
        auto message = callback.message;
        if (!session || !message) return;

        auto order = magic_enum::enum_cast<EOrder>(u16_cast(message->GetOrder())).value_or(EOrder::NONE);

        auto hostSid = session->GetSessionId();
        auto attackerSid = message->GetSession();
        auto host = CAccount.get<shared_t>(hostSid);
        auto room = CRoom.get<unique_t>(host->room_id);

        if (!host || !room) return;
        if (host->session_id != room->host_session_id)
        {
            auto orderName = magic_enum::enum_name(order);
            DEBUGLOG(yellow, "({}): host=({}) hostSid=({}) is not host of roomId=({})", orderName, host->nickname, hostSid, room->room_id);
            return;
        }

        auto projectileType = message->GetOption();

        if (projectileType != 1 && projectileType != 2)
        {
            auto order = magic_enum::enum_cast<EOrder>(u16_cast(message->GetOrder())).value_or(EOrder::NONE);
            auto orderName = magic_enum::enum_name(order);
            DEBUGLOG(yellow, "({}): invalid projectileType=({}) from sid=({})", orderName, projectileType, hostSid);
            return;
        }

        auto req = message->GetData<AddProjectileReq*>();
        room->projectile_owner_by_id[req->projectile_id] = req->attacker_unique_id.session;
        room->projectile_type_by_id[req->projectile_id] = projectileType;
        PACKETLOG(ACK, order, "roomId=({}) from host=({}) hostSid=({}) attackerSid=({}) projectileId=({}) projectileType=({})",
            host->room_id,
            host->nickname,
            hostSid,
            static_cast<uint32_t>(req->attacker_unique_id.session),
            req->projectile_id,
            static_cast<uint32_t>(projectileType));

        server->Broadcast(room->players_session_id, *message);
    }
}
