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
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "ipc disconnect player auth key: ({}), session id: ({})", auth_key, player_session_id);
                main_server->DisconnectPlayer(main_server, player_session_id, auth_key, Disconnect::Block);
            }
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
                default:
                {
                    BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::yellow, "Unhandled server IPC message ID: {}", msg_id);
                    break;
                }
            }
        }
    }
}