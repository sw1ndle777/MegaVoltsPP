#pragma once
namespace Game::Handlers
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Main;
    inline void ItemRepair(SCallbackData& callback, CMainServer* main_server)
    {
        auto session = callback.session;
        auto message = callback.message;
        if (!session || !message) return;
        //std::shared_lock lock(session->GetMutex());

        auto session_id = session->GetSessionId();
        auto acc_cache = CAccount.get<unique_t>(session_id);
        auto acc_index = acc_cache->acc_info.Index;
        auto items_count = static_cast<uint32_t>(message->GetOption());
        const auto& repairItemReq = reinterpret_cast<MainRepairItemSerialInfoReq*>(message->GetData());
        if (acc_index == -1) return;
        DatabaseUpdateCtx dctx{ .sid = session_id,.aid = acc_index };
        std::vector<ItemUpdateCtx> item_updates;


        std::vector<ItemSerialInfo> items_updated;
        uint32_t total_repair_price = 0;

        for (uint32_t i = 0; i < items_count; i++)
        {
            const auto& item_repair = main_server->GetPlayerItemInventory(acc_cache, repairItemReq->items[i]);
            if (!item_repair.has_value()) continue;
            auto item_info = CItemsInfo.get<shared_t>(item_repair.value().item_info.item_number.item_id);
            total_repair_price = total_repair_price + (item_info->Durability - item_repair.value().item_info.repair);
            ItemPatchCtx p
            {
                .sel = ItemSelector{.serial = item_repair.value().item_info.serial_info},
                .repair = item_info->Durability
            };
            dctx.ops.push_back(p);
            items_updated.push_back(repairItemReq->items[i]);
        }
        if (items_updated.empty() || acc_cache->acc_info.MicroPoints < total_repair_price) return;

        using enum CurrencyType;
        if (total_repair_price > 0) dctx.ops.emplace_back(AccountCurrencyDelta{ .type = MP, .value = total_repair_price, .is_reward = false });

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
            mp_cost = total_repair_price,
            serial_info_vec = std::move(items_updated),
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

                auto repairItemAckData = MainRepairItemAck(new_acc_cache->acc_info.MicroPoints, new_acc_cache->acc_info.RockTokens, serial_info_vec).Serialize();
                session->SendMsg(97, 0, 1, serial_info_vec.size(), reinterpret_cast<uint8_t*>(repairItemAckData.data()), repairItemAckData.size());

                DEBUGLOG(dark_cyan, "player ({}) spent {} mp to repair {} items", new_acc_cache->acc_info.Nickname.c_str(), mp_cost, serial_info_vec.size());
            });
    }
}