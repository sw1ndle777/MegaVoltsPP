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
        if (!acc) return;
        auto room_id = acc->room_id;
        acc.unlock();

        PACKETLOG(REQ, USER_RADIO, "sid=({}) voiceId=({})", sid, voiceId);

        CMessage radioMsg;
        radioMsg.SetOrder(OTHER_RADIO);
        radioMsg.SetData(voiceId);

        auto room = CRoom.get<shared_t>(room_id);
        if (!room) return;
        auto player_ids = room->players_session_id;
        room.unlock();
        server->Broadcast(player_ids, radioMsg);
    }
}