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
            auto session = callback.session;
            auto message = callback.message;
            if (!session || !message) return;

            std::shared_lock lock(session->GetMutex());
            CServer* server = callback.server;
            auto self_session_id = session->GetSessionId();
            auto self_player = cast_server->GetPlayerCacheUnique(self_session_id);
            auto room = cast_server->GetRoomCacheShared(self_player->room_id);
            self_player->state_id = PlayerInfo::State::StartMatch;
            static auto broadcast = [&](auto player_session_id, auto& msg)
            {
                msg.SetEncryptMethod(SendOption::EncryptionMethod::None);
                msg.SetSession(player_session_id);
                if (auto player_session = server->GetSessionById(player_session_id))
                    player_session->Send(msg);
                else
                {
                    BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::red, "couldn't broadcast packet to session id: ({})", player_session_id);
                    auto session_ids = server->GetSessions();
                    for (auto& [id, session] : *session_ids)
                    {
                        BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::red, "exists on cast session id: ({})", id);
                    }
                }     
            };

            CMessage msg = CMessage();
            msg.SetSession(session->GetSessionId());
            msg.SetCommand(message->GetOrder(), message->GetMission(), message->GetExtra(), message->GetOption());
            auto my_unique_id = NetEngine::Packets::Core::UniqueId(self_session_id, self_player->server_id).data;
            msg.SetData(reinterpret_cast<uint8_t*>(&my_unique_id), sizeof(my_unique_id));

            lock.unlock();
            for (const auto& id : room->players_session_id)
                broadcast(id, msg);
        }
    }
}