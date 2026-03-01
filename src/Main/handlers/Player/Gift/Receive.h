#pragma once
#include <BaseLib/CLogging.h>

namespace Game::Handlers
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Main;
    
    inline void GiftReceive(SCallbackData& callback, CMainServer* main_server)
    {
        auto session = callback.session;
        auto message = callback.message;
        if (!session || !message) return;
        
        auto session_id = session->GetSessionId();
        auto acc_cache = CAccount.get<unique_t>(session_id);
        auto acc_index = acc_cache->acc_info.Index;
        if (acc_index == -1) return;
        
        const auto& req = reinterpret_cast<MailBoxUpdateReq*>(message->GetData());

        DatabaseUpdateCtx dctx{ .sid = session_id, .aid = acc_index };
        using enum MailboxPatch::Op;
        using enum MailSide;
        
        std::vector<ShopItem> shop_items;
        std::vector<Item> new_items;
        std::vector<MailboxGift> gifts;
        
        std::vector<std::pair<uint32_t, int32_t>> item_sender_pairs;

        auto available_serials = main_server->FindLowestAvailableSerialIds(acc_cache->inventory_items, req->mail_count);
        if (available_serials.size() < req->mail_count)
        {
            DEBUGLOG(red, "Not enough unique serial IDs available. Requested: {}, got: {}", req->mail_count, available_serials.size());
            return;
        }

        for (uint32_t i = 0; i < req->mail_count; i++)
        {
            auto mail_id = req->mail_info[i].mail_id;
            auto mailbox_data = CMailboxData.get<shared_t>(mail_id);
            
            if (mailbox_data->gift_itemid == 0) continue;
            if (mailbox_data->receiver_account_id != acc_index) continue;
            if (mailbox_data->deleted_from_receiver) continue;
            
            dctx.ops.emplace_back(MailboxPatch{ .op = Delete, .mail_id = mail_id, .side = Receiver });
            
            auto item_info = CItemsInfo.get<shared_t>(mailbox_data->gift_itemid);
            if (!item_info->Id) continue;
            
            auto serial_index = available_serials[i];
            ShopItem new_item = { {mailbox_data->gift_itemid, item_info->Stock}, ItemExpire::Type::Unused, ItemSerialInfo(serial_index, 1, 1, Items::Origin::From_Game, Utility::GetUtcTimeNow()) };
            shop_items.push_back(new_item);
            
#if defined(RELEASE_1_0_3)
            const InventoryItemInfo& inv_item_info = { {mailbox_data->gift_itemid, item_info->Stock}, ItemExpire::Type::Unused, new_item.serial_info, item_info->Durability, 0 };
#else
            const InventoryItemInfo& inv_item_info = { mailbox_data->gift_itemid, ItemExpire::Type::Unused, new_item.serial_info, item_info->Durability, 0, 0, 0, 0, 0, main_server->AdjustItemType(item_info->Type) };
#endif
            new_items.push_back({ inv_item_info, item_info->Stock, false, 0, false });
            gifts.push_back({ mail_id, mailbox_data->time, new_item });
            
            // Track sender for logging
            item_sender_pairs.emplace_back(mailbox_data->gift_itemid, mailbox_data->sender_account_id);
        }

        if (shop_items.empty() || new_items.empty()) return;
        
        dctx.ops.push_back(ItemAddCtx{ .items = std::move(new_items) });

        auto validated = main_server->ValidateDatabaseUpdates(acc_cache, dctx);
        if (!validated.has_value())
        {
            DEBUGLOG(red, "ValidateDatabaseUpdates failed for [{}] [{}]: {}", acc_cache->acc_info.Index, acc_cache->acc_info.Nickname.c_str(), static_cast<int>(validated.error()));
            return;
        }
        acc_cache.unlock();

        [[maybe_unused]] auto ignored = BaseLib::DbPool->submit_task([main_server,
            session = std::move(callback.session),
            s_id = session_id,
            acc_index = acc_index,
            gifts = std::move(gifts),
            s_items = std::move(shop_items),
            item_sender_pairs = std::move(item_sender_pairs),
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
            LogContext log_ctx;

            for (size_t i = 0; i < item_sender_pairs.size(); ++i)
            {
                const auto& [item_id, sender_aid] = item_sender_pairs[i];

                std::optional<uint64_t> serial_data;
                for (const auto& added_item : v.items_added)
                {
                    if (added_item.item_info.item_number.item_id == item_id)
                    {
                        serial_data = added_item.item_info.serial_info.data;
                        break;
                    }
                }

                ItemLogEntry item_log;
                item_log.aid = acc_index;
                item_log.related_aid = sender_aid;
                item_log.action_type = ItemLog::ActionType::Received;
                item_log.item_id = item_id;
                if (serial_data.has_value())
                    item_log.serial_info = serial_data.value();
                item_log.origin_type = ItemLog::OriginType::GiftReceived;
                
                auto item_info = CItemsInfo.get<shared_t>(item_id);
                if (item_info->Id)
                {
                    item_log.item_type = static_cast<ItemLog::ItemType>(item_info->Type);
                }
                
                log_ctx.item_logs.push_back(item_log);
            }

            if (!log_ctx.empty())
            {
                auto log_result = BaseLib::Database->PersistLogs(log_ctx);
                if (!log_result.has_value())
                {
                    DEBUGLOG(red, "Failed to persist gift receive logs for player [{}]: {}",
                        new_acc_cache->acc_info.Nickname.c_str(),
                        log_result.error().message);
                }
            }

            for (auto& gift : gifts)
                session->SendMsg(66, 0, Mailbox::SendResult::Gift, 0, reinterpret_cast<uint8_t*>(&gift), sizeof(MailboxGift));

            if (!s_items.empty())
                session->SendMsg(99, 0, 37, static_cast<uint8_t>(s_items.size()), reinterpret_cast<uint8_t*>(s_items.data()), static_cast<uint16_t>(s_items.size() * sizeof(ShopItem)));

            uint32_t unopened_gifts = 0;
            auto mail_recv_ids = CGiftRecv.get<shared_t>(new_acc_cache->acc_info.Index);
            for (uint32_t i = 0; i < mail_recv_ids->size(); i++)
            {
                auto mail_id = mail_recv_ids->at(i);
                auto mailbox_data = CMailboxData.get<shared_t>(mail_id);
                if (mailbox_data->gift_itemid != 0) unopened_gifts++;
            }
            session->SendMsg(66, 0, 37, unopened_gifts);
        });
    }
}