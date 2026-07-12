#pragma once
namespace Game::Handlers
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Cast;
    inline void NpcSpawn(SCallbackData& callback, CCastServer* server)
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
        host.unlock();

        auto room = CRoom.get<shared_t>(room_id);
        if (!room)
        {
			DEBUGLOG(red, "NpcSpawn: invalid room for hostSid=({})", hostSid);
			return;
        }

        PACKETLOG(ACK, order, "roomId=({}) from host=({}) hostSid=({}))", room_id, host_name, hostSid);
        auto player_ids = room->players_session_id;
        room.unlock();
        server->Broadcast(player_ids, *message);
    }
}