#pragma once
namespace Game
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Main;

    namespace Handlers
    {
        inline void NormalShop(SCallbackData& callback, CMainServer* main_server)
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
            uint32_t items_count = static_cast<uint32_t>(callback.message->GetOption());
            auto session_id = session->GetSessionId();
            auto acc_cache = main_server->GetAccCacheUniqueBySessionId(session_id);
            auto acc_index = acc_cache->acc_info.Index;
            if (acc_index == -1) return;

            BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "player ({}) normal shop -> mission: ({}), extra: ({}), option: ({})", acc_cache->acc_info.Nickname.c_str(), callback.message->GetMission(), callback.message->GetExtra(), callback.message->GetOption());

            if (callback.message->GetMission() == 1) // rebuy expired item
            {
                const auto& buyItemIdReq = reinterpret_cast<MainBuyItemSerialInfoReq*>(callback.message->GetData());
                std::vector<ShopSerialInfo> items;
                for (uint32_t i = 0; i < items_count; i++)
                {
                    const auto& item_bought = buyItemIdReq->items[i];
                    if (main_server->IsItemInShop(item_bought.id))
                    {
                        auto item_info = main_server->GetItemInfoCache(item_bought.id);
                        items.push_back({ item_bought , Utility::GetUtcTimeNowPlusSeconds(item_info->LimitedTime) });
                        BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "player ({}) bought item id: ({})", acc_cache->acc_info.Nickname.c_str(), item_info->Id);
                    }
                }
                if (items.size() > 0)
                    send_msg(session, callback.message->GetOrder(), callback.message->GetMission(), callback.message->GetExtra(), static_cast<uint8_t>(items.size()), reinterpret_cast<uint8_t*>(items.data()), items.size() * sizeof(ShopSerialInfo));

            }
            else
            {
                const auto& buyItemIdReq = reinterpret_cast<MainBuyItemIdReq*>(callback.message->GetData());
                std::vector<ShopItem> shop_items;
                uint32_t rt_spent = 0;
                uint32_t mp_spent = 0;
                for (uint32_t i = 0; i < items_count; i++)
                {
                    const auto& item_bought_id = buyItemIdReq->items[i] & 0x7FFFFF;
                    BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "player ({}) request buy id: ({})", acc_cache->acc_info.Nickname.c_str(), item_bought_id);
                   
                    if (main_server->IsItemInShop(item_bought_id))
                    {
                        auto item_info = main_server->GetItemInfoCache(item_bought_id);
                        auto serial_index = main_server->FindLowestAvailableItemSerialInfoId(acc_cache->inventory_items);
                        ShopItem new_item = { {item_bought_id , item_info->Stock} , ItemExpire::Type::Unused,  ItemSerialInfo(serial_index, 1, 1, Items::Origin::From_Game, Utility::GetUtcTimeNow()) };
                        shop_items.push_back(new_item);
                    #if defined(RELEASE_1_0_3)
                        const InventoryItemInfo& inv_item_info = { {item_bought_id , item_info->Stock } ,ItemExpire::Type::Unused, new_item.serial_info, item_info->Durability, 0 };
                        main_server->AddPlayerItemInventory(acc_cache, { inv_item_info,item_info->Stock, false, 0, false });
                    #else
                        const InventoryItemInfo& inv_item_info = { item_bought_id ,ItemExpire::Type::Unused, new_item.serial_info, item_info->Durability, 0,0,0,0,0,main_server->AdjustItemType(item_info->Type) };
                        main_server->AddPlayerItemInventory(acc_cache, { inv_item_info,item_info->Stock, false, 0, false });
                    #endif

                        rt_spent = rt_spent + item_info->CashPrice;
                        mp_spent = mp_spent + item_info->PointPrice;
                        BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "player ({}) bought item id: ({}), new serial: ({})", acc_cache->acc_info.Nickname.c_str(), item_info->Id, new_item.serial_info.data);
                    }
                    else
                        BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "player ({}) request buy id: ({}) failed because it doesn't exist in vendor info", acc_cache->acc_info.Nickname.c_str(), item_bought_id);
                }
                if (rt_spent <= acc_cache->acc_info.RockTokens && mp_spent <= acc_cache->acc_info.MicroPoints)
                {
                    if (shop_items.size() > 0)
                    {
                        acc_cache->acc_info.RockTokens = acc_cache->acc_info.RockTokens - rt_spent;
                        acc_cache->acc_info.MicroPoints = acc_cache->acc_info.MicroPoints - mp_spent;
                        
                        
                        send_msg(session, callback.message->GetOrder(), 0, 0, static_cast<uint8_t>(shop_items.size()), reinterpret_cast<uint8_t*>(shop_items.data()), shop_items.size() * sizeof(ShopItem)); // buy item ack
                        //send_msg(session, 99, 0, 37, shop_items.size(), reinterpret_cast<uint8_t*>(shop_items.data()), shop_items.size() * sizeof(ShopItem)); // update inventory items ack
                        MainCurrencyUpdateAck currency_update_data{ acc_cache->acc_info.RockTokens, acc_cache->acc_info.MicroPoints, acc_cache->acc_info.Coins };
                        send_msg(session, 307, 0, 1, 0, reinterpret_cast<uint8_t*>(&currency_update_data), sizeof(currency_update_data)); // update currency ack
                        BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "player ({}) spent ({}) mp and ({}) rt", acc_cache->acc_info.Nickname.c_str(), mp_spent, rt_spent);
                    }
                }

            }
        }
        inline void CouponShop(SCallbackData& callback, CMainServer* main_server)
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
            const uint32_t items_count = static_cast<uint32_t>(callback.message->GetOption());
            auto session_id = session->GetSessionId();
            auto acc_cache = main_server->GetAccCacheUniqueBySessionId(session_id);
            auto acc_index = acc_cache->acc_info.Index;
            const auto& buyItemIdReq = reinterpret_cast<MainBuyItemIdReq*>(callback.message->GetData());
            if (acc_index == -1) return;
            BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "player ({}) coupon shop -> mission: ({}), extra: ({}), option: ({})", acc_cache->acc_info.Nickname.c_str(), callback.message->GetMission(), callback.message->GetExtra(), callback.message->GetOption());
            std::vector<ShopItem> items;
            uint32_t coupon_spent = 0;
            for (uint32_t i = 0; i < items_count; i++)
            {
                const auto& item_bought_id = buyItemIdReq->items[i] & 0x7FFFFF;
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "player ({}) request buy id: ({})", acc_cache->acc_info.Nickname.c_str(), item_bought_id);
                if (main_server->IsItemInShop(item_bought_id))
                {
                    auto item_info = main_server->GetItemInfoCache(item_bought_id);
                    auto serial_index = main_server->FindLowestAvailableItemSerialInfoId(acc_cache->inventory_items);
                    ShopItem new_item = { {item_bought_id , item_info->Stock } , ItemExpire::Type::Unused, ItemSerialInfo(serial_index, 1, 1, Items::Origin::From_Game, Utility::GetUtcTimeNow()) };
                    items.push_back(new_item);
                #if defined(RELEASE_1_0_3)
                    const InventoryItemInfo& inv_item_info = { {item_bought_id , item_info->Stock } ,ItemExpire::Type::Unused, new_item.serial_info, item_info->Durability, 0 };
                    main_server->AddPlayerItemInventory(acc_cache, { inv_item_info,item_info->Stock, false, 0, false });
                #else
                    const InventoryItemInfo& inv_item_info = { item_bought_id ,ItemExpire::Type::Unused, new_item.serial_info, item_info->Durability, 0,0,0,0,0,main_server->AdjustItemType(item_info->Type) };
                    main_server->AddPlayerItemInventory(acc_cache, { inv_item_info,item_info->Stock, false, 0, false });
                #endif
                    coupon_spent = coupon_spent + item_info->CouponPrice;
                    BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "player ({}) bought item id: ({}), serial: ({})", acc_cache->acc_info.Nickname.c_str(), item_info->Id, new_item.serial_info.data);
                }
                else
                    BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "player ({}) request buy id: ({}) failed because it doesn't exist in vendor info", acc_cache->acc_info.Nickname.c_str(), item_bought_id);
            }
            if (coupon_spent <= acc_cache->acc_info.Coupons)
            {
                if (items.size() > 0)
                {
                    //auto deleteItemData = MainDeleteItemAck({ItemSerialInfo(0, 0, 0, 0, 0) }).Serialize();
                    //send_msg(session, 89, 0, 1, 0, reinterpret_cast<uint8_t*>(deleteItemData.data()), deleteItemData.size());//delete coupons

                    acc_cache->acc_info.Coupons = acc_cache->acc_info.Coupons - coupon_spent;
                    
                    send_msg(session, callback.message->GetOrder(), 0, 1, items.size(), reinterpret_cast<uint8_t*>(items.data()), items.size() * sizeof(ShopItem)); // buy item ack
                    BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "player ({}) spent ({}) coupons", acc_cache->acc_info.Nickname.c_str(), coupon_spent);

                    
                }
            }
        }
        inline void BuyItem(SCallbackData& callback, CMainServer* main_server)
        {
            const auto& order = callback.message->GetOrder();
            switch (order)
            {
                case 87: NormalShop(callback, main_server); break;
                case 91: CouponShop(callback, main_server); break;
            }
        }
    }
    
}