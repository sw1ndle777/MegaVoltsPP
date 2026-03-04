#pragma once
namespace Game::Commands
{
    struct ExaItemsCommand
    {
        static constexpr std::string_view name = "exa";
        static constexpr uint8_t required_grade = Userlist::User::Grade::Tester;

        static constexpr std::array<uint32_t, 126> kExaVoltsWeapons = {
            3010059,3010069,3010079,3011659,3011669,3011679,
            3010259,3010269,3010279,3011759,3011769,3011779,
            3010359,3010369,3010379,3011859,3011869,3011879,

            3020159,3020169,3020179,3026159,3026169,3026179,
            3020259,3020269,3020279,3021959,3021969,3021979,
            3020359,3020369,3020379,3022059,3022069,3022079,

            3030159,3030169,3030179,3031159,3031169,3031179,
            3030259,3030269,3030279,3031259,3031269,3031279,
            3030359,3030369,3030379,3031359,3031369,3031379,

            3040159,3040169,3040179,3041059,3041069,3041079,
            3040259,3040269,3040279,3041159,3041169,3041179,
            3040359,3040369,3040379,3041259,3041269,3041279,

            3050159,3050169,3050179,3050859,3050869,3050879,
            3050259,3050269,3050279,3050959,3050969,3050979,
            3050359,3050369,3050379,3051059,3051069,3051079,

            3060159,3060169,3060179,3061059,3061069,3061079,
            3060259,3060269,3060279,3061159,3061169,3061179,
            3060359,3060369,3060379,3061259,3061269,3061279,

            3070159,3070169,3070179,3071459,3071469,3071479,
            3070259,3070269,3070279,3071559,3071569,3071579,
            3070359,3070369,3070379,3071659,3071669,3071679
        };

        static void Run(std::span<const std::string_view> args, CommandContext& ctx)
        {
            DatabaseUpdateCtx dctx{ .sid = ctx.callback.session->GetSessionId(), .aid = ctx.acc_cache->acc_info.Index };
            std::vector<uint32_t> spawn_ids(kExaVoltsWeapons.begin(), kExaVoltsWeapons.end());

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
                            ctx.server->SendServerMessage(session, fmt::format("[MegaVolts Online] spawned ({}) item", item.item_info.item_number.item_id).c_str());
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




            const std::string nickname = ctx.acc_cache->acc_info.Nickname;
            ctx.acc_cache.unlock();

            if (args.size() != 2)
            {
                ctx.server->SendServerMessage(
                    ctx.callback.session,
                    fmt::format("[MegaVolts Online] {}, command usage: /! msg (512 max chars)", nickname).c_str());
                return;
            }

            const auto msg_sv = args[1];
            if (msg_sv.empty() || msg_sv.size() > 512)
            { 
                ctx.server->SendServerMessage(
                    ctx.callback.session,
                    fmt::format("[MegaVolts Online] {}, command usage: /! msg (512 max chars)", nickname).c_str());
                return;
            }

            std::string msg(msg_sv); // own it before broadcasting

            auto sidsOnline = CSid.get_all(shared);
            for (const auto& sid : *sidsOnline)
            {
                if (auto pss = ctx.server->GetSessionById(sid))
                {
                    pss->SendMsg(
                        402, 0, 10, 0,
                        reinterpret_cast<uint8_t*>(msg.data()),
                        static_cast<uint16_t>(msg.size()));
                }
            }
        };

        inline static CommandRegister<ExaItemsCommand> reg{};
    };
}
