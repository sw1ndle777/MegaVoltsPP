#pragma once
namespace Game::Handlers
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Main;
    inline void MailSend(SCallbackData& callback, CMainServer* main_server)
    {
        auto session = callback.session;
        auto message = callback.message;
        if (!session) return;
        auto session_id = session->GetSessionId();
        auto acc_cache = CAccount.get<unique_t>(session_id);
        auto acc_index = acc_cache->acc_info.Index;
        if (acc_index == -1) return;
        const auto& req = reinterpret_cast<MailBoxSendReq*>(message->GetData());
        uint16_t target_sid = 0;
        auto target_nick = Utility::ReadStringSafe(req->nickname, sizeof(req->nickname));
        if (target_nick == acc_cache->acc_info.Nickname)
        {
            session->SendMsg(104, 0, Mailbox::SendResult::UserNotFound, 0);
            DEBUGLOG(red, "player ({}) tried to send mail to themselves", acc_cache->acc_info.Nickname.c_str());
            return;
        }
        auto target_acc_cache = CAccount.get_by_filter<shared_t>([&](const auto& /*id*/, auto& player) {
            return Utility::ToLowercase(player.acc_info.Nickname) == Utility::ToLowercase(target_nick);
            });
        if (target_acc_cache->acc_info.Index) target_sid = target_acc_cache->session_id;
        target_acc_cache.unlock();
        DatabaseUpdateCtx dctx{ .sid = session_id,.aid = acc_index };
        using enum MailboxPatch::Op;
        using enum MailSide;
        dctx.ops.emplace_back(MailboxPatch{ .op = Insert, .insert = MailInsert{
            .sender_nickname = acc_cache->acc_info.Nickname,
            .receiver_nickname = std::move(target_nick),
            .message = Utility::ReadStringSafe(req->msg, sizeof(req->msg))} }
            );
        auto validated = main_server->ValidateDatabaseUpdates(acc_cache, dctx);
        if (!validated.has_value())
        {
            using enum DbUpdateError;
            const auto& err = validated.error();
            if (err == MEMO_MAIL_BLOCKEDBY_SENDER || err == MEMO_MAIL_BLOCKEDBY_RECEIVER)
                session->SendMsg(104, 0, Mailbox::SendResult::Blacklist, 0);
            else if (err == MEMO_MAIL_FULL_SENDER)
                session->SendMsg(104, 0, Mailbox::SendResult::FullSender, 0);

            DEBUGLOG(red, "ValidateDatabaseUpdates failed for [{}] [{}]: {}", acc_cache->acc_info.Index, acc_cache->acc_info.Nickname.c_str(), static_cast<int>(validated.error()));
            return;
        }
        acc_cache.unlock();
        [[maybe_unused]] auto ignored = BaseLib::DbPool->submit_task([main_server, session = std::move(callback.session), s_id = session_id, ts_id = target_sid, v = std::move(validated.value())
        ]() mutable
            {
                if (!session) return;
                ResultDbUpdateInfo dbres;
                auto db_res = BaseLib::Database->UpdateAccount(v, dbres);
                if (!db_res.has_value())
                {
                    using enum DbError::Type;
                    const auto& err = db_res.error();
                    if (err.type == NicknameNotFound)
                        session->SendMsg(104, 0, Mailbox::SendResult::UserNotFound, 0);
                    else if (err.type == MailboxFull)
                        session->SendMsg(104, 0, Mailbox::SendResult::FullReceiver, 0);
                    else if (err.type == BlockedByReceiver)
                        session->SendMsg(104, 0, Mailbox::SendResult::Blacklist, 0);

                    DEBUGLOG(red, "UpdateAccount failed for [{}] [{}]: {}", s_id, static_cast<int>(err.type), err.message);
                    return;
                }
                auto new_acc_cache = CAccount.get<unique_t>(s_id);
                auto applied = main_server->ApplyDatabaseUpdates(new_acc_cache, v);
                if (!applied.has_value())
                {
                    DEBUGLOG(red, "ApplyDatabaseUpdates failed for [{}] [{}]: {}", new_acc_cache->acc_info.Index, new_acc_cache->acc_info.Nickname.c_str(), static_cast<int>(applied.error()));
                    return;
                }
                if (auto player_session = main_server->GetSessionById(ts_id))
                {
                    DEBUGLOG(dark_cyan, "player ({}) found online to receive mail", ts_id);
                    player_session->SendMsg(104, 0, Mailbox::SendResult::NewMail, 0);
                }
            });
    }
}