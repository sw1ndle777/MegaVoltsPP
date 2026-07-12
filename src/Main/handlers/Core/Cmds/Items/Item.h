#pragma once
namespace Game::Commands
{
    struct ItemCommand
    {
        static constexpr std::string_view name = "item";
        static constexpr uint8_t required_grade = Userlist::User::Grade::Tester;

        static void Run(std::span<const std::string_view> args, CommandContext& ctx)
        {
            if (args.size() <= 1)
            {
                ctx.server->SendServerMessage(ctx.callback.session, "[MegaVolts Online] command usage: /item item_id item_id2 (max 25 item ids)");
                return;
            }
            const auto item_args = args.subspan(1);

            if (ctx.acc_cache->inventory_items.size() + item_args.size() > ctx.acc_cache->acc_info.MaximumItems)
            {
                ctx.server->SendServerMessage(ctx.callback.session, fmt::format("[MegaVolts Online] you can't spawn {} items because your inventory will be over it's capacity.", args.size() - 1).c_str());
                return;
            }
            DatabaseUpdateCtx dctx{ .sid = ctx.callback.session->GetSessionId(), .aid = ctx.acc_cache->acc_info.Index };

            std::vector<uint32_t> spawn_ids;
            for (const auto& item_id_str : item_args)
            {
                uint32_t item_id = 0;

                if (auto id = Utility::ParseNumber<uint32_t>(item_id_str))
                    item_id = *id;
                else
                {
                    ctx.server->SendServerMessage(ctx.callback.session, fmt::format("[MegaVolts Online] invalid item id, error: ({})", id.error()));
                    return;
                }
                if (!CItemsInfo.contains(item_id))
                {
                    ctx.server->SendServerMessage(ctx.callback.session, fmt::format("[MegaVolts Online] item id {} does not exist", item_id));
                    return;
                }
                spawn_ids.push_back(item_id);

            }
            auto crafted_item = ctx.server->CraftInventoryItems(ctx.acc_cache, std::move(spawn_ids), NetEngine::Items::Origin::From_GM_Spawn);
            if (!crafted_item.has_value())
            {
                DEBUGLOG(red, "CraftInventoryItems failed for player [{}] [{}]: {}", ctx.acc_cache->acc_info.Index, ctx.acc_cache->acc_info.Nickname.c_str(), static_cast<int>(crafted_item.error()));
                ctx.server->SendServerMessage(ctx.callback.session, "[MegaVolts Online] failed to spawn item(s) (inventory full or invalid item)");
                return;
            }
            dctx.ops.push_back(crafted_item.value());

            auto validated = ctx.server->ValidateDatabaseUpdates(ctx.acc_cache, dctx);
            if (!validated.has_value())
            {
                DEBUGLOG(red, "ValidateDatabaseUpdates failed for [{}] [{}]: {}", ctx.acc_cache->acc_info.Index, ctx.acc_cache->acc_info.Nickname.c_str(), static_cast<int>(validated.error()));
                return;
            }

            // Apply to the cache while we still hold the caller's account lock.
            //
            // The old flow unlocked here and re-acquired the account lock on a DbPool
            // thread (CAccount.get<unique_t>) to apply afterwards. That was the only
            // place a packet-strand thread and a DbPool thread touched the same account
            // lock across threads, and it left a window between unlock and apply where a
            // second rapid /item read stale inventory and re-used the same serial ids
            // (duplicate-key INSERT -> silent spawn failure). Applying under the lock we
            // already hold removes both: no cross-thread re-lock, no serial race.
            auto applied = ctx.server->ApplyDatabaseUpdates(ctx.acc_cache, validated.value());
            if (!applied.has_value())
            {
                DEBUGLOG(red, "ApplyDatabaseUpdates failed for [{}] [{}]: {}", ctx.acc_cache->acc_info.Index, ctx.acc_cache->acc_info.Nickname.c_str(), static_cast<int>(applied.error()));
                return;
            }

            auto v = std::move(validated.value());
            auto session = ctx.callback.session;
            const auto aid = ctx.acc_cache->acc_info.Index;

            // Build the client inventory-update payload before unlocking (item stock comes
            // from CItemsInfo, a separate read-only cache).
            std::vector<ShopItem> shop_items;
            shop_items.reserve(v.items_added.size());
            for (const auto& item : v.items_added)
            {
                auto item_info = CItemsInfo.get<shared_t>(item.item_info.item_number.item_id);
                shop_items.push_back(ShopItem{ {item.item_info.item_number.item_id, item_info->Stock}, ItemExpire::Type::Unused, item.item_info.serial_info });
            }

            ctx.acc_cache.unlock();

            // Notify the client (async, strand-queued send) so the items appear without a relog.
            if (session && !shop_items.empty())
            {
                constexpr std::uint32_t max_packet_size = 1440;
                constexpr std::uint32_t full_header_size = 8;
                constexpr std::uint32_t split_size = (max_packet_size - full_header_size) / sizeof(ShopItem);
                const uint32_t total_fragments = (shop_items.size() + 1) <= split_size ? 1 : ((shop_items.size() + 1) / split_size) + 1;
                for (uint32_t f = 0; f < total_fragments; f++)
                {
                    const uint32_t start_index = f * split_size;
                    const uint32_t end_index = std::min(start_index + split_size, static_cast<uint32_t>(shop_items.size()));
                    if (start_index >= end_index) continue;
                    std::vector<ShopItem> items_batch(shop_items.begin() + start_index, shop_items.begin() + end_index);
                    session->SendMsg(99, 0, 37, static_cast<uint8_t>(items_batch.size()), reinterpret_cast<uint8_t*>(items_batch.data()), items_batch.size() * sizeof(ShopItem));
                }
                ctx.server->SendServerMessage(session, fmt::format("[MegaVolts Online] spawned {} item(s)", shop_items.size()).c_str());
            }

            // Persist to the DB off-thread. The cache is already authoritative; the task
            // takes no account lock, so it can't contend with the packet handlers.
            [[maybe_unused]] auto ignored = BaseLib::DbPool->submit_task([v = std::move(v), aid]() mutable
                {
                    ResultDbUpdateInfo dbres;
                    if (!BaseLib::Database->UpdateAccount(v, dbres).has_value())
                        DEBUGLOG(red, "item persist failed for aid [{}]", aid);
                });
        };

        inline static CommandRegister<ItemCommand> reg{};
    };
}
