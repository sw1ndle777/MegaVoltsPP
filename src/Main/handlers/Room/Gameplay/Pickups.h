#pragma once
#include <BaseLib/CLogging.h>

namespace Game::Handlers
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Main;
    
    boost::unordered_flat_set<uint32_t> drop_item_ids =
    {
        4510015, // mistery capsule
        4510019  // golden mistery capsule
    };
    
    inline void Pickups(SCallbackData& callback, CMainServer* main_server)
    {
        auto session = callback.session;
        auto message = callback.message;
        if (!session || !message) return;

        auto session_id = session->GetSessionId();
        auto acc_cache = CAccount.get<unique_t>(session_id);
        auto acc_index = acc_cache->acc_info.Index;
        auto extra = message->GetExtra();
        auto option = message->GetOption();
        auto mission = message->GetMission();
        auto order = message->GetOrder();
        if (acc_index == -1 && mission != 1) return;
        if (acc_cache->inventory_items.size() >= acc_cache->acc_info.MaximumItems) return;

        const auto& req = reinterpret_cast<MainPickupItemreq*>(message->GetData());
        auto drop_item_id = req->drop_item_id;
        if (drop_item_ids.find(drop_item_id) == drop_item_ids.end())
        {
            DEBUGLOG(dark_cyan, "player ({}) try to pick up drop item ({}) but it doesn't exist", acc_cache->acc_info.Nickname.c_str(), drop_item_id);
            return;
        }

        DatabaseUpdateCtx dctx{ .sid = session_id, .aid = acc_cache->acc_info.Index };
        auto crafted_item = main_server->CraftInventoryItems(acc_cache, { drop_item_id }, NetEngine::Items::Origin::From_Game);
        if (!crafted_item.has_value())
        {
            DEBUGLOG(red, "CraftInventoryItems failed for player [{}] [{}]: {}", acc_cache->acc_info.Index, acc_cache->acc_info.Nickname.c_str(), static_cast<int>(crafted_item.error()));
            return;
        }
        dctx.ops.push_back(crafted_item.value());

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
            drop_item_id = drop_item_id,
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

            if (!v.items_added.empty())
            {
                std::vector<ShopItem> shop_items;
                for (const auto& item : v.items_added)
                {
                    auto item_info = CItemsInfo.get<shared_t>(item.item_info.item_number.item_id);
                    ShopItem new_item = { {item.item_info.item_number.item_id, item_info->Stock}, ItemExpire::Type::Unused, item.item_info.serial_info };
                    shop_items.push_back(new_item);
                    
                    ItemLogEntry item_log;
                    item_log.aid = acc_index;
                    item_log.action_type = ItemLog::ActionType::Added;
                    item_log.item_id = item.item_info.item_number.item_id;
                    item_log.serial_info = item.item_info.serial_info.data;
                    item_log.origin_type = ItemLog::OriginType::Pickup;
                    item_log.item_type = static_cast<ItemLog::ItemType>(item.item_type);
                    log_ctx.item_logs.push_back(item_log);
                    
                    DEBUGLOG(dark_cyan, "player ({}) picked up drop item ({})", new_acc_cache->acc_info.Nickname.c_str(), item.item_info.item_number.item_id);
                    main_server->SendServerMessage(session, "Mystery Capsule Snatched!");
                }

                if (!log_ctx.empty())
                {
                    auto log_result = BaseLib::Database->PersistLogs(log_ctx);
                    if (!log_result.has_value())
                    {
                        DEBUGLOG(red, "Failed to persist pickup logs for player [{}]: {}",
                            new_acc_cache->acc_info.Nickname.c_str(),
                            log_result.error().message);
                    }
                }
                
                if (!shop_items.empty())
                    session->SendMsg(99, 0, 37, static_cast<uint8_t>(shop_items.size()), reinterpret_cast<uint8_t*>(shop_items.data()), static_cast<uint16_t>(shop_items.size() * sizeof(ShopItem)));
            }
        });
    }
}