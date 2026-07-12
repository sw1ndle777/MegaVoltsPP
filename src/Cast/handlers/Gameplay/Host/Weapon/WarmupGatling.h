#pragma once
namespace Game::Handlers
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Cast;
    inline void HostWarmupGatling(SCallbackData& callback, CCastServer* server)
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
        PACKETLOG(ACK, order, "roomId=({}) from host=({}) hostSid=({})", room_id, host_name, hostSid);
        auto player_ids = room->players_session_id;
        room.unlock();

        server->Broadcast(player_ids, *message);
    }
}