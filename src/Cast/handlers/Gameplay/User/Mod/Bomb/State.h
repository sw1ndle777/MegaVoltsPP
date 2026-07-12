#pragma once
#include "../../../Host/MatchEventIpc.h"
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

        // Match-timeline: record bomb plant/defuse progress (this is where the plant/defuse
        // intent actually arrives — the host packet isn't re-broadcast with this data).
        // mission: 0 defuser, 1 planter. extra: 38 START, 33 STOP, 41 FINISH -> phase 0/1/2.
        if (extra == 38 || extra == 33 || extra == 41)
        {
            auto acc = CAccount.get<shared_t>(sid);
            if (acc)
            {
                const auto roomId = acc->room_id;
                acc.unlock();
                const uint8_t role = static_cast<uint8_t>(mission);
                const uint8_t phase = (extra == 38) ? 0 : (extra == 33) ? 1 : 2;
                SendMatchTimelineEventIpc(server, static_cast<uint16_t>(roomId), sid, MatchEventType::Bomb, role, phase);
            }
        }
    }
}