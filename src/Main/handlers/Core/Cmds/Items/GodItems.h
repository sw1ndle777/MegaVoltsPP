#pragma once
namespace Game::Commands
{
    struct GodsItemsCommand
    {
        static constexpr std::string_view name = "gods";
        static constexpr uint8_t required_grade = Userlist::User::Grade::Tester;
        static constexpr std::array<uint32_t, 7> kGodWeapons = { 4626750,4626850,4626950,4627050,4627150,4627250,4627350 };
        static void Run(std::span<const std::string_view> args, CommandContext& ctx)
        {
            DatabaseUpdateCtx dctx{ .sid = ctx.callback.session->GetSessionId(), .aid = ctx.acc_cache->acc_info.Index };
            std::vector<uint32_t> spawn_ids(kGodWeapons.begin(), kGodWeapons.end());

            auto crafted_item = ctx.server->CraftInventoryItems(ctx.acc_cache, std::move(spawn_ids), NetEngine::Items::Origin::From_GM_Spawn);
            if (!crafted_item.has_value())
                DEBUGLOG(red, "CraftInventoryItems failed for player [{}] [{}]: {}", ctx.acc_cache->acc_info.Index, ctx.acc_cache->acc_info.Nickname.c_str(), static_cast<int>(crafted_item.error()));
            else
                dctx.ops.push_back(crafted_item.value());

            auto validated = ctx.server->ValidateDatabaseUpdates(ctx.acc_cache, dctx, true); // bypass inv limit check
            if (!validated.has_value())
            {
                DEBUGLOG(red, "ValidateDatabaseUpdates failed for [{}] [{}]: {}", ctx.acc_cache->acc_info.Index, ctx.acc_cache->acc_info.Nickname.c_str(), static_cast<int>(validated.error()));
                return;
            }

            ctx.acc_cache.unlock();

            [[maybe_unused]] auto ignored = BaseLib::DbPool->submit_task([ctx, session = std::move(ctx.callback.session),
                v = std::move(validated.value())
            ]() mutable
                {
                    if (!session) return;
                    ResultDbUpdateInfo dbres;
                    if (!BaseLib::Database->UpdateAccount(v, dbres).has_value()) return;
                    auto new_acc_cache = CAccount.get<unique_t>(session->GetSessionId());
                    auto applied = ctx.server->ApplyDatabaseUpdates(new_acc_cache, v);
                    if (!applied.has_value())
                    {
                        DEBUGLOG(red, "ApplyDatabaseUpdates failed for [{}] [{}]: {}", new_acc_cache->acc_info.Index, new_acc_cache->acc_info.Nickname.c_str(), static_cast<int>(applied.error()));
                        return;
                    }

                    if (!v.items_added.empty())
                    {
                        std::vector<ShopItem> shop_items;
                        for (const auto& item : v.items_added)
                        {
                            auto item_info = CItemsInfo.get<shared_t>(item.item_info.item_number.item_id);
                            ShopItem new_item = { {item.item_info.item_number.item_id , item_info->Stock} , ItemExpire::Type::Unused,  item.item_info.serial_info };
                            ctx.server->SendServerMessage(session, std::format("[MegaVolts Online] spawned ({}) item", item.item_info.item_number.item_id).c_str());
                            shop_items.push_back(new_item);
                        }

                        constexpr std::uint32_t max_packet_size = 1440;
                        constexpr std::uint32_t full_header_size = 8;
                        constexpr std::uint32_t split_size = (max_packet_size - full_header_size) / sizeof(ShopItem);
                        uint32_t total_fragments = (shop_items.size() + 1) <= split_size ? 1 : ((shop_items.size() + 1) / split_size) + 1;
                        if (!shop_items.empty())
                        {
                            for (auto i = 0; i < total_fragments; i++)
                            {
                                std::vector<ShopItem> items_batch;
                                const uint32_t start_index = i * split_size;
                                const uint32_t end_index = std::min(start_index + split_size, static_cast<uint32_t>(shop_items.size()));
                                for (auto i = start_index; i < end_index; i++)
                                    items_batch.push_back(shop_items[i]);

                                if (!items_batch.empty())
                                    session->SendMsg(99, 0, 37, static_cast<uint8_t>(items_batch.size()), reinterpret_cast<uint8_t*>(items_batch.data()), items_batch.size() * sizeof(ShopItem));
                            }
                        }
                    }
                });
        };

        inline static CommandRegister<GodsItemsCommand> reg{};
    };
}
