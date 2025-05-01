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

            auto session = callback.session;
            auto message = callback.message;
            if (!session || !message) return;

            std::unique_lock lock(session->GetMutex());
            CastConnectionReq* castConnecReq = (CastConnectionReq*)message->GetData();
            session->SetSessionId(castConnecReq->UniqueId.session);

            auto new_player = Player{ static_cast<uint16_t>(castConnecReq->UniqueId.session), 0, 0, 0, static_cast<uint16_t>(castConnecReq->UniqueId.server), PlayerInfo::State::Connected, false, false, castConnecReq->Authkey };

            cast_server->AddPlayerCache(castConnecReq->UniqueId.session, new_player);
            session->SendMsg(501, 0, 32, 1);

            BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "session id: ({}) connected with auth_key: ({}) from server_id: ({})", session->GetSessionId(), static_cast<uint64_t>(castConnecReq->Authkey), static_cast<uint32_t>(castConnecReq->UniqueId.server));

        }
    }
    
}