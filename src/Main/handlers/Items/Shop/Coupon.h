#pragma once
#include <BaseLib/CLogging.h>

namespace Game::Handlers
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Main;
    
    inline void ShopCoupon(SCallbackData& callback, CMainServer* main_server)
    {
        auto session = callback.session;
        auto message = callback.message;
        if (!session || !message) return;

        const uint32_t items_count = static_cast<uint32_t>(message->GetOption());
        auto session_id = session->GetSessionId();
        auto acc_cache = CAccount.get<unique_t>(session_id);
        auto acc_index = acc_cache->acc_info.Index;
        const auto& buyItemIdReq = reinterpret_cast<MainBuyItemIdReq*>(message->GetData());
        auto order = message->GetOrder();
        if (acc_index == -1) return;
        
        DEBUGLOG(dark_cyan, "player ({}) coupon shop -> mission: ({}), extra: ({}), option: ({})", acc_cache->acc_info.Nickname.c_str(), callback.message->GetMission(), callback.message->GetExtra(), callback.message->GetOption());
        
        auto coupon_before = acc_cache->acc_info.Coupons;
        
        std::vector<ShopItem> shop_items;
        std::vector<Item> new_items;
        std::vector<uint32_t> bought_item_ids;
        uint32_t coupon_spent = 0;
        
        DatabaseUpdateCtx dctx{ .sid = session_id, .aid = acc_index };
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
                coupon_spent += item_info->CouponPrice;
                bought_item_ids.push_back(item_bought_id);
                DEBUGLOG(dark_cyan, "player ({}) bought item id: ({}), serial: ({})", acc_cache->acc_info.Nickname.c_str(), item_info->Id, new_item.serial_info.data);
            }
            else
                DEBUGLOG(dark_cyan, "player ({}) request buy id: ({}) failed because it doesn't exist in vendor info", acc_cache->acc_info.Nickname.c_str(), item_bought_id);
        }

        if (shop_items.empty() || new_items.empty()) return;

        using enum CurrencyType;
        if (coupon_spent > 0) dctx.ops.emplace_back(AccountCurrencyDelta{ .type = COUPONS, .value = coupon_spent, .is_reward = false });

        dctx.ops.push_back(ItemAddCtx{ .items = std::move(new_items) });

        auto validated = main_server->ValidateDatabaseUpdates(acc_cache, dctx);
        if (!validated.has_value())
        {
            if (validated.error() == DbUpdateError::InsufficientCOUPONS)
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
            bought_item_ids = std::move(bought_item_ids),
            coupon_spent = coupon_spent,
            coupon_before = coupon_before,
            p_order = order,
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

            LogContext log_ctx;

            for (size_t i = 0; i < bought_item_ids.size() && i < v.items_added.size(); ++i)
            {
                ItemLogEntry item_log;
                item_log.aid = acc_index;
                item_log.action_type = ItemLog::ActionType::Added;
                item_log.item_id = bought_item_ids[i];
                item_log.serial_info = v.items_added[i].item_info.serial_info.data;
                item_log.origin_type = ItemLog::OriginType::ShopCoupon;
                item_log.coupon_delta = -static_cast<int32_t>(coupon_spent / bought_item_ids.size()); // Divide evenly
                item_log.item_type = static_cast<ItemLog::ItemType>(v.items_added[i].item_type);
                
                log_ctx.item_logs.push_back(item_log);
            }

            if (coupon_spent > 0)
            {
                CurrencyLogEntry coupon_log;
                coupon_log.aid = acc_index;
                coupon_log.currency_type = CurrencyLog::Type::Coupons;
                coupon_log.amount = -static_cast<int32_t>(coupon_spent);
                coupon_log.before_value = coupon_before;
                coupon_log.after_value = new_acc_cache->acc_info.Coupons;
                coupon_log.source_type = CurrencyLog::SourceType::ShopCoupon;
                log_ctx.currency_logs.push_back(coupon_log);
            }

            if (!log_ctx.empty())
            {
                auto log_result = BaseLib::Database->PersistLogs(log_ctx);
                if (!log_result.has_value())
                {
                    DEBUGLOG(red, "Failed to persist coupon shop logs for player [{}]: {}",
                        new_acc_cache->acc_info.Nickname.c_str(),
                        log_result.error().message);
                }
            }

            session->SendMsg(p_order, 0, 1, static_cast<uint8_t>(s_items.size()), reinterpret_cast<uint8_t*>(s_items.data()), s_items.size() * sizeof(ShopItem));
            DEBUGLOG(dark_cyan, "player ({}) spent {} coupon(s)", new_acc_cache->acc_info.Nickname.c_str(), coupon_spent);
        });
    }
}