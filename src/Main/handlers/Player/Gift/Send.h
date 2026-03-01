#pragma once
#include <BaseLib/CLogging.h>

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
        
        auto target_acc_cache = CAccount.get_by_filter<shared_t>([&](const auto&, auto& player) {
            return Utility::ToLowercase(player.acc_info.Nickname) == Utility::ToLowercase(target_nick);
        });
        
        int32_t target_aid = -1;
        if (target_acc_cache->acc_info.Index)
        {
            target_sid = target_acc_cache->session_id;
            target_aid = target_acc_cache->acc_info.Index;
        }
        target_acc_cache.unlock();
        
        if (target_aid == -1)
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
        
        auto rt_before = acc_cache->acc_info.RockTokens;
        auto mp_before = acc_cache->acc_info.MicroPoints;
        
        DatabaseUpdateCtx dctx{ .sid = session_id, .aid = acc_index };
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
            session = std::move(callback.session),
            ts_id = target_sid,
            s_id = session_id,
            acc_index = acc_index,
            target_aid = target_aid,
            item_id = item_id,
            rt_spent = rt_spent,
            mp_spent = mp_spent,
            rt_before = rt_before,
            mp_before = mp_before,
            v = std::move(validated.value())
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

            LogContext log_ctx;

            ItemLogEntry item_log;
            item_log.aid = acc_index;
            item_log.related_aid = target_aid;
            item_log.action_type = ItemLog::ActionType::Gifted;
            item_log.item_id = item_id;
            item_log.origin_type = ItemLog::OriginType::GiftSent;
            item_log.rt_delta = -static_cast<int32_t>(rt_spent);
            item_log.mp_delta = -static_cast<int32_t>(mp_spent);
            
            auto item_info = CItemsInfo.get<shared_t>(item_id);
            if (item_info->Id)
            {
                item_log.item_type = static_cast<ItemLog::ItemType>(item_info->Type);
            }
            
            log_ctx.item_logs.push_back(item_log);

            if (rt_spent > 0)
            {
                CurrencyLogEntry rt_log;
                rt_log.aid = acc_index;
                rt_log.currency_type = CurrencyLog::Type::RT;
                rt_log.amount = -static_cast<int32_t>(rt_spent);
                rt_log.before_value = rt_before;
                rt_log.after_value = new_acc_cache->acc_info.RockTokens;
                rt_log.source_type = CurrencyLog::SourceType::GiftSend;
                rt_log.related_item_id = item_id;
                log_ctx.currency_logs.push_back(rt_log);
            }
            if (mp_spent > 0)
            {
                CurrencyLogEntry mp_log;
                mp_log.aid = acc_index;
                mp_log.currency_type = CurrencyLog::Type::MP;
                mp_log.amount = -static_cast<int32_t>(mp_spent);
                mp_log.before_value = mp_before;
                mp_log.after_value = new_acc_cache->acc_info.MicroPoints;
                mp_log.source_type = CurrencyLog::SourceType::GiftSend;
                mp_log.related_item_id = item_id;
                log_ctx.currency_logs.push_back(mp_log);
            }

            if (!log_ctx.empty())
            {
                auto log_result = BaseLib::Database->PersistLogs(log_ctx);
                if (!log_result.has_value())
                {
                    DEBUGLOG(red, "Failed to persist gift send logs for player [{}]: {}",
                        new_acc_cache->acc_info.Nickname.c_str(),
                        log_result.error().message);
                }
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