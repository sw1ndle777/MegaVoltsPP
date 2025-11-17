#pragma once
namespace Game::Handlers
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Main;

    inline void FriendsRemove(SCallbackData& callback, CMainServer* main_server)
    {
        auto session = callback.session;
        auto message = callback.message;
        if (!session || !message) return;

        //std::shared_lock lock(session->GetMutex());
        auto sid = session->GetSessionId();
        auto acc = CAccount.get<unique_t>(sid);

        auto aid = acc->acc_info.Index;
        if (aid == -1) return;

        auto target_aid = *reinterpret_cast<int32_t*>(message->GetData());
        auto social_list = CSocial.get<shared_t>(sid);
        auto friends_accepted_count = std::count_if(social_list->begin(), social_list->end(),
            [](const BaseLib::SocialInfo& social) {
                return social.State == NetEngine::Socials::State::Accepted;
            });
        auto social_info = main_server->GetPlayerSocial(social_list, target_aid);
        social_list.unlock();
        if (!social_info.has_value()) return;
        if (social_info->get().State != NetEngine::Socials::State::Accepted) return;

        DatabaseUpdateCtx dctx{ .sid = sid, .aid = aid };
        dctx.ops.emplace_back(PlayerSocialPatch{ .op = PlayerSocialPatch::Op::Delete, .aid = aid, .targetAid = target_aid });

        auto validated = main_server->ValidateDatabaseUpdates(acc, dctx);
        if (!validated.has_value())
        {
            DEBUGLOG(red, "ValidateDatabaseUpdates failed for [{}] [{}]: {}", aid, acc->acc_info.Nickname.c_str(), static_cast<int>(validated.error()));
            return;
        }
        acc.unlock();

        [[maybe_unused]] auto ignored_result = BaseLib::DbPool->submit_task([main_server,
            session = std::move(callback.session),
            aid = aid,
            sid = sid,
            target_aid = target_aid,
            friends_accepted_count = friends_accepted_count,
            v = std::move(validated.value())
        ]() mutable
            {
                if (!session) return;

                auto new_acc_cache = CAccount.get<unique_t>(sid);
                ResultDbUpdateInfo dbres;

                if (!BaseLib::Database->UpdateAccount(v, dbres).has_value()) return;

                auto applied = main_server->ApplyDatabaseUpdates(new_acc_cache, v);
                if (!applied.has_value())
                {
                    DEBUGLOG(red, "ApplyDatabaseUpdates failed for [{}] [{}]: {}", new_acc_cache->acc_info.Index, new_acc_cache->acc_info.Nickname.c_str(), static_cast<int>(applied.error()));
                    return;
                }
                std::vector<PlayerFriendInfo> accepted_friends;
                std::vector<BaseLib::SocialInfo> friends;
                friends.reserve(friends_accepted_count - 1);

                auto socials = CSocial.get<shared_t>(sid);
                std::copy_if(socials->begin(), socials->end(), std::back_inserter(friends), [&](const BaseLib::SocialInfo& it)
                    {
                        return it.State == NetEngine::Socials::State::Accepted;
                    });
                socials.unlock();
                accepted_friends.reserve(friends.size());
                for (auto i = 0; i < friends.size(); i++)
                {
                    PlayerFriendInfo info{};
                    info.friend_id = friends[i].targetAid;
                    strcpy(info.nickname, friends[i].TargetNickname.c_str());
					auto friend_sid = *CAidSid.get<shared_t>(friends[i].targetAid);
                    auto friend_acc = CAccount.get<shared_t>(friend_sid);
                    if (friend_acc->acc_info.Index) info.unique_id = friend_acc->uid.data;
                    accepted_friends.push_back(info);
                }
				auto target_sid = *CAidSid.get<shared_t>(target_aid);
                if (target_sid)
                {
                    auto target_social_list = CSocial.get<unique_t>(target_sid);
                    std::erase_if(*target_social_list, [&](const BaseLib::SocialInfo& it)
                        {
                            return it.targetAid == aid;
						});
                    target_social_list.unlock();
                    //main_server->RemovePlayerSocials(target_social_list, aid);
                    //target_social_list.unlock();
                }
                if (accepted_friends.empty())
                {
                    session->SendMsg(61, 0, Userlist::ListResult::NoUsers, 0);
                    return;
                }
                constexpr size_t max_friends_per_fragment = 51;
                auto total_friends_fragments = (accepted_friends.size() == 0) ? 0 : (accepted_friends.size() / max_friends_per_fragment) + 1;
                for (auto i = 0; i < total_friends_fragments; i++)
                {
                    std::vector<PlayerFriendInfo> friends_batch;
                    auto user_list_result = (i == 0) ? Userlist::ListResult::Users : Userlist::ListResult::Users2;
                    auto start_index = i * max_friends_per_fragment;
                    auto end_index = std::min(start_index + 51, accepted_friends.size());
                    for (auto j = start_index; j < end_index; j++)
                        friends_batch.push_back(accepted_friends[j]);

                    session->SendMsg(63, 0, user_list_result, friends_batch.size(), reinterpret_cast<uint8_t*>(friends_batch.data()), friends_batch.size() * sizeof(PlayerFriendInfo));
                }

                DEBUGLOG(dark_cyan, "player ({}) removed from friendlist aid ({})", new_acc_cache->acc_info.Nickname.c_str(), target_aid);
            }
        );
    }
}
