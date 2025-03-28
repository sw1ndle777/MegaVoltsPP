#pragma once
namespace Game
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Main;

    namespace Handlers
    {
        MainAccountInfoAck GetNewAccInfoMsg(BaseLib::FrontAccount frontAccount, CMainServer* main_server, CSession* session, uint64_t server_time)
        {
            MainAccountInfoAck accInfoMsg = MainAccountInfoAck();
            auto session_id = session->GetSessionId();
            auto auth_key = frontAccount.AuthKey;
            if (frontAccount.ClanId)
            {
                BaseLib::ClanInfo clanInfo;
                if (!BaseLib::Database->GetClanInfo(frontAccount.ClanId, &clanInfo))
                {
                    BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan,
                        "session id: ({}) player clan id: ({}) doesn't exist in database",
                        session_id, frontAccount.ClanId);

                    accInfoMsg.ClanLogoFront = 0;
                    accInfoMsg.ClanLogoBack = 0;
                    std::strcpy(accInfoMsg.ClanName, "");
                }
                else
                {
                    accInfoMsg.ClanLogoFront = clanInfo.logo_front;
                    accInfoMsg.ClanLogoBack = clanInfo.logo_back;
                    std::strcpy(accInfoMsg.ClanName, clanInfo.name.c_str());
                    if (main_server->IsClanAlready(frontAccount.ClanId))
                    {
                        auto clan = main_server->GetClanCacheUnique(frontAccount.ClanId);
                        clan->online_members.push_back(session_id);
                        clan.unlock();
                    }
                    else
                    {
                        Clan newClan;
                        newClan.clan_id = frontAccount.ClanId;
                        newClan.logo_front = clanInfo.logo_front;
                        newClan.logo_back = clanInfo.logo_back;
                        newClan.clan_name = clanInfo.name;
                        newClan.online_members.push_back(session_id);
                        main_server->AddClanCache(frontAccount.ClanId, newClan);
                    }
                }
                accInfoMsg.ClanContribution = frontAccount.ClanContribution;
                accInfoMsg.ClanWins = frontAccount.ClanWins;
                accInfoMsg.ClanLoses = frontAccount.ClanLoses;
                accInfoMsg.ClanDraws = frontAccount.ClanDraws;
                accInfoMsg.ClanKills = frontAccount.ClanKills;
                accInfoMsg.ClanDeaths = frontAccount.ClanDeaths;
                accInfoMsg.ClanAssists = frontAccount.ClanAssists;
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
            accInfoMsg.Kills = frontAccount.Kills;
            accInfoMsg.Deaths = frontAccount.Deaths;
            accInfoMsg.Assists = frontAccount.Assists;
            accInfoMsg.Wins = frontAccount.Wins;
            accInfoMsg.Loses = frontAccount.Loses;
            accInfoMsg.Draws = frontAccount.Draws;
            accInfoMsg.Melee = frontAccount.MeleeKills;
            accInfoMsg.Rifle = frontAccount.RifleKills;
            accInfoMsg.Shotgun = frontAccount.ShotgunKills;
            accInfoMsg.Sniper = frontAccount.SniperKills;
            accInfoMsg.Gatling = frontAccount.GatlingKills;
            accInfoMsg.Bazooka = frontAccount.BazookaKills;
            accInfoMsg.Grenade = frontAccount.GrenadeKills;
            accInfoMsg.Headshots = frontAccount.Headshots;
            accInfoMsg.HighestKillStreak = frontAccount.HighestKillStreak;
            accInfoMsg.Unknown2 = 0;
            accInfoMsg.PlayTime = static_cast<uint32_t>(frontAccount.PlayTime);
            accInfoMsg.ClanId = frontAccount.ClanId;
            accInfoMsg.ClanPadding = 0;
            accInfoMsg.ZombieKillPoints = frontAccount.ZombieKills * 3;
            accInfoMsg.Infections = frontAccount.Infections;
            accInfoMsg.Unknown3 = 210;
            accInfoMsg.ServerTime = server_time;
            accInfoMsg.UniqueId = NetEngine::Packets::Core::UniqueId(session->GetSessionId(), 1).data;
            accInfoMsg.Grade = frontAccount.Grade;
            accInfoMsg.SelectedCharacter = frontAccount.SelectedCharacter;
            accInfoMsg.OwnedCharacters = 511;//all chars
            accInfoMsg.Level = frontAccount.Level + 1;
#if defined(RELEASE_1_0_3)
            accInfoMsg.Energy = 50;//frontAccount.Energy;
            accInfoMsg.Energy2 = frontAccount.Energy;
            accInfoMsg.GoldenMode = frontAccount.PCRoom;//PCROOM PC BANG PC ROOM
            accInfoMsg.unused = 38;

#else
            accInfoMsg.Coins = frontAccount.Coins;
            accInfoMsg.Energy = frontAccount.Energy;
#endif


            accInfoMsg.LuckyPoints = frontAccount.LuckyPoints;
            accInfoMsg.Experience = frontAccount.Experience;
            accInfoMsg.MicroPoints = frontAccount.MicroPoints;
            accInfoMsg.RockTokens = frontAccount.RockTokens;
            accInfoMsg.Tutorial = frontAccount.Tutorial;
            accInfoMsg.MaximumItems = frontAccount.MaximumItems;
            accInfoMsg.MaximumEnergy = frontAccount.MaximumEnergy;
            accInfoMsg.DailyAttempts = frontAccount.SingleWaveDailyAttempts;
            accInfoMsg.HighestWave = frontAccount.SingleWaveHighestWave;
            accInfoMsg.SinglewaveHighscore = frontAccount.SingleWaveHighScore;
            accInfoMsg.Unknown4 = 24;
            accInfoMsg.Story = frontAccount.Story;
            accInfoMsg.Achievements[0] = frontAccount.Achievement;
#if defined(RELEASE_1_1_1)
            accInfoMsg.VIPLevel = frontAccount.VIPExperience;
#endif
            accInfoMsg.AccountAuthkey = auth_key;
            //accInfoMsg.AccountId = frontAccount.Index;

            std::strcpy(accInfoMsg.Unused, "");
            std::strcpy(accInfoMsg.Nickname, frontAccount.Nickname.c_str());
            return accInfoMsg;
        }

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
            auto send_msg = [&](CSession* session, uint16_t order, uint8_t mission, uint8_t extra, uint8_t option, uint8_t* data = nullptr, uint16_t data_size = 0)
            {
                CMessage message(session->GetEncryptionKey());
                message.SetSession(session->GetSessionId());
                message.SetCommand(order, mission, extra, option);
                if (data_size > 0 && data != nullptr) message.SetData(data, data_size);
                session->Send(message);
            };
            std::shared_lock lock(callback.session->GetMutex());
            CSession* session = callback.session;
            auto session_id = session->GetSessionId();
            auto acc_cache = main_server->GetAccCacheUniqueBySessionId(session_id);
            auto acc_index = acc_cache->acc_info.Index;
            auto extra = callback.message->GetExtra();
            auto option = callback.message->GetOption();
            auto mission = callback.message->GetMission();
            if (acc_index == -1) return;
            if (acc_cache->inventory_items.size() >= acc_cache->acc_info.MaximumItems)
            {
                send_msg(session, 102, 1, Items::Package::Result::BoxInventoryFull, 0);
                return;
            }
            auto item = ItemSerialInfo(*reinterpret_cast<uint64_t*>(callback.message->GetData()));
            const auto& item_inv = main_server->GetPlayerItemInventory(acc_cache, item);

            if (!item_inv.has_value()) return;
           
            const auto& used_item = item_inv.value();
            auto item_used_info = main_server->GetItemInfoCache(used_item.item_info.item_number.item_id);
            if (mission == 1)
            {
                const auto& usePackageReq = reinterpret_cast<MainUsePackageItemReq*>(callback.message->GetData());
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
                        send_msg(session, 102, 1, Items::Package::Result::VoiceUnlock, 0, reinterpret_cast<uint8_t*>(&item), sizeof(ItemSerialInfo));
                        send_msg(session, 413, 0, 59, 0, reinterpret_cast<uint8_t*>(&unlocked_voices), sizeof(unlocked_voices));
                    }
                }
                if (used_item.item_info.item_number.item_id == 4303000) // Kill-Death reset
                {
                    acc_cache->acc_info.Kills = 0;
                    acc_cache->acc_info.Deaths = 0;
                    acc_cache->acc_info.Assists = 0;
                    send_msg(session, 102, 1, Items::Package::Result::StaticItems, 0, reinterpret_cast<uint8_t*>(&item), sizeof(ItemSerialInfo));
                }
                else if (used_item.item_info.item_number.item_id == 4302000) // Record reset
                {
                    acc_cache->acc_info.Wins = 0;
                    acc_cache->acc_info.Loses = 0;
                    acc_cache->acc_info.Draws = 0;
                    send_msg(session, 102, 1, Items::Package::Result::StaticItems, 0, reinterpret_cast<uint8_t*>(&item), sizeof(ItemSerialInfo));
                }
                else if (used_item.item_info.item_number.item_id == 4305005) // Battery recharge +500
                {
                    if (acc_cache->acc_info.Energy + 500 <= acc_cache->acc_info.MaximumEnergy)
                    {
                        acc_cache->acc_info.Energy = acc_cache->acc_info.Energy + 500;
                        send_msg(session, 102, 1, Items::Package::Result::StaticItems, 0, reinterpret_cast<uint8_t*>(&item), sizeof(ItemSerialInfo));
                    }
                }
                else if (used_item.item_info.item_number.item_id == 4305006) // Battery recharge +1000
                {
                    if (acc_cache->acc_info.Energy + 1000 <= acc_cache->acc_info.MaximumEnergy)
                    {
                        acc_cache->acc_info.Energy = acc_cache->acc_info.Energy + 1000;
                        send_msg(session, 102, 1, Items::Package::Result::StaticItems, 0, reinterpret_cast<uint8_t*>(&item), sizeof(ItemSerialInfo));
                    }
                }
                else if (used_item.item_info.item_number.item_id == 4305007) // Battery expansion
                {
                    if (acc_cache->acc_info.MaximumEnergy + 1000 <= 5000)
                    {
                        acc_cache->acc_info.MaximumEnergy = acc_cache->acc_info.MaximumEnergy + 1000;
                        send_msg(session, 102, 1, Items::Package::Result::StaticItems, 0, reinterpret_cast<uint8_t*>(&item), sizeof(ItemSerialInfo));
                    }
                }
                else if (used_item.item_info.item_number.item_id == 4305000) // Inventory expansion +10
                {
                    if (acc_cache->acc_info.MaximumItems + 10 <= 1000)
                    {
                        acc_cache->acc_info.MaximumItems = acc_cache->acc_info.MaximumItems + 10;
                        send_msg(session, 102, 1, Items::Package::Result::StaticItems, 0, reinterpret_cast<uint8_t*>(&item), sizeof(ItemSerialInfo));
                    }
                }
                else if (used_item.item_info.item_number.item_id == 4305001) // Inventory expansion +20
                {
                    if (acc_cache->acc_info.MaximumItems + 20 <= 1000)
                    {
                        acc_cache->acc_info.MaximumItems = acc_cache->acc_info.MaximumItems + 20;
                        send_msg(session, 102, 1, Items::Package::Result::StaticItems, 0, reinterpret_cast<uint8_t*>(&item), sizeof(ItemSerialInfo));
                    }
                }
                else if (used_item.item_info.item_number.item_id == 4305002) // Inventory expansion +40
                {
                    if (acc_cache->acc_info.MaximumItems + 40 <= 1000)
                    {
                        acc_cache->acc_info.MaximumItems = acc_cache->acc_info.MaximumItems + 40;
                        send_msg(session, 102, 1, Items::Package::Result::StaticItems, 0, reinterpret_cast<uint8_t*>(&item), sizeof(ItemSerialInfo));
                    }
                }
                else if (used_item.item_info.item_number.item_id == 4305003) // Inventory expansion +80
                {
                    if (acc_cache->acc_info.MaximumItems + 80 <= 1000)
                    {
                        acc_cache->acc_info.MaximumItems = acc_cache->acc_info.MaximumItems + 80;
                        send_msg(session, 102, 1, Items::Package::Result::StaticItems, 0, reinterpret_cast<uint8_t*>(&item), sizeof(ItemSerialInfo));
                    }
                }
                else if (used_item.item_info.item_number.item_id == 4810105) // Unlock simon
                {
                    send_msg(session, 102, 1, Items::Package::Result::StaticItems, 0, reinterpret_cast<uint8_t*>(&item), sizeof(ItemSerialInfo));
                }
                else if (used_item.item_info.item_number.item_id == 4810106) // Unlock amelia
                {
                    send_msg(session, 102, 1, Items::Package::Result::StaticItems, 0, reinterpret_cast<uint8_t*>(&item), sizeof(ItemSerialInfo));
                }
                else if (used_item.item_info.item_number.item_id == 4810107) // Unlock sharkill
                {
                    send_msg(session, 102, 1, Items::Package::Result::StaticItems, 0, reinterpret_cast<uint8_t*>(&item), sizeof(ItemSerialInfo));
                }
                else if (used_item.item_info.item_number.item_id == 4810108) // Unlock sophitia
                {
                    send_msg(session, 102, 1, Items::Package::Result::StaticItems, 0, reinterpret_cast<uint8_t*>(&item), sizeof(ItemSerialInfo));
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
                            send_msg(session, 307, 0x0, 0, 0, reinterpret_cast<uint8_t*>(&currency_update_data), sizeof(currency_update_data)); // currency update ack
                            send_msg(session, 102, 1, Items::Package::Result::Package, 2, reinterpret_cast<uint8_t*>(&new_item), sizeof(ShopItem));
                        }
                        else if (items_won.size() == 1 && items_won[0].ItemId > 4308000 && items_won[0].ItemId <= 4308020) // Coin Item
                        {
                            auto real_coin = items_won[0].ItemId - 4308000;
                            if (real_coin > 10) real_coin = (items_won[0].ItemId - 10) * 10 + 10;
                            if (acc_cache->acc_info.Coins + real_coin > 125)
                                send_msg(session, 102, 1, Items::Package::Result::CoinMax, 0);
                            else
                            {
                                acc_cache->acc_info.Coins = acc_cache->acc_info.Coins + real_coin;
                                ShopItem new_item = { items_won[0].ItemId, ItemExpire::Type::Unused, ItemSerialInfo(0, 0, 0, 0, 0) };
                                send_msg(session, 102, 1, Items::Package::Result::Package, 1, reinterpret_cast<uint8_t*>(&new_item), sizeof(ShopItem));
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
                                send_msg(session, 102, 1, Items::Package::Result::Package, 1, reinterpret_cast<uint8_t*>(&new_item), sizeof(ShopItem));
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
                            send_msg(session, 102, 1, Items::Package::Result::Package, items_to_send.size(), reinterpret_cast<uint8_t*>(items_to_send.data()), items_to_send.size() * sizeof(ShopItem));
                        }
                    }
                    else
                        send_msg(session, 102, 1, Items::Package::Result::Unknown1, 0, reinterpret_cast<uint8_t*>(&item), sizeof(ItemSerialInfo));
                }
            }
            else if (mission == 3)
            {
                if (extra == 53)
                {
                    const auto& changeNickReq = reinterpret_cast<MainUsePackageItemNicknameReq*>(callback.message->GetData());
                    const auto& nickname = changeNickReq->nickname;
                    if (!Utility::IsValidNickname(changeNickReq->nickname))
                    {
                        send_msg(session, 102, 1, Items::Package::Result::ChangeNicknameFail, 0);
                        return;
                    }
                    if (BaseLib::Database->NicknameExists(nickname))
                    {
                        send_msg(session, 102, 1, Items::Package::Result::ChangeNicknameFail, 0);
                        return;
                    }
                    auto itemPackageOpenData = MainUsePackageItemAck(item.data, nickname).Serialize(Items::Package::Result::ChangeNicknameSuccess);
                    acc_cache->acc_info.Nickname = nickname;
                    send_msg(session, 102, 1, Items::Package::Result::ChangeNicknameSuccess, 0, reinterpret_cast<uint8_t*>(itemPackageOpenData.data(), itemPackageOpenData.size()));
                    auto new_acc_info_msg = GetNewAccInfoMsg(acc_cache->acc_info, main_server, session, acc_cache->server_time);
                    send_msg(session, 413, 0, 1, 1, reinterpret_cast<uint8_t*>(&new_acc_info_msg), sizeof(MainAccountInfoAck));
                    /*
                    asio::post([main_server, session_id, item, nickname, session, send_msg]()
                    {
                        
                    });
                    */
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
                        const InventoryItemInfo& inv_item_info = { item.ItemId ,ItemExpire::Type::Unused, new_item.serial_info, item_info->Durability, 0,0,0,0,0,main_server->AdjustItemType(item_info->Type) };
                        main_server->AddPlayerItemInventory(acc_cache, { inv_item_info, item_info->Stock, false, 0, false });
                    #endif
                    }

                    send_msg(session, 102, 1, Items::Package::Result::Capsule, items_to_send.size(), reinterpret_cast<uint8_t*>(items_to_send.data()), items_to_send.size() * sizeof(ShopItem));
                    main_server->AddPlayerItemsDeleted(acc_cache, useHammerReq->mistery_capsule);
                    auto deleteItemData = MainDeleteItemAck({ useHammerReq->mistery_capsule }).Serialize();
                    send_msg(session, 89, 0, 1, 0, reinterpret_cast<uint8_t*>(deleteItemData.data()), deleteItemData.size());
                }
            }
            main_server->AddPlayerItemsDeleted(acc_cache, item);
            auto deleteItemData = MainDeleteItemAck({ item }).Serialize();
            send_msg(session, 89, 0, 1, 0, reinterpret_cast<uint8_t*>(deleteItemData.data()), deleteItemData.size());
        }
    }
    
}