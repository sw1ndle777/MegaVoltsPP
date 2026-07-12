#pragma once
namespace Game::Commands
{
    // /items <item_id> <count>  — spawn `count` copies of a single item.
    // (Sibling of /item which spawns one each of several ids: /item id1 id2 id3)
    struct ItemsCommand
    {
        static constexpr std::string_view name = "items";
        static constexpr uint8_t required_grade = Userlist::User::Grade::Tester;

        static void Run(std::span<const std::string_view> args, CommandContext& ctx)
        {
            if (args.size() != 3)
            {
                ctx.server->SendServerMessage(ctx.callback.session, "[MegaVolts Online] command usage: /items <item_id> <count>");
                return;
            }

            auto id = Utility::ParseNumber<uint32_t>(args[1]);
            if (!id.has_value())
            {
                ctx.server->SendServerMessage(ctx.callback.session, fmt::format("[MegaVolts Online] invalid item id, error: ({})", id.error()));
                return;
            }
            auto count = Utility::ParseNumber<uint32_t>(args[2]);
            if (!count.has_value() || *count == 0)
            {
                ctx.server->SendServerMessage(ctx.callback.session, "[MegaVolts Online] invalid count (must be >= 1)");
                return;
            }
            const uint32_t item_id = *id;
            const uint32_t spawn_count = *count;

            if (!CItemsInfo.contains(item_id))
            {
                ctx.server->SendServerMessage(ctx.callback.session, fmt::format("[MegaVolts Online] item id {} does not exist", item_id));
                return;
            }

            const auto current = ctx.acc_cache->inventory_items.size();
            const auto maximum = ctx.acc_cache->acc_info.MaximumItems;
            if (current + spawn_count > maximum)
            {
                ctx.server->SendServerMessage(ctx.callback.session,
                    fmt::format("[MegaVolts Online] you can't spawn {} items because your inventory will be over its capacity ({}/{}).", spawn_count, current, maximum));
                return;
            }

            DatabaseUpdateCtx dctx{ .sid = ctx.callback.session->GetSessionId(), .aid = ctx.acc_cache->acc_info.Index };

            std::vector<uint32_t> spawn_ids(spawn_count, item_id);

            auto crafted_item = ctx.server->CraftInventoryItems(ctx.acc_cache, std::move(spawn_ids), NetEngine::Items::Origin::From_GM_Spawn);
            if (!crafted_item.has_value())
            {
                DEBUGLOG(red, "CraftInventoryItems failed for player [{}] [{}]: {}", ctx.acc_cache->acc_info.Index, ctx.acc_cache->acc_info.Nickname.c_str(), static_cast<int>(crafted_item.error()));
                ctx.server->SendServerMessage(ctx.callback.session, "[MegaVolts Online] failed to spawn items (inventory full or invalid item)");
                return;
            }
            dctx.ops.push_back(crafted_item.value());

            auto validated = ctx.server->ValidateDatabaseUpdates(ctx.acc_cache, dctx);
            if (!validated.has_value())
            {
                DEBUGLOG(red, "ValidateDatabaseUpdates failed for [{}] [{}]: {}", ctx.acc_cache->acc_info.Index, ctx.acc_cache->acc_info.Nickname.c_str(), static_cast<int>(validated.error()));
                ctx.server->SendServerMessage(ctx.callback.session, "[MegaVolts Online] failed to spawn items");
                return;
            }

            // Apply to the cache under the lock we already hold (see Item.h for the full
            // rationale): removes the cross-thread account re-lock the DbPool task used to
            // do and the unlock-before-apply serial-id race.
            auto applied = ctx.server->ApplyDatabaseUpdates(ctx.acc_cache, validated.value());
            if (!applied.has_value())
            {
                DEBUGLOG(red, "ApplyDatabaseUpdates failed for [{}] [{}]: {}", ctx.acc_cache->acc_info.Index, ctx.acc_cache->acc_info.Nickname.c_str(), static_cast<int>(applied.error()));
                return;
            }

            auto v = std::move(validated.value());
            auto session = ctx.callback.session;
            const auto aid = ctx.acc_cache->acc_info.Index;

            // Build the client inventory-update payload before unlocking.
            std::vector<ShopItem> shop_items;
            shop_items.reserve(v.items_added.size());
            for (const auto& item : v.items_added)
            {
                auto item_info = CItemsInfo.get<shared_t>(item.item_info.item_number.item_id);
                shop_items.push_back(ShopItem{ {item.item_info.item_number.item_id, item_info->Stock}, ItemExpire::Type::Unused, item.item_info.serial_info });
            }

            ctx.acc_cache.unlock();

            // Push the new items to the client so they appear without a relog.
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
                ctx.server->SendServerMessage(session, fmt::format("[MegaVolts Online] spawned {} x item {}", shop_items.size(), item_id).c_str());
            }

            // Persist to the DB off-thread; the cache is already authoritative and the task
            // takes no account lock.
            [[maybe_unused]] auto ignored = BaseLib::DbPool->submit_task([v = std::move(v), aid]() mutable
                {
                    ResultDbUpdateInfo dbres;
                    if (!BaseLib::Database->UpdateAccount(v, dbres).has_value())
                        DEBUGLOG(red, "items persist failed for aid [{}]", aid);
                });
        }

        inline static CommandRegister<ItemsCommand> reg{};
    };
}
