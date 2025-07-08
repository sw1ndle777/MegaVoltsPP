#pragma once
namespace Game
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Main;

    namespace Handlers
    {
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

        void setVoice(uint64_t& data, uint32_t character, uint8_t voice) {
            uint8_t bit_position = (12 * character) + (voice - 4);
            data |= (1ULL << bit_position);
        }

        void unsetVoice(uint64_t& data, uint32_t character, uint8_t voice) {
            uint8_t bit_position = (12 * character) + (voice - 4);
            data &= ~(1ULL << bit_position);
        }

        bool isVoiceUnlocked(uint64_t data, uint32_t character, uint8_t voice) {
            uint8_t bit_position = (12 * character) + (voice - 4);
            return (data & (1ULL << bit_position)) != 0;
        }
        //std::unordered_map<uint32_t, uint32_t> coupon_map =
        boost::unordered_flat_map<uint32_t, uint32_t> coupon_map =
        {
            {4305019, 1}, {4305020, 5}, {4305021, 10}, {4305022, 15},
            {4305023, 20}, {4305024, 25}, {4305025, 0}, {4305026, 30},
            {4305027, 2}, {4305028, 3}, {4305029, 4}, {4305030, 6},
            {4305031, 7}, {4305032, 8}, {4305033, 9}, {4305034, 40},
            {4305035, 50}, {4305036, 100}
        };
        inline void PackageOpen(SCallbackData& callback, CMainServer* main_server)
        {
            auto session = callback.session;
            auto message = callback.message;
            if (!session || !message) return;

            std::shared_lock lock(session->GetMutex());
            auto session_id = session->GetSessionId();
            auto acc_cache = main_server->GetAccCacheUniqueBySessionId(session_id);
            auto acc_index = acc_cache->acc_info.Index;
            auto extra = message->GetExtra();
            auto option = message->GetOption();
            auto mission = message->GetMission();
            if (acc_index == -1) return;
            if (acc_cache->inventory_items.size() >= acc_cache->acc_info.MaximumItems)
            {
                session->SendMsg(102, 1, Items::Package::Result::BoxInventoryFull, 0);
                return;
            }
            auto item = ItemSerialInfo(*reinterpret_cast<uint64_t*>(message->GetData()));
            const auto& item_inv = main_server->GetPlayerItemInventory(acc_cache, item);

            if (!item_inv.has_value()) return;
           
            const auto& used_item = item_inv.value();
            auto item_used_info = main_server->GetItemInfoCache(used_item.item_info.item_number.item_id);
            if (mission == 1)
            {
                const auto& usePackageReq = reinterpret_cast<MainUsePackageItemReq*>(message->GetData());
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "player ({}) used package ({})", acc_cache->acc_info.Nickname.c_str(), item_used_info->Id);
                if (used_item.item_info.item_number.item_id >= 4810000 && used_item.item_info.item_number.item_id <= 4810015) // VoiceType Card
                {
                    auto voice_index = ((used_item.item_info.item_number.item_id - 4810000) % 4) + 4;
                    auto cha_id = ((used_item.item_info.item_number.item_id - 4810000) / 4);
                    BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "player want to use voice card for character: ({}) voice index: ({})", cha_id, voice_index);
                    uint64_t unlocked_voices = acc_cache->acc_info.VoiceType;
                    if (!isVoiceUnlocked(unlocked_voices, cha_id, voice_index))
                    {
                        BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "will unlock voice");
                        setVoice(unlocked_voices, cha_id, voice_index);
                        acc_cache->acc_info.VoiceType = unlocked_voices;
                        session->SendMsg(102, 1, Items::Package::Result::VoiceUnlock, 0, reinterpret_cast<uint8_t*>(&item), sizeof(ItemSerialInfo));
                        session->SendMsg(413, 0, 59, 0, reinterpret_cast<uint8_t*>(&unlocked_voices), sizeof(unlocked_voices));
                    }
                }
                if (used_item.item_info.item_number.item_id == 4303000) // Kill-Death reset
                {
                    acc_cache->acc_info.Kills = 0;
                    acc_cache->acc_info.Deaths = 0;
                    acc_cache->acc_info.Assists = 0;
                    session->SendMsg(102, 1, Items::Package::Result::StaticItems, 0, reinterpret_cast<uint8_t*>(&item), sizeof(ItemSerialInfo));
                }
                else if (used_item.item_info.item_number.item_id == 4302000) // Record reset
                {
                    acc_cache->acc_info.Wins = 0;
                    acc_cache->acc_info.Loses = 0;
                    acc_cache->acc_info.Draws = 0;
                    session->SendMsg(102, 1, Items::Package::Result::StaticItems, 0, reinterpret_cast<uint8_t*>(&item), sizeof(ItemSerialInfo));
                }
                else if (used_item.item_info.item_number.item_id == 4305005) // Battery recharge +500
                {
                    if (acc_cache->acc_info.Energy + 500 <= acc_cache->acc_info.MaximumEnergy)
                    {
                        acc_cache->acc_info.Energy = acc_cache->acc_info.Energy + 500;
                        session->SendMsg(102, 1, Items::Package::Result::StaticItems, 0, reinterpret_cast<uint8_t*>(&item), sizeof(ItemSerialInfo));
                    }
                }
                else if (used_item.item_info.item_number.item_id == 4305006) // Battery recharge +1000
                {
                    if (acc_cache->acc_info.Energy + 1000 <= acc_cache->acc_info.MaximumEnergy)
                    {
                        acc_cache->acc_info.Energy = acc_cache->acc_info.Energy + 1000;
                        session->SendMsg(102, 1, Items::Package::Result::StaticItems, 0, reinterpret_cast<uint8_t*>(&item), sizeof(ItemSerialInfo));
                    }
                }
                else if (used_item.item_info.item_number.item_id == 4305007) // Battery expansion
                {
                    if (acc_cache->acc_info.MaximumEnergy + 1000 <= 5000)
                    {
                        acc_cache->acc_info.MaximumEnergy = acc_cache->acc_info.MaximumEnergy + 1000;
                        session->SendMsg(102, 1, Items::Package::Result::StaticItems, 0, reinterpret_cast<uint8_t*>(&item), sizeof(ItemSerialInfo));
                    }
                }
                else if (used_item.item_info.item_number.item_id == 4305000) // Inventory expansion +10
                {
                    if (acc_cache->acc_info.MaximumItems + 10 <= 1000)
                    {
                        acc_cache->acc_info.MaximumItems = acc_cache->acc_info.MaximumItems + 10;
                        session->SendMsg(102, 1, Items::Package::Result::StaticItems, 0, reinterpret_cast<uint8_t*>(&item), sizeof(ItemSerialInfo));
                    }
                }
                else if (used_item.item_info.item_number.item_id == 4305001) // Inventory expansion +20
                {
                    if (acc_cache->acc_info.MaximumItems + 20 <= 1000)
                    {
                        acc_cache->acc_info.MaximumItems = acc_cache->acc_info.MaximumItems + 20;
                        session->SendMsg(102, 1, Items::Package::Result::StaticItems, 0, reinterpret_cast<uint8_t*>(&item), sizeof(ItemSerialInfo));
                    }
                }
                else if (used_item.item_info.item_number.item_id == 4305002) // Inventory expansion +40
                {
                    if (acc_cache->acc_info.MaximumItems + 40 <= 1000)
                    {
                        acc_cache->acc_info.MaximumItems = acc_cache->acc_info.MaximumItems + 40;
                        session->SendMsg(102, 1, Items::Package::Result::StaticItems, 0, reinterpret_cast<uint8_t*>(&item), sizeof(ItemSerialInfo));
                    }
                }
                else if (used_item.item_info.item_number.item_id == 4305003) // Inventory expansion +80
                {
                    if (acc_cache->acc_info.MaximumItems + 80 <= 1000)
                    {
                        acc_cache->acc_info.MaximumItems = acc_cache->acc_info.MaximumItems + 80;
                        session->SendMsg(102, 1, Items::Package::Result::StaticItems, 0, reinterpret_cast<uint8_t*>(&item), sizeof(ItemSerialInfo));
                    }
                }
                else if (used_item.item_info.item_number.item_id == 4810105) // Unlock simon
                {
                    session->SendMsg(102, 1, Items::Package::Result::StaticItems, 0, reinterpret_cast<uint8_t*>(&item), sizeof(ItemSerialInfo));
                }
                else if (used_item.item_info.item_number.item_id == 4810106) // Unlock amelia
                {
                    session->SendMsg(102, 1, Items::Package::Result::StaticItems, 0, reinterpret_cast<uint8_t*>(&item), sizeof(ItemSerialInfo));
                }
                else if (used_item.item_info.item_number.item_id == 4810107) // Unlock sharkill
                {
                    session->SendMsg(102, 1, Items::Package::Result::StaticItems, 0, reinterpret_cast<uint8_t*>(&item), sizeof(ItemSerialInfo));
                }
                else if (used_item.item_info.item_number.item_id == 4810108) // Unlock sophitia
                {
                    session->SendMsg(102, 1, Items::Package::Result::StaticItems, 0, reinterpret_cast<uint8_t*>(&item), sizeof(ItemSerialInfo));
                }
                else
                {
                    auto package = main_server->GetPackageInfo(used_item.item_info.item_number.item_id);
                    if (package->size() > 0)
                    {
                        const auto& items_won = main_server->ExtractPackageItemsWon(package);
                        if (items_won.size() == 1 && items_won[0].ItemId > 4400000 && items_won[0].ItemId <= 4410000) // MP Item
                        {
                            auto real_mp = (items_won[0].ItemId - 4400000) * 100;
                            acc_cache->acc_info.MicroPoints = acc_cache->acc_info.MicroPoints + real_mp;
                            ShopItem new_item = { items_won[0].ItemId, ItemExpire::Type::Unused, ItemSerialInfo(0, 0, 0, 0, 0) };
                            MainCurrencyUpdateAck currency_update_data = { acc_cache->acc_info.RockTokens, acc_cache->acc_info.MicroPoints, acc_cache->acc_info.Coins };
                            session->SendMsg(307, 0x0, 0, 0, reinterpret_cast<uint8_t*>(&currency_update_data), sizeof(currency_update_data)); // currency update ack
                            session->SendMsg(102, 1, Items::Package::Result::Package, 2, reinterpret_cast<uint8_t*>(&new_item), sizeof(ShopItem));
                        }
                        else if (items_won.size() == 1 && items_won[0].ItemId > 4308000 && items_won[0].ItemId <= 4308020) // Coin Item
                        {
                            auto real_coin = items_won[0].ItemId - 4308000;
                            if (real_coin > 10) real_coin = (items_won[0].ItemId - 10) * 10 + 10;
                            if (acc_cache->acc_info.Coins + real_coin > 125)
                                session->SendMsg(102, 1, Items::Package::Result::CoinMax, 0);
                            else
                            {
                                acc_cache->acc_info.Coins = acc_cache->acc_info.Coins + real_coin;
                                ShopItem new_item = { items_won[0].ItemId, ItemExpire::Type::Unused, ItemSerialInfo(0, 0, 0, 0, 0) };
                                session->SendMsg(102, 1, Items::Package::Result::Package, 1, reinterpret_cast<uint8_t*>(&new_item), sizeof(ShopItem));
                            }
                        }
                        else if (items_won.size() == 1 && items_won[0].ItemId > 4305019 && items_won[0].ItemId <= 4305036) // Coupon Item
                        {
                            auto new_coupons = 0;
                            auto it = coupon_map.find(items_won[0].ItemId);
                            if (it != coupon_map.end()) new_coupons = it->second;
      
                            if (acc_cache->acc_info.Coupons + new_coupons <= 250)
                            {
                                acc_cache->acc_info.Coupons = acc_cache->acc_info.Coupons + new_coupons;
                                ShopItem new_item = { (items_won[0].ItemId << 23) + 0xF4240, ItemExpire::Type::Unused, ItemSerialInfo(0, 0, 0, 0, 0) };
                                session->SendMsg(102, 1, Items::Package::Result::Package, 1, reinterpret_cast<uint8_t*>(&new_item), sizeof(ShopItem));
                            }
                        }
                        else
                        {
                            std::vector<ShopItem> items_to_send;
                            for (const auto& item : items_won)
                            {
                                auto serial_index = main_server->FindLowestAvailableItemSerialInfoId(acc_cache->inventory_items);
                                auto item_info = main_server->GetItemInfoCache(item.ItemId);
                                ShopItem new_item = { {item.ItemId ,item_info->Stock } , ItemExpire::Type::Unused ,ItemSerialInfo(serial_index, 1, 1, Items::Origin::From_Game, Utility::GetUtcTimeNow()) };
                                items_to_send.push_back(new_item);
                            #if defined(RELEASE_1_0_3)
                                InventoryItemInfo inv_item_info = { {item.ItemId , item_info->Stock} ,ItemExpire::Type::Unused, new_item.serial_info, item_info->Durability, 0 };
                                main_server->AddPlayerItemInventory(acc_cache, { inv_item_info, item_info->Stock, false, 0, false });
                            #else
                                const InventoryItemInfo& inv_item_info = { item.ItemId ,ItemExpire::Type::Unused, new_item.serial_info, item_info->Durability, 0,0,0,0,0,main_server->AdjustItemType(item_info->Type) };
                                main_server->AddPlayerItemInventory(acc_cache, { inv_item_info, item_info->Stock, false, 0, false });
                            #endif
                            }
                            session->SendMsg(102, 1, Items::Package::Result::Package, items_to_send.size(), reinterpret_cast<uint8_t*>(items_to_send.data()), items_to_send.size() * sizeof(ShopItem));
                        }
                    }
                    else
                        session->SendMsg(102, 1, Items::Package::Result::Unknown1, 0, reinterpret_cast<uint8_t*>(&item), sizeof(ItemSerialInfo));
                }
            }
            else if (mission == 3)
            {
                if (extra == 53)
                {
                    const auto& changeNickReq = reinterpret_cast<MainUsePackageItemNicknameReq*>(message->GetData());
                    const auto& nickname = changeNickReq->nickname;
                    if (!Utility::IsValidNickname(changeNickReq->nickname))
                    {
                        session->SendMsg(102, 1, Items::Package::Result::ChangeNicknameFail, 0);
                        return;
                    }
                    if (BaseLib::Database->NicknameExists(nickname))
                    {
                        session->SendMsg(102, 1, Items::Package::Result::ChangeNicknameFail, 0);
                        return;
                    }
                    auto itemPackageOpenData = MainUsePackageItemAck(item.data, nickname).Serialize(Items::Package::Result::ChangeNicknameSuccess);
                    acc_cache->acc_info.Nickname = nickname;
                    session->SendMsg(102, 1, Items::Package::Result::ChangeNicknameSuccess, 0, reinterpret_cast<uint8_t*>(itemPackageOpenData.data(), itemPackageOpenData.size()));
                    //auto new_acc_info_msg = GetNewAccInfoMsg(acc_cache->acc_info, main_server, session, acc_cache->server_time);


                    MainAccountInfoAck accInfoMsg = MainAccountInfoAck();
                    auto session_id = session->GetSessionId();
                    auto auth_key = acc_cache->acc_info.AuthKey;
                    if (acc_cache->acc_info.ClanId)
                    {
                        BaseLib::ClanInfo clanInfo;
                        if (!BaseLib::Database->GetClanInfo(acc_cache->acc_info.ClanId, &clanInfo))
                        {
                            BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan,
                                                     "session id: ({}) player clan id: ({}) doesn't exist in database",
                                                     session_id, acc_cache->acc_info.ClanId);

                            accInfoMsg.ClanLogoFront = 0;
                            accInfoMsg.ClanLogoBack = 0;
                            std::strcpy(accInfoMsg.ClanName, "");
                        }
                        else
                        {
                            accInfoMsg.ClanLogoFront = clanInfo.logo_front;
                            accInfoMsg.ClanLogoBack = clanInfo.logo_back;
                            std::strcpy(accInfoMsg.ClanName, clanInfo.name.c_str());
                            if (main_server->IsClanAlready(acc_cache->acc_info.ClanId))
                            {
                                auto clan = main_server->GetClanCacheUnique(acc_cache->acc_info.ClanId);
                                clan->online_members.push_back(session_id);
                                clan.unlock();
                            }
                            else
                            {
                                Clan newClan;
                                newClan.clan_id = acc_cache->acc_info.ClanId;
                                newClan.logo_front = clanInfo.logo_front;
                                newClan.logo_back = clanInfo.logo_back;
                                newClan.clan_name = clanInfo.name;
                                newClan.online_members.push_back(session_id);
                                main_server->AddClanCache(acc_cache->acc_info.ClanId, newClan);
                            }
                        }
                        accInfoMsg.ClanContribution = acc_cache->acc_info.ClanContribution;
                        accInfoMsg.ClanWins = acc_cache->acc_info.ClanWins;
                        accInfoMsg.ClanLoses = acc_cache->acc_info.ClanLoses;
                        accInfoMsg.ClanDraws = acc_cache->acc_info.ClanDraws;
                        accInfoMsg.ClanKills = acc_cache->acc_info.ClanKills;
                        accInfoMsg.ClanDeaths = acc_cache->acc_info.ClanDeaths;
                        accInfoMsg.ClanAssists = acc_cache->acc_info.ClanAssists;
                    }
                    else
                    {
                        accInfoMsg.ClanLogoFront = 0;
                        accInfoMsg.ClanLogoBack = 0;
                        std::strcpy(accInfoMsg.ClanName, "");
                        accInfoMsg.ClanLogoFront = 0;
                        accInfoMsg.ClanLogoBack = 0;
                        accInfoMsg.ClanContribution = 0;
                        accInfoMsg.ClanWins = 0;
                        accInfoMsg.ClanLoses = 0;
                        accInfoMsg.ClanDraws = 0;
                        accInfoMsg.ClanKills = 0;
                        accInfoMsg.ClanDeaths = 0;
                        accInfoMsg.ClanAssists = 0;
                    }

                    accInfoMsg.Diorama = 0;
                    accInfoMsg.Kills = acc_cache->acc_info.Kills;
                    accInfoMsg.Deaths = acc_cache->acc_info.Deaths;
                    accInfoMsg.Assists = acc_cache->acc_info.Assists;
                    accInfoMsg.Wins = acc_cache->acc_info.Wins;
                    accInfoMsg.Loses = acc_cache->acc_info.Loses;
                    accInfoMsg.Draws = acc_cache->acc_info.Draws;
                    accInfoMsg.Melee = acc_cache->acc_info.MeleeKills;
                    accInfoMsg.Rifle = acc_cache->acc_info.RifleKills;
                    accInfoMsg.Shotgun = acc_cache->acc_info.ShotgunKills;
                    accInfoMsg.Sniper = acc_cache->acc_info.SniperKills;
                    accInfoMsg.Gatling = acc_cache->acc_info.GatlingKills;
                    accInfoMsg.Bazooka = acc_cache->acc_info.BazookaKills;
                    accInfoMsg.Grenade = acc_cache->acc_info.GrenadeKills;
                    accInfoMsg.Headshots = acc_cache->acc_info.Headshots;
                    accInfoMsg.HighestKillStreak = acc_cache->acc_info.HighestKillStreak;
                    accInfoMsg.Unknown2 = 0;
                    accInfoMsg.PlayTime = static_cast<uint32_t>(acc_cache->acc_info.PlayTime);
                    accInfoMsg.ClanId = acc_cache->acc_info.ClanId;
                    accInfoMsg.ClanPadding = 0;
                    accInfoMsg.ZombieKillPoints = acc_cache->acc_info.ZombieKills * 3;
                    accInfoMsg.Infections = acc_cache->acc_info.Infections;
                    accInfoMsg.Unknown3 = 210;
                    accInfoMsg.ServerTime = acc_cache->server_time;
                    accInfoMsg.UniqueId = NetEngine::Packets::Core::UniqueId(session->GetSessionId(), 1).data;
                    accInfoMsg.Grade = acc_cache->acc_info.Grade;
                    accInfoMsg.SelectedCharacter = acc_cache->acc_info.SelectedCharacter;
                    accInfoMsg.OwnedCharacters = 511;//all chars
                    accInfoMsg.Level = acc_cache->acc_info.Level + 1;
                #if defined(RELEASE_1_0_3)
                    accInfoMsg.Energy = 50;//frontAccount.Energy;
                    accInfoMsg.Energy2 = acc_cache->acc_info.Energy;
                    accInfoMsg.GoldenMode = acc_cache->acc_info.PCRoom;//PCROOM PC BANG PC ROOM
                    accInfoMsg.unused = 38;

                #else
                    accInfoMsg.Coins = acc_cache->acc_info.Coins;
                    accInfoMsg.Energy = acc_cache->acc_info.Energy;
                #endif


                    accInfoMsg.LuckyPoints = acc_cache->acc_info.LuckyPoints;
                    accInfoMsg.Experience = acc_cache->acc_info.Experience;
                    accInfoMsg.MicroPoints = acc_cache->acc_info.MicroPoints;
                    accInfoMsg.RockTokens = acc_cache->acc_info.RockTokens;
                    accInfoMsg.Tutorial = acc_cache->acc_info.Tutorial;
                    accInfoMsg.MaximumItems = acc_cache->acc_info.MaximumItems;
                    accInfoMsg.MaximumEnergy = acc_cache->acc_info.MaximumEnergy;
                    accInfoMsg.DailyAttempts = acc_cache->acc_info.SingleWaveDailyAttempts;
                    accInfoMsg.HighestWave = acc_cache->acc_info.SingleWaveHighestWave;
                    accInfoMsg.SinglewaveHighscore = acc_cache->acc_info.SingleWaveHighScore;
                    accInfoMsg.Unknown4 = 24;
                    accInfoMsg.Story = acc_cache->acc_info.Story;
                    accInfoMsg.Achievements[0] = acc_cache->acc_info.Achievement;
                #if defined(RELEASE_1_1_1)
                    accInfoMsg.VIPLevel = acc_cache->acc_info.VIPExperience;
                #endif
                    accInfoMsg.AccountAuthkey = auth_key;
                    //accInfoMsg.AccountId = acc_cache->acc_info.Index;

                    std::strcpy(accInfoMsg.Unused, "");
                    std::strcpy(accInfoMsg.Nickname, acc_cache->acc_info.Nickname.c_str());

                    session->SendMsg(413, 0, 1, 1, reinterpret_cast<uint8_t*>(&accInfoMsg), sizeof(MainAccountInfoAck));
                }
                else if (extra == 26)
                {
                    const auto& useHammerReq = reinterpret_cast<MainUsePackageItemHammerReq*>(callback.message->GetData());
                    const auto& mistery_inv = main_server->GetPlayerItemInventory(acc_cache, useHammerReq->mistery_capsule);
                    if (!mistery_inv.has_value()) return;
                    auto package = main_server->GetPackageInfo(used_item.item_info.item_number.item_id);
                    if (package->size() <= 0) return;
                    const auto& items_won = main_server->ExtractPackageItemsWon(package);
                    std::vector<ShopItem> items_to_send;
                    for (const auto& item : items_won)
                    {
                        auto serial_index = main_server->FindLowestAvailableItemSerialInfoId(acc_cache->inventory_items);
                        auto item_info = main_server->GetItemInfoCache(item.ItemId);
                        ShopItem new_item = { {item.ItemId , item_info->Stock } ,ItemExpire::Type::Unused, ItemSerialInfo(serial_index, 1, 1, Items::Origin::From_Game, Utility::GetUtcTimeNow()) };
                        items_to_send.push_back(new_item);
                    #if defined(RELEASE_1_0_3)
                        InventoryItemInfo inv_item_info = { {item.ItemId , item_info->Stock} ,ItemExpire::Type::Unused, new_item.serial_info, item_info->Durability, 0 };
                        main_server->AddPlayerItemInventory(acc_cache, { inv_item_info, item_info->Stock, false, 0, false });
                    #else
                        InventoryItemInfo inv_item_info = { item.ItemId ,ItemExpire::Type::Unused, new_item.serial_info, item_info->Durability, 0,0,0,0,0,main_server->AdjustItemType(item_info->Type) };
                        main_server->AddPlayerItemInventory(acc_cache, { inv_item_info, item_info->Stock, false, 0, false });
                    #endif
                    }

                    session->SendMsg(102, 1, Items::Package::Result::Capsule, items_to_send.size(), reinterpret_cast<uint8_t*>(items_to_send.data()), items_to_send.size() * sizeof(ShopItem));
                    main_server->AddPlayerItemsDeleted(acc_cache, useHammerReq->mistery_capsule);
                    auto deleteItemData = MainDeleteItemAck({ useHammerReq->mistery_capsule }).Serialize();
                    session->SendMsg(89, 0, 1, 0, reinterpret_cast<uint8_t*>(deleteItemData.data()), deleteItemData.size());
                }
            }
            main_server->AddPlayerItemsDeleted(acc_cache, item);
            auto deleteItemData = MainDeleteItemAck({ item }).Serialize();
            session->SendMsg(89, 0, 1, 0, reinterpret_cast<uint8_t*>(deleteItemData.data()), deleteItemData.size());
        }
    }
    
}