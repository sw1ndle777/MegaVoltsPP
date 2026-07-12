#pragma once
namespace Game::Handlers
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Cast;
    inline void HostModMatchStart(SCallbackData& callback, CCastServer* server)
    {
        auto session = callback.session;
        auto message = callback.message;
        if (!session || !message) return;

        auto order = magic_enum::enum_cast<EOrder>(u16_cast(message->GetOrder())).value_or(EOrder::NONE);

        auto sid = message->GetSession();
        auto hostSid = session->GetSessionId();
		auto serverTick = message->GetData<uint64_t>();
        if (sid == hostSid) return;

        auto host = CAccount.get<shared_t>(hostSid);
        if (!host) return;
        auto room_id = host->room_id;
        auto host_name = host->nickname;
        auto host_sid_cached = host->session_id;
        host.unlock();

        auto user = CAccount.get<shared_t>(sid);
        if (!user) return;
        auto user_name = user->nickname;
        user.unlock();

        auto room = CRoom.get<shared_t>(room_id);
        if (!room) return;
        if (host_sid_cached != room->host_session_id)
        {
            auto orderName = magic_enum::enum_name(order);
            DEBUGLOG(yellow, "({}): host=({}) hostSid=({}) is not host of roomId=({})", orderName, host_name, hostSid, room_id);
            return;
        }
        room.unlock();

        PACKETLOG(ACK, order, "roomId=({}) user=({}) sid=({}) from host=({}) hostSid=({}) serverTick=({})", room_id, user_name, sid, host_name, hostSid, serverTick);
        server->Forward(sid, hostSid, *message);
    }
}