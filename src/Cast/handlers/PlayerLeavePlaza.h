#pragma once
namespace Game
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Cast;

    namespace Handlers
    {
        inline void PlayerLeavePlaza(SCallbackData& callback, CCastServer* cast_server)
        {
            auto session = callback.session;
            auto message = callback.message;
            if (!session || !message) return;

            std::shared_lock lock(session->GetMutex());
            auto self_session_id = session->GetSessionId();
            auto self_player = cast_server->GetPlayerCacheShared(self_session_id);
            if (self_player->in_plaza)
            {
                if (cast_server->IsPlazaAlready(self_player->plaza_id))
                {
                    auto plaza_cache = cast_server->GetPlazaCacheUnique(self_player->plaza_id);
                    auto remove_myself = std::remove(plaza_cache->players_session_id.begin(), plaza_cache->players_session_id.end(), self_session_id);
                    plaza_cache->players_session_id.erase(remove_myself, plaza_cache->players_session_id.end());
                    BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "session id: ({}) left plaza id: ({})", self_session_id, self_player->plaza_id);
                    if (!plaza_cache->players_session_id.size())
                    {
                        cast_server->RemovePlazaCache(self_player->plaza_id);
                        BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "session id: ({}) removed plaza id: ({})", self_session_id, self_player->plaza_id);
                    }
                }
                self_player->state_id = PlayerInfo::State::Lobby;
                self_player->in_plaza = false;
            }
            session->SendMsg(176, message->GetMission(), message->GetExtra(), message->GetOption());
        }
    }  
}