#pragma once
namespace Game::Handlers
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Main;

    inline void InjectEnergy(SCallbackData& callback, CMainServer* main_server)
    {
        auto session = callback.session;
        auto message = callback.message;
        if (!session || !message) return;
        //std::shared_lock lock(session->GetMutex());

        auto session_id = session->GetSessionId();
        auto acc_cache = CAccount.get<unique_t>(session_id);
        auto acc_index = acc_cache->acc_info.Index;
        auto option = message->GetOption();

        const auto& energyInjectItemReq = reinterpret_cast<MainUpgradeEnergyInjectReq*>(message->GetData());
        if (acc_cache->acc_info.Energy < energyInjectItemReq->energy) return;
        const auto& item_inv = main_server->GetPlayerItemInventory(acc_cache, energyInjectItemReq->item);
        if (!item_inv.has_value()) return;
        auto item_info = CItemsInfo.get<shared_t>(item_inv.value().item_info.item_number.item_id);
        if (!item_info->IsUpgradable) return;

        DatabaseUpdateCtx dctx{ .sid = session_id, .aid = acc_cache->acc_info.Index };

        //acc_cache->acc_info.Energy = acc_cache->acc_info.Energy - energyInjectItemReq->energy;
        using enum CurrencyType;
        if (energyInjectItemReq->energy > 0) dctx.ops.emplace_back(AccountCurrencyDelta{ .type = ENERGY, .value = energyInjectItemReq->energy, .is_reward = false });

        LogContext log_ctx;

        ItemLogEntry item_log;
        item_log.aid = acc_index;
        item_log.action_type = ItemLog::ActionType::EnergyInjected;
        item_log.item_id = item_inv.value().item_info.item_number.item_id;
        item_log.serial_info = energyInjectItemReq->item.data;
        item_log.origin_type = ItemLog::OriginType::Unknown;
        item_log.mp_delta = 0;

        CurrencyLogEntry currency_log;
        currency_log.aid = acc_index;
        currency_log.currency_type = CurrencyLog::Type::Energy;
        currency_log.amount = static_cast<int32_t>(energyInjectItemReq->energy);
        currency_log.before_value = acc_cache->acc_info.Energy;
        currency_log.after_value = acc_cache->acc_info.Energy - energyInjectItemReq->energy;
        currency_log.source_type = CurrencyLog::SourceType::ItemSell;
        currency_log.related_item_id = item_inv.value().item_info.item_number.item_id;

        log_ctx.item_logs.push_back(item_log);
        log_ctx.currency_logs.push_back(currency_log);

        ItemPatchCtx p
        {
            .sel = ItemSelector{.serial = item_inv.value().item_info.serial_info},
            .energy = item_inv.value().item_info.energy + energyInjectItemReq->energy,
        };
        dctx.ops.push_back(p);

        auto validated = main_server->ValidateDatabaseUpdates(acc_cache, dctx);
        if (!validated.has_value())
        {
            DEBUGLOG(red, "ValidateDatabaseUpdates failed for [{}] [{}]: {}", acc_cache->acc_info.Index, acc_cache->acc_info.Nickname.c_str(), static_cast<int>(validated.error()));
            return;
        }
        acc_cache.unlock();

        [[maybe_unused]] auto ignored = BaseLib::DbPool->submit_task([main_server,
            session = std::move(callback.session),
            p_option = option,
            inject_item = energyInjectItemReq->item,
            injected_energy = energyInjectItemReq->energy,
			logContext = std::move(log_ctx),
            v = std::move(validated.value())
        ]() mutable
            {
                if (!session) return;

                ResultDbUpdateInfo dbres;
                if (!BaseLib::Database->UpdateAccount(v, dbres).has_value()) return;

                auto new_acc_cache = CAccount.get<unique_t>(session->GetSessionId());

                auto applied = main_server->ApplyDatabaseUpdates(new_acc_cache, v);
                if (!applied.has_value())
                {
                    DEBUGLOG(red, "ApplyDatabaseUpdates failed for [{}] [{}]: {}", new_acc_cache->acc_info.Index, new_acc_cache->acc_info.Nickname.c_str(), static_cast<int>(applied.error()));
                    return;
                }
                MainInjectEnergyAck injectEnergyData = MainInjectEnergyAck(inject_item, injected_energy);
                session->SendMsg(101, 0, static_cast<uint8_t>(Items::Upgrade::Result::EnergyInjection), p_option, reinterpret_cast<uint8_t*>(&injectEnergyData), sizeof(MainInjectEnergyAck));

                BaseLib::Database->PersistLogs(logContext);
            });

    }
    inline void Reset(SCallbackData& callback, CMainServer* main_server)
    {
        auto session = callback.session;
        auto message = callback.message;
        if (!session || !message) return;
        //std::shared_lock lock(session->GetMutex());

        auto session_id = session->GetSessionId();
        auto acc_cache = CAccount.get<unique_t>(session_id);
		auto aid = acc_cache->acc_info.Index;
        if (!aid) return;
        const auto& upgradeResetItemReq = reinterpret_cast<MainUpgradeResetReq*>(message->GetData());
        const auto& item_inv = main_server->GetPlayerItemInventory(acc_cache, upgradeResetItemReq->item);
        if (!item_inv.has_value()) return;
        const auto& upgrade_reset_item = main_server->GetPlayerItemInventory(acc_cache, upgradeResetItemReq->upgrade_reset_item);
        if (!upgrade_reset_item.has_value()) return;
        auto upgrade_info = main_server->GetUpgradeInfoCache(item_inv.value().item_info.item_number.item_id);
        if (upgrade_info->GroupId == -1) return;
        auto item_info = CItemsInfo.get<shared_t>(upgrade_info->GroupId);
        if (item_info->Id == -1) return;

        DatabaseUpdateCtx dctx{ .sid = session_id, .aid = acc_cache->acc_info.Index };
        ItemPatchCtx p
        {
            .sel = ItemSelector{.serial = item_inv.value().item_info.serial_info},
            .repair = item_info->Durability,
            .new_item_id = item_info->Id

        };

        dctx.ops.push_back(p);
        dctx.ops.push_back(ItemDeleteCtx{ .serials = {upgrade_reset_item.value().item_info.serial_info} });

        const ShopItem& shop_item = { {item_info->Id , item_info->Stock }, item_inv.value().item_info.expire_date, item_inv.value().item_info.serial_info.data };
        MainResetUpgradeItemAck resetUpgradeItemData = MainResetUpgradeItemAck(shop_item, item_inv.value().item_info.serial_info, upgrade_reset_item.value().item_info.serial_info);

       

        


        auto validated = main_server->ValidateDatabaseUpdates(acc_cache, dctx);
        if (!validated.has_value())
        {
            DEBUGLOG(red, "ValidateDatabaseUpdates failed for [{}] [{}]: {}", acc_cache->acc_info.Index, acc_cache->acc_info.Nickname.c_str(), static_cast<int>(validated.error()));
            return;
        }

        LogContext log_ctx;

        ItemLogEntry item_log;
        item_log.aid = aid;
        item_log.action_type = ItemLog::ActionType::Reset;
        item_log.item_id = item_inv.value().item_info.item_number.item_id;
        item_log.serial_info = upgradeResetItemReq->item.data;
        item_log.origin_type = ItemLog::OriginType::Unknown;
        item_log.mp_delta = 0;

        CurrencyLogEntry currency_log;
        currency_log.aid = aid;
        currency_log.currency_type = CurrencyLog::Type::MP;
        currency_log.amount = 0;
        currency_log.before_value = acc_cache->acc_info.MicroPoints;
        currency_log.after_value = acc_cache->acc_info.MicroPoints;
        currency_log.source_type = CurrencyLog::SourceType::ItemSell;
        currency_log.related_item_id = item_inv.value().item_info.item_number.item_id;

        acc_cache.unlock();
        [[maybe_unused]] auto ignored = BaseLib::DbPool->submit_task([main_server,
            session = std::move(callback.session),
            resetUpgradeItemData = std::move(resetUpgradeItemData),
            v = std::move(validated.value()),
			logContext = std::move(log_ctx)
        ]() mutable
            {
                if (!session) return;

                ResultDbUpdateInfo dbres;
                if (!BaseLib::Database->UpdateAccount(v, dbres).has_value()) return;

                auto new_acc_cache = CAccount.get<unique_t>(session->GetSessionId());

                auto applied = main_server->ApplyDatabaseUpdates(new_acc_cache, v);
                if (!applied.has_value())
                {
                    DEBUGLOG(red, "ApplyDatabaseUpdates failed for [{}] [{}]: {}", new_acc_cache->acc_info.Index, new_acc_cache->acc_info.Nickname.c_str(), static_cast<int>(applied.error()));
                    return;
                }
                session->SendMsg(101, 0, static_cast<uint8_t>(Items::Upgrade::Result::UpgradeReset), 0, reinterpret_cast<uint8_t*>(&resetUpgradeItemData), sizeof(MainResetUpgradeItemAck));

                BaseLib::Database->PersistLogs(logContext);
            });
    }
    inline void Upgrade(SCallbackData& callback, CMainServer* main_server)
    {
        auto session = callback.session;
        auto message = callback.message;
        if (!session || !message) return;
        //std::shared_lock lock(session->GetMutex());

        const auto& upgradeItemReq = reinterpret_cast<MainUpgradeItemReq*>(message->GetData());

        auto session_id = session->GetSessionId();
        auto acc_cache = CAccount.get<unique_t>(session_id);
        auto acc_index = acc_cache->acc_info.Index;
        auto upgrade_info = message->GetOption();
        auto upgrade_type = message->GetMission();
        bool have_booster_item = false, have_energy_refund_item = false, have_protection_item = false;
        ItemSerialInfo booster_item = ItemSerialInfo(0, 0, 0, 0, 0), energy_refund_item = ItemSerialInfo(0, 0, 0, 0, 0), protection_item = ItemSerialInfo(0, 0, 0, 0, 0);

       

        if (upgrade_info == 1)
        {
            have_booster_item = true;
            booster_item.data = upgradeItemReq->item[1].data;
        }
        else if (upgrade_info == 2)
        {
            have_energy_refund_item = true;
            energy_refund_item.data = upgradeItemReq->item[1].data;
        }
        else if (upgrade_info == 3)
        {
            have_protection_item = true;
            protection_item.data = upgradeItemReq->item[1].data;
        }
        else if (upgrade_info == 4)
        {
            have_booster_item = true;
            booster_item.data = upgradeItemReq->item[1].data;
            have_energy_refund_item = true;
            energy_refund_item.data = upgradeItemReq->item[2].data;
        }
        else if (upgrade_info == 5)
        {
            have_booster_item = true;
            booster_item.data = upgradeItemReq->item[1].data;
            have_protection_item = true;
            protection_item.data = upgradeItemReq->item[2].data;
        }
        else if (upgrade_info == 6)
        {
            have_energy_refund_item = true;
            energy_refund_item.data = upgradeItemReq->item[1].data;
            have_protection_item = true;
            protection_item.data = upgradeItemReq->item[2].data;
        }
        else if (upgrade_info == 7)
        {
            have_booster_item = true;
            booster_item.data = upgradeItemReq->item[1].data;
            have_energy_refund_item = true;
            energy_refund_item.data = upgradeItemReq->item[2].data;
            have_protection_item = true;
            protection_item.data = upgradeItemReq->item[3].data;
        }
        else
        {
            have_booster_item = false;
            have_energy_refund_item = false;
            have_protection_item = false;
        }

        const auto& item_inv = main_server->GetPlayerItemInventory(acc_cache, upgradeItemReq->item[0]);

        if (!item_inv.has_value()) return;
        auto item_info = CItemsInfo.get<shared_t>(item_inv.value().item_info.item_number.item_id);
        if (item_info->Id == -1) return;
        auto upgrade_collection = main_server->GetUpgradeCollectionInfoCache(static_cast<Items::Upgrade::Type>(upgrade_type + 1), item_inv.value().item_info.item_number.item_id);
        if (upgrade_collection->empty()) return;
        auto current_level = main_server->GetUpgradeLevel(upgrade_collection, item_inv.value().item_info.item_number.item_id);
        const auto& current_upgrade = main_server->GetUpgradeInfoNext(upgrade_collection, item_inv.value().item_info.item_number.item_id);
        DEBUGLOG(dark_cyan, "player ({}) wants to upgrade item: ({}) -> ({})", acc_cache->acc_info.Nickname.c_str(), item_info->Id, current_upgrade.ItemId);
        const auto& current_item_info = item_inv.value().item_info;
        if (current_item_info.energy < current_upgrade.UseExp)
        {
            DEBUGLOG(dark_cyan, "player ({}) has no battery to upgrade item: ({})", acc_cache->acc_info.Nickname.c_str(), item_info->Id);
            return;
        }
        if (current_item_info.repair != item_info->Durability)
        {
            session->SendMsg(101, 0, static_cast<uint8_t>(Items::Upgrade::Result::RepairItem), 0);
            DEBUGLOG(dark_cyan, "player ({}) has no funds to upgrade item: ({})", acc_cache->acc_info.Nickname.c_str(), item_info->Id);
            return;
        }
        if (acc_cache->acc_info.MicroPoints < current_upgrade.BuyPoint || acc_cache->acc_info.RockTokens < current_upgrade.BuyCash)
        {
            session->SendMsg(101, 0, static_cast<uint8_t>(Items::Upgrade::Result::NotEnoughPoints), 0);
            DEBUGLOG(dark_cyan, "player ({}) doesn't have enough funds to upgrade item: ({}) -> ({})", acc_cache->acc_info.Nickname.c_str(), item_info->Id, current_upgrade.ItemId);
            return;
        }
        const auto& max_probability = std::max_element(upgrade_collection->begin(), upgrade_collection->end(),
            [](const BaseLib::UpgradeInfo& a, const BaseLib::UpgradeInfo& b) {
                return a.Probability < b.Probability;
            })->Probability;
        auto cur_probability = current_upgrade.Probability;
        auto extracted_number = Utility::Random::CustomGen(0, max_probability);
        DEBUGLOG(dark_cyan, "player ({}) upgraded item: ({}) -> ({}) with a success rate of ({})", acc_cache->acc_info.Nickname.c_str(), item_info->Id, current_upgrade.ItemId, (static_cast<float>(cur_probability) / static_cast<float>(max_probability)) * 100.f);
        bool is_upgrade_successful = (extracted_number >= 0) && (extracted_number <= cur_probability);
        bool no_downgrade = false;
        DatabaseUpdateCtx dctx{ .sid = session_id, .aid = acc_cache->acc_info.Index };
        std::vector<NetEngine::Packets::Main::ItemSerialInfo> item_deletes;
        std::vector<uint8_t> upgradeItemAckData;
        if (have_booster_item)
        {
            const auto& inv_booster = main_server->GetPlayerItemInventory(acc_cache, booster_item);
            if (inv_booster.has_value())
            {
                if (inv_booster.value().item_info.item_number.item_id == 4305004)
                {
                    item_deletes.push_back(booster_item);
                    //ain_server->AddPlayerItemsDeleted(acc_cache, booster_item);
                    is_upgrade_successful = true;
                }
            }
        }
        if (is_upgrade_successful)
        {
            auto upgraded_item_info = CItemsInfo.get<shared_t>(current_upgrade.ItemId);
            if (upgraded_item_info->Id != -1)
            {
                ItemPatchCtx p
                {
                    .sel = ItemSelector{.serial = item_inv.value().item_info.serial_info},
                    .repair = upgraded_item_info->Durability,
                    .energy = 0,
                    .new_item_id = upgraded_item_info->Id

                };
                dctx.ops.push_back(p);
                //main_server->UpdatePlayerItemUpgrade(acc_cache, item_inv.value().item_info.serial_info, upgraded_item_info->Id, upgraded_item_info->Durability, 0);
                ShopItem shop_item = { {upgraded_item_info->Id , upgraded_item_info->Stock }, item_inv.value().item_info.expire_date, item_inv.value().item_info.serial_info.data };
                upgradeItemAckData = MainUpgradeItemAck(shop_item, item_inv.value().item_info.serial_info, booster_item, energy_refund_item, protection_item).Serialize(upgrade_info, static_cast<uint8_t>(Items::Upgrade::Result::UpgradeSuccess));
                //session->SendMsg(101, upgrade_type, static_cast<uint8_t>(Items::Upgrade::Result::UpgradeSuccess), upgrade_info, reinterpret_cast<uint8_t*>(upgradeItemAckData.data()), upgradeItemAckData.size());
                DEBUGLOG(dark_cyan, "player ({}) upgraded item: ({})", acc_cache->acc_info.Nickname.c_str(), item_info->Id);
            }
        }
        else
        {
            auto down_cur_probability = current_upgrade.HoldProbability;
            auto max_holdprobability = std::max_element(upgrade_collection->begin(), upgrade_collection->end(),
                [](const BaseLib::UpgradeInfo& a, const BaseLib::UpgradeInfo& b) {
                    return a.HoldProbability < b.HoldProbability;
                })->HoldProbability;
            auto down_extracted_number = Utility::Random::CustomGen(0, max_holdprobability);

            no_downgrade = (down_extracted_number >= 0 && down_extracted_number <= down_cur_probability);
            DEBUGLOG(dark_cyan, "player ({}) item: ({}) -> ({}) has a no downgrade probability rate: ({})", acc_cache->acc_info.Nickname.c_str(), item_info->Id, current_upgrade.ItemId, (static_cast<float>(down_cur_probability) / static_cast<float>(max_holdprobability)) * 100.f);
            if (have_protection_item)
            {
                const auto& inv_protection = main_server->GetPlayerItemInventory(acc_cache, protection_item);
                if (inv_protection.has_value() && inv_protection.value().item_info.item_number.item_id == 4305018)
                {
                    item_deletes.push_back(protection_item);
                    //main_server->AddPlayerItemsDeleted(acc_cache, protection_item);
                    no_downgrade = true;
                }
            }
            uint32_t energy_to_refund = 0;
            if (have_energy_refund_item)
            {
                const auto& inv_refund_item = main_server->GetPlayerItemInventory(acc_cache, energy_refund_item);
                if (inv_refund_item.has_value())
                {
                    if (inv_refund_item.value().item_info.item_number.item_id == 4305014)
                    {
                        energy_to_refund = (current_upgrade.UseExp / 100) * 30;
                        item_deletes.push_back(energy_refund_item);
                    }
                    else if (inv_refund_item.value().item_info.item_number.item_id == 4305015)
                    {
                        energy_to_refund = (current_upgrade.UseExp / 100) * 50;
                        item_deletes.push_back(energy_refund_item);
                    }
                    else if (inv_refund_item.value().item_info.item_number.item_id == 4305016)
                    {
                        energy_to_refund = (current_upgrade.UseExp / 100) * 100;
                        item_deletes.push_back(energy_refund_item);
                    }
                }
            }
            if (current_level <= 3)
                no_downgrade = true;

            if (no_downgrade)
            {
                ItemPatchCtx p
                {
                    .sel = ItemSelector{.serial = item_inv.value().item_info.serial_info},
                    .energy = energy_to_refund
                };
                dctx.ops.push_back(p);
                //main_server->UpdatePlayerItemEnergy(acc_cache, item_inv.value().item_info.serial_info, energy_to_refund);
                upgradeItemAckData = MainUpgradeItemAck(ShopItem(), item_inv.value().item_info.serial_info, booster_item, energy_refund_item, protection_item).Serialize(upgrade_info, static_cast<uint8_t>(Items::Upgrade::Result::UpgradeFailLow));
                //session->SendMsg(101, static_cast<uint8_t>(Items::Upgrade::FailType::NoChange), static_cast<uint8_t>(Items::Upgrade::Result::UpgradeFailLow), upgrade_info, reinterpret_cast<uint8_t*>(upgradeItemAckData.data()), upgradeItemAckData.size());
                DEBUGLOG(dark_cyan, "player ({}) failed to upgrade item: ({})", acc_cache->acc_info.Nickname.c_str(), item_info->Id);
            }
            else
            {
                const auto& previous_upgrade = main_server->GetUpgradeInfoPrev(upgrade_collection, item_inv.value().item_info.item_number.item_id);
                auto previous_upgrade_item_info = CItemsInfo.get<shared_t>(previous_upgrade.ItemId);
                if (previous_upgrade_item_info->Id != -1)
                {
                    ItemPatchCtx p
                    {
                        .sel = ItemSelector{.serial = item_inv.value().item_info.serial_info},
                        .repair = previous_upgrade_item_info->Durability,
                        .energy = 0,
                        .new_item_id = previous_upgrade_item_info->Id,
                    };
                    dctx.ops.push_back(p);
                    // main_server->UpdatePlayerItemUpgrade(acc_cache, item_inv.value().item_info.serial_info, previous_upgrade_item_info->Id, previous_upgrade_item_info->Durability, 0);
                    const ShopItem& shop_item = { {previous_upgrade_item_info->Id, previous_upgrade_item_info->Stock} , item_inv.value().item_info.expire_date , item_inv.value().item_info.serial_info.data };
                    upgradeItemAckData = MainUpgradeItemAck(shop_item, item_inv.value().item_info.serial_info, booster_item, energy_refund_item, protection_item).Serialize(upgrade_info, static_cast<uint8_t>(Items::Upgrade::Result::UpgradeFailHigh));
                    //session->SendMsg(101, upgrade_type, static_cast<uint8_t>(Items::Upgrade::Result::UpgradeFailHigh), upgrade_info, reinterpret_cast<uint8_t*>(upgradeItemAckData.data()), upgradeItemAckData.size());
                    DEBUGLOG(dark_cyan, "player ({}) failed to upgrade and downgraded item: ({})", acc_cache->acc_info.Nickname.c_str(), item_info->Id);
                }
            }
        }
        const auto& next_upgrade = main_server->GetUpgradeInfoNext(upgrade_collection, item_inv.value().item_info.item_number.item_id);

        using enum CurrencyType;
        if (next_upgrade.BuyCash > 0) dctx.ops.emplace_back(AccountCurrencyDelta{ .type = RT, .value = next_upgrade.BuyCash, .is_reward = false });
        if (next_upgrade.BuyPoint > 0) dctx.ops.emplace_back(AccountCurrencyDelta{ .type = MP, .value = next_upgrade.BuyPoint, .is_reward = false });

        if (!item_deletes.empty())
            dctx.ops.push_back(ItemDeleteCtx{ .serials = item_deletes });

        auto validated = main_server->ValidateDatabaseUpdates(acc_cache, dctx);
        if (!validated.has_value())
        {
            DEBUGLOG(red, "ValidateDatabaseUpdates failed for [{}] [{}]: {}", acc_cache->acc_info.Index, acc_cache->acc_info.Nickname.c_str(), static_cast<int>(validated.error()));
            return;
        }

        LogContext log_ctx;
        if(!item_deletes.empty())
        {
			for (auto& serial : item_deletes)
            {

                const auto& item_deleted = main_server->GetPlayerItemInventory(acc_cache, serial);
                if (!item_deleted.has_value()) continue;


                ItemLogEntry item_log;
                item_log.aid = acc_index;
                item_log.action_type = ItemLog::ActionType::Deleted;
                item_log.item_id = item_deleted.value().item_info.item_number.item_id;
                item_log.serial_info = serial.data;
                item_log.origin_type = ItemLog::OriginType::Unknown;
                item_log.mp_delta = 0;
                log_ctx.item_logs.push_back(item_log);
            }
        }
        ItemLogEntry item_log;
        item_log.aid = acc_index;
        item_log.action_type = ItemLog::ActionType::Upgraded;
        item_log.item_id = item_inv.value().item_info.item_number.item_id;
        item_log.serial_info = upgradeItemReq->item[0].data;
        item_log.origin_type = ItemLog::OriginType::Unknown;
        item_log.mp_delta = static_cast<int32_t>(next_upgrade.BuyPoint);

        CurrencyLogEntry currency_log;
        currency_log.aid = acc_index;
        currency_log.currency_type = CurrencyLog::Type::MP;
        currency_log.amount = static_cast<int32_t>(next_upgrade.BuyPoint);
        currency_log.before_value = acc_cache->acc_info.MicroPoints;
        currency_log.after_value = acc_cache->acc_info.MicroPoints - static_cast<int32_t>(next_upgrade.BuyPoint);
        currency_log.source_type = CurrencyLog::SourceType::ItemUpgrade;
        currency_log.related_item_id = item_inv.value().item_info.item_number.item_id;
        log_ctx.item_logs.push_back(item_log);
        log_ctx.currency_logs.push_back(currency_log);


        acc_cache.unlock();
        [[maybe_unused]] auto ignored = BaseLib::DbPool->submit_task([main_server,
            session = std::move(callback.session),
            upgrade_info = upgrade_info,
            upgrade_type = upgrade_type,
            is_upgrade_successful = is_upgrade_successful,
            no_downgrade = no_downgrade,
            upgradeItemAckData = std::move(upgradeItemAckData),
            v = std::move(validated.value()),
			logContext = std::move(log_ctx)
        ]() mutable
            {
                if (!session) return;

                ResultDbUpdateInfo dbres;
                if (!BaseLib::Database->UpdateAccount(v, dbres).has_value()) return;

                auto new_acc_cache = CAccount.get<unique_t>(session->GetSessionId());

                auto applied = main_server->ApplyDatabaseUpdates(new_acc_cache, v);
                if (!applied.has_value())
                {
                    DEBUGLOG(red, "ApplyDatabaseUpdates failed for [{}] [{}]: {}", new_acc_cache->acc_info.Index, new_acc_cache->acc_info.Nickname.c_str(), static_cast<int>(applied.error()));
                    return;
                }

                const uint8_t type_param =
                    is_upgrade_successful
                    ? upgrade_type
                    : (no_downgrade
                        ? static_cast<uint8_t>(Items::Upgrade::FailType::NoChange)
                        : upgrade_type);

                const uint8_t result_param =
                    is_upgrade_successful
                    ? static_cast<uint8_t>(Items::Upgrade::Result::UpgradeSuccess)
                    : (no_downgrade
                        ? static_cast<uint8_t>(Items::Upgrade::Result::UpgradeFailLow)
                        : static_cast<uint8_t>(Items::Upgrade::Result::UpgradeFailHigh));

                session->SendMsg(101, type_param, result_param, upgrade_info, reinterpret_cast<uint8_t*>(upgradeItemAckData.data()), upgradeItemAckData.size());
                BaseLib::Database->PersistLogs(logContext);
            });
    }

    inline void ItemUpgrade(SCallbackData& callback, CMainServer* main_server)
    {
        const auto& extra = callback.message->GetExtra();
        switch (extra)
        {
        case 0: InjectEnergy(callback, main_server); break;
        case 37: Upgrade(callback, main_server); break;
        case 53: Reset(callback, main_server); break;
        }
    }
}