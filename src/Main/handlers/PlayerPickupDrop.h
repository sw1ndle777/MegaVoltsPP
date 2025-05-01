#pragma once
namespace Game
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Main;

    namespace Handlers
    {

        //std::unordered_map<uint32_t, uint32_t> coupon_map =
        boost::unordered_flat_set<uint32_t> drop_item_ids =
        {
            4510015, // mistery capsule
            4510019 // golden mistery capsule
        };
        inline void PlayerPickupDrop(SCallbackData& callback, CMainServer* main_server)
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
                //session->SendMsg(102, 1, Items::Package::Result::BoxInventoryFull, 0);
                return;
            }

            if (mission == 1)
            {
                const auto& pickupDropReq = reinterpret_cast<MainPickupItemreq*>(message->GetData());
                auto drop_item_id = pickupDropReq->drop_item_id;
                if (drop_item_ids.find(drop_item_id) == drop_item_ids.end()) return;
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "player ({}) picked up drop item ({})", acc_cache->acc_info.Nickname.c_str(), drop_item_id);
              
                auto serial_index = main_server->FindLowestAvailableItemSerialInfoId(acc_cache->inventory_items);
                auto item_info = main_server->GetItemInfoCache(drop_item_id);
                ShopItem new_item = { {drop_item_id ,item_info->Stock } , ItemExpire::Type::Unused ,ItemSerialInfo(serial_index, 1, 1, Items::Origin::From_Game, Utility::GetUtcTimeNow()) };

                InventoryItemInfo inv_item_info = { {drop_item_id , item_info->Stock} ,ItemExpire::Type::Unused, new_item.serial_info, item_info->Durability, 0 };
                main_server->AddPlayerItemInventory(acc_cache, { inv_item_info, item_info->Stock, false, 0, false });
                session->SendMsg(102, 1, Items::Package::Result::StaticItems, 1, reinterpret_cast<uint8_t*>(&new_item), sizeof(ShopItem));

            }
        }
    }
}