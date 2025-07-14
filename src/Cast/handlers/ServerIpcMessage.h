#pragma once
namespace Game
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Cast;

    namespace Handlers
    {
        inline void CastServerInfo(uint64_t auth_key, CCastServer* cast_server)
        {
            HANDLE m_process_handle = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, false, GetCurrentProcessId());
            auto cpu_usage = Utility::GetCpuUsage(m_process_handle);
            auto mem_usage = static_cast<uint32_t>(Utility::GetMemoryUsage(m_process_handle));
            CloseHandle(m_process_handle);
            auto sessions_count = static_cast<uint16_t>(cast_server->GetSessions()->size());
            struct ServerInfo
            {
                uint64_t auth_key{};
                uint16_t count{};
                uint32_t mem{};
                double cpu{};
            }info;
            info.auth_key = auth_key;
            info.count = sessions_count;
            info.mem = mem_usage;
            info.cpu = cpu_usage;

            cast_server->SendMainIpc(PacketIds::Ipc::CastToMainAckServerInfo, Utility::ToVector(info));
        }
        inline void DisconnectPlayer(uint64_t auth_key, CCastServer* cast_server)
        {
            //auto player_session = cast_server->GetSessionByAuthKey(auth_key);
            //auto player_session_id = player_session->GetSessionId();
            auto player = cast_server->GetPlayerCacheSharedByAuthKey(auth_key);
            auto player_session_id = player->session_id;
            BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "ipc disconnect player auth key: ({}), session id: ({})", auth_key, player_session_id);
            player.unlock();
            if (auto player_session = cast_server->GetSessionByIdNoLock(player_session_id))
            {
                player_session->Disconnect();
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "session id: ({}) disconnected at ipc's request", player_session_id);
            }
            else
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "session id: ({}) disconnected at ipc's request ERROR SESSION ID NOT FOUND", player_session_id);
        }
        inline void ChangeNewHost(uint64_t auth_key, uint16_t room_id, CCastServer* cast_server)
        {
            if (cast_server->IsRoomAlready(room_id))
            {
                //auto player_session = cast_server->GetSessionByAuthKey(auth_key);
                //auto player_session_id = player_session->GetSessionId();

                auto player = cast_server->GetPlayerCacheSharedByAuthKey(auth_key);
                auto player_session_id = player->session_id;
                player.unlock();
                auto room = cast_server->GetRoomCacheUnique(room_id);
                if (cast_server->IsSessionIdAlready(player_session_id, room->players_session_id))
                {
                    room->host_session_id = player_session_id;
                    BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "room id: ({}) new host session id: ({})", room_id, player_session_id);
                }
            }
        }
        inline void ServerIpcMessage(std::shared_ptr<CSession> session, const uint32_t& msg_id, const uint32_t& data_size, const std::vector<uint8_t>& payload, CCastServer* cast_server)
        {
            BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "server_ipc_msg id: ({}), size: ({}) ", msg_id, data_size);

            switch (msg_id)
            {
                case PacketIds::Ipc::MainToCastDisconnectPlayer:
                {
                    auto auth_key = Utility::FromVector<uint64_t>(payload);
                    DisconnectPlayer(auth_key, cast_server);
                    break;
                }
                case PacketIds::Ipc::MainToCastHostChange:
                {
                    struct RoomAuthData
                    {
                        uint16_t room_id;
                        uint64_t auth_key;
                    };
                    auto room_auth_data = Utility::FromVector<RoomAuthData>(payload);
                    ChangeNewHost(room_auth_data.auth_key, room_auth_data.room_id, cast_server);
                    break;
                }
                case PacketIds::Ipc::MainToCastReqServerInfo:
                {
                    auto auth_key = Utility::FromVector<uint64_t>(payload);
                    CastServerInfo(auth_key, cast_server);
                    break;
                }
                case PacketIds::Ipc::MainToCastSendPingAssure:
                {
                    BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::yellow, "send ping assure from cast");
                    struct MainToCastSendPingAssureInfo
                    {
                        uint32_t session_id;
                    };
                    auto data = Utility::FromVector<MainToCastSendPingAssureInfo>(payload);
                    if (auto player_session = cast_server->GetSessionByIdNoLock(data.session_id))
                    {
						player_session->SendMsg(0, 0, 0, 0); // send keep alive ack
                        /*
                        CMessage castPingAck = CMessage(player_session->GetEncryptionKey());
                        castPingAck.SetSession(player_session->GetSessionId());
                        castPingAck.SetCommand(72, 1, 0x00, 0);
                        player_session->Send(castPingAck);
                        */
                        //player_session->Send(castPingAck);
                        //player_session->Send(castPingAck);
                        //castPingAck.SetOption(3);
                        //player_session->Send(castPingAck);
                    }
                    break;
                }
                case PacketIds::Ipc::MainToCastSendPacket:
                {
                    struct MainToCastSendPacketInfo
                    {
                        uint32_t session_id;
                        uint32_t data_size;
                        uint32_t item_id;
                    };
                    auto data = Utility::FromVector<MainToCastSendPacketInfo>(payload);
                    if (auto player_session = cast_server->GetSessionByIdNoLock(data.session_id))
                    {
                        struct new_data {
                            uint32_t item_id;
                            uint32_t unk;
                        } news_data;
                        news_data.item_id = data.item_id;
                        player_session->SendMsg(96, 0, 0, 0, reinterpret_cast<uint8_t*>(&news_data), sizeof(news_data));
                    }
                    break;
                }
                default:
                {
                    BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::yellow, "Unhandled server IPC message ID: {}", msg_id);
                    break;
                }
            }
        }
    }
}