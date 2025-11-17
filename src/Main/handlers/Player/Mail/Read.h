#pragma once
namespace Game::Handlers
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Main;
    inline void MailRead(SCallbackData& callback, CMainServer* main_server)
    {
        auto session = callback.session;
        auto message = callback.message;
        if (!session || !message) return;
        //std::shared_lock lock(session->GetMutex());

        auto session_id = session->GetSessionId();
        auto acc_cache = CAccount.get<unique_t>(session_id);

        auto acc_index = acc_cache->acc_info.Index;
        if (acc_index == -1) return;

        const auto& req = reinterpret_cast<MailBoxUpdateReq*>(message->GetData());
        DatabaseUpdateCtx dctx{ .sid = session_id,.aid = acc_index };
        using enum MailboxPatch::Op;
        using enum MailSide;
        for (auto i = 0; i < req->mail_count; i++)
            dctx.ops.emplace_back(MailboxPatch{ .op = MarkRead, .mail_id = req->mail_info[i].mail_id, .read = true, .side = Receiver });

        auto validated = main_server->ValidateDatabaseUpdates(acc_cache, dctx);
        if (!validated.has_value())
        {
            DEBUGLOG(red, "ValidateDatabaseUpdates failed for [{}] [{}]: {}", acc_cache->acc_info.Index, acc_cache->acc_info.Nickname.c_str(), static_cast<int>(validated.error()));
            return;
        }
        acc_cache.unlock();

        [[maybe_unused]] auto ignored = BaseLib::DbPool->submit_task([main_server, session = std::move(callback.session), s_id = session_id, v = std::move(validated.value())
        ]() mutable
            {
                if (!session) return;
                ResultDbUpdateInfo dbres;
                if (!BaseLib::Database->UpdateAccount(v, dbres).has_value()) return;
                auto new_acc_cache = CAccount.get<unique_t>(s_id);
                auto applied = main_server->ApplyDatabaseUpdates(new_acc_cache, v);
                if (!applied.has_value())
                {
                    DEBUGLOG(red, "ApplyDatabaseUpdates failed for [{}] [{}]: {}", new_acc_cache->acc_info.Index, new_acc_cache->acc_info.Nickname.c_str(), static_cast<int>(applied.error()));
                    return;
                }
            });
    }
}