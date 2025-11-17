#pragma once
namespace Game::Handlers
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Main;
    enum VoiceType : uint8_t
    {
        VoiceA = 0,
        VoiceB = 1,
        VoiceC = 2,
        VoiceD = 3,
        SpecialVoiceA = 4,
        SpecialVoiceB = 5,
        SpecialVoiceC = 6,
        SpecialVoiceD = 7
    };

    void setVoice(uint64_t& data, uint32_t character, uint8_t voice)
    {
        uint8_t bit_position = (12 * character) + (voice - 4);
        data |= (1ULL << bit_position);
    }
    void unsetVoice(uint64_t& data, uint32_t character, uint8_t voice)
    {
        uint8_t bit_position = (12 * character) + (voice - 4);
        data &= ~(1ULL << bit_position);
    }
    bool isVoiceUnlocked(uint64_t data, uint32_t character, uint8_t voice)
    {
        uint8_t bit_position = (12 * character) + (voice - 4);
        return (data & (1ULL << bit_position)) != 0;
    }
    boost::unordered_flat_map<uint32_t, uint32_t> coupon_map =
    {
        {4305019, 1}, {4305020, 5}, {4305021, 10}, {4305022, 15},
        {4305023, 20}, {4305024, 25}, {4305025, 0}, {4305026, 30},
        {4305027, 2}, {4305028, 3}, {4305029, 4}, {4305030, 6},
        {4305031, 7}, {4305032, 8}, {4305033, 9}, {4305034, 40},
        {4305035, 50}, {4305036, 100}
    };

    using enum NetEngine::Items::Package::CoinItemId;
    inline const boost::unordered_flat_map<NetEngine::Items::Package::CoinItemId, uint32_t> CoinRewards
    {
        { COIN_1, 1 },
        { COIN_2, 2 },
        { COIN_3, 3 },
        { COIN_4, 4 },
        { COIN_5, 5 },
        { COIN_6, 6 },
        { COIN_7, 7 },
        { COIN_8, 8 },
        { COIN_9, 9 },
        { COIN_10, 10 },
        { COIN_20, 20 },
        { COIN_30, 30 },
        { COIN_40, 40 },
        { COIN_50, 50 },
        { COIN_60, 60 },
        { COIN_70, 70 },
        { COIN_80, 80 },
        { COIN_90, 90 },
        { COIN_100, 100 },
    };
    inline bool IsCoinItem(uint32_t item_id)
    {
        return CoinRewards.contains(static_cast<NetEngine::Items::Package::CoinItemId>(item_id));
    }
    inline uint32_t GetCoinReward(uint32_t item_id)
    {
        auto it = CoinRewards.find(static_cast<NetEngine::Items::Package::CoinItemId>(item_id));
        if (it != CoinRewards.end())
            return it->second;
        return 0;
    }


    using enum NetEngine::Items::Package::CouponItemId;
    inline const boost::unordered_flat_map<NetEngine::Items::Package::CouponItemId, uint32_t> CouponRewards
    {
        { COUPON_1, 1 },
        { COUPON_2, 2 },
        { COUPON_3, 3 },
        { COUPON_4, 4 },
        { COUPON_5, 5 },
        { COUPON_6, 6 },
        { COUPON_7, 7 },
        { COUPON_8, 8 },
        { COUPON_9, 9 },
        { COUPON_10, 10 },
        { COUPON_15, 15 },
        { COUPON_20, 20 },
        { COUPON_25, 25 },
        { COUPON_30, 30 },
        { COUPON_1_1, 1 }, // old leftover in client
        { COUPON_40, 40 },
        { COUPON_50, 50 },
        { COUPON_100, 100 },
    };
    inline bool IsCouponItem(uint32_t item_id, bool& out)
    {
        out = CouponRewards.contains(static_cast<NetEngine::Items::Package::CouponItemId>(item_id));;
        return out;
    }
    inline uint32_t GetCouponReward(uint32_t item_id)
    {
        auto it = CouponRewards.find(static_cast<NetEngine::Items::Package::CouponItemId>(item_id));
        if (it != CouponRewards.end())
            return it->second;
        return 0;
    }

    using enum NetEngine::Items::Package::MicroPointsItemId;
    inline const boost::unordered_flat_map<NetEngine::Items::Package::MicroPointsItemId, uint32_t> MicroPointRewards
    {
        { POINTS_100, 100 },
        { POINTS_200, 200 },
        { POINTS_300, 300 },
        { POINTS_400, 400 },
        { POINTS_500, 500 },
        { POINTS_600, 600 },
        { POINTS_700, 700 },
        { POINTS_800, 800 },
        { POINTS_900, 900 },
        { POINTS_1000, 1000 },
        { POINTS_1100, 1100 },
        { POINTS_1200, 1200 },
        { POINTS_1300, 1300 },
        { POINTS_1400, 1400 },
        { POINTS_1500, 1500 },
        { POINTS_1600, 1600 },
        { POINTS_1700, 1700 },
        { POINTS_1800, 1800 },
        { POINTS_1900, 1900 },
        { POINTS_2000, 2000 },
        { POINTS_3000, 3000 },
        { POINTS_3500, 3500 },
        { POINTS_4000, 4000 },
        { POINTS_5000, 5000 },
        { POINTS_6000, 6000 },
        { POINTS_7000, 7000 },
        { POINTS_8000, 8000 },
        { POINTS_9000, 9000 },
        { POINTS_10000, 10000 },
        { POINTS_20000, 20000 },
        { POINTS_30000, 30000 },
        { POINTS_50000, 50000 },
        { POINTS_100000, 100000 },
        { POINTS_150000, 150000 },
        { POINTS_500000, 500000 },
        { POINTS_1000000, 1000000 },
    };
    inline bool IsMicroPointsItem(uint32_t item_id, bool& out)
    {
        out = MicroPointRewards.contains(static_cast<NetEngine::Items::Package::MicroPointsItemId>(item_id));
        return out;
    }
    inline uint32_t GetMicroPointsReward(uint32_t item_id)
    {
        auto it = MicroPointRewards.find(static_cast<NetEngine::Items::Package::MicroPointsItemId>(item_id));
        if (it != MicroPointRewards.end())
            return it->second;
        return 0;
    }

    using enum NetEngine::Items::Package::VoiceItemId;
    inline const boost::unordered_flat_set<NetEngine::Items::Package::VoiceItemId> VoiceItems
    {
        NAOMI_A, NAOMI_B, NAOMI_C, NAOMI_D,
        KNOX_A,  KNOX_B,  KNOX_C,  KNOX_D,
        PANDORA_A, PANDORA_B, PANDORA_C, PANDORA_D,
        CHIP_A, CHIP_B, CHIP_C, CHIP_D,
    };
    inline bool IsVoiceItem(uint32_t item_id, bool& out)
    {
        out = VoiceItems.contains(static_cast<NetEngine::Items::Package::VoiceItemId>(item_id));
        return out;
    }
    inline uint32_t GetVoiceIndex(uint32_t item_id)
    {
        return ((item_id - to_u(NAOMI_A)) % 4) + 4;
    }
    inline uint32_t GetVoiceCharacter(uint32_t item_id)
    {
        return (item_id - to_u(NAOMI_A)) / 4;
    }

    using enum NetEngine::Items::Package::ItemIds;
    inline const boost::unordered_flat_map<NetEngine::Items::Package::ItemIds, uint32_t> InvExpands
    {
        { INV_EXPAND_10, 10 },
        { INV_EXPAND_20, 20 },
        { INV_EXPAND_40, 40 },
        { INV_EXPAND_80, 80 },
    };
    inline bool IsInvExpandItem(uint32_t item_id, bool& out)
    {
        out = InvExpands.contains(static_cast<NetEngine::Items::Package::ItemIds>(item_id));
        return out;
    }
    inline uint32_t GetInvExpandSize(uint32_t item_id)
    {
        auto it = InvExpands.find(static_cast<NetEngine::Items::Package::ItemIds>(item_id));
        if (it != InvExpands.end())
            return it->second;
        return 0;
    }
    inline const boost::unordered_flat_map<NetEngine::Items::Package::ItemIds, uint32_t> BatteryRecharges
    {
        { BATTERY_RECHARGE_500, 500 },
        { BATTERY_RECHARGE_1000, 1000 }
    };
    inline bool IsBatteryRechargeItem(uint32_t item_id, bool& out)
    {
        out = BatteryRecharges.contains(static_cast<NetEngine::Items::Package::ItemIds>(item_id));
        return out;
    }
    inline uint32_t GetBatteryRechargeAmount(uint32_t item_id)
    {
        auto it = BatteryRecharges.find(static_cast<NetEngine::Items::Package::ItemIds>(item_id));
        if (it != BatteryRecharges.end())
            return it->second;
        return 0;
    }
    inline bool IsRecordResetItem(uint32_t item_id, bool& out)
    {
        out = item_id == to_u(WIN_LOSE_RESET);
        return out;
    }
    inline bool IsKillDeathResetItem(uint32_t item_id, bool& out)
    {
        out = item_id == to_u(KILL_DEATH_RESET);
        return out;
    }
    inline bool IsBatteryExpansionItem(uint32_t item_id, bool& out)
    {
        out = item_id == to_u(BATTERY_EXPAND);
        return out;
    }
    inline void ProcessItemsWon(CMainServer* main_server,
        const std::vector<BaseLib::PackageInfo>& items_won,
        const std::vector<uint32_t>& serials,
        std::vector<ShopItem>& items_to_send,
        std::vector<Item>& new_items)
    {
        size_t serial_idx = 0;
        for (const auto& item : items_won)
        {
            auto item_info = CItemsInfo.get<shared_t>(item.ItemId);
            const auto sid = serials[serial_idx++];
            const ItemSerialInfo serial_info(sid, 1, 1, Items::Origin::From_Game, Utility::GetUtcTimeNow());
            ShopItem new_item = { {item.ItemId ,item_info->Stock } , ItemExpire::Type::Unused , serial_info };
#if defined(RELEASE_1_0_3)
            InventoryItemInfo inv_item_info = { {item.ItemId , item_info->Stock} ,ItemExpire::Type::Unused, new_item.serial_info, item_info->Durability, 0 };
#else
            const InventoryItemInfo& inv_item_info = { item.ItemId ,ItemExpire::Type::Unused, new_item.serial_info, item_info->Durability, 0,0,0,0,0,main_server->AdjustItemType(item_info->Type) };
#endif
            items_to_send.push_back(new_item);
            new_items.push_back({ inv_item_info, item_info->Stock, false, 0, false });
        }
    }

    inline void PackageOpen(SCallbackData& callback, CMainServer* main_server)
    {
        auto session = callback.session;
        auto message = callback.message;
        if (!session || !message) return;

        //std::shared_lock lock(session->GetMutex());

        auto session_id = session->GetSessionId();
        auto acc_cache = CAccount.get<unique_t>(session_id);
        auto acc_index = acc_cache->acc_info.Index;
        if (acc_index == -1) return;

        auto extra = message->GetExtra();
        auto option = message->GetOption();
        auto mission = message->GetMission();

        if (acc_cache->inventory_items.size() >= acc_cache->acc_info.MaximumItems)
        {
            session->SendMsg(102, 1, Items::Package::Result::PACKAGE_INVEN_FULL, 0);
            return;
        }

        auto item = ItemSerialInfo(*reinterpret_cast<uint64_t*>(message->GetData()));
        const auto& item_inv = main_server->GetPlayerItemInventory(acc_cache, item);

        if (!item_inv.has_value()) return;

        const auto& used_item = item_inv.value();
        auto item_used_info = CItemsInfo.get<shared_t>(used_item.item_info.item_number.item_id);
        const auto is_normal_package = mission == 1;
        const auto is_nickname_change = mission == 3 && extra == 53;
        const auto is_hammer_package = mission == 3 && extra == 26;
        const auto item_id = used_item.item_info.item_number.item_id;

        DatabaseUpdateCtx dctx{ .sid = session_id,.aid = acc_index };
        using enum CurrencyType;


        std::vector<ShopItem> items_to_send;
        std::vector<Item> new_items;
        std::vector<ItemSerialInfo> serials_to_delete;
        serials_to_delete.push_back(item);
        uint32_t currency_item_id = 0;
        uint64_t unlocked_voices = 0;
        bool is_vc_item = false,
            is_kd_reset_item = false,
            is_record_reset_item = false,
            is_battery_recharge_item = false,
            is_battery_expansion_item = false,
            is_inv_expand_item = false,
            is_mp_item = false,
            is_coupon_item = false;
        if (is_normal_package)
        {
            const auto& req = reinterpret_cast<MainUsePackageItemReq*>(message->GetData());
            DEBUGLOG(dark_cyan, "player ({}) used package ({})", acc_cache->acc_info.Nickname.c_str(), item_used_info->Id);

            if (IsVoiceItem(item_id, is_vc_item))
            {
                auto voice_index = GetVoiceIndex(item_id);
                auto cha_id = GetVoiceCharacter(item_id);
                DEBUGLOG(dark_cyan, "player want to use voice card for character: ({}) voice index: ({})", cha_id, voice_index);
                unlocked_voices = acc_cache->acc_info.VoiceType;
                if (!isVoiceUnlocked(unlocked_voices, cha_id, voice_index))
                {
                    setVoice(unlocked_voices, cha_id, voice_index);
                    dctx.ops.emplace_back(AccountInfoPatch{ .voice_type = unlocked_voices });
                }
            }
            else if (IsKillDeathResetItem(item_id, is_kd_reset_item)) // Kill-Death reset
                dctx.ops.emplace_back(AccountInfoPatch{ .kills = 0, .deaths = 0, .assists = 0 });
            else if (IsRecordResetItem(item_id, is_record_reset_item)) // Record reset
                dctx.ops.emplace_back(AccountInfoPatch{ .wins = 0, .loses = 0, .draws = 0 });
            else if (IsBatteryRechargeItem(item_id, is_battery_recharge_item)) // Battery recharge
                dctx.ops.emplace_back(AccountCurrencyDelta{ .type = ENERGY, .value = GetBatteryRechargeAmount(item_id), .is_reward = true });
            else if (IsBatteryExpansionItem(item_id, is_battery_expansion_item)) // Battery expansion
                dctx.ops.emplace_back(AccountInfoPatch{ .maximum_energy = acc_cache->acc_info.MaximumEnergy + 1000 });
            else if (IsInvExpandItem(item_id, is_inv_expand_item)) // Inventory expansion
                dctx.ops.emplace_back(AccountInfoPatch{ .maximum_items = acc_cache->acc_info.MaximumItems + GetInvExpandSize(item_id) });
            else
            {
                auto package = CPackagesInfo.get<shared_t>(used_item.item_info.item_number.item_id);
                if (!package->size()) return;
                const auto& items_won = main_server->ExtractPackageItemsWon(package);
                const auto item_count = items_won.size();
                if (IsMicroPointsItem(items_won[0].ItemId, is_mp_item) && item_count == 1) // MicroPoints reward
                    dctx.ops.emplace_back(AccountCurrencyDelta{ .type = MP, .value = GetMicroPointsReward(items_won[0].ItemId), .is_reward = true });
                else if (IsCouponItem(items_won[0].ItemId, is_coupon_item) && item_count == 1) // Coupon reward
                    dctx.ops.emplace_back(AccountCurrencyDelta{ .type = COUPONS, .value = GetCouponReward(items_won[0].ItemId), .is_reward = true });
                else
                {
                    auto serials = main_server->FindLowestAvailableSerialIds(acc_cache->inventory_items, item_count);
                    if (serials.size() < item_count)
                    {
                        DEBUGLOG(red,
                            "Not enough unique serial IDs available. Requested: {}, got: {}",
                            item_count, serials.size());
                        return;
                    }
                    new_items.reserve(item_count);
                    items_to_send.reserve(item_count);
                    ProcessItemsWon(main_server, items_won, serials, items_to_send, new_items);
                }
                if (is_mp_item || is_coupon_item) currency_item_id = items_won[0].ItemId;
                if (is_mp_item)
                    DEBUGLOG(dark_cyan, "player ({}) used package ({}) and won {} micro points", acc_cache->acc_info.Nickname.c_str(), item_used_info->Id, GetMicroPointsReward(items_won[0].ItemId));
                if (is_coupon_item)
                    DEBUGLOG(dark_cyan, "player ({}) used package ({}) and won {} coupons", acc_cache->acc_info.Nickname.c_str(), item_used_info->Id, GetCouponReward(items_won[0].ItemId));
            }
        }
        else if (is_hammer_package)
        {
            const auto& req = reinterpret_cast<MainUsePackageItemHammerReq*>(callback.message->GetData());
            const auto& mistery_inv = main_server->GetPlayerItemInventory(acc_cache, req->mistery_capsule);
            if (!mistery_inv.has_value()) return;
            serials_to_delete.push_back(mistery_inv.value().item_info.serial_info);

            auto package = CPackagesInfo.get<shared_t>(used_item.item_info.item_number.item_id);
            if (!package->size()) return;
            const auto& items_won = main_server->ExtractPackageItemsWon(package);
            const auto item_count = items_won.size();
            auto serials = main_server->FindLowestAvailableSerialIds(acc_cache->inventory_items, item_count);
            if (serials.size() < item_count)
            {
                DEBUGLOG(red,
                    "Not enough unique serial IDs available. Requested: {}, got: {}",
                    item_count, serials.size());
                return;
            }
            new_items.reserve(item_count);
            items_to_send.reserve(item_count);
            ProcessItemsWon(main_server, items_won, serials, items_to_send, new_items);
        }
        else if (is_nickname_change)
        {
            const auto& req = reinterpret_cast<MainUsePackageItemNicknameReq*>(message->GetData());
            dctx.ops.emplace_back(AccountInfoPatch{ .nickname = req->nickname });
        }

        if (!new_items.empty()) dctx.ops.push_back(ItemAddCtx{ .items = std::move(new_items) });
        if (!serials_to_delete.empty()) dctx.ops.push_back(ItemDeleteCtx{ .serials = serials_to_delete });

        auto is_unknown_package = !is_normal_package && !is_nickname_change && !is_hammer_package;
        if (is_unknown_package)
        {
            auto serial_info = used_item.item_info.serial_info;

            DEBUGLOG(red, "Unknown package or item used by player [{}]: {} serial [{}]",
                acc_cache->acc_info.Nickname.c_str(), is_unknown_package ? "package" : "item", serial_info.data);
            session->SendMsg(102, 1, Items::Package::Result::Unknown1, 0, reinterpret_cast<uint8_t*>(&serial_info), sizeof(ItemSerialInfo));
            return;
        }

        auto validated = main_server->ValidateDatabaseUpdates(acc_cache, dctx);
        if (!validated.has_value())
        {
            using enum DbUpdateError;
            const auto err = validated.error();
            if (err == MaxEnergyReachedAlready ||
                err == MaxInventoryItemsReachedAlready ||
                err == MpFull ||
                err == CouponsFull ||
                err == EnergyFull)
            {
                session->SendMsg(102, 1, Items::Package::Result::PACKAGE_INVEN_FULL, 0);
                return;
            }
            else if (err == AVA_CREATE_BANNAME || err == AVA_CREATE_SHORTNAME)
            {
                session->SendMsg(102, 1, Items::Package::Result::MSG_NICKNAME_CHANGE_FAIL, 0);
                session->SendMsg(69, err == AVA_CREATE_SHORTNAME ? NicknameChange::Errors::AVA_CREATE_SHORTNAME : NicknameChange::Errors::ID_CREATE_NO_PERMISSION, 0, 0);
                return;
            }
            return;
        }
        acc_cache.unlock();
        [[maybe_unused]] auto ignored = BaseLib::DbPool->submit_task([main_server,
            session = std::move(callback.session),
            s_id = session_id,
            items_to_send = std::move(items_to_send),
            serials_to_delete = std::move(serials_to_delete),
            is_normal_package = is_normal_package,
            is_nickname_change = is_nickname_change,
            is_hammer_package = is_hammer_package,
            used_item = used_item,
            unlocked_voices = unlocked_voices,
            currency_item_id = currency_item_id,
            is_vc_item = is_vc_item,
            is_kd_reset_item = is_kd_reset_item,
            is_record_reset_item = is_record_reset_item,
            is_battery_recharge_item = is_battery_recharge_item,
            is_battery_expansion_item = is_battery_expansion_item,
            is_inv_expand_item = is_inv_expand_item,
            is_mp_item = is_mp_item,
            is_coupon_item = is_coupon_item,
            v = std::move(validated.value())
        ]() mutable
        {
            if (!session) return;
            ResultDbUpdateInfo dbres;
            auto db_res = BaseLib::Database->UpdateAccount(v, dbres);
            if (!db_res)
            {
                using enum DbError::Type;
                const auto& err = db_res.error();
                if (err.type == DuplicateNickname)
                {
                    session->SendMsg(102, 1, Items::Package::Result::MSG_NICKNAME_CHANGE_FAIL, 0);
                    session->SendMsg(69, NicknameChange::Errors::AVA_CREATE_OVERLAPPEDNAME, 0, 0);
                }

                return;
            }
            auto new_acc_cache = CAccount.get<unique_t>(s_id);
            auto applied = main_server->ApplyDatabaseUpdates(new_acc_cache, v);
            if (!applied.has_value())
            {
                DEBUGLOG(red, "ApplyDatabaseUpdates failed for [{}] [{}]: {}", new_acc_cache->acc_info.Index, new_acc_cache->acc_info.Nickname.c_str(), static_cast<int>(applied.error()));
                return;
            }
            auto serial_info = used_item.item_info.serial_info;

            if (is_normal_package)
            {
                auto is_package = !is_vc_item && !is_kd_reset_item && !is_record_reset_item &&
                    !is_battery_recharge_item && !is_battery_expansion_item &&
                    !is_inv_expand_item && !is_mp_item && !is_coupon_item;

                if (is_vc_item)
                {
                    session->SendMsg(102, 1, Items::Package::Result::MSG_ITEM_VOICE_OPEN, 0, reinterpret_cast<uint8_t*>(&serial_info), sizeof(ItemSerialInfo));
                    session->SendMsg(413, 0, 59, 0, reinterpret_cast<uint8_t*>(&unlocked_voices), sizeof(unlocked_voices));
                }
                if (is_kd_reset_item)
                    session->SendMsg(102, 1, Items::Package::Result::INITIALIZE_KILL_DEATH_COMPLETE, 0, reinterpret_cast<uint8_t*>(&serial_info), sizeof(ItemSerialInfo));

                if (is_record_reset_item)
                    session->SendMsg(102, 1, Items::Package::Result::INITIALIZE_WIN_LOSE_COMPLETE, 0, reinterpret_cast<uint8_t*>(&serial_info), sizeof(ItemSerialInfo));

                if (is_battery_recharge_item ||
                    is_battery_expansion_item ||
                    is_inv_expand_item)
                    session->SendMsg(102, 1, Items::Package::Result::StaticItems, 0, reinterpret_cast<uint8_t*>(&serial_info), sizeof(ItemSerialInfo));

                if (is_mp_item || is_coupon_item)
                {
                    InventoryItemNumber _inv = is_coupon_item ? InventoryItemNumber{ to_u(COUPON_SLOT), GetCouponReward(currency_item_id) } : currency_item_id;
                    ShopItem new_item = { _inv, ItemExpire::Type::Unused, ItemSerialInfo(0, 0, 0, 0, 0) };
                    session->SendMsg(102, 1, Items::Package::Result::Package, is_coupon_item ? 1 : 2, reinterpret_cast<uint8_t*>(&new_item), sizeof(ShopItem));
                }
                if (is_package)
                    session->SendMsg(102, 1, Items::Package::Result::Package, items_to_send.size(), reinterpret_cast<uint8_t*>(items_to_send.data()), items_to_send.size() * sizeof(ShopItem));
            }
            if (is_hammer_package)
                session->SendMsg(102, 1, Items::Package::Result::Capsule, items_to_send.size(), reinterpret_cast<uint8_t*>(items_to_send.data()), items_to_send.size() * sizeof(ShopItem));

            if (is_nickname_change)
            {
                auto itemPackageOpenData = MainUsePackageItemAck(serial_info, new_acc_cache->acc_info.Nickname.c_str()).Serialize(Items::Package::Result::MSG_NICKNAME_CHANGE_SUCCESS);
                session->SendMsg(102, 1, Items::Package::Result::MSG_NICKNAME_CHANGE_SUCCESS, 0, reinterpret_cast<uint8_t*>(itemPackageOpenData.data(), itemPackageOpenData.size()));
                auto clan_id = new_acc_cache->acc_info.ClanId;
                auto has_clan = CClan.contains(clan_id);
                std::string clan_name = "";
                uint32_t logo_front = 0, logo_back = 0;
                if (has_clan)
                {
                    auto clan_info = CClan.get<shared_t>(clan_id);
                    logo_front = clan_info->logo_front;
                    logo_back = clan_info->logo_back;
                    clan_name = clan_info->clan_name;
                }
                auto accInfoMsg = main_server->CraftAccInfoAck(new_acc_cache, 1, clan_name.c_str(), logo_front, logo_back);
                session->SendMsg(413, 0, 1, 1, reinterpret_cast<uint8_t*>(&accInfoMsg), sizeof(MainAccountInfoAck));
            }

            if (serials_to_delete.empty()) return;
            auto deleteItemData = MainDeleteItemAck({ std::move(serials_to_delete) }).Serialize();
            session->SendMsg(89, 0, 1, 0, reinterpret_cast<uint8_t*>(deleteItemData.data()), deleteItemData.size());
        });
    }
}