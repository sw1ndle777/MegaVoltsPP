#pragma once
namespace Game::Handlers
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Cast;
    inline void HostModMatchLoad(SCallbackData& callback, CCastServer* server)
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
        auto uid = NetEngine::Packets::Core::UniqueId(hostSid, host->server_id).data;
        host.unlock();

		auto state = message->GetOption();
        CMessage msg = CMessage();
        msg.SetCommand(message->GetOrder(), message->GetMission(), message->GetExtra(), state);
        msg.SetData(reinterpret_cast<uint8_t*>(&uid), sizeof(uid));

        auto room = CRoom.get<shared_t>(room_id);
        if (!room) return;
		PACKETLOG(ACK, order, "roomId=({}) from host=({}) hostSid=({}) state=({}) uid=({})", room_id, host_name, hostSid, state, uid);
        auto player_ids = room->players_session_id;
        room.unlock();

        server->Broadcast(player_ids, msg);
    }
}