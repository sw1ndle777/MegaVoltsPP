#pragma once
namespace Game::Handlers
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Cast;
    struct unknown4Req
    {
        uint8_t idk1;
		uint8_t idk2;
        uint16_t idk3;
    };
    inline void UserModBombIdk(SCallbackData& callback, CCastServer* server)
    {
        auto session = callback.session;
        auto message = callback.message;
        if (!session || !message) return;

        auto sid = session->GetSessionId();
        auto acc = CAccount.get<shared_t>(sid);
        auto room = CRoom.get<shared_t>(acc->room_id);
        acc.unlock();
		auto req = message->GetData<unknown4Req*>();
		PACKETLOG(REQ, USER_MOD_BOMB_IDK, "sid=({}) idk1=({}) idk2=({}) idk3=({})", sid, (uint32_t)req->idk1, (uint32_t)req->idk2, req->idk3);
        server->Broadcast(room->players_session_id, *message);
    }
}