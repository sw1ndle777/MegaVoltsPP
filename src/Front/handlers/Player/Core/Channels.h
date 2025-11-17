#pragma once
namespace Game::Handlers
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets;
    using enum PacketDir;
#pragma pack(push, 1)
    struct ChannelInfo
    {
        uint32_t id;
        uint16_t cid;
        uint16_t sid;
    };
#pragma pack(pop)

    inline void Channels(SCallbackData& callback, CFrontServer* front_server)
    {
        auto session = callback.session;
        if (!session) return;
		auto sid = session->GetSessionId();

        ChannelInfo channels = { .id = 1, .cid = 1, .sid = 1 };

        session->SendMsg(INFO_SERVER, 0, 0, 1, reinterpret_cast<uint8_t*>(&channels), sizeof(ChannelInfo));
        PACKETLOG(ACK, INFO_SERVER, "sid=({})", sid); 
    }
}