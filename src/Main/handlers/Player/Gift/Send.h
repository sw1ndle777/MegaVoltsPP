#pragma once
namespace Game::Handlers
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Main;
    inline void GiftSend(SCallbackData& callback, CMainServer* main_server)
    {
        auto session = callback.session;
        auto message = callback.message;
        if (!session) return;
        auto session_id = session->GetSessionId();
        auto acc_cache = CAccount.get<unique_t>(session_id);
        auto acc_index = acc_cache->acc_info.Index;
        if (acc_index == -1) return;
        const auto& req = reinterpret_cast<MailGiftSendReq*>(message->GetData());
        uint16_t target_sid = 0;
        auto target_nick = Utility::ReadStringSafe(req->nickname, sizeof(req->nickname));
        if (target_nick == acc_cache->acc_info.Nickname)
        {
            session->SendMsg(64, 0, Mailbox::GiftResult::USER_OFFLINE, 0);
            DEBUGLOG(red, "player ({}) tried to send mail to themselves", acc_cache->acc_info.Nickname.c_str());
            return;
        }
        auto target_acc_cache = CAccount.get_by_filter<shared_t>([&](const auto& /*id*/, auto& player) {
            return Utility::ToLowercase(player.acc_info.Nickname) == Utility::ToLowercase(target_nick);
            });
        if (target_acc_cache->acc_info.Index) target_sid = target_acc_cache->session_id;
        target_acc_cache.unlock();
        if (!target_sid)
        {
            session->SendMsg(64, 0, Mailbox::GiftResult::USER_OFFLINE, 0);
            return;
        }
        auto item_id = req->vendor_item_id;
        auto msg_size = message->GetDataSize() - sizeof(req->nickname) - sizeof(item_id);



        if (!CVendorItems.contains(item_id))
        {
            DEBUGLOG(red, "player ({}) tried to send non vendor item ({}) as gift", acc_cache->acc_info.Nickname.c_str(), item_id);
            return;
        }
        auto item_info = CItemsInfo.get<shared_t>(item_id);
        if (!item_info->Id) return;
        auto rt_spent = item_info->CashPrice;
        auto mp_spent = item_info->PointPrice;
        DatabaseUpdateCtx dctx{ .sid = session_id,.aid = acc_index };
        using enum CurrencyType;
        if (rt_spent > 0) dctx.ops.emplace_back(AccountCurrencyDelta{ .type = RT, .value = rt_spent, .is_reward = false });
        if (mp_spent > 0) dctx.ops.emplace_back(AccountCurrencyDelta{ .type = MP, .value = mp_spent, .is_reward = false });


        using enum MailboxPatch::Op;
        using enum MailSide;
        dctx.ops.emplace_back(MailboxPatch{ .op = Insert, .insert = MailInsert{
            .sender_nickname = acc_cache->acc_info.Nickname,
            .receiver_nickname = std::move(target_nick),
            .message = Utility::ReadStringSafe(req->msg, msg_size),
            .gift_item_id = req->vendor_item_id} }
            );
        auto validated = main_server->ValidateDatabaseUpdates(acc_cache, dctx);
        if (!validated.has_value())
        {
            using enum DbUpdateError;
            const auto& err = validated.error();
            if (err == InsufficientMP || err == InsufficientRT)
                session->SendMsg(64, 0, Mailbox::GiftResult::NOT_ENOUGH_CASH, 0);
            else if (err == MEMO_MAIL_BLOCKEDBY_SENDER || err == MEMO_MAIL_BLOCKEDBY_RECEIVER)
                session->SendMsg(64, 0, Mailbox::GiftResult::BLACKLIST_ERROR_10, 0);
            else if (err == MEMO_MAIL_FULL_SENDER)
                session->SendMsg(64, 0, Mailbox::GiftResult::MEMO_GIFT_FULL_SENDER, 0);

            DEBUGLOG(red, "ValidateDatabaseUpdates failed for [{}] [{}]: {}", acc_cache->acc_info.Index, acc_cache->acc_info.Nickname.c_str(), static_cast<int>(validated.error()));
            return;
        }
        acc_cache.unlock();
        [[maybe_unused]] auto ignored = BaseLib::DbPool->submit_task([main_server,
            session = std::move(callback.session), ts_id = target_sid, s_id = session_id, item_id = item_id, v = std::move(validated.value())
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
                        session->SendMsg(64, 0, Mailbox::GiftResult::USER_OFFLINE, 0);
                    else if (err.type == MailboxFull)
                        session->SendMsg(64, 0, Mailbox::GiftResult::MEMO_GIFT_FULL_RECIEVER, 0);
                    else if (err.type == BlockedByReceiver)
                        session->SendMsg(64, 0, Mailbox::GiftResult::BLACKLIST_ERROR_10, 0);

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
                session->SendMsg(64, 0, Mailbox::GiftResult::PRESENT_SEND_SUCCESS, 0, reinterpret_cast<uint8_t*>(&item_id), sizeof(item_id));
                if (auto player_session = main_server->GetSessionById(ts_id))
                {
                    DEBUGLOG(dark_cyan, "player ({}) found online to receive gift", ts_id);
                    player_session->SendMsg(64, 0, Mailbox::SendResult::NewMail, 0);
                }
            });
    }
}