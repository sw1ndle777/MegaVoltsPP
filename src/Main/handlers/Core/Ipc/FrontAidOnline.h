#pragma once
namespace Game::Handlers
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Front;
    struct ReqAidOnline
    {
        uint32_t sid;
        int32_t aid;
    };
    struct AcknowledgeAidOnline
    {
        uint32_t sid;
        int32_t aid;
        bool isOnline;
    };
    inline void IpcFrontAidOnline(const std::vector<uint8_t>& payload, CMainServer* main_server)
    {
        auto req = Utility::FromVector<ReqAidOnline>(payload);
        AcknowledgeAidOnline ack =
        {
            .sid = req.sid,
            .aid = req.aid,
            .isOnline = CAidSid.contains(req.aid)
        };
        main_server->SendFrontIpc(PacketIds::Ipc::MainToFrontAcknowledgeAidOnline, Utility::ToVector(ack));
    }
}