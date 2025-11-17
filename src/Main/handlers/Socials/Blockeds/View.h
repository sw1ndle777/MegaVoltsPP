#pragma once
namespace Game::Handlers
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Main;
    inline void BlockedsView(SCallbackData& callback, CMainServer* main_server)
    {
        auto session = callback.session;
        auto message = callback.message;
        if (!session || !message) return;
        //std::shared_lock lock(session->GetMutex());

        auto sid = session->GetSessionId();
        auto acc = CAccount.get<shared_t>(sid);
        auto aid = acc->acc_info.Index;
        acc.unlock();
        if (aid == -1) return;
        auto social_list = CSocial.get<shared_t>(sid);
        std::vector<PlayerBlockedInfo> blockeds_info;
        for (const auto& social : *social_list)
        {
            if (social.State != NetEngine::Socials::State::Blocked) continue;
            PlayerBlockedInfo info;
            strcpy(info.nickname, social.TargetNickname.c_str());
            info.acc_id = social.targetAid;
            blockeds_info.push_back(info);
        }
        social_list.unlock();
        if (blockeds_info.empty())
        {
            session->SendMsg(54, 0, Userlist::Blocked::ListResult::NotUser, 0);
            return;
        }
        session->SendMsg(54, 0, Userlist::Blocked::ListResult::UsersBlocked, blockeds_info.size(), reinterpret_cast<uint8_t*>(blockeds_info.data()), blockeds_info.size() * sizeof(PlayerBlockedInfo));
    }
}