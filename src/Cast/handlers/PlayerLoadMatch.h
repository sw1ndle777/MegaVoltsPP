#pragma once
namespace Game
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Cast;

    namespace Handlers
    {
        inline void PlayerLoadMatch(SCallbackData& callback, CCastServer* cast_server)
        {
            std::shared_lock lock(callback.session->GetMutex());
            CSession* session = callback.session;
            CServer* server = callback.server;
            auto self_session_id = session->GetSessionId();
            auto self_player = cast_server->GetPlayerCacheUnique(self_session_id);
            auto room = cast_server->GetRoomCacheShared(self_player->room_id);
            self_player->state_id = PlayerInfo::State::StartMatch;
            auto broadcast = [&](auto player_session_id, auto& msg)
            {
                msg.SetEncryptMethod(SendOption::EncryptionMethod::None);
                msg.SetSession(player_session_id);
                if (auto player_session = server->GetSessionById(player_session_id))
                    player_session->Send(msg);
                else
                    BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::red, "couldn't broadcast packet to session id: ({})", player_session_id);
            };

            CMessage msg = CMessage();
            msg.SetSession(session->GetSessionId());
            msg.SetCommand(callback.message->GetOrder(), callback.message->GetMission(), callback.message->GetExtra(), callback.message->GetOption());
            auto my_unique_id = NetEngine::Packets::Core::UniqueId(self_session_id, self_player->server_id).data;
            msg.SetData(reinterpret_cast<uint8_t*>(&my_unique_id), sizeof(my_unique_id));

            lock.unlock();
            for (const auto& id : room->players_session_id)
                broadcast(id, msg);
        }
    }
}