#pragma once
namespace Game
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Main;

    namespace Handlers
    {
        inline void SellItem(SCallbackData& callback, CMainServer* main_server)
        {
            auto session = callback.session;
            auto message = callback.message;
            if (!session || !message) return;
            std::shared_lock lock(session->GetMutex());

            uint32_t items_count = static_cast<uint32_t>(message->GetOption());
            auto session_id = session->GetSessionId();
            auto acc_cache = main_server->GetAccCacheUniqueBySessionId(session_id);
            auto acc_index = acc_cache->acc_info.Index;
            const auto& sellItemReq = reinterpret_cast<MainSellItemSerialInfoReq*>(message->GetData());
            if (acc_index == -1) return;
            const auto& item_sold = main_server->GetPlayerItemInventory(acc_cache, sellItemReq->item);
            if (!item_sold.has_value())
            {
                MainSellItemAck sell_item_data = { ItemSerialInfo(0, 0, 0, 0, 0), 0 };
                session->SendMsg(100, 0, 21, 0, reinterpret_cast<uint8_t*>(&sell_item_data), sizeof(sell_item_data));
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "player ({}) failed to sell unknown item serial info: ({})", acc_cache->acc_info.Nickname.c_str(), sellItemReq->item.data);
                return;
            }
            auto item_info = main_server->GetItemInfoCache(item_sold.value().item_info.item_number.item_id);
            acc_cache->acc_info.MicroPoints = acc_cache->acc_info.MicroPoints + item_info->SellPointPrice;
            main_server->AddPlayerItemsDeleted(acc_cache, sellItemReq->item);
            MainSellItemAck sell_item_data = { sellItemReq->item, item_info->SellPointPrice };
            session->SendMsg(100, 0, 1, 0, reinterpret_cast<uint8_t*>(&sell_item_data), sizeof(sell_item_data));

            MainCurrencyUpdateAck currency_update_data = { acc_cache->acc_info.RockTokens, acc_cache->acc_info.MicroPoints, acc_cache->acc_info.Coins };
            session->SendMsg(307, 0, 1, 0, reinterpret_cast<uint8_t*>(&currency_update_data), sizeof(currency_update_data));

            BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "player ({}) sold item: ({})", acc_cache->acc_info.Nickname.c_str(), item_info->Id);
        }
    }
}