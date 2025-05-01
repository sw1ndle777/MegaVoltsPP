#pragma once
namespace Game
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Main;

    namespace Handlers
    {

        inline void InjectEnergy(SCallbackData& callback, CMainServer* main_server)
        {
            auto session = callback.session;
            auto message = callback.message;
            if (!session || !message) return;
            std::shared_lock lock(session->GetMutex());

            auto session_id = session->GetSessionId();
            auto acc_cache = main_server->GetAccCacheUniqueBySessionId(session_id);
            auto acc_index = acc_cache->acc_info.Index;
            auto option = message->GetOption();

            const auto& energyInjectItemReq = reinterpret_cast<MainUpgradeEnergyInjectReq*>(message->GetData());
            if (acc_cache->acc_info.Energy < energyInjectItemReq->energy) return;
            const auto& item_inv = main_server->GetPlayerItemInventory(acc_cache, energyInjectItemReq->item);
            if (!item_inv.has_value()) return;
            auto item_info = main_server->GetItemInfoCache(item_inv.value().item_info.item_number.item_id);
            if (!item_info->IsUpgradable) return;
            acc_cache->acc_info.Energy = acc_cache->acc_info.Energy - energyInjectItemReq->energy;
            main_server->UpdatePlayerItemEnergy(acc_cache, energyInjectItemReq->item, item_inv.value().item_info.energy + energyInjectItemReq->energy);
            MainInjectEnergyAck injectEnergyData = MainInjectEnergyAck(energyInjectItemReq->item, energyInjectItemReq->energy);
            session->SendMsg(101, 0, static_cast<uint8_t>(Items::Upgrade::Result::EnergyInjection), option, reinterpret_cast<uint8_t*>(&injectEnergyData), sizeof(MainInjectEnergyAck));
        }
        inline void Reset(SCallbackData& callback, CMainServer* main_server)
        {
            auto session = callback.session;
            auto message = callback.message;
            if (!session || !message) return;
            std::shared_lock lock(session->GetMutex());

            auto session_id = session->GetSessionId();
            auto acc_cache = main_server->GetAccCacheUniqueBySessionId(session_id);
            const auto& upgradeResetItemReq = reinterpret_cast<MainUpgradeResetReq*>(message->GetData());
            const auto& item_inv = main_server->GetPlayerItemInventory(acc_cache, upgradeResetItemReq->item);
            if (!item_inv.has_value()) return;
            const auto& upgrade_reset_item = main_server->GetPlayerItemInventory(acc_cache, upgradeResetItemReq->upgrade_reset_item);
            if (!upgrade_reset_item.has_value()) return;
            auto upgrade_info = main_server->GetUpgradeInfoCache(item_inv.value().item_info.item_number.item_id);
            if (upgrade_info->GroupId == -1) return;
            auto item_info = main_server->GetItemInfoCache(upgrade_info->GroupId);
            if (item_info->Id == -1) return;
           
            main_server->UpdatePlayerItemUpgrade(acc_cache, upgradeResetItemReq->item, item_info->Id, item_info->Durability, 0);
            main_server->AddPlayerItemsDeleted(acc_cache, upgrade_reset_item.value().item_info.serial_info);

            const ShopItem& shop_item = { {item_info->Id , item_info->Stock }, item_inv.value().item_info.expire_date, item_inv.value().item_info.serial_info.data };
            MainResetUpgradeItemAck resetUpgradeItemData = MainResetUpgradeItemAck(shop_item, item_inv.value().item_info.serial_info, upgrade_reset_item.value().item_info.serial_info);
            session->SendMsg(101, 0, static_cast<uint8_t>(Items::Upgrade::Result::UpgradeReset), 0, reinterpret_cast<uint8_t*>(&resetUpgradeItemData), sizeof(MainResetUpgradeItemAck));
        }
        inline void Upgrade(SCallbackData& callback, CMainServer* main_server)
        {
            auto session = callback.session;
            auto message = callback.message;
            if (!session || !message) return;
            std::shared_lock lock(session->GetMutex());

            const auto& upgradeItemReq = reinterpret_cast<MainUpgradeItemReq*>(message->GetData());

            auto session_id = session->GetSessionId();
            auto acc_cache = main_server->GetAccCacheUniqueBySessionId(session_id);
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
            auto item_info = main_server->GetItemInfoCache(item_inv.value().item_info.item_number.item_id);
            if (item_info->Id == -1) return;
            auto upgrade_collection = main_server->GetUpgradeCollectionInfoCache(static_cast<Items::Upgrade::Type>(upgrade_type + 1), item_inv.value().item_info.item_number.item_id);
            if (upgrade_collection->empty()) return;
            auto current_level = main_server->GetUpgradeLevel(upgrade_collection, item_inv.value().item_info.item_number.item_id);
            const auto& current_upgrade = main_server->GetUpgradeInfoNext(upgrade_collection, item_inv.value().item_info.item_number.item_id);
            BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "player ({}) wants to upgrade item: ({}) -> ({})", acc_cache->acc_info.Nickname.c_str(), item_info->Id, current_upgrade.ItemId);
            const auto& current_item_info = item_inv.value().item_info;
            if (current_item_info.energy < current_upgrade.UseExp)
            {
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "player ({}) has no battery to upgrade item: ({})", acc_cache->acc_info.Nickname.c_str(), item_info->Id);
                return;
            }
            if (current_item_info.repair != item_info->Durability)
            {
                session->SendMsg(101, 0, static_cast<uint8_t>(Items::Upgrade::Result::RepairItem), 0);
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "player ({}) has no funds to upgrade item: ({})", acc_cache->acc_info.Nickname.c_str(), item_info->Id);
                return;
            }
            if (acc_cache->acc_info.MicroPoints < current_upgrade.BuyPoint || acc_cache->acc_info.RockTokens < current_upgrade.BuyCash)
            {
                session->SendMsg(101, 0, static_cast<uint8_t>(Items::Upgrade::Result::NotEnoughPoints), 0);
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "player ({}) doesn't have enough funds to upgrade item: ({}) -> ({})", acc_cache->acc_info.Nickname.c_str(), item_info->Id, current_upgrade.ItemId);
                return;
            }
            const auto& max_probability = std::max_element(upgrade_collection->begin(), upgrade_collection->end(),
                [](const BaseLib::UpgradeInfo& a, const BaseLib::UpgradeInfo& b) {
                    return a.Probability < b.Probability;
                })->Probability;
            auto cur_probability = current_upgrade.Probability;
            auto extracted_number = Utility::Random::CustomGen(0, max_probability);
            BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "player ({}) upgraded item: ({}) -> ({}) with a success rate of ({})", acc_cache->acc_info.Nickname.c_str(), item_info->Id, current_upgrade.ItemId, (static_cast<float>(cur_probability) / static_cast<float>(max_probability)) * 100.f);
            bool is_upgrade_successful = (extracted_number >= 0) && (extracted_number <= cur_probability);
            if (have_booster_item)
            {
                const auto& inv_booster = main_server->GetPlayerItemInventory(acc_cache, booster_item);
                if (inv_booster.has_value())
                {
                    if (inv_booster.value().item_info.item_number.item_id == 4305004)
                    {
                        main_server->AddPlayerItemsDeleted(acc_cache, booster_item);
                        is_upgrade_successful = true;
                    }
                }
            }
            if (is_upgrade_successful)
            {
                auto upgraded_item_info = main_server->GetItemInfoCache(current_upgrade.ItemId);
                if (upgraded_item_info->Id != -1)
                {
                    main_server->UpdatePlayerItemUpgrade(acc_cache, item_inv.value().item_info.serial_info, upgraded_item_info->Id, upgraded_item_info->Durability, 0);
                    ShopItem shop_item = { {upgraded_item_info->Id , upgraded_item_info->Stock }, item_inv.value().item_info.expire_date, item_inv.value().item_info.serial_info.data };
                    auto upgradeItemAckData = MainUpgradeItemAck(shop_item, item_inv.value().item_info.serial_info, booster_item, energy_refund_item, protection_item).Serialize(upgrade_info, static_cast<uint8_t>(Items::Upgrade::Result::UpgradeSuccess));
                    session->SendMsg(101, upgrade_type, static_cast<uint8_t>(Items::Upgrade::Result::UpgradeSuccess), upgrade_info, reinterpret_cast<uint8_t*>(upgradeItemAckData.data()), upgradeItemAckData.size());
                    BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "player ({}) upgraded item: ({})", acc_cache->acc_info.Nickname.c_str(), item_info->Id);
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

                bool no_downgrade = (down_extracted_number >= 0 && down_extracted_number <= down_cur_probability);
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "player ({}) item: ({}) -> ({}) has a no downgrade probability rate: ({})", acc_cache->acc_info.Nickname.c_str(), item_info->Id, current_upgrade.ItemId, (static_cast<float>(down_cur_probability) / static_cast<float>(max_holdprobability)) * 100.f);
                if (have_protection_item)
                {
                    const auto& inv_protection = main_server->GetPlayerItemInventory(acc_cache, protection_item);
                    if (inv_protection.has_value() && inv_protection.value().item_info.item_number.item_id == 4305018)
                    {
                        main_server->AddPlayerItemsDeleted(acc_cache, protection_item);
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
                            main_server->AddPlayerItemsDeleted(acc_cache, energy_refund_item);
                        }
                        else if (inv_refund_item.value().item_info.item_number.item_id == 4305015)
                        {
                            energy_to_refund = (current_upgrade.UseExp / 100) * 50;
                            main_server->AddPlayerItemsDeleted(acc_cache, energy_refund_item);
                        }
                        else if (inv_refund_item.value().item_info.item_number.item_id == 4305016)
                        {
                            energy_to_refund = (current_upgrade.UseExp / 100) * 100;
                            main_server->AddPlayerItemsDeleted(acc_cache, energy_refund_item);
                        }
                    }
                }
                if (current_level <= 3)
                    no_downgrade = true;

                if (no_downgrade)
                {
                    main_server->UpdatePlayerItemEnergy(acc_cache, item_inv.value().item_info.serial_info, energy_to_refund);
                    auto upgradeItemAckData = MainUpgradeItemAck(ShopItem(), item_inv.value().item_info.serial_info, booster_item, energy_refund_item, protection_item).Serialize(upgrade_info, static_cast<uint8_t>(Items::Upgrade::Result::UpgradeFailLow));
                    session->SendMsg(101, static_cast<uint8_t>(Items::Upgrade::FailType::NoChange), static_cast<uint8_t>(Items::Upgrade::Result::UpgradeFailLow), upgrade_info, reinterpret_cast<uint8_t*>(upgradeItemAckData.data()), upgradeItemAckData.size());;
                    BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "player ({}) failed to upgrade item: ({})", acc_cache->acc_info.Nickname.c_str(), item_info->Id);
                }
                else
                {
                    const auto& previous_upgrade = main_server->GetUpgradeInfoPrev(upgrade_collection, item_inv.value().item_info.item_number.item_id);
                    auto previous_upgrade_item_info = main_server->GetItemInfoCache(previous_upgrade.ItemId);
                    if (previous_upgrade_item_info->Id != -1)
                    {
                        main_server->UpdatePlayerItemUpgrade(acc_cache, item_inv.value().item_info.serial_info, previous_upgrade_item_info->Id, previous_upgrade_item_info->Durability, 0);
                        const ShopItem& shop_item = { {previous_upgrade_item_info->Id, previous_upgrade_item_info->Stock} , item_inv.value().item_info.expire_date , item_inv.value().item_info.serial_info.data };
                        auto upgradeItemAckData = MainUpgradeItemAck(shop_item, item_inv.value().item_info.serial_info, booster_item, energy_refund_item, protection_item).Serialize(upgrade_info, static_cast<uint8_t>(Items::Upgrade::Result::UpgradeFailHigh));
                        session->SendMsg(101, upgrade_type, static_cast<uint8_t>(Items::Upgrade::Result::UpgradeFailHigh), upgrade_info, reinterpret_cast<uint8_t*>(upgradeItemAckData.data()), upgradeItemAckData.size());
                        BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "player ({}) failed to upgrade and downgraded item: ({})", acc_cache->acc_info.Nickname.c_str(), item_info->Id);
                    }
                }
            }
            const auto& next_upgrade = main_server->GetUpgradeInfoNext(upgrade_collection, item_inv.value().item_info.item_number.item_id);
            if (next_upgrade.BuyCash > 0)
                acc_cache->acc_info.RockTokens = acc_cache->acc_info.RockTokens - next_upgrade.BuyCash;

            if (next_upgrade.BuyPoint > 0)
                acc_cache->acc_info.MicroPoints = acc_cache->acc_info.MicroPoints - next_upgrade.BuyPoint;
        }

        inline void UpgradeItem(SCallbackData& callback, CMainServer* main_server)
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
    
}