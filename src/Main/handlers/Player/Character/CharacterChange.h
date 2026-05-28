#pragma once
namespace Game::Handlers
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Main;
    inline void CharacterChange(SCallbackData& callback, CMainServer* main_server)
    {
        auto session = callback.session;
        auto message = callback.message;
        if (!session || !message) return;

        //std::shared_lock lock(session->GetMutex());
        CServer* server = callback.server;
        auto session_id = session->GetSessionId();
        auto acc = CAccount.get<unique_t>(session_id);
        if (!acc->acc_info.Index) return;
        auto character = static_cast<Character::Type>(message->GetOption());
        if (acc->acc_info.SelectedCharacter == character)
        {
            session->SendMsg(74, 0, CharacterSelectInfo::Result::Ok, static_cast<uint8_t>(character));
            return;
        }
        DatabaseUpdateCtx dctx{ .sid = session_id,.aid = acc->acc_info.Index };
        dctx.ops.emplace_back(AccountInfoPatch{ .selected_character = static_cast<uint32_t>(character) });


        auto validated = main_server->ValidateDatabaseUpdates(acc, dctx);
        if (!validated.has_value())
        {
            DEBUGLOG(red, "ValidateDatabaseUpdates failed for [{}] [{}]: {}", acc->acc_info.Index, acc->acc_info.Nickname.c_str(), static_cast<int>(validated.error()));
            return;
        }
        acc.unlock();
        [[maybe_unused]] auto ignored = BaseLib::DbPool->submit_task([main_server, session = std::move(callback.session),
            s_id = session_id,
            character = character,
            v = std::move(validated.value())
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
                main_server->RefreshPlayerHealthCache(*new_acc_cache, !new_acc_cache->playing);
                main_server->SendCastPlayerHealthSync(new_acc_cache->session_id, new_acc_cache->max_health, new_acc_cache->current_health);
                if (auto pss = main_server->GetSessionById(s_id))
                    pss->SendMsg(74, 0, CharacterSelectInfo::Result::Ok, static_cast<uint8_t>(character));
            });
    }
}
