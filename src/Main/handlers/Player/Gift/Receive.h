#pragma once
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
        //std::shared_lock lock(session->GetMutex());
        auto session_id = session->GetSessionId();
        auto acc_cache = CAccount.get<unique_t>(session_id);
        auto acc_index = acc_cache->acc_info.Index;
        if (acc_index == -1) return;
        const auto& req = reinterpret_cast<MailBoxUpdateReq*>(message->GetData());

        DatabaseUpdateCtx dctx{ .sid = session_id,.aid = acc_index };
        using enum MailboxPatch::Op;
        using enum MailSide;
        std::vector<ShopItem> shop_items;
        std::vector<Item> new_items;
        std::vector<MailboxGift> gifts;

        // it's okay to reserve even if we don't use all of them in case of some mails not having gifts
        auto available_serials = main_server->FindLowestAvailableSerialIds(acc_cache->inventory_items, req->mail_count);
        if (available_serials.size() < req->mail_count)
        {
            DEBUGLOG(red,
                "Not enough unique serial IDs available. Requested: {}, got: {}",
                req->mail_count, available_serials.size());
            return;
        }

        for (uint32_t i = 0; i < req->mail_count; i++)
        {
            auto mail_id = req->mail_info[i].mail_id;
            auto mailbox_data = CMailboxData.get<shared_t>(mail_id);
            if (mailbox_data->gift_itemid == 0) continue;
            if (mailbox_data->receiver_account_id != acc_index) continue;
            if (mailbox_data->deleted_from_receiver) continue; // already received
            dctx.ops.emplace_back(MailboxPatch{ .op = Delete, .mail_id = mail_id, .side = Receiver });
            auto item_info = CItemsInfo.get<shared_t>(mailbox_data->gift_itemid);
            if (!item_info->Id) continue;
            auto serial_index = available_serials[i];
            ShopItem new_item = { {mailbox_data->gift_itemid , item_info->Stock} , ItemExpire::Type::Unused,  ItemSerialInfo(serial_index, 1, 1, Items::Origin::From_Game, Utility::GetUtcTimeNow()) };
            shop_items.push_back(new_item);
#if defined(RELEASE_1_0_3)
            const InventoryItemInfo& inv_item_info = { {mailbox_data->gift_itemid , item_info->Stock } ,ItemExpire::Type::Unused, new_item.serial_info, item_info->Durability, 0 };
#else
            const InventoryItemInfo& inv_item_info = { mailbox_data->gift_itemid ,ItemExpire::Type::Unused, new_item.serial_info, item_info->Durability, 0,0,0,0,0,main_server->AdjustItemType(item_info->Type) };
#endif
            new_items.push_back({ inv_item_info,item_info->Stock, false, 0, false });
            gifts.push_back({ mail_id, mailbox_data->time, new_item });
            dctx.ops.emplace_back(MailboxPatch{ .op = Delete, .mail_id = mail_id, .side = Receiver });
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
            gifts = std::move(gifts),
            s_items = std::move(shop_items),
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
                session->SendMsg(66, 0, 37, unopened_gifts); // remainder of unopened mails
            });
    }
}