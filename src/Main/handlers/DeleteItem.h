#pragma once
namespace Game
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Main;

    namespace Handlers
    {
        inline void DeleteItem(SCallbackData& callback, CMainServer* main_server)
        {
            auto session = callback.session;
            auto message = callback.message;
            if (!session || !message) return;

            std::shared_lock lock(session->GetMutex());
            auto session_id = session->GetSessionId();
            auto acc_cache = main_server->GetAccCacheUniqueBySessionId(session_id);
            auto acc_index = acc_cache->acc_info.Index;
            const auto& deleteItemReq = reinterpret_cast<MainDeleteItemSerialInfoReq*>(callback.message->GetData());
            if (acc_index == -1) return;
            std::vector<ItemSerialInfo> items_deleted;
            for (uint32_t i = 0; i < deleteItemReq->item_count; i++)
            {
                const auto& item_deleted = main_server->GetPlayerItemInventory(acc_cache, deleteItemReq->items[i]);
                if (!item_deleted.has_value()) continue;
                auto item_info = main_server->GetItemInfoCache(item_deleted.value().item_info.item_number.item_id);
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "player ({}) item id: ({})", acc_cache->acc_info.Nickname.c_str(), item_deleted.value().item_info.item_number.item_id);
                items_deleted.push_back(deleteItemReq->items[i]);
            }
            if (items_deleted.size() > 0)
            {
                auto deleteItemData = MainDeleteItemAck(items_deleted).Serialize();
                session->SendMsg(89, 0, 1, 0, reinterpret_cast<uint8_t*>(deleteItemData.data()), deleteItemData.size());
                main_server->AddPlayerItemsDeleted(acc_cache, items_deleted);
            }
        }
    }
    
}