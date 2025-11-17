#pragma once
namespace Game::Handlers
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Main;
    inline void FriendsView(SCallbackData& callback, CMainServer* main_server)
    {
        auto session = callback.session;
        auto message = callback.message;
        if (!session || !message) return;
        //std::shared_lock lock(session->GetMutex());

        auto sid = session->GetSessionId();
        auto acc = CAccount.get<shared_t>(sid);

        auto aid = acc->acc_info.Index;
        acc.unlock();
        if (!aid) return;


        std::vector<PlayerFriendInfo> accepted_friends;
        std::vector<BaseLib::SocialInfo> friends;
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

        if (accepted_friends.empty())
        {
            session->SendMsg(63, 0, Userlist::ListResult::NoUsers, 0);
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
    }
}