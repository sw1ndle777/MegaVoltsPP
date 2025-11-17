#pragma once
namespace Game::Handlers
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Main;
    inline void GachaponSpin(SCallbackData& callback, CMainServer* main_server)
    {
        auto session = callback.session;
        auto message = callback.message;
        if (!session || !message) return;

        //std::shared_lock lock(session->GetMutex());
        CServer* server = callback.server;
        auto session_id = session->GetSessionId();
        auto acc_cache = CAccount.get<unique_t>(session_id);
        auto acc_index = acc_cache->acc_info.Index;
        auto req = reinterpret_cast<MainGachaponSpinReq*>(message->GetData());
        if (acc_index == -1) return;

        auto spin_count = static_cast<uint32_t>(message->GetOption());
        auto gachapon_info = CGachaponsInfo.get<shared_t>(req->gachapon_id);
        if (!spin_count || gachapon_info->Id == -1 || acc_cache->acc_info.Level < gachapon_info->LimitedGrade)
        {
            session->SendMsg(92, 0, Items::Gachapon::Spin::Result::Stuck, 0);
            return;
        }
        auto is_sale = CGachaponSaleInfo.contains(gachapon_info->Id);
        auto sale = CGachaponSaleInfo.get<shared_t>(gachapon_info->Id);
        uint32_t gachapon_price = 0;
        if (is_sale)
        {
            const auto now = Utility::GetUtcTimeNow();
            if (sale->start_date <= now && sale->end_date >= now) gachapon_price = sale->sale_price;
            else { sale.unlock();  CGachaponSaleInfo.erase(gachapon_info->Id); gachapon_price = gachapon_info->Price; }
        }
        else gachapon_price = gachapon_info->Price;
        if (gachapon_price == 0 && gachapon_info->Type != 3)
        {
            DEBUGLOG(dark_cyan, "cannot spin gachapon with price 0");
            session->SendMsg(92, 0, Items::Gachapon::Spin::Result::Stuck, 0);
            return;
        }
        const uint32_t coupon_chance =
            (gachapon_info->Type == Items::Gachapon::Type::RT) ? 30
            : (gachapon_info->Type == Items::Gachapon::Type::MP) ? 15 : 0;

        gachapon_info.unlock(); // GetGachaponInfo uses gachapon_info
        std::vector<GachaponPackageItem> lucky_items; // lucky items pre-extracted
        {
            auto lucky_gachapon_info = CGachaponsInfo.get<shared_t>(21);
            main_server->ExtractGachaponItemsWon(lucky_gachapon_info, lucky_items, coupon_chance);
            lucky_gachapon_info.unlock();
        }
        gachapon_info.lock();
        std::vector<std::vector<GachaponPackageItem>> rolls;
        rolls.reserve(spin_count);
        uint32_t fake_lucky_points = acc_cache->acc_info.LuckyPoints, money_spent = 0, coupons_delta = 0;
        auto grant_lucky = [&]
            {
                fake_lucky_points = 0;
                rolls.push_back(lucky_items);
            };
        using enum NetEngine::Items::Package::CouponItemId;
        using enum NetEngine::Items::Package::ItemIds;
        auto do_spin = [&]
            {
                std::vector<GachaponPackageItem> items;
                if (main_server->ExtractGachaponItemsWon(gachapon_info, items, coupon_chance))
                    items.push_back(GachaponPackageItem((acc_cache->acc_info.Coupons + 1 > 250) ? to_u(COUPON_1_PACKAGE) : to_u(COUPON_1)));

                fake_lucky_points += gachapon_info->LuckyPoint;
                money_spent += gachapon_price;
                rolls.push_back(std::move(items));
            };
        for (uint32_t i = 0; i < spin_count; i++)
        {
            if (fake_lucky_points >= 1000 && spin_count == 1)
                grant_lucky();
            else if (fake_lucky_points < 1000 && spin_count == 1)
                do_spin();
            else
            {
                do_spin();
                if (fake_lucky_points >= 1000 && i < spin_count - 1)
                    grant_lucky();
            }
        }
        const size_t items_count = std::accumulate(rolls.begin(), rolls.end(), size_t{ 0 }, [](size_t acc, const auto& v) { return acc + v.size(); });
        if (acc_cache->inventory_items.size() + items_count > acc_cache->acc_info.MaximumItems)
        {
            session->SendMsg(92, 0, Items::Gachapon::Spin::Result::InventoryFull, 0);
            return;
        }
        auto serials = main_server->FindLowestAvailableSerialIds(acc_cache->inventory_items, static_cast<uint32_t>(items_count));
        if (serials.size() < items_count)
        {
			DEBUGLOG(red, "Not enough serial IDs for spin. Needed {}, got {}", items_count, serials.size());
            session->SendMsg(92, 0, Items::Gachapon::Spin::Result::Stuck, 0);
            return;
        }
        using Roll = std::pair<uint32_t, std::vector<ShopItem>>;
        std::vector<Roll> packets;
        packets.reserve(rolls.size());
        std::vector<Item> items_to_add;
        items_to_add.reserve(items_count);
        std::vector<std::string> rare_names_for_announce;
        using enum CurrencyType;

        DatabaseUpdateCtx dctx{ .sid = session_id, .aid = acc_index };
        size_t serial_idx = 0;
        for (const auto& roll : rolls)
        {
            std::vector<ShopItem> this_roll_packet;
            this_roll_packet.reserve(roll.size());
            const auto lucky_type = (!roll.empty() ? roll[0].LuckyType : 0);
            for (const auto& won : roll)
            {
                const bool is_rare = (won.ItemType == Items::Gachapon::Rarity::Rare);
                auto item_info = CItemsInfo.get<shared_t>(won.ItemId);
                DEBUGLOG(dark_cyan, "player ({}) won from capsule {} item: ({})", acc_cache->acc_info.Nickname.c_str(), is_rare ? "rare" : "normal", item_info->Id);
                if (won.LuckyType == Items::Gachapon::LuckyType::CopperLucky)
                {
                    dctx.ops.emplace_back(AccountCurrencyDelta{ .type = MP, .value = 1000, .is_reward = true });
                    this_roll_packet.push_back({ {won.ItemId, item_info->Stock}, ItemExpire::Type::Unused, ItemSerialInfo(0,0,0,0,0) });
                    continue;
                }
                if (won.ItemId == to_u(COUPON_1))
                {
                    coupons_delta++;
                    this_roll_packet.push_back({ {won.ItemId, item_info->Stock}, ItemExpire::Type::Unused, ItemSerialInfo(0,0,0,0,0) });
                    continue;
                }
                const auto sid = serials[serial_idx++];
                const ItemSerialInfo serial_info(sid, 1, 1, Items::Origin::From_Game, Utility::GetUtcTimeNow());
                ShopItem sitem{ {won.ItemId, item_info->Stock}, ItemExpire::Type::Unused, serial_info };
                this_roll_packet.push_back(sitem);
#if defined(RELEASE_1_0_3)
                const InventoryItemInfo inv_info = { {won.ItemId, item_info->Stock}, ItemExpire::Type::Unused, serial_info, item_info->Durability, 0 };
#else
                const InventoryItemInfo inv_info = { won.ItemId, ItemExpire::Type::Unused, serial_info, item_info->Durability, 0,0,0,0,0, main_server->AdjustItemType(item_info->Type) };
#endif
                items_to_add.push_back({ inv_info, item_info->Stock, false, 0, false });
                if (is_rare) rare_names_for_announce.push_back(item_info->Name);
            }
            packets.emplace_back(lucky_type, std::move(this_roll_packet));
        }
        switch (gachapon_info->Type)
        {
        case Items::Gachapon::Type::RT:
            if (money_spent > 0) dctx.ops.emplace_back(AccountCurrencyDelta{ .type = RT, .value = money_spent, .is_reward = false });
            break;
        case Items::Gachapon::Type::MP:
            if (money_spent > 0) dctx.ops.emplace_back(AccountCurrencyDelta{ .type = MP, .value = money_spent, .is_reward = false });
            break;
        default: break;
        }
        if (!items_to_add.empty())
            dctx.ops.push_back(ItemAddCtx{ .items = std::move(items_to_add) });

        if (coupons_delta > 0)
            dctx.ops.emplace_back(AccountCurrencyDelta{ .type = COUPONS, .value = coupons_delta, .is_reward = true });

        const bool any_lucky = std::any_of(rolls.begin(), rolls.end(), [](const auto& r) { return !r.empty() && r[0].LuckyType > Items::Gachapon::LuckyType::NoLucky; });
        dctx.ops.emplace_back(AccountInfoPatch{ .lucky_points = fake_lucky_points });
        auto validated = main_server->ValidateDatabaseUpdates(acc_cache, dctx);
        if (!validated.has_value())
        {
            using enum DbUpdateError;
            const auto err = validated.error();
            if (err == InsufficientRT ||
                err == InsufficientMP)
            {
                const auto code = (gachapon_info->Type == Items::Gachapon::Type::Coin) ? Items::Gachapon::Error::NoCoin
                    : (gachapon_info->Type == Items::Gachapon::Type::RT) ? Items::Gachapon::Error::NoRT
                    : Items::Gachapon::Error::NoMP;
                session->SendMsg(92, 0, Items::Gachapon::Spin::Result::MoneyError, code);
            }
            else if (err == InventoryFull)
                session->SendMsg(92, 0, Items::Gachapon::Spin::Result::InventoryFull, 0);
            else
            {
				DEBUGLOG(red, "ValidateDatabaseUpdates failed for [{}] [{}]: {}", acc_cache->acc_info.Index, acc_cache->acc_info.Nickname.c_str(), static_cast<int>(err));
                session->SendMsg(92, 0, Items::Gachapon::Spin::Result::Stuck, 0);
            }
            return;
        }
        acc_cache.unlock();
        [[maybe_unused]] auto ignored = BaseLib::DbPool->submit_task([main_server,
            session = std::move(callback.session),
            s_id = session_id,
            rolls = std::move(packets),
            lucky_items_announce = std::move(rare_names_for_announce),
            money_spent = money_spent,
            is_sale = is_sale,
            gacha_type = gachapon_info->Type,
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
                for (auto& roll : rolls)
                {
                    auto lucky_type = roll.first;
                    using ST = Items::Gachapon::Spin::Type;
                    auto spin_type = (lucky_type > Items::Gachapon::LuckyType::NoLucky) ? ST::LuckySpin : is_sale ? ST::NormalSpinSale : ST::NormalSpin;
                    auto& items = roll.second;
                    session->SendMsg(92, spin_type, Items::Gachapon::Spin::Result::SpinSuccess, static_cast<uint8_t>(items.size()), reinterpret_cast<uint8_t*>(items.data()), items.size() * sizeof(ShopItem));
                }
                const auto is_rt = gacha_type == Items::Gachapon::Type::RT;
                DEBUGLOG(dark_cyan, "player ({}) spent {} {}", new_acc_cache->acc_info.Nickname.c_str(), money_spent, is_rt ? "rt" : "mp");
                auto player_name = new_acc_cache->acc_info.Nickname.c_str();
                auto sids = CSid.get_all(shared);
                for (const auto& sid : *sids)
                {
                    if (auto s = main_server->GetSessionById(sid))
                        for (auto& announcement : lucky_items_announce)
                            main_server->SendServerMessage(s.get(), fmt::format("[{}] won a [{}] item from the capsule machine.", player_name, announcement.c_str()).c_str());
                }
            });
    }
}