#pragma once
namespace Game::Handlers
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Main;
    inline void FriendsAdd(SCallbackData& callback, CMainServer* main_server)
    {
        auto session = callback.session;
        auto message = callback.message;
        if (!session || !message) return;

        //std::shared_lock lock(session->GetMutex());
        auto sid = session->GetSessionId();
        auto acc = CAccount.get<unique_t>(sid);

        auto aid = acc->acc_info.Index;
        if (aid == -1) return;

        auto social_list = CSocial.get<shared_t>(sid);

        auto accepted_count = std::count_if(social_list->begin(), social_list->end(),
            [](const BaseLib::SocialInfo& social) {
                return social.State == NetEngine::Socials::State::Accepted;
            });

        using namespace Userlist::Friends;
        if (accepted_count >= 100)
        {
            session->SendMsg(61, 0, AddResult::ListFull, ListState::YourListIsFull);
            return;
        }

        auto type = message->GetExtra();
        std::string target_nickname = "";
        NetEngine::Packets::Core::UniqueId target_uid = { 0 };
        int32_t target_aid = -1;
        DatabaseUpdateCtx dctx{ .sid = sid, .aid = aid };
        DEBUGLOG(green, "player ({}) add friend req type: {}", acc->acc_info.Nickname.c_str(), static_cast<int>(type));
        auto social_state = (type == RequestResult::RequestSend) ? NetEngine::Socials::State::Pending : NetEngine::Socials::State::Accepted;
        if (type == RequestResult::RequestSend)
        {
            auto req = reinterpret_cast<MainPlayerFriendAddSendReq*>(message->GetData());
            target_nickname = Utility::ReadMicrovoltsString(req->nickname, sizeof(req->nickname));
        }
        else if (type == RequestResult::RequestRecv)
        {
            auto req = reinterpret_cast<MainPlayerFriendAddRecvReq*>(message->GetData());
            target_nickname = Utility::ReadMicrovoltsString(req->nickname, sizeof(req->nickname));
            target_uid.data = req->unique_id;
            target_aid = req->player_id;
        }
        if (strcmp(acc->acc_info.Nickname.c_str(), target_nickname.c_str()) == 0)
        {
            session->SendMsg(61, 0, AddResult::PlayerNotFound, 0);
            return;
        }
        if (type == RequestResult::RequestSend)
        {
            
            auto target_acc = CAccount.get_by_filter<shared_t>([&](const auto& /*id*/, auto& player) {
                return Utility::ToLowercase(player.acc_info.Nickname) == Utility::ToLowercase(target_nickname);
                });
            if (target_acc->acc_info.Index)
            {
                target_uid.data = target_acc->uid.data;
                target_aid = target_acc->acc_info.Index;
            }
        }
        DEBUGLOG(green, "player ({}) add friend req target: {} [{}]", acc->acc_info.Nickname.c_str(), target_nickname.c_str(), target_aid);
        auto social_info = main_server->GetPlayerSocial(social_list, aid);
        if (social_info.has_value())
        {
            if (social_info->get().State == NetEngine::Socials::State::Blocked)
            {
                DEBUGLOG(yellow, "player ({}) tried to add blocked player ({})", acc->acc_info.Nickname.c_str(), target_nickname.c_str());
                session->SendMsg(61, 0, AddResult::PlayerBlocked, 0);
                return;
            }
            else if (social_info->get().State == NetEngine::Socials::State::Accepted)
            {
                DEBUGLOG(yellow, "player ({}) tried to add already added player ({})", acc->acc_info.Nickname.c_str(), target_nickname.c_str());
                return;
            }
        }
        social_list.unlock();
        if (target_uid.session)
        {
            auto target_social_list = CSocial.get<shared_t>(target_uid.session);

            auto target_accepted_count = std::count_if(target_social_list->begin(), target_social_list->end(),
                [](const BaseLib::SocialInfo& social) {
                    return social.State == NetEngine::Socials::State::Accepted;
                });

            if (target_accepted_count >= 100)
            {
                session->SendMsg(61, 0, AddResult::ListFull, ListState::OtherListIsFull);
                DEBUGLOG(yellow, "player ({}) tried to add friend ({}) but their friends list is full", acc->acc_info.Nickname.c_str(), target_nickname.c_str());
                return;
            }

            auto target_social_info = main_server->GetPlayerSocial(target_social_list, target_aid);
            if (target_social_info.has_value())
            {
                if (target_social_info->get().State == NetEngine::Socials::State::Blocked)
                {
                    DEBUGLOG(yellow, "player ({}) tried to add player ({}) that already blocked him", acc->acc_info.Nickname.c_str(), target_nickname.c_str());
                    session->SendMsg(61, 0, AddResult::PlayerBlocked, 0);
                    return;
                }
                else if (target_social_info->get().State == NetEngine::Socials::State::Accepted)
                {
                    DEBUGLOG(yellow, "player ({}) tried to add player ({}) that already accepted him", acc->acc_info.Nickname.c_str(), target_nickname.c_str());
                    return;
                }
                return;
            }
            DEBUGLOG(green, "player ({}) add friend req target is online: {} [{}]", acc->acc_info.Nickname.c_str(), target_nickname.c_str(), target_aid);
        }

        dctx.ops.emplace_back(PlayerSocialPatch{ .op = PlayerSocialPatch::Op::InsertOrUpdate, .aid = aid, .targetAid = target_aid, .State = social_state, .TargetNickname = target_nickname, });
        DEBUGLOG(green, "player ({}) add friend req db patch created: {} [{}], state ({})", acc->acc_info.Nickname.c_str(), target_nickname.c_str(), target_aid, static_cast<uint8_t>(social_state));

        auto validated = main_server->ValidateDatabaseUpdates(acc, dctx);
        if (!validated.has_value())
        {
            DEBUGLOG(red, "ValidateDatabaseUpdates failed for [{}] [{}]: {}", aid, acc->acc_info.Nickname.c_str(), static_cast<int>(validated.error()));
            return;
        }
        acc.unlock();


        [[maybe_unused]] auto ignored_result = BaseLib::DbPool->submit_task([main_server,
            session = std::move(callback.session),
            sid = sid,
            aid = aid,
            target_aid = target_aid,
            target_nickname = target_nickname,
            target_uid = target_uid,
            social_state = social_state,
            v = std::move(validated.value())
        ]() mutable
            {
                if (!session) return;

                auto new_acc_cache = CAccount.get<unique_t>(sid);
                ResultDbUpdateInfo dbres;

                if (!BaseLib::Database->UpdateAccount(v, dbres).has_value())
                {
                    DEBUGLOG(red, "UpdateAccount failed for [{}] [{}]: {}", new_acc_cache->acc_info.Index, new_acc_cache->acc_info.Nickname.c_str());
                    if (dbres.target_not_found)
                        session->SendMsg(61, 0, AddResult::PlayerNotFound, 0);

                    if (dbres.target_blocked || dbres.player_blocked)
                        session->SendMsg(61, 0, AddResult::PlayerBlocked, 0);

                    return;
                }
                DEBUGLOG(green, "UpdateAccount succeeded for [{}] [{}]", new_acc_cache->acc_info.Index, new_acc_cache->acc_info.Nickname.c_str());
                auto applied = main_server->ApplyDatabaseUpdates(new_acc_cache, v);
                if (!applied.has_value())
                {
                    DEBUGLOG(red, "ApplyDatabaseUpdates failed for [{}] [{}]: {}", new_acc_cache->acc_info.Index, new_acc_cache->acc_info.Nickname.c_str(), static_cast<int>(applied.error()));
                    session->SendMsg(61, 0, AddResult::PlayerNotFound, 0);
                    return;
                }
                DEBUGLOG(green, "ApplyDatabaseUpdates succeeded for [{}] [{}]", new_acc_cache->acc_info.Index, new_acc_cache->acc_info.Nickname.c_str());
                PlayerFriendInfo newFriendInfo = { new_acc_cache->uid.data , aid , new_acc_cache->acc_info.Nickname.c_str() };

                DEBUGLOG(green, "player ({}) add friend req db applied: {} [{}], state ({})", new_acc_cache->acc_info.Nickname.c_str(), target_nickname.c_str(), target_aid, static_cast<uint8_t>(social_state));

                if (social_state == NetEngine::Socials::State::Pending)
                {
                    if (auto pss = main_server->GetSessionById(target_uid.session))
                        pss->SendMsg(61, 0, AddResult::SendSingle, 0, reinterpret_cast<uint8_t*>(&newFriendInfo), sizeof(PlayerFriendInfo));

                    DEBUGLOG(dark_cyan, "player ({}) sent friend request to player ({})", new_acc_cache->acc_info.Nickname.c_str(), target_nickname.c_str());
                }
                else if (social_state == NetEngine::Socials::State::Accepted)
                {
                    PlayerFriendInfo otherFriendInfo = { target_uid.data , target_aid , target_nickname.c_str() };
                    session->SendMsg(61, 0, Userlist::Friends::AddResult::FriendAccepted, 0, reinterpret_cast<uint8_t*>(&otherFriendInfo), sizeof(PlayerFriendInfo));
                    if (auto pss = main_server->GetSessionById(target_uid.session))
                    {
                        /*
                        BaseLib::SocialInfo info = { target_aid, aid, NetEngine::Socials::State::Accepted, new_acc_cache->acc_info.Nickname };
                        auto target_social_list = CSocial.get<shared_t>(target_uid.session);
                        main_server->IsSocialsAlready(target_social_list, info.targetAid) ?
                            main_server->UpdatePlayerSocials(target_social_list, info) :
                            main_server->AddPlayerSocials(target_social_list, info);
                        target_social_list.unlock();
                        */
                        pss->SendMsg(61, 0, AddResult::FriendAccepted, 0, reinterpret_cast<uint8_t*>(&newFriendInfo), sizeof(PlayerFriendInfo));
                        pss->SendMsg(61, 0, AddResult::UpdateList, 0, reinterpret_cast<uint8_t*>(&newFriendInfo), sizeof(PlayerFriendInfo));
                    }
                    DEBUGLOG(dark_cyan, "player ({}) accepted friend request from player ({})", new_acc_cache->acc_info.Nickname.c_str(), target_nickname.c_str());
                }
            }
        );
    }
}