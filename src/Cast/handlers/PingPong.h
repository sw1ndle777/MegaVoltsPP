#pragma once
namespace Game
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Cast;

    namespace Handlers
    {
        inline void PingPong(SCallbackData& callback, CCastServer* cast_server)
        {
            std::shared_lock lock(callback.session->GetMutex());
            CSession* session = callback.session;

            CMessage castPingAck = CMessage(session->GetEncryptionKey());
            castPingAck.SetSession(session->GetSessionId());
            castPingAck.SetCommand(72, callback.message->GetMission(), 0x00, callback.message->GetOption());
            session->Send(castPingAck);
        }
    }
}