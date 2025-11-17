#pragma once
namespace Game::Handlers
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Main;

    inline void BlockedsRemove(SCallbackData& callback, CMainServer* main_server)
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
        if (!main_server->IsBlockedAlready(social_list, target_aid)) { social_list.unlock(); return; }
        social_list.unlock();

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
            sid = sid,
            target_aid = target_aid,
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
                session->SendMsg(53, 0, 1, 0);
                DEBUGLOG(dark_cyan, "player ({}) unblocked account id ({})", new_acc_cache->acc_info.Nickname.c_str(), target_aid);
            }
        );
    }
}
