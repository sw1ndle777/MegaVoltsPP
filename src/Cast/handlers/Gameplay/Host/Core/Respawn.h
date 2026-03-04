#pragma once
namespace Game::Handlers
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Cast;
 
    inline void HostRespawn(SCallbackData& callback, CCastServer* server)
    {
        auto session = callback.session;
        auto message = callback.message;
        if (!session || !message) return;

        auto order = magic_enum::enum_cast<EOrder>(u16_cast(message->GetOrder())).value_or(EOrder::NONE);

        auto req = message->GetData<RespawnRequest*>();
		auto userSid = static_cast<uint16_t>(req->uid.session);
        auto hostSid = session->GetSessionId();
        auto host = CAccount.get<shared_t>(hostSid);
        if (!host) return;
        auto hostName = host->nickname;
        auto roomId = host->room_id;
        host.unlock();

        auto room = CRoom.get<shared_t>(roomId);
        if (!room) return;
        if (host->session_id != room->host_session_id)
        {
            auto orderName = magic_enum::enum_name(order);
            DEBUGLOG(yellow, "({}): host=({}) hostSid=({}) is not host of roomId=({})", orderName, hostName, hostSid, roomId);
            return;
        }

        auto user = CAccount.get<unique_t>(userSid);
        if (!user) return;
        user->health = 1000000;

        PACKETLOG(ACK, order, "roomId=({}) user=({}) sid=({}) from host=({}) hostSid=({}) pos=({} {} {}) rot=({})",
            roomId, user->nickname, userSid, hostName, hostSid,
            Utility::XMConvertHalfToFloat(req->x), Utility::XMConvertHalfToFloat(req->y), Utility::XMConvertHalfToFloat(req->z), Utility::XMConvertHalfToFloat(req->rotation));

        user.unlock();
        server->Broadcast(room->players_session_id, *message);
    }
}