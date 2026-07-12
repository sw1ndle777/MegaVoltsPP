#pragma once
namespace Game::Handlers
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Core;
    using namespace NetEngine::Packets::Cast;

#pragma pack(push, 1)
    struct PickupItem
    {
        uint32_t idk, id;
    };
    struct PickupReq
    {
        UniqueId uid;//0x0000
        uint32_t count;//0x0004
        PickupItem item[81];
    };

#pragma pack(pop)

    inline void UserItemKitdropGet(SCallbackData& callback, CCastServer* server)
    {
        auto session = callback.session;
        auto message = callback.message;
        if (!session || !message) return;

        auto hostSid = message->GetSession();
        auto sid = session->GetSessionId();
		auto option = message->GetOption();
        for (uint8_t i = 0; i < option; i++)
        {
			auto req = reinterpret_cast<PickupReq*>(message->GetData());

            for (uint32_t j = 0; j < req->count && j < 81; j++)
            {
                // item[j].id is the packed KitDropInfo word: itemId:23, itemType:5, flag:1.
                // Decode it so it matches the Spawn/Info logs (raw kept for cross-checks).
                const uint32_t packed = req->item[j].id;
                const uint32_t realItemId = packed & 0x7FFFFF;
                const uint32_t itemType = (packed >> 23) & 0x1F;
                const uint32_t flag = (packed >> 28) & 0x1;
                PACKETLOG(REQ, ITEM_KITDROP_GET, "sid=({}) hostSid=({}) uid=({}) size=({}) itemIdk=({}) raw=({}) itemId=({}) itemType=({}) flag=({})",
                    sid, hostSid, req->uid.data, req->count, req->item[j].idk, packed, realItemId, itemType, flag);
            }
        }
       

        server->Forward(hostSid, sid, *message);
    }
}