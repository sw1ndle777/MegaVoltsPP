#pragma once
namespace Game
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Front;

    namespace Handlers
    {
        inline void ServerDisconnect(std::shared_ptr<CSession> session, CFrontServer* front_server)
        {
            std::shared_lock lock(session->GetMutex());
            EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "session id: ({}) disconnected", session->GetSessionId());
            front_server->RemoveSession(session->GetSessionId());
        }
    }
    
}