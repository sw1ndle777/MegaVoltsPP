#pragma once
namespace Game
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Front;

    namespace Handlers
    {
        inline void DeletePlayer(std::uint64_t auth_key, CFrontServer* front_server)
        {
            if (front_server->RemovePlayerCache(auth_key))
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "ipc delete player auth key: ({})", auth_key);
            else
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "ipc delete player auth key: ({}) not found", auth_key);
        }
        inline void ServerIpcMessage(std::shared_ptr<CSession> session, const std::uint32_t& msg_id, const std::uint32_t& data_size, const std::vector<std::uint8_t>& payload, CFrontServer* front_server)
        {
            BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "server_ipc_msg id: ({}), size: ({}) ", msg_id, data_size);

            switch (msg_id)
            {
                case PacketIds::Ipc::MainToFrontDisconnectPlayer:
                {
                    auto auth_key = Utility::FromVector<std::uint64_t>(payload);
                    auto player_cache = front_server->GetPlayerCacheUnique(auth_key);
                    if (player_cache->auth_key)
                    {
                        if (player_cache->forcefully_logged_out)
                        {
                            player_cache->forcefully_logged_out = false;
                        }
                        else
                            DeletePlayer(auth_key, front_server);
                    }
                    player_cache.unlock();
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