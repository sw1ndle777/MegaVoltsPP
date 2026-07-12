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
		auto hostSidCached = host->session_id;
        host.unlock();

		auto userSid = static_cast<uint16_t>(req->uid.session);
		auto user = CAccount.get<shared_t>(userSid);
		if (!user) return;
		auto userName = user->nickname;
		user.unlock();

        auto room = CRoom.get<shared_t>(roomId);
		if (!room) return;
		if (hostSidCached != room->host_session_id)
        {
			auto orderName = magic_enum::enum_name(order);
			DEBUGLOG(yellow, "({}): host=({}) hostSid=({}) is not host of roomId=({})", orderName, hostName, hostSid, roomId);
            return;
        }
        // itemId is the packed word (itemId:23 | itemType:5 | flag:1) — decode it so it reads
        // like the kitdrop logs. In zombie mode a give-weapon of a 408xxxx/412xxxx item is the
        // "became zombie / zombie-king" signal.
        const uint32_t packedGive = req->itemId;
        const uint32_t giveItemId = packedGive & 0x7FFFFF;
        const uint32_t giveItemType = (packedGive >> 23) & 0x1F;
        PACKETLOG(ACK, order, "roomId=({}) user=({}) sid=({}) from host=({}) hostSid=({}) raw=({}) itemId=({}) itemType=({})", roomId, userName, userSid, hostName, hostSid, packedGive, giveItemId, giveItemType);
        auto player_ids = room->players_session_id;
        room.unlock();
        server->Broadcast(player_ids, *message);
    }
}