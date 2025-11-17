#pragma once
namespace Game::Handlers
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Cast;
    inline void UserRadio(SCallbackData& callback, CCastServer* server)
    {
        auto session = callback.session;
        auto message = callback.message;
        if (!session || !message) return;
		auto voiceId = message->GetData<uint32_t>();

        auto sid = session->GetSessionId();
        auto acc = CAccount.get<shared_t>(sid);
        auto room = CRoom.get<shared_t>(acc->room_id);
        acc.unlock();

        PACKETLOG(REQ, USER_RADIO, "sid=({}) voiceId=({})", sid, voiceId);

        CMessage radioMsg;
        radioMsg.SetOrder(OTHER_RADIO);
        radioMsg.SetData(voiceId);

		

        server->Broadcast(room->players_session_id, radioMsg);
    }
}