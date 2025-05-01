#pragma once
namespace Game
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Main;

    namespace Handlers
    {
        inline void PlayerBlock(SCallbackData& callback, CMainServer* main_server)
        {
            auto session = callback.session;
            if (!session) return;

            CMessage msgCopy = *callback.message;
            BaseLib::DbPool->submit_task([main_server, session = std::move(callback.session), message = std::move(msgCopy)]() mutable
            {
                std::shared_lock lock(session->GetMutex());
                auto session_id = session->GetSessionId();
                auto acc_cache = main_server->GetAccCacheUniqueBySessionId(session_id);
            
                auto acc_index = acc_cache->acc_info.Index;
                if (acc_index == -1) return;

                const auto& blockedAddReq = reinterpret_cast<MainPlayerBlockedAddReq*>(message.GetData());
                const auto& target_nickname = Utility::ReadMicrovoltsString(blockedAddReq->nickname, sizeof(blockedAddReq->nickname));
                auto target_acc_cache = main_server->GetAccCacheUniqueByNickname(target_nickname.c_str());

                if (target_acc_cache->acc_info.Index == -1)
                {
                    uint32_t target_acc_id = 0;
                    if (!BaseLib::Database->NicknameExists(target_nickname.c_str(), target_acc_id))
                    {
                        session->SendMsg(52, 0, Userlist::Blocked::AddResult::Offline, 0);
                        return;
                    }
                    auto blockeds = main_server->GetBlockedsList(session_id);
                    if (main_server->IsBlockedAlready(blockeds, target_acc_id)) return;
                    blockeds.unlock();
                    BlockedInfo newBlockedInfo = { acc_index, static_cast<int32_t>(target_acc_id), 0, target_nickname.c_str() };
                    FriendInfo delFriendInfo = { acc_index, static_cast<int32_t>(target_acc_id) };
                    FriendInfo delFriendInfo2 = { static_cast<int32_t>(target_acc_id), acc_index };
                    MainPlayerBlockedAddAck blocked_data = { target_acc_id, target_nickname.c_str() };
                    session->SendMsg(52, 0, Userlist::Blocked::AddResult::Success, 0, reinterpret_cast<uint8_t*>(&blocked_data), sizeof(MainPlayerBlockedAddAck));

                    auto post_acc_cache = main_server->GetAccCacheUniqueByNickname(target_nickname.c_str());

                    main_server->RemovePlayerFriends(session_id, target_acc_id);
                    main_server->AddPlayerFriendsDeleted(post_acc_cache, delFriendInfo);
                    main_server->RemovePlayerBlockedsDeleted(post_acc_cache, target_acc_id);
                    main_server->AddPlayerBlockedsAdded(post_acc_cache, newBlockedInfo);
                    main_server->AddPlayerBlockeds(session_id, newBlockedInfo);
                    BaseLib::Database->DeletePlayerFriends({ delFriendInfo, delFriendInfo2 });
                    BaseLib::Database->InsertPlayerBlockeds(acc_cache->blockeds_added);

                    return;
                }
                auto blockeds = main_server->GetBlockedsList(session_id);
                if (main_server->IsBlockedAlready(blockeds, target_acc_cache->acc_info.Index)) return;
                blockeds.unlock();
                const BlockedInfo& newBlockedInfo = { acc_index, target_acc_cache->acc_info.Index, target_acc_cache->session_id ? static_cast<uint32_t>(target_acc_cache->session_id) : 0, target_nickname.c_str() };

                if (target_acc_cache->session_id)
                {
                    MainPlayerBlockedAddAck blocked_data = { static_cast<uint32_t>(target_acc_cache->acc_info.Index), target_nickname.c_str() };
                    session->SendMsg(52, 0, Userlist::Blocked::AddResult::Success, 0, reinterpret_cast<uint8_t*>(&blocked_data), sizeof(MainPlayerBlockedAddAck));
                    main_server->RemovePlayerFriends(target_acc_cache->session_id, acc_index);
                    main_server->AddPlayerFriendsDeleted(target_acc_cache, { target_acc_cache->acc_info.Index, acc_index });
                }
                main_server->RemovePlayerFriends(session_id, target_acc_cache->acc_info.Index);
                main_server->AddPlayerFriendsDeleted(acc_cache, { acc_index , target_acc_cache->acc_info.Index });
                main_server->RemovePlayerBlockedsDeleted(acc_cache, target_acc_cache->acc_info.Index);
                main_server->AddPlayerBlockedsAdded(acc_cache, newBlockedInfo);
                main_server->AddPlayerBlockeds(session_id, newBlockedInfo);
                BaseLib::Database->InsertPlayerBlockeds(acc_cache->blockeds_added);
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "player ({}) blocked ({})", acc_cache->acc_info.Nickname.c_str(), target_acc_cache->acc_info.Nickname.c_str());
            });
        }
        inline void PlayerUnblock(SCallbackData& callback, CMainServer* main_server)
        {
            auto session = callback.session;
            if (!session) return;

            CMessage msgCopy = *callback.message;
            BaseLib::DbPool->submit_task([main_server, session = std::move(callback.session), message = std::move(msgCopy)]() mutable
            {
                std::shared_lock lock(session->GetMutex());
                auto session_id = session->GetSessionId();
                auto acc_cache = main_server->GetAccCacheUniqueBySessionId(session_id);
            
                auto acc_index = acc_cache->acc_info.Index;
                if (acc_index == -1) return;
                auto blockeds = main_server->GetBlockedsList(session_id);
                auto blockedRemoveReq = reinterpret_cast<MainPlayerBlockedRemoveReq*>(message.GetData());
                if (!main_server->IsBlockedAlready(blockeds, blockedRemoveReq->player_id)) return;
                blockeds.unlock();
                main_server->RemovePlayerBlockedsAdded(acc_cache, blockedRemoveReq->player_id);
                main_server->AddPlayerBlockedsDeleted(acc_cache, {acc_index, static_cast<int32_t>(blockedRemoveReq->player_id) });
                main_server->RemovePlayerBlockeds(session_id, blockedRemoveReq->player_id);
                BaseLib::Database->DeletePlayerBlockeds(acc_cache->blockeds_deleted);
                session->SendMsg(53, 0, 0, 1);
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "player ({}) unblocked account id ({})", acc_cache->acc_info.Nickname.c_str(), blockedRemoveReq->player_id);

            });
        }
        inline void PlayerBlockList(SCallbackData& callback, CMainServer* main_server)
        {
            auto session = callback.session;
            auto message = callback.message;
            if (!session || !message) return;
            std::shared_lock lock(session->GetMutex());

            auto session_id = session->GetSessionId();
            auto acc_cache = main_server->GetAccCacheSharedBySessionId(session_id);
            
            auto acc_index = acc_cache->acc_info.Index;
            acc_cache.unlock();

            if (acc_index == -1) return;
            auto blockeds = main_server->GetBlockedsList(session_id);
            std::vector<PlayerBlockedInfo> blockeds_info;
            for (const auto& blocked_info : *blockeds)
                blockeds_info.push_back({ blocked_info.blocked_account_id, blocked_info.blocked_nickname.c_str() });
            blockeds.unlock();
            if (blockeds_info.size() > 0)
                session->SendMsg(54, 0, Userlist::Blocked::ListResult::UsersBlocked, blockeds_info.size(), reinterpret_cast<uint8_t*>(blockeds_info.data()), blockeds_info.size() * sizeof(PlayerBlockedInfo));
            else
                session->SendMsg(54, 0, Userlist::Blocked::ListResult::NotUser, 0);
        }
        inline void PlayerClanList(SCallbackData& callback, CMainServer* main_server)
        {
            auto session = callback.session;
            auto message = callback.message;
            if (!session || !message) return;
            std::shared_lock lock(session->GetMutex());

            auto session_id = session->GetSessionId();
            auto acc_cache = main_server->GetAccCacheSharedBySessionId(session_id);
            
            auto acc_index = acc_cache->acc_info.Index;
            if (acc_index == -1) return;
            auto clan_id = acc_cache->acc_info.ClanId;
            acc_cache.unlock();
            if (clan_id)
            {
                if (main_server->IsClanAlready(clan_id))
                {
                    auto clan_info = main_server->GetClanCacheShared(clan_id);
                    std::vector<PlayerClanInfo> clan_members;
                    for (const auto& member_session_id : clan_info->online_members)
                    {
                        if (member_session_id == session_id) continue;
                        auto member_acc_cache = main_server->GetAccCacheSharedBySessionId(member_session_id);
                        auto member_unique_id = NetEngine::Packets::Core::UniqueId(member_session_id, 1).data;
                        auto clan_member_info = PlayerClanInfo(member_acc_cache->acc_info.Nickname.c_str(), member_unique_id, member_acc_cache->acc_info.Level + 1);
                        clan_members.push_back(clan_member_info);
                    }

                    if (clan_members.size() > 0)
                        session->SendMsg(57, 0, Userlist::Clan::ListResult::UsersClan, clan_members.size(), reinterpret_cast<uint8_t*>(clan_members.data()), clan_members.size() * sizeof(PlayerClanInfo));
                    else
                        session->SendMsg(57, 0, Userlist::Clan::ListResult::NotUser, 0);

                }
            }
        }
        inline void PlayerAddFriend(SCallbackData& callback, CMainServer* main_server)
        {
            auto session = callback.session;
            CServer* server = callback.server;
            if (!session) return;

            CMessage msgCopy = *callback.message;
            BaseLib::DbPool->submit_task([server, main_server, session = std::move(callback.session), message = std::move(msgCopy)]() mutable
            {
                std::shared_lock lock(session->GetMutex());
                
                auto session_id = session->GetSessionId();
                auto acc_cache = main_server->GetAccCacheUniqueBySessionId(session_id);
           
                auto acc_index = acc_cache->acc_info.Index;
                auto request_type = message.GetExtra();
                if (acc_index != -1)
                {

                    auto friends = main_server->GetFriendsList(session_id);

                    std::vector<PlayerFriendInfo> friends_accepted;
                    for (auto const& friend_info : *friends)
                        if (friend_info.state == Userlist::Friends::State::Accepted)
                            friends_accepted.push_back({ (friend_info.friend_session_id != 0) ? NetEngine::Packets::Core::UniqueId(friend_info.friend_session_id, 1).data : NetEngine::Packets::Core::UniqueId(0).data ,friend_info.friend_account_id,  friend_info.friend_nickname.c_str() });

                    friends.unlock();

                    BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "player ({}) friend request type: ({})", acc_cache->acc_info.Nickname.c_str(), request_type);

                    if (request_type == Userlist::Friends::RequestResult::RequestSend)
                    {
                        if (friends_accepted.size() >= 100)
                        {
                            session->SendMsg(61, 0, Userlist::Friends::AddResult::ListFull, Userlist::Friends::ListState::YourListIsFull);
                            return;
                        }
                        const auto& friendAddSendReq = reinterpret_cast<MainPlayerFriendAddSendReq*>(message.GetData());
                        const auto& target_nickname = Utility::ReadMicrovoltsString(friendAddSendReq->nickname, sizeof(friendAddSendReq->nickname));
                        const auto& my_nickname = acc_cache->acc_info.Nickname;
                        if (strcmp(acc_cache->acc_info.Nickname.c_str(), target_nickname.c_str()) == 0)
                        {
                            session->SendMsg(61, 0, Userlist::Friends::AddResult::PlayerNotFound, 0);
                            return;
                        }
                    
                        auto target_acc_cache = main_server->GetAccCacheUniqueByNickname(target_nickname);
                        if (target_acc_cache->acc_info.Index == -1)
                        {
                            uint32_t target_index = 0;
                            if (!BaseLib::Database->NicknameExists(target_nickname.c_str(), target_index))
                            {
                                session->SendMsg(61, 0, Userlist::Friends::AddResult::PlayerNotFound, 0);
                                return;
                            }
                            if (main_server->IsFriendsAlready(friends_accepted, target_index)) return;
                            std::vector<BaseLib::FriendInfo> acc_friends;
                            std::vector<BaseLib::BlockedInfo> acc_blockeds;
                            std::vector<PlayerFriendInfo> target_friends_accepted;
                            BaseLib::Database->GetPlayerBlockeds(static_cast<int32_t>(target_index), acc_blockeds);
                            auto my_blockeds = main_server->GetBlockedsList(session_id);
                            if (main_server->IsBlockedAlready(acc_blockeds, acc_index) || main_server->IsBlockedAlready(my_blockeds, target_index))
                            {
                                session->SendMsg(61, 0, Userlist::Friends::AddResult::PlayerBlocked, 0);
                                return;
                            }

                            if (BaseLib::Database->GetPlayerFriends(static_cast<int32_t>(target_index), acc_friends))
                            {
                                auto accepted_friends_count = std::count_if(acc_friends.begin(), acc_friends.end(),
                                    [](const BaseLib::FriendInfo& friend_info) {
                                    return friend_info.state == Userlist::Friends::State::Accepted;
                                });
                                if (accepted_friends_count >= 100)
                                {
                                    session->SendMsg(61, 0, Userlist::Friends::AddResult::ListFull, Userlist::Friends::ListState::OtherListIsFull);
                                    return;
                                }
                                BaseLib::Database->InsertPlayerFriends({ { static_cast<int32_t>(target_index), acc_index, Userlist::Friends::State::Pending, 0, my_nickname.c_str() } });
                                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "player ({}) sent friend request pending in database to player ({})", my_nickname.c_str(), target_nickname.c_str());
                            }

                            return;
                        }

                        if (main_server->IsFriendsAlready(friends_accepted, target_acc_cache->acc_info.Index)) return;
                        auto target_blockeds = main_server->GetBlockedsList(target_acc_cache->session_id);
                        auto my_blockeds = main_server->GetBlockedsList(session_id);
                        if (main_server->IsBlockedAlready(target_blockeds, acc_index) || main_server->IsBlockedAlready(my_blockeds, target_acc_cache->acc_info.Index))
                        {
                            session->SendMsg(61, 0, Userlist::Friends::AddResult::PlayerBlocked, 0);
                            return;
                        }
                        auto target_friends = main_server->GetFriendsList(target_acc_cache->session_id);
                        auto accepted_friends_count = std::count_if(target_friends->begin(), target_friends->end(),
                            [](const BaseLib::FriendInfo& friend_info) {
                                return friend_info.state == Userlist::Friends::State::Accepted;
                            });
                        target_friends.unlock();
                        if (accepted_friends_count >= 100)
                        {
                            session->SendMsg(61, 0, Userlist::Friends::AddResult::ListFull, Userlist::Friends::ListState::OtherListIsFull);
                            return;
                        }
                        PlayerFriendInfo newFriendInfo = { NetEngine::Packets::Core::UniqueId(session_id, 1).data , acc_index , acc_cache->acc_info.Nickname.c_str() };
                        if (auto target_session = server->GetSessionById(target_acc_cache->session_id))
                        {
                            target_session->SendMsg(61, 0, Userlist::Friends::AddResult::SendSingle, 0, reinterpret_cast<uint8_t*>(&newFriendInfo), sizeof(PlayerFriendInfo));
                            BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "player ({}) sent friend request to player ({})", acc_cache->acc_info.Nickname.c_str(), target_acc_cache->acc_info.Nickname.c_str());
                        }  
                    }
                    else if (request_type == Userlist::Friends::RequestResult::RequestRecv)
                    {
                        if (friends_accepted.size() >= 100)
                        {
                            session->SendMsg(61, 0, Userlist::Friends::AddResult::ListFull, Userlist::Friends::ListState::YourListIsFull);
                            return;
                        }
                        const auto& friendAddRecvReq = reinterpret_cast<MainPlayerFriendAddRecvReq*>(message.GetData());
                        auto sender_uniqueId = NetEngine::Packets::Core::UniqueId(friendAddRecvReq->unique_id);
                        auto sender_acc = main_server->GetAccCacheUniqueBySessionId(sender_uniqueId.session);
                        const auto& my_nickname = acc_cache->acc_info.Nickname;
                        if (sender_acc->acc_info.Index == -1)
                        {
                            auto sender_index = sender_acc->acc_info.Index;
                            const auto& sender_nickname = sender_acc->acc_info.Nickname;

                            std::vector<BaseLib::FriendInfo> acc_friends;
                            std::vector<PlayerFriendInfo> target_friends_accepted;

                            if (BaseLib::Database->GetPlayerFriends(friendAddRecvReq->player_id, acc_friends))
                            {

                                auto accepted_friends_count = std::count_if(acc_friends.begin(), acc_friends.end(),
                                    [](const BaseLib::FriendInfo& friend_info) {
                                    return friend_info.state == Userlist::Friends::State::Accepted;
                                });
                                if (accepted_friends_count >= 100)
                                {
                                    session->SendMsg(61, 0, Userlist::Friends::AddResult::ListFull, Userlist::Friends::ListState::OtherListIsFull);
                                    return;
                                }
                                auto my_acc_cache = main_server->GetAccCacheUniqueBySessionId(session_id);
                                const FriendInfo& current = { acc_index ,sender_index, Userlist::Friends::State::Accepted, 0, sender_nickname.c_str() };
                                main_server->AddPlayerFriendsAccepted(my_acc_cache, current);
                                main_server->AddPlayerFriends(session_id, current);
                                BaseLib::Database->UpdatePlayerFriends(sender_index, acc_index, Userlist::Friends::State::Accepted);
                                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "player ({}) accepted friend request pending database from player ({})", my_nickname.c_str(), sender_nickname.c_str());
                            }
                            return;
                        }
                        auto target_friends = main_server->GetFriendsList(sender_uniqueId.session);
                        auto accepted_friends_count = std::count_if(target_friends->begin(), target_friends->end(),
                            [](const BaseLib::FriendInfo& friend_info) {
                            return friend_info.state == Userlist::Friends::State::Accepted;
                        });
                        target_friends.unlock();
                        if (accepted_friends_count >= 100)
                        {
                            session->SendMsg(61, 0, Userlist::Friends::AddResult::ListFull, Userlist::Friends::ListState::OtherListIsFull);
                            return;
                        }
                        PlayerFriendInfo newFriendInfoForSender = { NetEngine::Packets::Core::UniqueId(session_id, 1).data , acc_index, acc_cache->acc_info.Nickname.c_str() };
                        if (auto target_session = server->GetSessionById(sender_uniqueId.session))
                        {
                            target_session->SendMsg(61, 0, Userlist::Friends::AddResult::FriendAccepted, 0, reinterpret_cast<uint8_t*>(&newFriendInfoForSender), sizeof(PlayerFriendInfo));
                            target_session->SendMsg(61, 0, Userlist::Friends::AddResult::UpdateList, 0, reinterpret_cast<uint8_t*>(&newFriendInfoForSender), sizeof(PlayerFriendInfo));
                            PlayerFriendInfo newFriendInfoForCurrent = { sender_uniqueId.data , sender_acc->acc_info.Index , sender_acc->acc_info.Nickname.c_str() };
                            session->SendMsg(61, 0, Userlist::Friends::AddResult::FriendAccepted, 0, reinterpret_cast<uint8_t*>(&newFriendInfoForCurrent), sizeof(PlayerFriendInfo));
                            session->SendMsg(61, 0, Userlist::Friends::AddResult::UpdateList, 0, reinterpret_cast<uint8_t*>(&newFriendInfoForCurrent), sizeof(PlayerFriendInfo));
                            FriendInfo current = { acc_index, sender_acc->acc_info.Index, Userlist::Friends::State::Accepted, sender_uniqueId.session, sender_acc->acc_info.Nickname.c_str() };
                            FriendInfo sender = { sender_acc->acc_info.Index, acc_index, Userlist::Friends::State::Accepted, session_id, acc_cache->acc_info.Nickname.c_str() };
                            main_server->AddPlayerFriendsAccepted(acc_cache, current);
                            main_server->AddPlayerFriends(session_id, current);
                            main_server->AddPlayerFriendsAccepted(sender_acc, sender);
                            main_server->AddPlayerFriends(sender_uniqueId.session, sender);
                            BaseLib::Database->InsertPlayerFriends(acc_cache->friends_accepted);
                            BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "player ({}) accepted friend request from player ({})", acc_cache->acc_info.Nickname.c_str(), sender_acc->acc_info.Nickname.c_str());
                        }
                    }
                }

            });
        }
        inline void PlayerRemoveFriend(SCallbackData& callback, CMainServer* main_server)
        {
            auto session = callback.session;
            if (!session) return;

            CMessage msgCopy = *callback.message;
            BaseLib::DbPool->submit_task([main_server, session = std::move(callback.session), message = std::move(msgCopy)]() mutable
            {
                std::shared_lock lock(session->GetMutex());
                auto session_id = session->GetSessionId();
                auto acc_cache = main_server->GetAccCacheUniqueBySessionId(session_id);
                auto acc_index = acc_cache->acc_info.Index;
            
                if (acc_index != -1)
                {
                    auto friendRemoveReq = reinterpret_cast<MainPlayerFriendRemoveReq*>(message.GetData());

                    FriendInfo delFriendInfo = { acc_index,static_cast<int32_t>(friendRemoveReq->player_id) };
                    FriendInfo delFriendInfo2 = { static_cast<int32_t>(friendRemoveReq->player_id) , acc_index};
                    main_server->RemovePlayerFriends(session_id, friendRemoveReq->player_id);
                    main_server->AddPlayerFriendsDeleted(acc_cache, delFriendInfo);
                    auto target_acc_cache = main_server->GetAccCacheUniqueByAccountId(friendRemoveReq->player_id);
                    if (target_acc_cache->acc_info.Index != -1)
                    {
                        main_server->RemovePlayerFriends(target_acc_cache->session_id, acc_index);
                        main_server->AddPlayerFriendsDeleted(target_acc_cache, delFriendInfo2);
                        BaseLib::Database->DeletePlayerFriends(acc_cache->friends_deleted);
                        BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "player ({}) removed friend ({})", acc_cache->acc_info.Nickname.c_str(), target_acc_cache->acc_info.Nickname.c_str());
                    }
                    else
                    {
                        const auto& acc_nickname = acc_cache->acc_info.Nickname;
                        BaseLib::Database->DeletePlayerFriends({ delFriendInfo, delFriendInfo2 });
                        BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "player ({}) removed friend account id ({})", acc_nickname.c_str(), friendRemoveReq->player_id);

                    }
                    auto friends = main_server->GetFriendsList(session_id);//deadlock

                    std::vector<PlayerFriendInfo> friends_accepted;
                    for (auto const& friend_info : *friends)
                        if (friend_info.state == Userlist::Friends::State::Accepted)
                            friends_accepted.push_back({ (friend_info.friend_session_id != 0) ? NetEngine::Packets::Core::UniqueId(friend_info.friend_session_id, 1).data : NetEngine::Packets::Core::UniqueId(0).data , friend_info.friend_account_id, friend_info.friend_nickname.c_str() });

                    friends.unlock();

                    if (friends_accepted.size() <= 0)
                    {
                        session->SendMsg(61, 0, Userlist::ListResult::NoUsers, 0);
                        return;
                    }
                    uint32_t total_friends_fragments = (friends_accepted.size() == 0) ? 0 : (friends_accepted.size() / 51) + 1;
                    for (uint32_t i = 0; i < total_friends_fragments; i++)
                    {
                        std::vector<PlayerFriendInfo> friends_batch;
                        uint8_t user_list_result = (i == 0) ? Userlist::ListResult::Users : Userlist::ListResult::Users2;
                        uint32_t start_index = i * 51;
                        uint32_t end_index = std::min(start_index + 51, static_cast<uint32_t>(friends_accepted.size()));
                        for (auto j = start_index; j < end_index; j++)
                            friends_batch.push_back(friends_accepted[j]);

                        session->SendMsg(63, 0, user_list_result, friends_batch.size(), reinterpret_cast<uint8_t*>(friends_batch.data()), friends_batch.size() * sizeof(PlayerFriendInfo));
                    }
                }
            });
        }
        inline void PlayerFriendList(SCallbackData& callback, CMainServer* main_server)
        {
            auto session = callback.session;
            auto message = callback.message;
            if (!session || !message) return;
            std::shared_lock lock(session->GetMutex());

            auto session_id = session->GetSessionId();
            auto acc_cache = main_server->GetAccCacheSharedBySessionId(session_id);
            
            auto acc_index = acc_cache->acc_info.Index;
            acc_cache.unlock();
            if (acc_index != -1)
            {
                auto friends = main_server->GetFriendsList(session_id);

                if (friends->size() <= 0)
                {
                    session->SendMsg(63, 0, Userlist::ListResult::NoUsers, 0);
                    return;
                }
                std::vector<PlayerFriendInfo> friends_accepted;
                for (auto const& friend_info : *friends)
                    if (friend_info.state == Userlist::Friends::State::Accepted)
                        friends_accepted.push_back({ (friend_info.friend_session_id != 0) ? NetEngine::Packets::Core::UniqueId(friend_info.friend_session_id, 1).data : NetEngine::Packets::Core::UniqueId(0).data , friend_info.friend_account_id, friend_info.friend_nickname.c_str() });


                if (friends_accepted.size() <= 0)
                {
                    session->SendMsg(63, 0, Userlist::ListResult::NoUsers, 0);
                    return;
                }
                uint32_t total_friends_fragments = (friends_accepted.size() == 0) ? 0 : (friends_accepted.size() / 51) + 1;
                for (uint32_t i = 0; i < total_friends_fragments; i++)
                {
                    std::vector<PlayerFriendInfo> friends_batch;
                    uint8_t user_list_result = (i == 0) ? Userlist::ListResult::Users : Userlist::ListResult::Users2;
                    uint32_t start_index = i * 51;
                    uint32_t end_index = std::min(start_index + 51, static_cast<uint32_t>(friends_accepted.size()));
                    for (auto j = start_index; j < end_index; j++)
                        friends_batch.push_back(friends_accepted[j]);

                    session->SendMsg(63, 0, user_list_result, friends_batch.size(), reinterpret_cast<uint8_t*>(friends_batch.data()), friends_batch.size() * sizeof(PlayerFriendInfo));
                }
            }

        }
        inline void PlayerSocials(SCallbackData& callback, CMainServer* main_server)
        {
            const auto& order = callback.message->GetOrder();
            switch (order)
            {
                case 52: PlayerBlock(callback, main_server); break;
                case 53: PlayerUnblock(callback, main_server); break;
                case 54: PlayerBlockList(callback, main_server); break;
                case 57: PlayerClanList(callback, main_server); break;
                case 61: PlayerAddFriend(callback, main_server); break;
                case 62: PlayerRemoveFriend(callback, main_server); break;
                case 63: PlayerFriendList(callback, main_server); break;
            }
        }
        inline void PlayerInviteJoin(SCallbackData& callback, CMainServer* main_server)
        {
            auto session = callback.session;
            auto message = callback.message;
            if (!session || !message) return;
            std::shared_lock lock(session->GetMutex());

            CServer* server = callback.server;
            auto session_id = session->GetSessionId();
            auto option = message->GetOption();
            auto extra = message->GetExtra();

            auto acc_cache = main_server->GetAccCacheSharedBySessionId(session_id);

            
            BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "[InviteJoin] option: ({}), extra: ({})", option, extra);
            switch (option) 
            {
                case 1: {//JOIN
                    auto joinReq = reinterpret_cast<MainPlayerBlockedRemoveReq*>(message->GetData());
                    BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "[InviteJoin] player want to join player id ({})", joinReq->player_id);
                    auto target_acc_cache = main_server->GetAccCacheUniqueByAccountId(joinReq->player_id);
                    if (target_acc_cache->in_party)
                    {
                        session->SendMsg(163, 0, 9, 0);//generic fail msg, you can only invite to party not join
                        return;
                    }
                    if (target_acc_cache->in_plaza)
                    {
                        if (acc_cache->in_plaza && acc_cache->plaza_id == target_acc_cache->plaza_id) //in the same plaza dont do anything
                        {
                            session->SendMsg(163, 0, 9, 0);
                            return;
                        }
                        auto current_plaza = main_server->GetPlazaCacheUnique(target_acc_cache->plaza_id);
                        if (current_plaza->session_ids.size() >= current_plaza->max_players) //plaza is full
                        {
                            session->SendMsg(163, 0, 14, 0);
                            return;
                        }
                        auto join_confirm_ack_data = MainUserJoinConfirmAck(1, (uint16_t)target_acc_cache->plaza_id, 1).Serialize();
                        session->SendMsg(163, 0, 0, 1, reinterpret_cast<uint8_t*>(join_confirm_ack_data.data()), join_confirm_ack_data.size());
                        return;
                    }
                    if (acc_cache->in_room) 
                    {
                        if (target_acc_cache->in_room && acc_cache->room_id == target_acc_cache->room_id)  //is in same room
                        {
                            session->SendMsg(163, 0, 9, 0);//generic fail msg
                            return;
                        }
                        if (acc_cache->state == 8) //is ready or host
                        {
                            session->SendMsg(163, 0, 5, 0);
                            return;
                        }
                    }
                    if (target_acc_cache->acc_info.Index != -1) 
                    {
                        BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "[InviteJoin] found target join player acc info nickname: ({})", target_acc_cache->acc_info.Nickname);
                        if (!target_acc_cache->in_room) 
                        {
                            session->SendMsg(163, 0, 2, 0);//target is in lobby
                            return;
                        }
                       
                        if (!main_server->IsRoomAlready(target_acc_cache->room_id))
                        {
                            session->SendMsg(163, 0, 2, 0);//target is in lobby
                            return;
                        }
                        //now need info for current room
                        auto room_cache = main_server->GetRoomCacheUnique(target_acc_cache->room_id);
                        if (room_cache->max_players == room_cache->neutralteam_session_ids.size() || room_cache->max_players == (room_cache->blueteam_session_ids.size() + room_cache->redteam_session_ids.size())) //room is full
                        {
                            session->SendMsg(163, 0, 14, 0);
                            return;
                        }
                        BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "[InviteJoin] target join player is okay to join and will receive confirmation");
                        if (extra == 28) //send him cumfirmation
                        {
                            auto join_confirm_ack_data = MainUserJoinConfirmAck(1, room_cache->room_id, room_cache->channel_id).Serialize();
                            session->SendMsg(163, 0, room_cache->has_password ? 44 : 0, 0, reinterpret_cast<uint8_t*>(join_confirm_ack_data.data()), join_confirm_ack_data.size());
                            return;
                        }
                    }
                    else 
                    {
                        BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "[InviteJoin] target join player cannot find cache");
                        session->SendMsg(163, 0, 13, 0);//target is logged out!
                    }
                    break;
                }
                case 2: //INVITE
                {
                    if (!acc_cache->in_room && !acc_cache->in_party)
                    {
                        BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "[InviteJoin] invite but self isnt in any room or party");
                        return;
                    }

                    const auto& inviteReq = reinterpret_cast<MainPlayerBlockedAddReq*>(message->GetData());
                    const auto& target_nickname = Utility::ReadMicrovoltsString(inviteReq->nickname, sizeof(inviteReq->nickname));
                    BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "[InviteJoin] player want to invite player ({})", target_nickname);
                    auto target_acc_cache = main_server->GetAccCacheUniqueByNickname(target_nickname.c_str());
                    if (target_acc_cache->acc_info.Index != -1) 
                    {
                        BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "[InviteJoin] found target invite player acc info nickname: ({})", target_acc_cache->acc_info.Nickname);
                        if (target_acc_cache->in_room) 
                        {
                            if (target_acc_cache->room_id != acc_cache->room_id)  //invite someone who is in another room
                            {
                                if (target_acc_cache->playing) 
                                {
                                    session->SendMsg(319, 0, 5, 0);//target is in battle!
                                    return;
                                }
                            }
                            else //is already in your room
                            {
                                session->SendMsg(319, 0, 11, 0);//Invite fail
                                return;
                            }
                        }
                        if (acc_cache->in_party)
                        {
                            auto party_cache = main_server->GetPartyCacheUnique(acc_cache->party_id);
                            if (target_acc_cache->in_party && target_acc_cache->party_id == acc_cache->party_id)//in same party already
                            {
                                session->SendMsg(319, 0, 11, 0);
                                return;
                            }
                            if (party_cache->members.size() >= party_cache->max_members)//party is full
                            {
                                session->SendMsg(319, 0, 7, 0);
                                return;
                            }
                            BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "[InviteJoin] party is okay to propose the player an invite");
                            MainUserInvitePartyAck inviteInfo = { 1, acc_cache->acc_info.Nickname.c_str(), acc_cache->party_id };
                            auto sender_uniqueId = NetEngine::Packets::Core::UniqueId(target_acc_cache->session_id, 1);
                            if (auto target_session = server->GetSessionById(sender_uniqueId.session))
                            {
                                target_session->SendMsg(319, 0, 60, 0, reinterpret_cast<uint8_t*>(&inviteInfo), sizeof(inviteInfo));
                            }
                            return;
                        }
                        if (!main_server->IsRoomAlready(acc_cache->room_id))
                        {
                            session->SendMsg(319, 0, 11, 0);//Invite fail
                            return;
                        }
                        //now need info for current room
                        auto room_cache = main_server->GetRoomCacheUnique(acc_cache->room_id);
                        if (room_cache->max_players == room_cache->neutralteam_session_ids.size() || room_cache->max_players == (room_cache->blueteam_session_ids.size() + room_cache->redteam_session_ids.size())) //room is full
                        {
                            session->SendMsg(319, 0, 7, 0);
                            return;
                        }
                        //all good, now send him an invite
                        BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "[InviteJoin] room is okay to propose the player an invite");
                        auto sender_uniqueId = NetEngine::Packets::Core::UniqueId(target_acc_cache->session_id, 1);
                        if (auto target_session = server->GetSessionById(sender_uniqueId.session))
                        {
                            auto invite_ack_data = MainUserInviteAck(1, room_cache->room_id, room_cache->channel_id, acc_cache->acc_info.Nickname.c_str(), room_cache->title.c_str(), room_cache->password.c_str()).Serialize(room_cache->has_password);
                            target_session->SendMsg(319, 0, room_cache->has_password ? 44 : 0, 0, reinterpret_cast<uint8_t*>(invite_ack_data.data()), invite_ack_data.size());
                        }
                    }
                    else 
                    {
                        BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "[InviteJoin] target invite player cannot find cache");
                        session->SendMsg(319, 0, 6, 0);//target is logged out!
                    }
                    break;
                }
            }
        }
    }
}