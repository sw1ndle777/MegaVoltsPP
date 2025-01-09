#pragma once
namespace Game
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Cast;

    namespace Handlers
    {
        inline void PlayerNicknames(SCallbackData& callback, CCastServer* cast_server)
        {
            std::shared_lock lock(callback.session->GetMutex());
            CSession* session = callback.session;
            CServer* server = callback.server;
            auto host_session_id = callback.message->GetSession();
            auto player_session_id = session->GetSessionId();

            auto message = callback.message;
            message->SetEncryptMethod(SendOption::EncryptionMethod::None);
            message->SetSession(host_session_id);
            PlayerNicknameRoomInMatchInfo* nickname_info = (PlayerNicknameRoomInMatchInfo*)callback.message->GetData();
            if (auto forwarded_session = server->GetSessionById(host_session_id))
            {
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "player session id: ({}) forward packet to session id: ({})", player_session_id, host_session_id);
                //forward player to host
                forwarded_session->Send(*message);
            }
            else
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "player session id: ({}) couldn't forward packet to session id: ({})", player_session_id, host_session_id);
        }
    }
}