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
        auto room = CRoom.get<shared_t>(host->room_id);

        if (!host || !room) return;
        if (host->session_id != room->host_session_id)
        {
            auto orderName = magic_enum::enum_name(order);
            DEBUGLOG(yellow, "({}): host=({}) hostSid=({}) is not host of roomId=({})", orderName, host->nickname, hostSid, room->room_id);
            return;
        }

        auto uid = NetEngine::Packets::Core::UniqueId(hostSid, host->server_id).data;
		auto state = message->GetOption();
        CMessage msg = CMessage();
        msg.SetCommand(message->GetOrder(), message->GetMission(), message->GetExtra(), state);
        msg.SetData(reinterpret_cast<uint8_t*>(&uid), sizeof(uid));

		PACKETLOG(ACK, order, "roomId=({}) from host=({}) hostSid=({}) state=({}) uid=({})", host->room_id, host->nickname, hostSid, state, uid);

        server->Broadcast(room->players_session_id, msg);
    }
}