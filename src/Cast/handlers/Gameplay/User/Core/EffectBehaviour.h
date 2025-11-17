#pragma once
namespace Game::Handlers
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Cast;
    using namespace NetEngine::Packets::Main;

#pragma pack(push, 1)

    struct InfoReq
    {
        uint32_t uid;//0x0000
		uint32_t idk1;//0x0004
        union 
        {
            struct 
            {
                uint32_t idk2 : 16;
                uint32_t tick1 : 24;
				uint32_t tick2 : 24;
            };
			uint64_t data;//0x0008
        };
        InventoryItemNumber item;
    };

#pragma pack(pop)

    inline void UserEffectBehaviour(SCallbackData& callback, CCastServer* server)
    {
        auto session = callback.session;
        auto message = callback.message;
        if (!session || !message) return;

        auto sid = session->GetSessionId();
		auto option = message->GetOption();
        for (uint8_t i = 0; i < option; i++)
        {
			auto req = reinterpret_cast<InfoReq*>(message->GetData() + (i * sizeof(InfoReq)));
            PACKETLOG(REQ, USER_EFFECT_BEHAVIOUR, "sid=({}) uid=({}) idk1=({}) idk2=({}) tick1=({}) tick2=({}) itemId=({}) itemStock=({})", 
				sid, req->uid, req->idk1, (uint32_t)req->idk2, (uint32_t)req->tick1, (uint32_t)req->tick2, (uint32_t)req->item.item_id, (uint32_t)req->item.stock);
        }

        auto hostSid = message->GetSession();
        server->Forward(hostSid, sid, *message);
    }
}