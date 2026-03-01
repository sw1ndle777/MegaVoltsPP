#pragma once
#include <BaseLib/CLogging.h>

namespace Game::Handlers
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Main;
    
    inline void ShopNormal(SCallbackData& callback, CMainServer* main_server)
    {
        auto session = callback.session;
        auto message = callback.message;
        if (!session || !message) return;
        
        uint32_t items_count = static_cast<uint32_t>(message->GetOption());
        auto session_id = session->GetSessionId();
        auto acc_cache = CAccount.get<unique_t>(session_id);
        auto acc_index = acc_cache->acc_info.Index;
        auto order = message->GetOrder();
        auto extra = message->GetExtra();
        auto mission = message->GetMission();
        if (acc_index == -1) return;

        DEBUGLOG(dark_cyan, "player ({}) normal shop -> mission: ({}), extra: ({}), option: ({})", acc_cache->acc_info.Nickname.c_str(), message->GetMission(), message->GetExtra(), message->GetOption());
        
        auto rt_before = acc_cache->acc_info.RockTokens;
        auto mp_before = acc_cache->acc_info.MicroPoints;
        
        DatabaseUpdateCtx dctx{ .sid = session_id, .aid = acc_index };
        uint32_t rt_spent = 0;
        uint32_t mp_spent = 0;
        std::vector<ShopSerialInfo> rebuy_items;
        std::vector<ItemPatchCtx> item_patches;
        std::vector<uint32_t> bought_item_ids;
        
        if (mission)
        {
            const auto& buyItemIdReq = reinterpret_cast<MainBuyItemSerialInfoReq*>(message->GetData());

            for (uint32_t i = 0; i < items_count; i++)
            {
                const auto& item_bought = buyItemIdReq->items[i];
                const auto& item_inv = main_server->GetPlayerItemInventory(acc_cache, item_bought);
                if (!item_inv.has_value()) continue;

                if (CVendorItems.contains(item_bought.id))
                {
                    auto item_info = CItemsInfo.get<shared_t>(item_bought.id);
                    auto new_time = Utility::GetUtcTimeNowPlusSeconds(item_info->LimitedTime);
                    rebuy_items.push_back({ item_bought, new_time });

                    ItemPatchCtx p
                    {
                        .sel = ItemSelector{.serial = item_inv.value().item_info.serial_info},
                        .expire_date = new_time,
                    };
                    item_patches.push_back(std::move(p));

                    rt_spent += item_info->CashPrice;
                    mp_spent += item_info->PointPrice;
                    bought_item_ids.push_back(item_bought.id);
                    DEBUGLOG(dark_cyan, "player ({}) bought item id: ({})", acc_cache->acc_info.Nickname.c_str(), item_info->Id);
                }
            }
        }
        else
        {
            const auto& buyItemIdReq = reinterpret_cast<MainBuyItemIdReq*>(message->GetData());
            std::vector<ShopItem> shop_items;
            std::vector<Item> new_items;

            auto available_serials = main_server->FindLowestAvailableSerialIds(acc_cache->inventory_items, items_count);
            if (available_serials.size() < items_count)
            {
                DEBUGLOG(red, "Not enough unique serial IDs available. Requested: {}, got: {}", items_count, available_serials.size());
                return;
            }

            for (uint32_t i = 0; i < items_count; i++)
            {
                const auto& item_bought_id = buyItemIdReq->items[i] & 0x7FFFFF;
                DEBUGLOG(dark_cyan, "player ({}) request buy id: ({})", acc_cache->acc_info.Nickname.c_str(), item_bought_id);

                if (CVendorItems.contains(item_bought_id))
                {
                    auto item_info = CItemsInfo.get<shared_t>(item_bought_id);
                    auto serial_index = available_serials[i];
                    ShopItem new_item = { {item_bought_id, item_info->Stock}, ItemExpire::Type::Unused, ItemSerialInfo(serial_index, 1, 1, Items::Origin::From_Game, Utility::GetUtcTimeNow()) };
                    shop_items.push_back(new_item);
#if defined(RELEASE_1_0_3)
                    const InventoryItemInfo& inv_item_info = { {item_bought_id, item_info->Stock}, ItemExpire::Type::Unused, new_item.serial_info, item_info->Durability, 0 };
#else
                    const InventoryItemInfo& inv_item_info = { item_bought_id, ItemExpire::Type::Unused, new_item.serial_info, item_info->Durability, 0, 0, 0, 0, 0, main_server->AdjustItemType(item_info->Type) };
#endif
                    new_items.push_back({ inv_item_info, item_info->Stock, false, 0, false });
                    rt_spent += item_info->CashPrice;
                    mp_spent += item_info->PointPrice;
                    bought_item_ids.push_back(item_bought_id);
                    DEBUGLOG(dark_cyan, "player ({}) attempt bought item id: ({}), new serial: ({})", acc_cache->acc_info.Nickname.c_str(), item_info->Id, new_item.serial_info.data);
                }
                else
                    DEBUGLOG(dark_cyan, "player ({}) request buy id: ({}) failed because it doesn't exist in vendor info", acc_cache->acc_info.Nickname.c_str(), item_bought_id);
            }

            if (mission && rebuy_items.empty()) return;
            else if (shop_items.empty() || new_items.empty()) return;

            using enum CurrencyType;
            if (rt_spent > 0) dctx.ops.emplace_back(AccountCurrencyDelta{ .type = RT, .value = rt_spent, .is_reward = false });
            if (mp_spent > 0) dctx.ops.emplace_back(AccountCurrencyDelta{ .type = MP, .value = mp_spent, .is_reward = false });

            if (!mission)
                dctx.ops.push_back(ItemAddCtx{ .items = std::move(new_items) });
            else
                dctx.ops.insert(dctx.ops.end(), item_patches.begin(), item_patches.end());

            auto validated = main_server->ValidateDatabaseUpdates(acc_cache, dctx);
            if (!validated.has_value())
            {
                if (validated.error() == DbUpdateError::InsufficientMP || validated.error() == DbUpdateError::InsufficientRT)
                    DEBUGLOG(dark_cyan, "player ({}) doesn't have enough currency to buy items", acc_cache->acc_info.Nickname.c_str());
                else if (validated.error() == DbUpdateError::InventoryFull)
                    DEBUGLOG(dark_cyan, "player ({}) has too many items in inventory, cannot add more", acc_cache->acc_info.Nickname.c_str());
                else
                    DEBUGLOG(red, "ValidateDatabaseUpdates failed for [{}] [{}]: {}", acc_cache->acc_info.Index, acc_cache->acc_info.Nickname.c_str(), static_cast<int>(validated.error()));
                return;
            }
            acc_cache.unlock();

            [[maybe_unused]] auto ignored = BaseLib::DbPool->submit_task([main_server,
                session = std::move(callback.session),
                s_id = session_id,
                acc_index = acc_index,
                p_mission = mission,
                p_order = order,
                p_extra = extra,
                r_items = std::move(rebuy_items),
                s_items = std::move(shop_items),
                bought_item_ids = std::move(bought_item_ids),
                rt_spent = rt_spent,
                mp_spent = mp_spent,
                rt_before = rt_before,
                mp_before = mp_before,
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

                for (size_t i = 0; i < bought_item_ids.size(); ++i)
                {
                    auto item_id = bought_item_ids[i];
                    
                    ItemLogEntry item_log;
                    item_log.aid = acc_index;
                    item_log.action_type = ItemLog::ActionType::Added;
                    item_log.item_id = item_id;
                    item_log.origin_type = ItemLog::OriginType::Shop;
                    
                    if (!p_mission && i < v.items_added.size())
                    {
                        item_log.serial_info = v.items_added[i].item_info.serial_info.data;
                        item_log.item_type = static_cast<ItemLog::ItemType>(v.items_added[i].item_type);
                    }
                    
                    log_ctx.item_logs.push_back(item_log);
                }

                if (rt_spent > 0)
                {
                    CurrencyLogEntry rt_log;
                    rt_log.aid = acc_index;
                    rt_log.currency_type = CurrencyLog::Type::RT;
                    rt_log.amount = -static_cast<int32_t>(rt_spent);
                    rt_log.before_value = rt_before;
                    rt_log.after_value = new_acc_cache->acc_info.RockTokens;
                    rt_log.source_type = CurrencyLog::SourceType::Shop;
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
                    mp_log.source_type = CurrencyLog::SourceType::Shop;
                    log_ctx.currency_logs.push_back(mp_log);
                }

                if (!log_ctx.empty())
                {
                    auto log_result = BaseLib::Database->PersistLogs(log_ctx);
                    if (!log_result.has_value())
                    {
                        DEBUGLOG(red, "Failed to persist shop logs for player [{}]: {}",
                            new_acc_cache->acc_info.Nickname.c_str(),
                            log_result.error().message);
                    }
                }

                if (p_mission)
                    session->SendMsg(p_order, p_mission, p_extra, static_cast<uint8_t>(r_items.size()), reinterpret_cast<uint8_t*>(r_items.data()), r_items.size() * sizeof(ShopSerialInfo));
                else
                    session->SendMsg(p_order, 0, 0, static_cast<uint8_t>(s_items.size()), reinterpret_cast<uint8_t*>(s_items.data()), s_items.size() * sizeof(ShopItem));
                
                MainCurrencyUpdateAck cur
                {
                    new_acc_cache->acc_info.RockTokens,
                    new_acc_cache->acc_info.MicroPoints,
                    new_acc_cache->acc_info.Coins
                };
                session->SendMsg(307, 0, 1, 0, reinterpret_cast<uint8_t*>(&cur), sizeof(cur));
                DEBUGLOG(dark_cyan, "player ({}) spent {} mp and {} rt", new_acc_cache->acc_info.Nickname.c_str(), mp_spent, rt_spent);
            });
        }
    }
}