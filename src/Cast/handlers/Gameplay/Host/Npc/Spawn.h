#pragma once
namespace Game::Handlers
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Cast;
    inline void NpcSpawn(SCallbackData& callback, CCastServer* server)
    {
        auto session = callback.session;
        auto message = callback.message;
        if (!session || !message) return;

        auto order = magic_enum::enum_cast<EOrder>(u16_cast(message->GetOrder())).value_or(EOrder::NONE);

        auto sid = session->GetSessionId();
        auto acc = CAccount.get<shared_t>(sid);
        auto room = CRoom.get<shared_t>(acc->room_id);
        acc.unlock();
        server->Broadcast(room->players_session_id, *message);
    }
}