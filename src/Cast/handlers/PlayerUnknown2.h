#pragma once
namespace Game
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Cast;

    namespace Handlers
    {
        inline void PlayerUnknown2(SCallbackData& callback, CCastServer* cast_server)
        {
            std::shared_lock lock(callback.session->GetMutex());
            CSession* session = callback.session;
            CServer* server = callback.server;
            auto player_session_id = callback.message->GetSession();
            auto host_session_id = session->GetSessionId();
            auto message = callback.message;
            message->SetEncryptMethod(SendOption::EncryptionMethod::None);
            message->SetSession(host_session_id);

            if (auto forwarded_session = server->GetSessionById(player_session_id))
            {
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "host session id: ({}) forward packet to session id: ({})", host_session_id, player_session_id);
                //host forward to player
                forwarded_session->Send(*message);
            }
            else
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "host session id: ({}) couldn't forward packet to session id: ({})", host_session_id, player_session_id);
        }
    }
}