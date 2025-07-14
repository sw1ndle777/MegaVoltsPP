#pragma once
namespace Game
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Cast;

    namespace Handlers
    {
        inline void PlayerJoinRoom(SCallbackData& callback, CCastServer* cast_server)
        {
            auto session = callback.session;
            auto message = callback.message;
            if (!session || !message) return;

            std::shared_lock lock(session->GetMutex());
            auto host_session_id = message->GetSession();
            auto self_session_id = session->GetSessionId();

            BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "session id: ({}) attempt to join host session id: ({})'s room", self_session_id, host_session_id);

            auto host_player = cast_server->GetPlayerCacheShared(host_session_id);
            auto self_player = cast_server->GetPlayerCacheUnique(self_session_id);
            auto room = cast_server->GetRoomCacheUnique(host_player->room_id);


            if (cast_server->IsSessionIdAlready(self_session_id, room->players_session_id))
            {
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "session id: ({}) already in room, removing him", self_session_id);
                auto remove_myself = std::remove(room->players_session_id.begin(), room->players_session_id.end(), self_session_id);
                room->players_session_id.erase(remove_myself, room->players_session_id.end());
                return;
            }

            BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "session id: ({}) joined room id: ({})", self_session_id, host_player->room_id);
            room->players_session_id.push_back(self_session_id);
            self_player->room_id = host_player->room_id;
            self_player->host_session_id = host_player->session_id;
            self_player->in_room = true;
            self_player->state_id = PlayerInfo::State::WaitingRoom;
        }
    }
}