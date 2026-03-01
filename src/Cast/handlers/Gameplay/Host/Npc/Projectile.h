#pragma once
namespace Game::Handlers
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Cast;
    inline void NpcProjectile(SCallbackData& callback, CCastServer* server)
    {
        auto session = callback.session;
        auto message = callback.message;
        if (!session || !message) return;

        auto order = magic_enum::enum_cast<EOrder>(u16_cast(message->GetOrder())).value_or(EOrder::NONE);

        auto hostSid = session->GetSessionId();
        auto host = CAccount.get<shared_t>(hostSid);
        auto room = CRoom.get<shared_t>(host->room_id);

        if (!host || !room)
        {
            DEBUGLOG(red, "NpcProjectile: invalid host or room for hostSid=({})", hostSid);
            return;
        }
           

        PACKETLOG(ACK, order, "roomId=({}) from host=({}) hostSid=({}))", host->room_id, host->nickname, hostSid);
        host.unlock();
        server->Broadcast(room->players_session_id, *message);
    }
}