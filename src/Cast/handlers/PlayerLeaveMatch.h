#pragma once
namespace Game
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Cast;

    namespace Handlers
    {
        inline void PlayerLeaveMatch(SCallbackData& callback, CCastServer* cast_server)
        {
            std::shared_lock lock(callback.session->GetMutex());
            CSession* session = callback.session;
            CServer* server = callback.server;
            auto self_session_id = session->GetSessionId();
            auto self_player = cast_server->GetPlayerCacheUnique(self_session_id);
            self_player->state_id = PlayerInfo::State::WaitingRoom;
        }
    }
}