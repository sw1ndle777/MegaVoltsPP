#pragma once
namespace Game::Handlers
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Main;
    inline void ItemDelete(SCallbackData& callback, CMainServer* main_server)
    {
        auto session = callback.session;
        auto message = callback.message;
        if (!session || !message) return;

        //std::shared_lock lock(session->GetMutex());
        auto session_id = session->GetSessionId();
        auto acc_cache = CAccount.get<unique_t>(session_id);
        auto acc_index = acc_cache->acc_info.Index;
        const auto& deleteItemReq = reinterpret_cast<MainDeleteItemSerialInfoReq*>(callback.message->GetData());
        if (acc_index == -1) return;
        std::vector<ItemSerialInfo> items_deleted;
        LogContext log_ctx;
        for (uint32_t i = 0; i < deleteItemReq->item_count; i++)
        {
            const auto& item_deleted = main_server->GetPlayerItemInventory(acc_cache, deleteItemReq->items[i]);
            if (!item_deleted.has_value()) continue;
            auto item_info = CItemsInfo.get<shared_t>(item_deleted.value().item_info.item_number.item_id);
            DEBUGLOG(dark_cyan, "player ({}) item id: ({}) serial: ({})", acc_cache->acc_info.Nickname.c_str(), item_deleted.value().item_info.item_number.item_id, deleteItemReq->items[i].data);
            items_deleted.push_back(deleteItemReq->items[i]);


            ItemLogEntry item_log;
            item_log.aid = acc_index;
            item_log.action_type = ItemLog::ActionType::Deleted;
            item_log.item_id = item_deleted.value().item_info.item_number.item_id;
            item_log.serial_info = deleteItemReq->items[i].data;
            item_log.origin_type = ItemLog::OriginType::Unknown;
            item_log.mp_delta = 0;
            log_ctx.item_logs.push_back(item_log);
        }

        if (items_deleted.empty()) return;
        DatabaseUpdateCtx dctx{ .sid = session_id, .aid = acc_index };
        dctx.ops.push_back(ItemDeleteCtx{ .serials = items_deleted });

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
            itm_deleted = std::move(items_deleted),
			logContext = std::move(log_ctx),
            v = std::move(validated.value())]() mutable
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
                auto deleteItemData = MainDeleteItemAck(itm_deleted).Serialize();
                session->SendMsg(89, 0, 1, 0, reinterpret_cast<uint8_t*>(deleteItemData.data()), deleteItemData.size());

                BaseLib::Database->PersistLogs(logContext);
            });
    }
}