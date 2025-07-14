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
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "player ({}) item id: ({}) serial: ({})", acc_cache->acc_info.Nickname.c_str(), item_deleted.value().item_info.item_number.item_id, deleteItemReq->items[i].data);
                items_deleted.push_back(deleteItemReq->items[i]);
            }
            /*
            if (items_deleted.size() > 0)
            {
                auto deleteItemData = MainDeleteItemAck(items_deleted).Serialize();
                session->SendMsg(89, 0, 1, 0, reinterpret_cast<uint8_t*>(deleteItemData.data()), deleteItemData.size());
                main_server->AddPlayerItemsDeleted(acc_cache, items_deleted);
            }
            */
            acc_cache.unlock();
            if (items_deleted.size() > 0)
            {
				[[maybe_unused]] auto ignored_result = BaseLib::DbPool->submit_task([main_server,
                    session = std::move(callback.session),
                    s_id = std::move(session_id),
                    acc_id = std::move(acc_index),
                    itm_deleted = std::move(items_deleted)
                ]() mutable
                    {
                        if (!session) return;

                        //BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "serial: ({})", itm_deleted[0].data);

                        auto session_id = session->GetSessionId();
                        auto acc_cache = main_server->GetAccCacheUniqueBySessionId(session_id);
                        auto deleteItemData = MainDeleteItemAck(itm_deleted).Serialize();
                        if (BaseLib::Database->NewDeleteInventoryItems(acc_id, itm_deleted))
                        {
                            session->SendMsg(89, 0, 1, 0, reinterpret_cast<uint8_t*>(deleteItemData.data()), deleteItemData.size());
                        }
                        else
                        {
                            BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "failed to delete item of user [{}] [{}]", acc_cache->acc_info.Index, acc_cache->acc_info.Nickname.c_str());
                        }
                    }
                );
            }
        }
    }
    
}