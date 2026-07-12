#pragma once
#include <BaseLib/CLogging.h>

namespace Game::Handlers
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Main;
    
    inline void GachaponPity(SCallbackData& callback, CMainServer* main_server)
    {
        auto session = callback.session;
        auto message = callback.message;
        if (!session || !message) return;

        CServer* server = callback.server;
        auto session_id = session->GetSessionId();
        auto acc_cache = CAccount.get<unique_t>(session_id);
        auto acc_index = acc_cache->acc_info.Index;
        auto req = reinterpret_cast<MainGachaponPityReq*>(message->GetData());
        if (acc_index == -1) return;

        auto gachapon_info = CGachaponsInfo.get<shared_t>(req->gachapon_id);
        uint32_t pity_points = 0;
        if (gachapon_info->Id == static_cast<uint32_t>(-1))
        {
            session->SendMsg(ITEM_GACHA_PITY, 0, 0, 0, reinterpret_cast<uint8_t*>(&pity_points), sizeof(pity_points));
            return;
        }

        
        auto gacha_type = gachapon_info->Type;
        auto pit = std::find_if(acc_cache->gacha_pity.begin(), acc_cache->gacha_pity.end(),
            [&](const GachaPityEntry& e) { return e.gacha_type == gacha_type; });
        if (pit != acc_cache->gacha_pity.end())
            pity_points = pit->lucky_points;

        session->SendMsg(ITEM_GACHA_PITY, 0,0,0, reinterpret_cast<uint8_t*>(&pity_points), sizeof(pity_points));
    }
}