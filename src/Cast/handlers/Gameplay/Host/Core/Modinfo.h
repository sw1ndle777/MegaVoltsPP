#pragma once
namespace Game::Handlers
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Cast;
    inline void HostModinfo(SCallbackData& callback, CCastServer* server)
    {
        auto session = callback.session;
        auto message = callback.message;
        if (!session || !message) return;
        
        auto order = magic_enum::enum_cast<EOrder>(u16_cast(message->GetOrder())).value_or(EOrder::NONE);

        auto userSid = callback.message->GetSession();
        auto hostSid = session->GetSessionId();
        auto host = CAccount.get<shared_t>(hostSid);
        if (!host) return;
        auto hostName = host->nickname;
        auto roomId = host->room_id;
        host.unlock();

        auto user = CAccount.get<shared_t>(userSid);
        if (!user) return;
        auto userName = user->nickname;
        user.unlock();

        PACKETLOG(ACK, order, "roomId=({})  user=({}) sid=({}) from host=({}) hostSid=({})", roomId, userName, userSid, hostName, hostSid);
        server->Forward(userSid, hostSid, *message);
    }
}