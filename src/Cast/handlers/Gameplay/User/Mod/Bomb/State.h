#pragma once
namespace Game::Handlers
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Core;
    using namespace NetEngine::Packets::Cast;
    struct BombStateReq
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
            Utility::XMConvertHalfToFloat(req->pos.x), Utility::XMConvertHalfToFloat(req->pos.y), Utility::XMConvertHalfToFloat(req->pos.z),
			Utility::XMConvertHalfToFloat(req->dir.x), Utility::XMConvertHalfToFloat(req->dir.y), Utility::XMConvertHalfToFloat(req->dir.z));
        server->Forward(hostSid, sid, *message);
    }
}