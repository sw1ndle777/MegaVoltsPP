#pragma once
namespace Game::Handlers
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Main;
    inline void CurrencyUpdate(SCallbackData& callback, CMainServer* main_server)
    {
        auto session = callback.session;
        auto message = callback.message;
        if (!session || !message) return;
        //std::shared_lock lock(session->GetMutex());
        auto acc_cache = CAccount.get<shared_t>(session->GetSessionId());
        if (acc_cache->acc_info.Index == -1) return;
        MainCurrencyUpdateAck currency_update_data = { acc_cache->acc_info.RockTokens, acc_cache->acc_info.MicroPoints, acc_cache->acc_info.Coins };
        session->SendMsg(307, 0, 0, 0, reinterpret_cast<uint8_t*>(&currency_update_data), sizeof(currency_update_data));
    }
}