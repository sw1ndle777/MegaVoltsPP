#pragma once
namespace Game
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Main;

    namespace Handlers
    {
        inline void LobbyUserList(SCallbackData& callback, CMainServer* main_server)
        {
            auto send_msg = [&](CSession* session, std::uint16_t order, std::uint8_t mission, std::uint8_t extra, std::uint8_t option, std::uint8_t* data = nullptr, std::size_t data_size = 0)
            {
                CMessage message(session->GetEncryptionKey());
                message.SetSession(session->GetSessionId());
                message.SetCommand(order, mission, extra, option);
                if (data_size > 0 && data != nullptr) message.SetData(data, data_size);
                session->Send(message);
            };
            std::shared_lock lock(callback.session->GetMutex());
            CSession* session = callback.session;
            auto session_id = session->GetSessionId();
            auto acc_cache = main_server->GetAccCacheSharedBySessionId(session_id);
            auto acc_index = acc_cache->acc_info.Index;
            if (acc_index == -1) return;
           
            std::shared_lock acc_lock(main_server->GetAccountsCacheMutex());
            if (accounts_cache.size() <= 1)
            {
                send_msg(session, 84, 0, Userlist::ListResult::NoUsers, 0);
                return;
            }
            std::vector<PlayerAgoraInfo> user_list;
           
            for (const auto& user : accounts_cache)
            {
                if (user.first != session_id)
                {
                    if (acc_cache->in_plaza && user.second.in_plaza && !acc_cache->in_room && !user.second.in_room)
                        user_list.push_back({ user.second.acc_info.Nickname, NetEngine::Packets::Core::UniqueId(user.first, 1).data , user.second.acc_info.Level + 1, 0, 0 });
                    else if (!acc_cache->in_plaza && !user.second.in_plaza && !acc_cache->in_room && !user.second.in_room)
                        user_list.push_back({ user.second.acc_info.Nickname, NetEngine::Packets::Core::UniqueId(user.first, 1).data , user.second.acc_info.Level + 1, 0, 0 });
                }
            }
            
                   

            if (user_list.size() <= 0) 
            {
                send_msg(session, 84, 0, Userlist::ListResult::NoUsers, 0);
                return;
            }
            std::uint32_t total_users_fragments = (user_list.size() == 0) ? 0 : (user_list.size() / 51) + 1;
            for (std::uint32_t i = 0; i < total_users_fragments; i++)
            {
                std::vector<PlayerAgoraInfo> users_batch;
                std::uint8_t user_list_result = (i == 0) ? Userlist::ListResult::Users : Userlist::ListResult::Users2;
                std::uint32_t start_index = i * 51;
                std::uint32_t end_index = std::min(start_index + 51, static_cast<std::uint32_t>(user_list.size()));
                for (auto j = start_index; j < end_index; j++)
                    users_batch.push_back(user_list[j]);


                send_msg(session, 84, 0, user_list_result, users_batch.size(), reinterpret_cast<uint8_t*>(users_batch.data()), users_batch.size() * sizeof(PlayerAgoraInfo));
            }     
        }
    }
}