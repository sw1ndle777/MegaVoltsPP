#pragma once
namespace Game::Handlers
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Core;
    using namespace NetEngine::Packets::Cast;
#pragma pack(push, 1)
    struct HostGiveWeaponReq
    {
        UniqueId uid;
        uint32_t itemId;
	};
#pragma pack(pop)
    inline void HostGiveWeapon(SCallbackData& callback, CCastServer* server)
    {
        auto session = callback.session;
        auto message = callback.message;
        if (!session || !message) return;

        auto order = magic_enum::enum_cast<EOrder>(u16_cast(message->GetOrder())).value_or(EOrder::NONE);
        auto req = message->GetData<HostGiveWeaponReq*>();

        auto hostSid = session->GetSessionId();
        auto host = CAccount.get<shared_t>(hostSid);
        if (!host) return;
		auto hostName = host->nickname;
		auto roomId = host->room_id;
        host.unlock();

		auto userSid = static_cast<uint16_t>(req->uid.session);
		auto user = CAccount.get<shared_t>(userSid);
        auto room = CRoom.get<shared_t>(roomId);

		if (!user || !room) return;
		if (host->session_id != room->host_session_id)
        {
			auto orderName = magic_enum::enum_name(order);
			DEBUGLOG(yellow, "({}): host=({}) hostSid=({}) is not host of roomId=({})", orderName, hostName, hostSid, roomId);
            return;
        }
        PACKETLOG(ACK, order, "roomId=({}) user=({}) sid=({}) from host=({}) hostSid=({}) itemId=({})", roomId, user->nickname, userSid, hostName, hostSid, req->itemId);
        server->Broadcast(room->players_session_id, *message);
    }
}