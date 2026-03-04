#pragma once
namespace Game
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Cast;

    namespace Handlers
    {



        inline void ServerIpcMessage(std::shared_ptr<CSession> session, const uint32_t& msg_id, const uint32_t& data_size, const std::vector<uint8_t>& payload, CCastServer* cast_server)
        {
            DEBUGLOG(dark_cyan, "server_ipc_msg id: ({}), size: ({}) ", msg_id, data_size);

            switch (msg_id)
            {
            case PacketIds::Ipc::MainToCastDisconnectPlayer: IpcMainDisconnect(payload, cast_server); break;
			case PacketIds::Ipc::MainToCastHostChange: IpcMainHostChange(payload, cast_server); break;
			case PacketIds::Ipc::MainToCastReqServerInfo: IpcMainServerInfo(payload, cast_server); break;
			case PacketIds::Ipc::MainToCastSendPingAssure: IpcMainPingAssure(payload, cast_server); break;
			case PacketIds::Ipc::MainToCastAuthorizePlayer: IpcMainAuthorize(payload, cast_server); break;
            default:
            {
                DEBUGLOG(yellow, "Unhandled server IPC message ID: {}", msg_id);
                break;
            }
            }
        }
    }
}