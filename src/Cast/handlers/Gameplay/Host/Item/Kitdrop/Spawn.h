#pragma once
namespace Game::Handlers
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Cast;
    inline void HostItemKitdropSpawn(SCallbackData& callback, CCastServer* server)
    {
        auto session = callback.session;
        auto message = callback.message;
        if (!session || !message) return;

        auto order = magic_enum::enum_cast<EOrder>(u16_cast(message->GetOrder())).value_or(EOrder::NONE);
		auto req = message->GetData<KitDropInfo*>();
		auto userId = static_cast<uint16_t>(req->uid.session);

        auto hostSid = session->GetSessionId();
        auto host = CAccount.get<shared_t>(hostSid);
        if (!host) return;
        auto hostName = host->nickname;
        auto roomId = host->room_id;
        host.unlock();

        auto room = CRoom.get<unique_t>(roomId);
        if (!room) return;
        if (hostSid != room->host_session_id)
        {
            auto orderName = magic_enum::enum_name(order);
            DEBUGLOG(yellow, "({}): host=({}) hostSid=({}) is not host of roomId=({})", orderName, hostName, hostSid, roomId);
        }

        // Remember this drop's item (packed word) so the pickup handler can resolve what was
        // actually grabbed — the pickup packet only carries the dropId, not the item.
        room->kit_item_by_drop[req->id] = req->data;
        auto player_ids = room->players_session_id;
        room.unlock();
        server->Broadcast(player_ids, *message);

        // Log the dropped kit so item ids can be identified (bomb drops, ammo/health,
        // zombie special weapons, elimination radar, CTB batteries, etc.). Null-safe:
        // system/objective drops (e.g. CTB batteries) have no owning player, so only
        // unlock when the lookup actually returned a handle.
        std::string userName = "???";
        if (auto user = CAccount.get<shared_t>(userId))
        {
            userName = user->nickname;
            user.unlock();
        }
        PACKETLOG(ACK, order, "roomId=({}) user=({}) sid=({}) from host=({}) hostSid=({}) dropId=({}) itemId=({}) itemType=({}) flag=({}) pos=({}, {}, {}) dissapearTick=({})",
            roomId, userName, userId, hostName, hostSid, req->id,
            static_cast<uint32_t>(req->itemId), static_cast<uint32_t>(req->itemType), static_cast<uint32_t>(req->flag),
            req->x, req->y, req->z, req->dissapear_tick);
    }
}