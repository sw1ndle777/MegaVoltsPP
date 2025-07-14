#pragma once
namespace Game
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Cast;

    namespace Handlers
    {
        inline void LeaveRoom(SCallbackData& callback, CCastServer* cast_server)
        {
            auto session = callback.session;
            if (!session) return;

            std::shared_lock lock(session->GetMutex());
            auto self_session_id = session->GetSessionId();
            auto self_player = cast_server->GetPlayerCacheUnique(self_session_id);
            if (self_player->in_room)
            {
                if (cast_server->IsRoomAlready(self_player->room_id))
                {
                    auto room_cache = cast_server->GetRoomCacheUnique(self_player->room_id);
                    auto remove_myself = std::remove(room_cache->players_session_id.begin(), room_cache->players_session_id.end(), self_session_id);
                    room_cache->players_session_id.erase(remove_myself, room_cache->players_session_id.end());
                    BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "session id: ({}) left room id: ({})", self_session_id, self_player->room_id);
                    if (!room_cache->players_session_id.size())
                    {
                        cast_server->RemoveRoomCache(self_player->room_id);
                        cast_server->SetRoomIdAvailable(self_player->room_id);
                        BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "session id: ({}) removed room id: ({})", self_session_id, self_player->room_id);
                    }
                }
                self_player->host_session_id = 0;
                self_player->state_id = PlayerInfo::State::Lobby;
                self_player->in_room = false;
            }
        }
    }
}