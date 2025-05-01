#pragma once
namespace Game
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Main;

    namespace Handlers
    {
        inline void RepairItem(SCallbackData& callback, CMainServer* main_server)
        {
            auto session = callback.session;
            auto message = callback.message;
            if (!session || !message) return;
            std::shared_lock lock(session->GetMutex());

            auto session_id = session->GetSessionId();
            auto acc_cache = main_server->GetAccCacheUniqueBySessionId(session_id);
            auto acc_index = acc_cache->acc_info.Index;
            auto items_count = static_cast<uint32_t>(message->GetOption());
            const auto& repairItemReq = reinterpret_cast<MainRepairItemSerialInfoReq*>(message->GetData());
            if (acc_index == -1) return;
            std::vector<ItemSerialInfo> items_updated;
            std::vector<uint32_t> items_durabilities;
            uint32_t total_repair_price = 0;
            
            for (uint32_t i = 0; i < items_count; i++)
            {
                const auto& item_repair = main_server->GetPlayerItemInventory(acc_cache, repairItemReq->items[i]);
                if (!item_repair.has_value()) continue;
                auto item_info = main_server->GetItemInfoCache(item_repair.value().item_info.item_number.item_id);
                total_repair_price = total_repair_price + (item_info->Durability - item_repair.value().item_info.repair);
                items_updated.push_back(repairItemReq->items[i]);
                items_durabilities.push_back(item_info->Durability);
            }
            if (items_updated.empty() || acc_cache->acc_info.MicroPoints < total_repair_price) return;
            if (!main_server->UpdatePlayerItemsRepair(acc_cache, items_updated, items_durabilities)) return;
            acc_cache->acc_info.MicroPoints = acc_cache->acc_info.MicroPoints - total_repair_price;
            auto repairItemAckData = MainRepairItemAck(acc_cache->acc_info.MicroPoints, acc_cache->acc_info.RockTokens, items_updated).Serialize();
            session->SendMsg(97, 0, 1, items_updated.size(), reinterpret_cast<uint8_t*>(repairItemAckData.data()), repairItemAckData.size());
        }
    }
}