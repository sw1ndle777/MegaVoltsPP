#pragma once
namespace Game::Handlers
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Main;
    inline void BlockedsAdd(SCallbackData& callback, CMainServer* main_server)
    {
        auto session = callback.session;
        auto message = callback.message;
        if (!session || !message) return;

        //std::shared_lock lock(session->GetMutex());
        auto sid = session->GetSessionId();
        auto acc = CAccount.get<unique_t>(sid);

        auto aid = acc->acc_info.Index;
        if (aid == -1) return;

        const auto& req = reinterpret_cast<MainPlayerBlockedAddReq*>(message->GetData());
        const auto& target_nickname = Utility::ReadMicrovoltsString(req->nickname, sizeof(req->nickname));
        if (acc->acc_info.Nickname == target_nickname) return;
        auto target_acc = CAccount.get_by_filter<unique_t>([&](const auto& /*id*/, auto& player) {
            return Utility::ToLowercase(player.acc_info.Nickname) == Utility::ToLowercase(target_nickname);
            });
        auto target_aid = target_acc->acc_info.Index;

        DatabaseUpdateCtx dctx{ .sid = sid, .aid = aid };
        dctx.ops.emplace_back(PlayerSocialPatch{ .op = PlayerSocialPatch::Op::InsertOrUpdate, .aid = aid, .targetAid = target_aid, .State = NetEngine::Socials::State::Blocked, .TargetNickname = target_nickname });

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
            target_nickname = std::move(target_nickname),
            v = std::move(validated.value())
        ]() mutable
            {
                if (!session) return;

                auto new_acc_cache = CAccount.get<unique_t>(sid);
                ResultDbUpdateInfo dbres;

                if (!BaseLib::Database->UpdateAccount(v, dbres).has_value())
                {
                    if (dbres.target_not_found)
                        session->SendMsg(52, 0, Userlist::Blocked::AddResult::Offline, 0);
                    return;
                }

                auto applied = main_server->ApplyDatabaseUpdates(new_acc_cache, v);
                if (!applied.has_value())
                {
                    DEBUGLOG(red, "ApplyDatabaseUpdates failed for [{}] [{}]: {}", new_acc_cache->acc_info.Index, new_acc_cache->acc_info.Nickname.c_str(), static_cast<int>(applied.error()));
                    session->SendMsg(52, 0, Userlist::Blocked::AddResult::Offline, 0);
                    return;
                }
                /*
				auto target_sid = *CAidSid->get<shared_t>(target_aid);
                if (target_sid)
                {
                    auto target_social_list = CSocial.get<shared_t>(target_sid);
                    std::erase_if(*target_social_list, [&](const auto& social)
                        {
                            return social.Aid == aid;
						});
                    main_server->RemovePlayerSocials(target_social_list, aid);
                    target_social_list.unlock();
                }
                */

                MainPlayerBlockedAddAck blocked_data = { target_aid, target_nickname.c_str() };
                session->SendMsg(52, 0, Userlist::Blocked::AddResult::Success, 0, reinterpret_cast<uint8_t*>(&blocked_data), sizeof(MainPlayerBlockedAddAck));
                DEBUGLOG(dark_cyan, "player ({}) blocked ({})", new_acc_cache->acc_info.Nickname.c_str(), target_nickname.c_str());
            }
        );
    }
}