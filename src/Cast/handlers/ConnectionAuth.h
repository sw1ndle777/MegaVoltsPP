#pragma once
namespace Game
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Cast;

    namespace Handlers
    {
        inline void ConnectionAuth(SCallbackData& callback, CCastServer* cast_server)
        {
            std::unique_lock lock(callback.session->GetMutex());
            CSession* session = callback.session;
            CastConnectionReq* castConnecReq = (CastConnectionReq*)callback.message->GetData();
            session->SetSessionId(castConnecReq->UniqueId.session);

            auto new_player = Player{ static_cast<std::uint16_t>(castConnecReq->UniqueId.session), 0, 0, 0, static_cast<std::uint16_t>(castConnecReq->UniqueId.server), PlayerInfo::State::Connected, false, false, castConnecReq->Authkey };

            cast_server->AddPlayerCache(castConnecReq->UniqueId.session, new_player);


            CMessage castConnAckMsg = CMessage(session->GetEncryptionKey());
            castConnAckMsg.SetSession(session->GetSessionId());
            castConnAckMsg.SetCommand(501, 0, 32, 1);
            session->Send(castConnAckMsg);

            BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "session id: ({}) connected with auth_key: ({}) from server_id: ({})", session->GetSessionId(), static_cast<std::uint64_t>(castConnecReq->Authkey), static_cast<std::uint32_t>(castConnecReq->UniqueId.server));
        }
    }
    
}