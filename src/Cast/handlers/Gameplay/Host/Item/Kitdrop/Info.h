#pragma once
namespace Game::Handlers
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Core;
    using namespace NetEngine::Packets::Cast;



    inline void HostItemKitdropInfo(SCallbackData& callback, CCastServer* server)
    {
        auto session = callback.session;
        auto message = callback.message;
        if (!session || !message) return;

        auto order = magic_enum::enum_cast<EOrder>(u16_cast(message->GetOrder())).value_or(EOrder::NONE);
        auto cnt = message->GetOption();

        auto hostSid = session->GetSessionId();
        auto host = CAccount.get<shared_t>(hostSid);
        if (!host) return;
        auto hostName = host->nickname;
        auto roomId = host->room_id;
        host.unlock();

        auto room = CRoom.get<shared_t>(roomId);
        if (!room) return;
        /*
        if (host->session_id != room->host_session_id)
        {
            auto orderName = magic_enum::enum_name(order);
            DEBUGLOG(yellow, "({}): host=({}) hostSid=({}) is not host of roomId=({})", orderName, hostName, hostSid, roomId);
            return;
        }
        */
        for (uint8_t i = 0; i < cnt; i++)
        {
            auto req = reinterpret_cast<KitDropInfo*>(message->GetData() + (i * sizeof(KitDropInfo)));
			auto userId = static_cast<uint16_t>(req->uid.session);
			auto user = CAccount.get<shared_t>(userId);
            if (!user) continue;
            PACKETLOG(ACK, order, "roomId=({}) user=({}) sid=({}) from host=({}) hostSid=({}) dropId=({}) itemId=({}) itemType=({}) flag=({}) pos=({}, {}, {}), dissapearTick=({})",
				roomId,  user->nickname, userId, hostName, hostSid, req->id,
                static_cast<uint32_t>(req->itemId), static_cast<uint32_t>(req->itemType), static_cast<uint32_t>(req->flag), 
                req->x, req->y, req->z, req->dissapear_tick);
        }

       
        server->Broadcast(room->players_session_id, *message);
    }
}