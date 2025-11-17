#pragma once
namespace Game::Handlers
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Main;
    inline void ClanView(SCallbackData& callback, CMainServer* main_server)
    {
        auto session = callback.session;
        auto message = callback.message;
        if (!session || !message) return;
        //std::shared_lock lock(session->GetMutex());

        auto sid = session->GetSessionId();
        auto acc = CAccount.get<shared_t>(sid);

        auto aid = acc->acc_info.Index;
        if (aid == -1) return;
        auto clan_id = acc->acc_info.ClanId;
        acc.unlock();
        if (!clan_id && !CClan.contains(clan_id)) return;

        auto clan_info = CClan.get<shared_t>(clan_id);
        std::vector<PlayerClanInfo> clan_members;
        for (const auto& id : clan_info->online_members)
        {
            if (id == sid) continue;
            auto member_acc = CAccount.get<shared_t>(id);
            auto clan_member_info = PlayerClanInfo(member_acc->acc_info.Nickname.c_str(), member_acc->uid.data, member_acc->acc_info.Level + 1);
            clan_members.push_back(clan_member_info);
        }
        if (clan_members.empty())
        {
            session->SendMsg(57, 0, Userlist::Clan::ListResult::NotUser, 0);
            return;
        }
        session->SendMsg(57, 0, Userlist::Clan::ListResult::UsersClan, clan_members.size(), reinterpret_cast<uint8_t*>(clan_members.data()), clan_members.size() * sizeof(PlayerClanInfo));
    }
}