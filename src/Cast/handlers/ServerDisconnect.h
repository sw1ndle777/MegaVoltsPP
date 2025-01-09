#pragma once
namespace Game
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Cast;

    namespace Handlers
    {
        inline void ServerDisconnect(std::shared_ptr<CSession> session, CCastServer* cast_server)
        {
            std::shared_lock lock(session->GetMutex());
            BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "session id: ({}) disconnected", session->GetSessionId());
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
                self_player->in_plaza = false;
            }
            if (self_player->in_room)
            {
                if (cast_server->IsRoomAlready(self_player->room_id))
                {
                    auto room_cache = cast_server->GetRoomCacheUnique(self_player->room_id);
                    auto remove_myself = std::remove(room_cache->players_session_id.begin(), room_cache->players_session_id.end(), self_session_id);
                    room_cache->players_session_id.erase(remove_myself, room_cache->players_session_id.end());
                    BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "session id: ({}) left room id: ({})", self_session_id, self_player->room_id);
                    if(!room_cache->players_session_id.size())
                    {
                        cast_server->RemoveRoomCache(self_player->room_id);
                        cast_server->SetRoomIdAvailable(self_player->room_id, true);
                        BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "session id: ({}) removed room id: ({})", self_session_id, self_player->room_id);
                    }
                }
                self_player->in_room = false;
            }
            self_player->state_id = PlayerInfo::State::Disconnected;
            cast_server->RemoveSession(self_session_id);
            cast_server->RemovePlayerCache(self_session_id);
        }
    }   
}