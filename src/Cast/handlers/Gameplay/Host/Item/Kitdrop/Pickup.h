#pragma once
#include "../../MatchEventIpc.h"
namespace Game::Handlers
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Cast;
    inline void HostItemKitdropPickup(SCallbackData& callback, CCastServer* server)
    {
        auto session = callback.session;
        auto message = callback.message;
        if (!session || !message) return;

        auto order = magic_enum::enum_cast<EOrder>(u16_cast(message->GetOrder())).value_or(EOrder::NONE);
        // The pickup packet carries the dropId (kit instance), not the catalog item.
        auto dropId = message->GetData<uint32_t>();

        auto hostSid = session->GetSessionId();
        auto host = CAccount.get<shared_t>(hostSid);
        if (!host) return;
        auto hostName = host->nickname;
        auto roomId = host->room_id;
        host.unlock();

        auto userSid = message->GetSession();

        auto room = CRoom.get<unique_t>(roomId);
        if (!room) return;
        if (hostSid != room->host_session_id)
        {
            auto orderName = magic_enum::enum_name(order);
            DEBUGLOG(yellow, "({}): host=({}) hostSid=({}) is not host of roomId=({})", orderName, hostName, hostSid, roomId);
            return;
        }

        // Resolve the dropId to the item recorded at spawn, then forget the entry.
        uint32_t packed = 0;
        if (auto it = room->kit_item_by_drop.find(dropId); it != room->kit_item_by_drop.end())
        {
            packed = it->second;
            room->kit_item_by_drop.erase(it);
        }
        const uint32_t realItemId = packed & 0x7FFFFF;
        const uint32_t itemType = (packed >> 23) & 0x1F;

        auto player_ids = room->players_session_id;
        room.unlock();
        server->Broadcast(player_ids, *message);

		auto user = CAccount.get<shared_t>(userSid);
        if (!user) return;
        // dropId is the kit instance; itemId/itemType are resolved from the spawn record
        // (0 if the drop wasn't tracked, e.g. spawned before this match's reset).
        PACKETLOG(ACK, order, "roomId=({}) user=({}) userSid=({}) from host=({}) hostSid=({}) dropId=({}) itemId=({}) itemType=({})", roomId, user->nickname, userSid, hostName, hostSid, dropId, realItemId, itemType);
        user.unlock();

        // Match-timeline: record the pickup with the resolved item (sub_a = itemType, value = itemId).
        SendMatchTimelineEventIpc(server, static_cast<uint16_t>(roomId), userSid, MatchEventType::ItemPickup, static_cast<uint8_t>(itemType), 0, realItemId);
    }
}