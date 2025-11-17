#pragma once
namespace Game::Handlers
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Main;
    inline void ItemSell(SCallbackData& callback, CMainServer* main_server)
    {
        auto session = callback.session;
        auto message = callback.message;
        if (!session || !message) return;
        //std::shared_lock lock(session->GetMutex());

        uint32_t items_count = static_cast<uint32_t>(message->GetOption());
        auto session_id = session->GetSessionId();
        auto acc_cache = CAccount.get<unique_t>(session_id);
        auto acc_index = acc_cache->acc_info.Index;
        const auto& sellItemReq = reinterpret_cast<MainSellItemSerialInfoReq*>(message->GetData());
        if (acc_index == -1) return;
        const auto& item_sold = main_server->GetPlayerItemInventory(acc_cache, sellItemReq->item);
        if (!item_sold.has_value())
        {
            MainSellItemAck sell_item_data = { ItemSerialInfo(0, 0, 0, 0, 0), 0 };
            session->SendMsg(100, 0, 21, 0, reinterpret_cast<uint8_t*>(&sell_item_data), sizeof(sell_item_data));
            DEBUGLOG(dark_cyan, "player ({}) failed to sell unknown item serial info: ({})", acc_cache->acc_info.Nickname.c_str(), sellItemReq->item.data);
            return;
        }
        auto item_info = CItemsInfo.get<shared_t>(item_sold.value().item_info.item_number.item_id);
        DatabaseUpdateCtx dctx{ .sid = session_id,.aid = acc_index };


        using enum CurrencyType;
        dctx.ops.emplace_back(AccountCurrencyDelta{ .type = MP, .value = item_info->SellPointPrice, .is_reward = true });
        dctx.ops.push_back(ItemDeleteCtx{ .serials = { item_sold.value().item_info.serial_info } });

        auto validated = main_server->ValidateDatabaseUpdates(acc_cache, dctx);
        if (!validated.has_value())
        {
            DEBUGLOG(red, "ValidateDatabaseUpdates failed for [{}] [{}]: {}", acc_cache->acc_info.Index, acc_cache->acc_info.Nickname.c_str(), static_cast<int>(validated.error()));
            return;
        }

        acc_cache.unlock();
        [[maybe_unused]] auto ignored_result = BaseLib::DbPool->submit_task([main_server, session = std::move(callback.session), mp_reward = item_info->SellPointPrice, itm_sold = item_sold.value().item_info.serial_info, item_id = item_info->Id, v = std::move(validated.value())
        ]() mutable
            {
                if (!session) return;

                ResultDbUpdateInfo dbres;
                if (!BaseLib::Database->UpdateAccount(v, dbres).has_value()) return;

                auto new_acc_cache = CAccount.get<unique_t>(session->GetSessionId());

                auto applied = main_server->ApplyDatabaseUpdates(new_acc_cache, v);
                if (!applied.has_value())
                {
                    DEBUGLOG(red, "ApplyDatabaseUpdates failed for [{}] [{}]: {}", new_acc_cache->acc_info.Index, new_acc_cache->acc_info.Nickname.c_str(), static_cast<int>(applied.error()));
                    return;
                }

                MainSellItemAck sell_item_data = { itm_sold, mp_reward };
                session->SendMsg(100, 0, 1, 0, reinterpret_cast<uint8_t*>(&sell_item_data), sizeof(sell_item_data));

                MainCurrencyUpdateAck currency_update_data = { new_acc_cache->acc_info.RockTokens, new_acc_cache->acc_info.MicroPoints, new_acc_cache->acc_info.Coins };
                session->SendMsg(307, 0, 1, 0, reinterpret_cast<uint8_t*>(&currency_update_data), sizeof(currency_update_data));

                DEBUGLOG(dark_cyan, "player ({}) sold item: ({}) for {} mp", new_acc_cache->acc_info.Nickname.c_str(), item_id, mp_reward);
            }
        );
    }
}