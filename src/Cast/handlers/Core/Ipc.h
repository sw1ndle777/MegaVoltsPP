#pragma once
namespace Game
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Cast;

    namespace Handlers
    {

#pragma pack(push, 1)

        struct ServerInfoHeader
        {
            uint64_t auth_key;
            uint16_t count;
            uint32_t mem;
            double   cpu;
        };

        struct PlayerInfoWire
        {
            uint16_t session_id;
            uint16_t room_id;
            uint16_t plaza_id;
            uint8_t  flags;
        };

#pragma pack(pop)

        inline uint8_t MakeFlags(bool in_room, bool in_plaza)
        {
            return (in_room ? 0x1 : 0) | (in_plaza ? 0x2 : 0);
        }

        inline void CastServerInfo(uint64_t auth_key, CCastServer* cast_server)
        {
            HANDLE m_process_handle = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, false, GetCurrentProcessId());
            auto cpu_usage = Utility::GetCpuUsage(m_process_handle);
            auto mem_usage = static_cast<uint32_t>(Utility::GetMemoryUsage(m_process_handle));
            CloseHandle(m_process_handle);
			auto sessions_list = cast_server->GetSessions();
           

            ServerInfoHeader hdr
            {
                .auth_key = auth_key,
                .count = static_cast<uint16_t>(sessions_list->size()),
                .mem = mem_usage,
                .cpu = cpu_usage
            };

            std::vector<uint8_t> out;
            out.resize(sizeof(hdr) + hdr.count * sizeof(PlayerInfoWire));
            std::memcpy(out.data(), &hdr, sizeof(hdr));
            std::vector<PlayerInfoWire> players;
            for (auto& sid : *sessions_list)
            {
                auto player = CAccount.get<shared_t>(sid.first);
                auto& room_id = player->room_id;
                auto& plaza_id = player->plaza_id;
                auto& in_room = player->in_room;
                auto& in_plaza = player->in_plaza;

                players.emplace_back(PlayerInfoWire{
                    .session_id = sid.first,
                    .room_id = room_id,
                    .plaza_id = plaza_id,
                    .flags = MakeFlags(in_room, in_plaza)
                });
            }

            auto* p = reinterpret_cast<PlayerInfoWire*>(out.data() + sizeof(hdr));
            for (size_t i = 0; i < players.size(); i++) 
            {
                const auto& src = players[i];
                p[i] = src;
            }

            cast_server->SendMainIpc(PacketIds::Ipc::CastToMainAckServerInfo, std::move(out));
        }
        inline void DisconnectPlayer(uint64_t auth_key, CCastServer* cast_server)
        {
			auto sid = *CAuthKey.get<shared_t>(auth_key);
            DEBUGLOG(dark_cyan, "ipc disconnect player auth key: ({}), sid=({})", auth_key, sid);
            if (auto pss = cast_server->GetSessionByIdNoLock(sid))
            {
                pss->Disconnect();
                DEBUGLOG(dark_cyan, "sid=({}) disconnected at ipc's request", sid);
            }
            else
                DEBUGLOG(dark_cyan, "sid=({}) disconnected at ipc's request ERROR SESSION ID NOT FOUND", sid);
        }
        inline void ChangeNewHost(uint64_t auth_key, uint16_t room_id, CCastServer* cast_server)
        {
            if (!CRoom.contains(room_id)) return;
			auto sid = *CAuthKey.get<shared_t>(auth_key);
            auto room = CRoom.get<unique_t>(room_id);
            if (!std::ranges::contains(room->players_session_id, sid)) return;
            room->host_session_id = sid;
            DEBUGLOG(dark_cyan, "roomId=({}) new host sid=({})", room_id, sid);
        }
        inline void ServerIpcMessage(std::shared_ptr<CSession> session, const uint32_t& msg_id, const uint32_t& data_size, const std::vector<uint8_t>& payload, CCastServer* cast_server)
        {
            DEBUGLOG(dark_cyan, "server_ipc_msg id: ({}), size: ({}) ", msg_id, data_size);

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
                    DEBUGLOG(yellow, "send ping assure from cast");
                    auto sid = Utility::FromVector<uint32_t>(payload);
                    if (auto player_session = cast_server->GetSessionByIdNoLock(sid))
                        player_session->SendMsg(0, 0, 0, 0); // send keep alive ack
                    break;
                }
                case PacketIds::Ipc::MainToCastAuthorizePlayer:
                {
                    struct AuthoriseData
                    {
                        NetEngine::Packets::Core::UniqueId old_uid;
                        NetEngine::Packets::Core::UniqueId uid;
                        uint64_t key;
                        char nickname[16];
                    };
                    auto auth_data = Utility::FromVector<AuthoriseData>(payload);
                    auto new_player = Player{ static_cast<uint16_t>(auth_data.uid.session), 0, 0, 0, static_cast<uint16_t>(auth_data.uid.server), PlayerInfo::State::Connected, false, false, auth_data.key, auth_data.nickname };
					CAccount.insert(new_player.session_id, new_player);
                    if (cast_server->AdoptSid(auth_data.old_uid.session, auth_data.uid.session))
                    {
                        if (auto pss = cast_server->GetSessionById(auth_data.uid.session))
                        {
                            pss->SendMsg(501, 0, 32, 1);
                            DEBUGLOG(dark_cyan, "authorized player ({}) with auth key: ({}), sid=({}), server: ({})", new_player.nickname.c_str(), new_player.auth_key, static_cast<uint16_t>(new_player.session_id), static_cast<uint16_t>(new_player.server_id));
                        }
                    }
                    else
                    {
						DEBUGLOG(red, "failed to adopt session id for player ({}) with auth key: ({}), sid=({}), server: ({})", new_player.nickname.c_str(), new_player.auth_key, static_cast<uint16_t>(new_player.session_id), static_cast<uint16_t>(new_player.server_id));
                    }
                    
                    break;
                }
                default:
                {
                    DEBUGLOG(yellow, "Unhandled server IPC message ID: {}", msg_id);
                    break;
                }
            }
        }
    }
}