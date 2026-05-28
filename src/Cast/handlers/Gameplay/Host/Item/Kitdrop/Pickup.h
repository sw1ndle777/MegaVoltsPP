#pragma once
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
		auto itemId = message->GetData<uint32_t>();

		

        auto hostSid = session->GetSessionId();
        auto host = CAccount.get<shared_t>(hostSid);
        if (!host) return;
        auto hostName = host->nickname;
        auto roomId = host->room_id;
        host.unlock();

        auto userSid = message->GetSession();

        auto room = CRoom.get<shared_t>(roomId);
        if (!host || !room) return;
        if (hostSid != room->host_session_id)
        {
            auto orderName = magic_enum::enum_name(order);
            DEBUGLOG(yellow, "({}): host=({}) hostSid=({}) is not host of roomId=({})", orderName, hostName, hostSid, roomId);
            return;
        }

        server->Broadcast(room->players_session_id, *message);

		auto user = CAccount.get<shared_t>(userSid);
        if (!user) return;
        PACKETLOG(ACK, order, "roomId=({}) user=({}) userSid=({}) from host=({}) hostSid=({}) userSid=({}) itemId=({})", roomId, user->nickname, userSid, hostName, hostSid, userSid, itemId);

        
    }
}