#pragma once
namespace Game::Handlers
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Cast;

#pragma pack(push, 1)

    struct Pickup2Req
    {
        uint32_t type, id;
    };

#pragma pack(pop)

    inline void UserItemPickup(SCallbackData& callback, CCastServer* server)
    {
        auto session = callback.session;
        auto message = callback.message;
        if (!session || !message) return;

        auto hostSid = message->GetSession();
        auto sid = session->GetSessionId();
		auto req = message->GetData<Pickup2Req*>();
        PACKETLOG(REQ, ITEM_PICKUP, "sid=({}) hostSid=({} itemType=({}) itemId=({})", sid, hostSid, req->type, req->id);

        server->Forward(hostSid, sid, *message);
    }
}