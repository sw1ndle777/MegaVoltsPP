#pragma once
namespace Game
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Front;

    namespace Handlers
    {
        inline void DisconnectPlayerMultipleLogin(std::uint64_t auth_key, CMainServer* main_server)
        {
            auto player = main_server->GetAccCacheSharedByAuthKey(auth_key);
            auto player_session_id = player->session_id;
            if (player_session_id)
            {
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "ipc disconnect player request auth key: ({}), session id: ({})", auth_key, player_session_id);
                main_server->DisconnectPlayer(main_server, player_session_id, auth_key, Disconnect::Block);
            }
            else
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "ipc disconnect player request auth key: ({}), session id: ({}) ERROR SESSION ID NULL", auth_key, player_session_id);
            player.unlock();
        }
        inline void ServerIpcMessage(std::shared_ptr<CSession> session, const std::uint32_t& msg_id, const std::uint32_t& data_size, const std::vector<std::uint8_t>& payload, CMainServer* main_server)
        {
            BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "server_ipc_msg id: ({}), size: ({}) ", msg_id, data_size);

            switch (msg_id)
            {
                case PacketIds::Ipc::FrontToMainDisconnectPlayer:
                {
                    auto auth_key = Utility::FromVector<std::uint64_t>(payload);
                    DisconnectPlayerMultipleLogin(auth_key, main_server);
                    break;
                }
                case PacketIds::Ipc::CastToMainAckServerInfo:
                {
                    struct ServerInfo
                    {
                        std::uint64_t auth_key{};
                        std::uint16_t count{};
                        std::uint32_t mem{};
                        double cpu{};
                    };
                    auto info = Utility::FromVector<ServerInfo>(payload);
                    auto player = main_server->GetAccCacheSharedByAuthKey(info.auth_key);
                    auto player_session_id = player->session_id;
                    player.unlock();
                    if (player_session_id)
                    {
                        if (auto player_session = main_server->GetSessionById(player_session_id))
                        {
                            auto msg = fmt::format("[MegaVolts Online] Cast Info: Sessions Online: {}, Memory Usage: {} MB, Cpu Usage: {:.2f}%",
                                static_cast<std::uint16_t>(info.count),
                                static_cast<std::uint32_t>(info.mem),
                                static_cast<double>(info.cpu));
                            main_server->SendServerMessage(player_session.get(), msg.c_str());
                        }
                    }
                    break;
                }
                case PacketIds::Ipc::CastToMainPlayerAuthorizeInfo:
                {
                    struct PlayerAuthorizeCastToMainInfo
                    {
                        std::uint32_t session_id;
                    };
                    auto info = Utility::FromVector<PlayerAuthorizeCastToMainInfo>(payload);
                    if (auto player_session = main_server->GetSessionById(info.session_id))
                    {
                        auto msg = fmt::format("[CAST] session id: ({})", static_cast<std::uint32_t>(info.session_id));
                        main_server->SendServerMessage(player_session.get(), msg.c_str());
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