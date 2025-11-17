#pragma once
namespace Game::Handlers
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Front;
    inline void ServerIpc(std::shared_ptr<CSession> session, const uint32_t& msg_id, const uint32_t& data_size, const std::vector<uint8_t>& payload, CFrontServer* front_server)
    {
        DEBUGLOG(dark_cyan, "ipc=({}) size=({}) ", msg_id, data_size);
		//DEBUGLOG(dark_cyan, "ipc: ({}), size: ({}) ", msg_id, data_size);
        switch (msg_id)
        {
        case PacketIds::Ipc::MainToFrontAcknowledgeAidOnline: IpcMainAidOnline(payload, front_server); break;
        case PacketIds::Ipc::MainToFrontAcknowledgeAidDisconnected: IpcMainDisconnect(payload, front_server); break;
        default:
        {
            DEBUGLOG(yellow, "unknown ipc=({}) size=({})", msg_id, data_size);
            break;
        }
        }
    }
}