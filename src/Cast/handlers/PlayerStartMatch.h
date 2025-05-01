#pragma once
namespace Game
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Cast;

    namespace Handlers
    {
        inline void PlayerStartMatch(SCallbackData& callback, CCastServer* cast_server)
        {
            auto session = callback.session;
            auto message = callback.message;
            if (!session || !message) return;

            std::shared_lock lock(session->GetMutex());
            CServer* server = callback.server;
            auto player_session_id = message->GetSession();
            auto host_session_id = session->GetSessionId();
            if (player_session_id != host_session_id)
            {
                message->SetEncryptMethod(SendOption::EncryptionMethod::None);
                message->SetSession(host_session_id);
                if (auto forwarded_session = server->GetSessionById(player_session_id))
                {
                    BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "host session id: ({}) forward packet to session id: ({})", host_session_id, player_session_id);
                    //host forward to player
                    forwarded_session->Send(*message);
                    auto self_player = cast_server->GetPlayerCacheUnique(player_session_id);
                    self_player->state_id = PlayerInfo::State::StartMatch;
                }
                else
                    BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "host session id: ({}) couldn't forward packet to session id: ({})", host_session_id, player_session_id);
            }
        }
    } 
}