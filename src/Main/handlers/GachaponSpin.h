#pragma once
namespace Game
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Main;

    namespace Handlers
    {
        inline void GachaponSpin(SCallbackData& callback, CMainServer* main_server)
        {
            auto send_msg = [&](CSession* session, std::uint16_t order, std::uint8_t mission, std::uint8_t extra, std::uint8_t option, std::uint8_t* data = nullptr, std::uint16_t data_size = 0)
            {
                CMessage message(session->GetEncryptionKey());
                message.SetSession(session->GetSessionId());
                message.SetCommand(order, mission, extra, option);
                if (data_size > 0 && data != nullptr) message.SetData(data, data_size);
                session->Send(message);
            };
            std::shared_lock lock(callback.session->GetMutex());
            CSession* session = callback.session;
            CServer* server = callback.server;
            auto session_id = session->GetSessionId();
            auto acc_cache = main_server->GetAccCacheUniqueBySessionId(session_id);
            auto acc_index = acc_cache->acc_info.Index;
            auto spin_count = callback.message->GetOption();
            auto gachaponSpinReq = reinterpret_cast<MainGachaponSpinReq*>(callback.message->GetData());
            if (acc_index == -1) return;
            auto gachapon_info = main_server->GetGachaponInfo(gachaponSpinReq->gachapon_id);
            if (gachapon_info->Id == -1 || acc_cache->acc_info.Level < gachapon_info->LimitedGrade)
            {
                send_msg(session, 92, 0, Items::Gachapon::Spin::Result::Stuck, 0);
                return;
            }
            std::vector<GachaponPackageItem> items_extracted;
            std::uint32_t fake_lucky_points = acc_cache->acc_info.LuckyPoints;
            std::uint32_t coupon_chance = (gachapon_info->Type == Items::Gachapon::Type::RT) ? 30 : (gachapon_info->Type == Items::Gachapon::Type::MP) ? 15 : 0;
            std::uint32_t money_spent = 0;
            
            gachapon_info.unlock();
            std::vector<GachaponPackageItem> lucky_items;
            auto lucky_gachapon_info = main_server->GetGachaponInfo(21);//main_server->GetLuckyGachaponInfo();
            main_server->ExtractGachaponItemsWon(lucky_gachapon_info, lucky_items, coupon_chance);
            lucky_gachapon_info.unlock();
            gachapon_info.lock();

            auto is_gachapon_sale = main_server->IsGachaponSaleInfoAlready(gachapon_info->Id);
            auto gachapon_sale_info = main_server->GetGachaponSaleCacheShared(gachapon_info->Id);
            std::uint32_t gachapon_price = 0;
            if (is_gachapon_sale)
            {
                if (gachapon_sale_info->start_date <= Utility::GetUtcTimeNow() && gachapon_sale_info->end_date >= Utility::GetUtcTimeNow())
                    gachapon_price = gachapon_sale_info->sale_price;
                else
                {
                    main_server->RemoveGachaponSaleCache(gachapon_info->Id);
                    gachapon_price = gachapon_info->Price;
                }
            }
            else
                gachapon_price = gachapon_info->Price;

            if (gachapon_price == 0)
            {
                send_msg(session, 92, 0, Items::Gachapon::Spin::Result::Stuck, 0);
                return;
            }

            std::vector<std::vector<GachaponPackageItem>> extracted_items;
            for (std::uint32_t i = 0; i < spin_count; i++)
            {
               
                if (fake_lucky_points >= 1000 && spin_count == 1)
                {             
                    extracted_items.push_back(lucky_items);
                    fake_lucky_points = 0;
                }
                else if (fake_lucky_points < 1000 && spin_count == 1)
                {
                    std::vector<GachaponPackageItem> items;

                    if (main_server->ExtractGachaponItemsWon(gachapon_info, items, coupon_chance))
                        items.push_back(GachaponPackageItem((acc_cache->acc_info.Coupons + 1 > 250) ? 4306001 : 4305019));

                    fake_lucky_points += gachapon_info->LuckyPoint;
                    money_spent += gachapon_price;
                    extracted_items.push_back(items);
                    
                }
                else
                {
                    std::vector<GachaponPackageItem> items;
                    if (main_server->ExtractGachaponItemsWon(gachapon_info, items, coupon_chance))
                        items.push_back(GachaponPackageItem((acc_cache->acc_info.Coupons + 1 > 250) ? 4306001 : 4305019));

                    
                    fake_lucky_points += gachapon_info->LuckyPoint;
                    money_spent += gachapon_price;
                    extracted_items.push_back(items);
                    

                    if (fake_lucky_points >= 1000 && i < spin_count - 1)
                    {
                        extracted_items.push_back(lucky_items);
                        fake_lucky_points = 0;
                    }
                }
            }


            const auto& items_count = std::accumulate(extracted_items.begin(), extracted_items.end(), static_cast<size_t>(0),
                [](size_t sum, const std::vector<GachaponPackageItem>& innerVec) {
                    return sum + innerVec.size();
                });

            if(acc_cache->inventory_items.size() + items_count > acc_cache->acc_info.MaximumItems)
            {
                send_msg(session, 92, 0, Items::Gachapon::Spin::Result::InventoryFull, 0);
                return;
            }

            auto insufficient_coin = gachapon_info->Type == Items::Gachapon::Type::Coin && acc_cache->acc_info.Coins < money_spent;
            auto insufficient_rt = gachapon_info->Type == Items::Gachapon::Type::RT && acc_cache->acc_info.RockTokens < money_spent;
            auto insufficient_mp = gachapon_info->Type == Items::Gachapon::Type::MP && acc_cache->acc_info.MicroPoints < money_spent;
            if(insufficient_coin || insufficient_rt || insufficient_mp)
            {
                send_msg(session, 92, 0, Items::Gachapon::Spin::Result::MoneyError, (insufficient_coin) ? Items::Gachapon::Error::NoCoin : (insufficient_rt) ? Items::Gachapon::Error::NoRT : Items::Gachapon::Error::NoMP);
                return;
            }
            switch (gachapon_info->Type) {
                case 0://COIN
                    //??
                break;
                case 1://CASH
                    BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "will update cash had: ({}) and spent: ({})", acc_cache->acc_info.RockTokens, money_spent);
                    acc_cache->acc_info.RockTokens -= money_spent;
                break;
                case 2://POINT
                    BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "will update point had: ({}) and spent: ({})", acc_cache->acc_info.MicroPoints, money_spent);
                    acc_cache->acc_info.MicroPoints -= money_spent;
                break;
            }
            std::vector<std::string> lucky_items_announce;
            for (const auto& item : extracted_items)
            {
                auto lucky_type = (item.size() > 0) ? item[0].LuckyType : 0;
                std::vector<ShopItem> items_to_send;
                for (const auto& extracted : item)
                {
                    auto is_rare = extracted.ItemType == Items::Gachapon::Rarity::Rare;
                    auto item_info = main_server->GetItemInfoCache(extracted.ItemId);
                    BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "player ({}) won from capsule {} item: ({} - {})", acc_cache->acc_info.Nickname.c_str(), is_rare ? "rare" : "normal", item_info->Name.c_str(), item_info->NameTime.c_str());

                    if (extracted.LuckyType == Items::Gachapon::LuckyType::CopperLucky)
                    {
                        acc_cache->acc_info.MicroPoints += 1000;
                        items_to_send.push_back({ {extracted.ItemId , item_info->Stock} ,ItemExpire::Type::Unused, ItemSerialInfo(0, 0, 0, 0, 0) });
                        continue;
                    }
                    if (extracted.ItemId == 4305019)
                    {
                        acc_cache->acc_info.Coupons += 1;
                        items_to_send.push_back({ {extracted.ItemId , item_info->Stock} ,ItemExpire::Type::Unused, ItemSerialInfo(0, 0, 0, 0, 0) });
                    #if defined(RELEASE_1_0_3)
                        InventoryItemInfo inv_item_info = { {extracted.ItemId , item_info->Stock} ,ItemExpire::Type::Unused, ItemSerialInfo(0, 0, 0, 0, 0), item_info->Durability, 0 };
                        main_server->AddPlayerItemInventory(acc_cache, { inv_item_info, item_info->Stock, false, 0, false });
                    #else
                        const InventoryItemInfo& inv_item_info = { extracted.ItemId ,ItemExpire::Type::Unused, ItemSerialInfo(0, 0, 0, 0, 0), item_info->Durability, 0,0,0,0,0,main_server->AdjustItemType(item_info->Type) };
                        main_server->AddPlayerItemInventory(acc_cache, { inv_item_info, item_info->Stock, false, 0, false });
                    #endif

                    }
                    else
                    {
                        auto serial_index = main_server->FindLowestAvailableItemSerialInfoId(acc_cache->inventory_items);
                        auto serial_info = ItemSerialInfo(serial_index, 1, 1, Items::Origin::From_Game, Utility::GetUtcTimeNow());
                        items_to_send.push_back({ {extracted.ItemId , item_info->Stock} ,ItemExpire::Type::Unused, serial_info });
                    #if defined(RELEASE_1_0_3)
                        InventoryItemInfo inv_item_info = { {extracted.ItemId , item_info->Stock} ,ItemExpire::Type::Unused, serial_info, item_info->Durability, 0 };
                        main_server->AddPlayerItemInventory(acc_cache, { inv_item_info, item_info->Stock, false, 0, false });
                    #else
                        const InventoryItemInfo& inv_item_info = { extracted.ItemId ,ItemExpire::Type::Unused, serial_info, item_info->Durability, 0,0,0,0,0,main_server->AdjustItemType(item_info->Type) };
                        main_server->AddPlayerItemInventory(acc_cache, { inv_item_info, item_info->Stock, false, 0, false });
                    #endif
                        if (is_rare)
                        {
                            //GachaponWonItemMsg itemWon = { gachaponSpinReq->gachapon_id, extracted.ItemId };
                            //GachaponAnnouncement announcement = { itemWon, acc_cache->acc_info.Nickname.c_str() };
                            lucky_items_announce.push_back(item_info->Name);
                        }



                    }
                }
                if (lucky_type > Items::Gachapon::LuckyType::NoLucky)
                {
                    acc_cache->acc_info.LuckyPoints = 0;
                    send_msg(session, 92, Items::Gachapon::Spin::Type::LuckySpin, Items::Gachapon::Spin::Result::SpinSuccess, static_cast<std::uint8_t>(items_to_send.size()), reinterpret_cast<uint8_t*>(items_to_send.data()), items_to_send.size() * sizeof(ShopItem));
                }
                else
                {
                    acc_cache->acc_info.LuckyPoints += gachapon_info->LuckyPoint;
                    send_msg(session, 92, Items::Gachapon::Spin::Type::NormalSpin, Items::Gachapon::Spin::Result::SpinSuccess, static_cast<std::uint8_t>(items_to_send.size()), reinterpret_cast<uint8_t*>(items_to_send.data()), items_to_send.size() * sizeof(ShopItem));
                }
            }
            acc_cache.unlock();
            std::shared_lock acc_lock(main_server->GetAccountsCacheMutex());
            for (const auto& lobby_player : accounts_cache)
            {
                auto lobby_player_session_id = lobby_player.first;
                auto lobby_player_acc_id = lobby_player.second.acc_info.Index;
                if (lobby_player_acc_id == acc_index) continue;

                if (auto player_session = server->GetSessionById(lobby_player_session_id))
                    for (auto& announcement : lucky_items_announce)
                        main_server->SendServerMessage(player_session.get(), std::format("[{}] won a [{}] item from the capsule machine.", acc_cache->acc_info.Nickname, announcement.c_str()).c_str());
                        //send_msg(player_session.get(), 402, Announcement::Gacha::RareNotice, Announcement::Chat::Type::GameMessage, lucky_items_announce.size(), reinterpret_cast<uint8_t*>(&announcement), sizeof(announcement));
            }
        }
    }
}