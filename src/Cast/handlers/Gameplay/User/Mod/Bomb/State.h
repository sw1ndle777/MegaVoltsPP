#pragma once
namespace Game::Handlers
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Core;
    using namespace NetEngine::Packets::Cast;
    using namespace DirectX::PackedVector;
    struct BombStateReq
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
    inline void UserModBombState(SCallbackData& callback, CCastServer* server)
    {
        auto session = callback.session;
        auto message = callback.message;
        if (!session || !message) return;

        auto hostSid = message->GetSession();
        auto sid = session->GetSessionId();
		auto option = message->GetOption();
		auto extra = message->GetExtra();
		auto mission = message->GetMission();
		auto req = reinterpret_cast<BombStateReq*>(message->GetData());
		PACKETLOG(ACK, USER_MOD_BOMB_STATE, "sid=({}) hostSid=({}) option=({}) extra=({}) mission=({}) uid=({}) pos=({}, {}, {}) dir=({}, {}, {})",
            sid, hostSid, option, extra, mission, req->uid.data,
            XMConvertHalfToFloat(req->pos.x), XMConvertHalfToFloat(req->pos.y), XMConvertHalfToFloat(req->pos.z),
			XMConvertHalfToFloat(req->dir.x), XMConvertHalfToFloat(req->dir.y), XMConvertHalfToFloat(req->dir.z));
        server->Forward(hostSid, sid, *message);
    }
}