#pragma once
namespace Game
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Cast;

    namespace Handlers
    {
        inline void CreateRoom(SCallbackData& callback, CCastServer* cast_server)
        {
            std::shared_lock lock(callback.session->GetMutex());
            CSession* session = callback.session;
            CServer* server = callback.server;
            auto self_session_id = session->GetSessionId();
            auto self_player = cast_server->GetPlayerCacheUnique(self_session_id);
            uint16_t current_room_id = 0;
            server->GetNextAvailableRoomId(current_room_id);          
            Game::Room new_room = { current_room_id , self_session_id };
            new_room.players_session_id.push_back(self_session_id);
            cast_server->AddRoomCache(current_room_id, new_room);
            //server->SetRoomIdAvailable(current_room_id);
            self_player->room_id = current_room_id;
            self_player->host_session_id = self_session_id;
            self_player->in_room = true;
            self_player->state_id = PlayerInfo::State::WaitingRoom;
            BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "session id: ({}) created room id: ({})", self_session_id, current_room_id);
        }
    }
}