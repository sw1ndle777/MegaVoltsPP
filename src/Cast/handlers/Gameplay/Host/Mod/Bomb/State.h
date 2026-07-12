#pragma once
#include "../../MatchEventIpc.h"
namespace Game::Handlers
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Core;
    using namespace NetEngine::Packets::Cast;
#pragma pack(push, 1)
    struct BombStateReq2
    {
        UniqueId uid;
        struct
        {
            uint16_t x, y, z;
        }pos;
        struct
        {
            uint16_t x, y, z;
        }dir;
    };
#pragma pack(pop)

    enum class EMission : uint8_t
    {
        DEFUSER = 0,
        PLANTER = 1
    };

    enum class EExtra : uint8_t
    {
        START = 38,
        STOP = 33,
        FINISH = 41
    };

    inline void HostModBombState(SCallbackData& callback, CCastServer* server)
    {
        auto session = callback.session;
        auto message = callback.message;
        if (!session || !message) return;

        auto order = magic_enum::enum_cast<EOrder>(u16_cast(message->GetOrder())).value_or(EOrder::NONE);

        auto hostSid = session->GetSessionId();
        auto host = CAccount.get<shared_t>(hostSid);
        if (!host) return;
        auto hostName = host->nickname;
        auto roomId = host->room_id;
        host.unlock();

        auto extra = magic_enum::enum_cast<EExtra>(message->GetExtra());
        auto mission = magic_enum::enum_cast<EMission>(message->GetMission());
		auto state = (extra && mission) ? fmt::format("{}_{}", magic_enum::enum_name(*mission), magic_enum::enum_name(*extra)) : "UNKNOWN";

		auto req = message->GetData<BombStateReq2*>();
		//auto userId = static_cast<uint16_t>(req->uid.session);
		//auto user = CAccount.get<shared_t>(userId);
        auto room = CRoom.get<shared_t>(roomId);
        if (!room) return;
        if (hostSid != room->host_session_id)
        {
            auto orderName = magic_enum::enum_name(order);
            DEBUGLOG(yellow, "({}): host=({}) hostSid=({}) is not host of roomId=({})", orderName, hostName, hostSid, roomId);
            return;
        }
        /*
        PACKETLOG(ACK, order, "roomId=({}) user=({}) sid=({}) from host=({}) hostSid=({}) state=({}) pos=({}, {}, {}) dir=({}, {}, {})",
            roomId, user->nickname, userId, hostName, hostSid, state,
            Utility::XMConvertHalfToFloat(req->pos.x), Utility::XMConvertHalfToFloat(req->pos.y), Utility::XMConvertHalfToFloat(req->pos.z),
			Utility::XMConvertHalfToFloat(req->dir.x), Utility::XMConvertHalfToFloat(req->dir.y), Utility::XMConvertHalfToFloat(req->dir.z));
        */
        auto player_ids = room->players_session_id;
        room.unlock();
        server->Broadcast(player_ids, *message);

        // Defuse progress is host-broadcast (it doesn't arrive as a USER_MOD_BOMB_STATE
        // like the plant does), so the match-timeline defuse event is emitted here. Plant
        // stays in the user handler so it isn't double-counted.
        const auto defuserSid = static_cast<uint16_t>(req->uid.session);
        // Diagnostic: confirm whether defuse reaches the host and with which codes.
        DEBUGLOG(cyan, "BombState host: state=({}) mission_raw=({}) extra_raw=({}) userSid=({}) roomId=({})",
            state, message->GetMission(), message->GetExtra(), defuserSid, roomId);
        if (mission && extra && *mission == EMission::DEFUSER)
        {
            const uint8_t phase = (*extra == EExtra::START) ? 0 : (*extra == EExtra::STOP) ? 1 : 2;
            SendMatchTimelineEventIpc(server, static_cast<uint16_t>(roomId), defuserSid, MatchEventType::Bomb, 0 /*defuser*/, phase);
        }
    }
}