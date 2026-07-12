#pragma once
namespace Game::Handlers
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Core;
    using namespace NetEngine::Packets::Cast;
#pragma pack(push, 1)
    struct UseReq
    {
        uint32_t itemId;
		UniqueId uid;
    };
#pragma pack(pop)
    inline void HostItemUse(SCallbackData& callback, CCastServer* server)
    {
        auto session = callback.session;
        auto message = callback.message;
        if (!session || !message) return;

        auto order = magic_enum::enum_cast<EOrder>(u16_cast(message->GetOrder())).value_or(EOrder::NONE);
		auto req = message->GetData<UseReq*>();
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

        auto player_ids = room->players_session_id;
        room.unlock();
        server->Broadcast(player_ids, *message);

		auto userSid = static_cast<uint16_t>(req->uid.session);
		auto user = CAccount.get<shared_t>(userSid);
		if (!user) return;

        PACKETLOG(ACK, order, "roomId=({}) user=({}) userSid=({}) from host=({}) hostSid=({}) itemId=({})", roomId, user->nickname, userSid, hostName, hostSid, req->itemId);

        
    }
}