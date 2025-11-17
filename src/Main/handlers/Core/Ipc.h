#pragma once
namespace Game::Handlers
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Front;
    inline void ServerIpc(std::shared_ptr<CSession> session, const uint32_t& msg_id, const uint32_t& data_size, const std::vector<uint8_t>& payload, CMainServer* main_server)
    {
        switch (msg_id)
        {
        case PacketIds::Ipc::CastToMainAckServerInfo: IpcCastMetrics(payload, main_server); break;
        case PacketIds::Ipc::CastToMainPlayerAuthorizeInfo: IpcCastSid(payload, main_server); break;
        case PacketIds::Ipc::FrontToMainTryLoginPlayer: IpcFrontAidOnline(payload, main_server); break;
        case PacketIds::Ipc::FrontToMainDisconnectPlayer: IpcFrontDisconnect(payload, main_server); break;
        case PacketIds::Ipc::CastToMainAcknowledgeAuthPlayer: IpcCastAuthorize(payload, main_server); break;
        default:
        {
            DEBUGLOG(yellow, "unknown ipc=({}) size=({})", msg_id, data_size);
            break;
        }
        }
    }
}