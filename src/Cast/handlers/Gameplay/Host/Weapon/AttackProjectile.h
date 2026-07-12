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

        auto projectileType = message->GetOption();

        if (projectileType != 1 && projectileType != 2)
        {
            auto orderName = magic_enum::enum_name(order);
            DEBUGLOG(yellow, "({}): invalid projectileType=({}) from sid=({})", orderName, projectileType, hostSid);
            return;
        }

        auto req = message->GetData<AddProjectileReq*>();
        auto proj = CRoomProjectiles.get_or_emplace(room_id);
        proj->owner_by_id[req->projectile_id] = req->attacker_unique_id.session;
        proj->type_by_id[req->projectile_id] = projectileType;
        proj.unlock();

        PACKETLOG(ACK, order, "roomId=({}) from host=({}) hostSid=({}) attackerSid=({}) projectileId=({}) projectileType=({})",
            room_id,
            host_name,
            hostSid,
            static_cast<uint32_t>(req->attacker_unique_id.session),
            req->projectile_id,
            static_cast<uint32_t>(projectileType));

        auto player_ids = room->players_session_id;
        room.unlock();
        server->Broadcast(player_ids, *message);
    }
}
