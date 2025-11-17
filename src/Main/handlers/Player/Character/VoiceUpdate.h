#pragma once
namespace Game::Handlers
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Main;
    inline void VoiceUpdate(SCallbackData& callback, CMainServer* main_server)
    {
        auto session = callback.session;
        auto message = callback.message;
        if (!session || !message) return;

        //std::shared_lock lock(session->GetMutex());
        auto session_id = session->GetSessionId();
        auto acc = CAccount.get<unique_t>(session_id);
        auto aid = acc->acc_info.Index;
        if (aid == -1) return;
        auto voice_type = message->GetOption();

        if (acc->acc_info.VoiceType == static_cast<int>(voice_type))
        {
            session->SendMsg(160, 0, 0, voice_type);
            return;
        }

        DatabaseUpdateCtx dctx{ .sid = session_id,.aid = aid };
        dctx.ops.emplace_back(AccountInfoPatch{ .voice_type = static_cast<uint32_t>(voice_type) });


        auto validated = main_server->ValidateDatabaseUpdates(acc, dctx);
        if (!validated.has_value())
        {
            DEBUGLOG(red, "ValidateDatabaseUpdates failed for [{}] [{}]: {}", acc->acc_info.Index, acc->acc_info.Nickname.c_str(), static_cast<int>(validated.error()));
            return;
        }
        acc.unlock();
        [[maybe_unused]] auto ignored = BaseLib::DbPool->submit_task([main_server, session = std::move(callback.session),
            s_id = session_id,
            voice_type = voice_type,
            v = std::move(validated.value())
        ]() mutable
            {
                if (!session) return;
                ResultDbUpdateInfo dbres;
                if (!BaseLib::Database->UpdateAccount(v, dbres).has_value())
                {
                }
                auto new_acc_cache = CAccount.get<unique_t>(s_id);
                auto applied = main_server->ApplyDatabaseUpdates(new_acc_cache, v);
                if (!applied.has_value())
                {
                    DEBUGLOG(red, "ApplyDatabaseUpdates failed for [{}] [{}]: {}", new_acc_cache->acc_info.Index, new_acc_cache->acc_info.Nickname.c_str(), static_cast<int>(applied.error()));
                    return;
                }
                if (auto pss = main_server->GetSessionById(s_id))
                {
                    pss->SendMsg(160, 0, 0, voice_type);
                    DEBUGLOG(dark_cyan, "player=({}) selected voice=({})", new_acc_cache->acc_info.Nickname.c_str(), voice_type);
                }

            });
    }
}