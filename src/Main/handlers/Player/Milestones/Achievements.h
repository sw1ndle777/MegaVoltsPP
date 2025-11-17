#pragma once
namespace Game::Handlers
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Main;
    bool checkAchievement(uint64_t& tier, int achievementId)
    {
        assert(achievementId >= 0 && achievementId < 64 && "Achievement ID must be between 0 and 63.");
        return (tier & (1ULL << achievementId)) != 0;
    }

    void doAchievement(uint64_t& tier, int achievementId)
    {
        assert(achievementId >= 0 && achievementId < 64 && "Achievement ID must be between 0 and 63.");
        tier |= (1ULL << achievementId);
    }
    inline void Achievements(SCallbackData& callback, CMainServer* main_server)
    {
        auto session = callback.session;
        auto message = callback.message;
        if (!session || !message) return;
        //std::shared_lock lock(session->GetMutex());

        auto session_id = session->GetSessionId();
        auto acc_cache = CAccount.get<unique_t>(session_id);

        auto acc_index = acc_cache->acc_info.Index;
        if (acc_index == -1) return;

        auto achievement_done = acc_cache->acc_info.Achievement;
        auto desired_achiv = message->GetOption();
        DEBUGLOG(dark_cyan, "player want to complete achievement: ({})", desired_achiv);

        if (desired_achiv <= 0 || desired_achiv >= 64) return;

        auto current_coll = CCollectionInfo.get<shared_t>(desired_achiv);
        if (current_coll->missionType != 3 || checkAchievement(achievement_done, desired_achiv)) return;
        doAchievement(achievement_done, desired_achiv);

        DatabaseUpdateCtx dctx{ .sid = session_id, .aid = acc_cache->acc_info.Index };

        dctx.ops.push_back(AccountInfoPatch{ .achievement_tier1 = achievement_done });
        if (current_coll->rewardPoint > 0)
        {
            using enum CurrencyType;
            dctx.ops.emplace_back(AccountCurrencyDelta{ .type = MP, .value = current_coll->rewardPoint, .is_reward = true });
        }

        auto validated = main_server->ValidateDatabaseUpdates(acc_cache, dctx);
        if (!validated.has_value())
        {
            DEBUGLOG(red, "ValidateDatabaseUpdates failed for [{}] [{}]: {}", acc_cache->acc_info.Index, acc_cache->acc_info.Nickname.c_str(), static_cast<int>(validated.error()));
            return;
        }
        acc_cache.unlock();
        [[maybe_unused]] auto ignored = BaseLib::DbPool->submit_task([main_server, session = std::move(callback.session), s_id = session_id, v = std::move(validated.value())]() mutable
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