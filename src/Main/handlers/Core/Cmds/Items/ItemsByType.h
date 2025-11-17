#pragma once
namespace Game::Commands
{
    struct AllItemsByTypeCommand
    {
        static constexpr std::string_view name = "allitems";
        static constexpr uint8_t required_grade = Userlist::User::Grade::Tester;

        static void Run(std::span<const std::string_view> args, CommandContext& ctx)
        {
            if (args.size() <= 1)
            {
                ctx.server->SendServerMessage(ctx.callback.session, "[MegaVolts Online] command usage: /allitems item_type");
                return;
            }

            DatabaseUpdateCtx dctx{ .sid = ctx.callback.session->GetSessionId(), .aid = ctx.acc_cache->acc_info.Index };
            uint32_t item_type = 0;

            if (auto id = Utility::ParseNumber<uint32_t>(args[1]))
                item_type = *id;
            else
            {
                ctx.server->SendServerMessage(ctx.callback.session, fmt::format("[MegaVolts Online] invalid item type, error: ({})", id.error()));
                return;
            }
            auto my_character = static_cast<Character::Type>(ctx.acc_cache->acc_info.SelectedCharacter);
            using enum Character::Type;
            auto ids = CItemsType.get<shared_t>(item_type);
            std::vector<uint32_t> spawn_ids;
            for (const auto& id : *ids)
            {
                auto item_info = CItemsInfo.get<shared_t>(id);
                if (!item_info->Id) continue;
                if (item_info->Type != item_type) continue;
                if (item_info->LimitedTime) continue;
                if (my_character == Naomi && !item_info->IsNaomiUsable) continue;
                if (my_character == Kai && !item_info->IsKaiUsable) continue;
                if (my_character == Pandora && !item_info->IsPandoraUsable) continue;
                if (my_character == CHIP && !item_info->IsChipUsable) continue;
                if (my_character == Knox && !item_info->IsKnoxUsable) continue;
                if (item_info->IsUpgradable)
                {
                    auto is_weapon = item_info->Type == Items::WeaponItems::Type::Melee ||
                        item_info->Type == Items::WeaponItems::Type::Rifle ||
                        item_info->Type == Items::WeaponItems::Type::Shotgun ||
                        item_info->Type == Items::WeaponItems::Type::Sniper ||
                        item_info->Type == Items::WeaponItems::Type::Gatling ||
                        item_info->Type == Items::WeaponItems::Type::Bazooka ||
                        item_info->Type == Items::WeaponItems::Type::Grenade;

                    if (is_weapon)
                    {
                        auto upgrade_level = id % 10;
                        if (upgrade_level != 9) continue;
                        spawn_ids.push_back(id);
                    }
                    else
                        spawn_ids.push_back(id);
                }
                else
                    spawn_ids.push_back(id);
            }

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
                            ctx.server->SendServerMessage(session, std::format("[MegaVolts Online] spawned ({}) item(s)", shop_items.size()));
                        }
                    }
                });
        };

        inline static CommandRegister<AllItemsByTypeCommand> reg{};
    };
}