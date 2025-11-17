#pragma once
namespace Game::Handlers
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Core;
    using namespace NetEngine::Packets::Cast;
    using namespace DirectX::PackedVector;
#pragma pack(push, 1)
    struct BombStateReq2
    {
        UniqueId uid;
        struct
        {
            HALF x, y, z;
        }pos;
        struct
        {
            HALF x, y, z;
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
		auto userId = static_cast<uint16_t>(req->uid.session);
		auto user = CAccount.get<shared_t>(userId);
        auto room = CRoom.get<shared_t>(roomId);
        if (!user || !room) return;
        if (host->session_id != room->host_session_id)
        {
            auto orderName = magic_enum::enum_name(order);
            DEBUGLOG(yellow, "({}): host=({}) hostSid=({}) is not host of roomId=({})", orderName, host->nickname, hostSid, room->room_id);
            return;
        }

        PACKETLOG(ACK, order, "roomId=({}) user=({}) sid=({}) from host=({}) hostSid=({}) state=({}) pos=({}, {}, {}) dir=({}, {}, {})",
            roomId, user->nickname, userId, hostName, hostSid, state,
            XMConvertHalfToFloat(req->pos.x), XMConvertHalfToFloat(req->pos.y), XMConvertHalfToFloat(req->pos.z),
			XMConvertHalfToFloat(req->dir.x), XMConvertHalfToFloat(req->dir.y), XMConvertHalfToFloat(req->dir.z));

        server->Broadcast(room->players_session_id, *message);
    }
}