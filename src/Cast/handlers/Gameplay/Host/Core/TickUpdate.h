#pragma once
namespace Game::Handlers
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Cast;
    inline void HostTickUpdate(SCallbackData& callback, CCastServer* server)
    {
        auto session = callback.session;
        auto message = callback.message;
        if (!session || !message) return;

        auto order = magic_enum::enum_cast<EOrder>(u16_cast(message->GetOrder())).value_or(EOrder::NONE);
        auto roomTick = message->GetData<uint32_t>();

        auto userSid = callback.message->GetSession();
        auto hostSid = session->GetSessionId();
        auto host = CAccount.get<shared_t>(hostSid);
        if (!host) return;
        auto hostName = host->nickname;
        auto roomId = host->room_id;
        host.unlock();

        server->Forward(userSid, hostSid, *message);
        
    }
}