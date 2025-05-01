#pragma once
namespace Game
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Cast;

    namespace Handlers
    {
        inline void PlayerWeaponAttack(SCallbackData& callback, CCastServer* cast_server)
        {
            auto session = callback.session;
            auto message = callback.message;
            if (!session || !message) return;

            std::shared_lock lock(session->GetMutex());

            CServer* server = callback.server;
            auto host_session_id = message->GetSession();
            auto self_session_id = session->GetSessionId();

            message->SetEncryptMethod(SendOption::EncryptionMethod::None);
            message->SetSession(self_session_id);

            if (auto forwarded_session = server->GetSessionById(host_session_id))
            {
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "session id: ({}) forward packet to host session id: ({})", self_session_id, host_session_id);
                //forward to host
                forwarded_session->Send(*message);
            }
            else
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::red, "session id: ({}) couldn't forward packet to host session id: ({})", self_session_id, host_session_id);
        }
    }
}