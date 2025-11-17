#pragma once
namespace Game::Handlers
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Main;
    inline void UsersView(SCallbackData& callback, CMainServer* main_server)
    {
        auto session = callback.session;
        auto message = callback.message;
        if (!session || !message) return;

        //std::shared_lock lock(session->GetMutex());
        auto sid = session->GetSessionId();
        auto acc = CAccount.get<shared_t>(sid);
        auto aid = acc->acc_info.Index;
        if (aid == -1) return;


        auto sids = CSid.get_all(shared);
       
        if (sids->size() <= 1)
        {
            session->SendMsg(84, 0, Userlist::ListResult::NoUsers, 0);
            return;
        }
		auto in_plaza = acc->in_plaza;
		auto in_room = acc->in_room;
		auto in_party = acc->in_party;
        acc.unlock();
        std::vector<PlayerAgoraInfo> user_list;

        for (const auto& id : *sids)
        {
            if(id == sid) continue;
			auto other_acc = CAccount.get<shared_t>(id);

            const bool bothInPlazaFree =
                in_plaza && other_acc->in_plaza &&
                !in_room && !other_acc->in_room &&
                !in_party && !other_acc->in_party;

            const bool bothOutPlazaAndUserFree =
                !in_plaza && !other_acc->in_plaza &&
                !other_acc->in_room && !other_acc->in_party;

            if (!bothInPlazaFree && !bothOutPlazaAndUserFree) continue;

            uint32_t front = 0, back = 0;
            if (other_acc->acc_info.ClanId)
            {
                auto clan_info = CClan.get<shared_t>(other_acc->acc_info.ClanId);
                front = clan_info->logo_front;
                back = clan_info->logo_back;
                clan_info.unlock();
            }

            user_list.push_back({ other_acc->acc_info.Nickname, other_acc->uid.data , other_acc->acc_info.Level + 1, front, back });
            /*
            if (user.first != session_id)
            {
                if (acc_cache->in_plaza && user.second.in_plaza && !acc_cache->in_room && !user.second.in_room && !acc_cache->in_party && !user.second.in_party) {
                    uint32_t clan_front_icon = 0, clan_back_icon = 0;
                    if (user.second.acc_info.ClanId) {
                        auto clan_info = CClan.get<shared_t>(user.second.acc_info.ClanId);
                        clan_front_icon = clan_info->logo_front;
                        clan_back_icon = clan_info->logo_back;
                        clan_info.unlock();
                    }
                    user_list.push_back({ user.second.acc_info.Nickname, NetEngine::Packets::Core::UniqueId(user.first, 1).data , user.second.acc_info.Level + 1, clan_front_icon, clan_back_icon });
                }
                else if (!acc_cache->in_plaza && !user.second.in_plaza && !user.second.in_room && !user.second.in_party) {
                    uint32_t clan_front_icon = 0, clan_back_icon = 0;
                    if (user.second.acc_info.ClanId) {
                        auto clan_info = CClan.get<shared_t>(user.second.acc_info.ClanId);
                        clan_front_icon = clan_info->logo_front;
                        clan_back_icon = clan_info->logo_back;
                        clan_info.unlock();
                    }
                    user_list.push_back({ user.second.acc_info.Nickname, NetEngine::Packets::Core::UniqueId(user.first, 1).data , user.second.acc_info.Level + 1, clan_front_icon, clan_back_icon });
                }
            }*/
        }

        if (user_list.size() <= 0)
        {
            session->SendMsg(84, 0, Userlist::ListResult::NoUsers, 0);
            return;
        }
        uint32_t total_users_fragments = (user_list.size() == 0) ? 0 : (user_list.size() / 51) + 1;
        for (uint32_t i = 0; i < total_users_fragments; i++)
        {
            std::vector<PlayerAgoraInfo> users_batch;
            uint8_t user_list_result = (i == 0) ? Userlist::ListResult::Users : Userlist::ListResult::Users2;
            uint32_t start_index = i * 51;
            uint32_t end_index = std::min(start_index + 51, static_cast<uint32_t>(user_list.size()));
            for (auto j = start_index; j < end_index; j++)
                users_batch.push_back(user_list[j]);


            session->SendMsg(84, 0, user_list_result, users_batch.size(), reinterpret_cast<uint8_t*>(users_batch.data()), users_batch.size() * sizeof(PlayerAgoraInfo));
        }
    }
}