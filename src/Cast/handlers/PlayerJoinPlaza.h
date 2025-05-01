#pragma once
namespace Game
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Cast;

    namespace Handlers
    {
        inline void PlayerJoinPlaza(SCallbackData& callback, CCastServer* cast_server)
        {
            auto session = callback.session;
            auto message = callback.message;
            if (!session || !message) return;

            std::shared_lock lock(session->GetMutex());
            CServer* server = callback.server;

            auto self_session_id = session->GetSessionId();
            auto self_player = cast_server->GetPlayerCacheUnique(self_session_id);

            auto plazaReq = reinterpret_cast<CastJoinPlazaReq*>(message->GetData());
            auto plaza_id = plazaReq->plaza_id;

           

            
            BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "session id: ({}) attempt to join plaza id: ({}), mission: ({}),  extra: ({}), option: ({})", self_session_id, plaza_id, message->GetMission(), message->GetExtra(), message->GetOption());



            auto join_plaza_exists = cast_server->IsPlazaAlready(plaza_id);
            auto new_plaza = cast_server->GetPlazaCacheShared(plaza_id);
            if (cast_server->IsSessionIdAlready(self_session_id, new_plaza->players_session_id))
            {
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "session id: ({}) already in plaza id: ({})", self_session_id, plaza_id);
                new_plaza.unlock();
                return;
            }
            new_plaza.unlock();
            auto old_plaza_id = self_player->plaza_id;

            if (plaza_id != old_plaza_id)
            {
                self_player->in_plaza = false;
                if (cast_server->IsPlazaAlready(old_plaza_id))
                {
                    auto old_plaza = cast_server->GetPlazaCacheUnique(old_plaza_id);
                    auto remove_myself = std::remove(old_plaza->players_session_id.begin(), old_plaza->players_session_id.end(), self_session_id);
                    old_plaza->players_session_id.erase(remove_myself, old_plaza->players_session_id.end());
                    BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "session id: ({}) left plaza id: ({})", self_session_id, old_plaza_id);

                    if (!old_plaza->players_session_id.size())
                    {
                        cast_server->RemovePlazaCache(old_plaza_id);
                        BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "session id: ({}) removed plaza id: ({})", self_session_id, old_plaza_id);
                    }
                    old_plaza.unlock();
                }
            }

            if (join_plaza_exists)
            {
                auto new_plaza = cast_server->GetPlazaCacheUnique(plaza_id);
                new_plaza->players_session_id.push_back(self_session_id);
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "session id: ({}) joined plaza id: ({})", self_session_id, plaza_id);
                new_plaza.unlock();
            }
            else
            {
                Game::Plaza new_plaza = { static_cast<uint16_t>(plaza_id) };
                new_plaza.players_session_id.push_back(self_session_id);
                cast_server->AddPlazaCache(plaza_id, new_plaza);
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "plaza id: ({}) doesn't exist, auto create", plaza_id);
            }

            self_player->plaza_id = plaza_id;
            self_player->in_plaza = true;
            session->SendMsg(176, message->GetMission(), 6, message->GetOption());
        }
    }
}