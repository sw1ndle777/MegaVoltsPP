#pragma once
namespace Game::Handlers
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Main;
    struct JoinPlazaCtx
    {
        CMainServer* main;
        CSession* session;
        AccCacheResource& acc;
    };

    inline void TryRemovePlazaSid(PlazaCacheResource& plaza, uint32_t sid)
    {
        std::erase(plaza->session_ids, sid);
    }
    inline void TryBroadcastPlazaPlayerDisconnect(JoinPlazaCtx& ctx, PlazaCacheResource& plaza, uint32_t sid)
    {
        if (ctx.acc->is_invisible) return;
        auto my_uid = NetEngine::Packets::Core::UniqueId(sid, 1).data;
        for (const auto& id : plaza->session_ids)
        {
            if (id == sid) continue;
            if (auto player_session = ctx.main->GetSessionById(id))
                player_session->SendMsg(425, 0, 0, 1, reinterpret_cast<uint8_t*>(&my_uid), sizeof(my_uid));
        }
    }
    inline void TryRemovePlazaPlayer(JoinPlazaCtx& ctx, uint32_t plaza_id)
    {
        auto plaza = CPlaza.get<unique_t>(plaza_id);
        auto& ids = plaza->session_ids;
        auto sid = ctx.session->GetSessionId();
        if (std::ranges::contains(ids, sid)) { plaza.unlock(); return; }
        TryBroadcastPlazaPlayerDisconnect(ctx, plaza, sid);
        TryRemovePlazaSid(plaza, sid);
        DEBUGLOG(dark_cyan, "sid=({}) left plaza id: ({})", sid, plaza_id);
        plaza.unlock();
    }

    inline void TryUpdatePlazaPlayerEquipInfo(JoinPlazaCtx& ctx, PlazaCacheResource& plaza)
    {
        if (!ctx.main->IsPlazaBroadcastable(plaza)) return;
        auto sid = ctx.session->GetSessionId();
        auto acc = CAccount.get<shared_t>(sid);
        auto uid = acc->uid;
        auto voice = acc->voice_id;
        bool invisible = acc->is_invisible;
		auto info1 = ctx.main->GetRoomUserPlayerInfo1(acc);
		auto info2 = ctx.main->GetRoomUserPlayerInfo2(acc);
		auto equiped_items = ctx.main->GetEquippedItems(acc);
        auto equipack = PlazaEquipInfoAck(acc->acc_info.Nickname, acc->uid, equiped_items, info1, info2);
        acc.unlock();
        auto& ids = plaza->session_ids;
        for (const auto& id : ids)
        {
            if (id == sid) continue;
            if (auto pss = ctx.main->GetSessionById(id))
            {
                if (!invisible)
                {
                    pss->SendMsg(424, 0, 0, 1, reinterpret_cast<uint8_t*>(&equipack), sizeof(equipack));
                    pss->SendMsg(314, 0, 0, voice, reinterpret_cast<uint8_t*>(&uid), sizeof(uid));
                }

				auto other_acc = CAccount.get<shared_t>(id);
				auto other_info1 = ctx.main->GetRoomUserPlayerInfo1(other_acc);
				auto other_info2 = ctx.main->GetRoomUserPlayerInfo2(other_acc);
				auto other_equiped_items = ctx.main->GetEquippedItems(other_acc);
				auto other_equipack = PlazaEquipInfoAck(other_acc->acc_info.Nickname, other_acc->uid, other_equiped_items, other_info1, other_info2);
                auto other_uid = NetEngine::Packets::Core::UniqueId(id, 1).data;
                auto other_voice = other_acc->voice_id;
                other_acc.unlock();

                ctx.session->SendMsg(424, 0, 0, 1, reinterpret_cast<uint8_t*>(&other_equipack), sizeof(other_equipack));
                ctx.session->SendMsg(314, 0, 0, other_voice, reinterpret_cast<uint8_t*>(&other_uid), sizeof(other_uid));

            }
        }
    }
    inline void TryJoinPlaza(JoinPlazaCtx& ctx, uint32_t newId, uint32_t oldId)
    {
        if (newId != oldId && ctx.main->IsPlazaAlready(oldId)) TryRemovePlazaPlayer(ctx, oldId);
        auto plaza = CPlaza.get<unique_t>(newId);
        ctx.acc->plaza_id = newId;
        ctx.acc->in_plaza = true;
        plaza->session_ids.push_back(ctx.session->GetSessionId());
        ctx.session->SendMsg(173, 0, EPlazaJoin::Result::Success, 0, reinterpret_cast<uint8_t*>(&newId), sizeof(newId)); // join plaza success
        DEBUGLOG(dark_cyan, "player ({}) join plaza -> plaza id: ({})", ctx.acc->acc_info.Nickname.c_str(), newId);
        ctx.acc.unlock();
        TryUpdatePlazaPlayerEquipInfo(ctx, plaza);
        plaza.unlock();
    }
    inline void TryJoinOrFindPlaza(JoinPlazaCtx& ctx, uint32_t newId)
    {
        auto plaza = CPlaza.get<shared_t>(newId);
        auto plaza_id = ctx.main->IsPlazaFull(plaza) ? ctx.main->FindFirstNonFullPlaza() : newId;
        plaza.unlock();
        TryJoinPlaza(ctx, plaza_id, ctx.acc->plaza_id);
    }
    inline uint16_t TryCreateAvailablePlaza(JoinPlazaCtx& ctx, uint32_t max_players = 32)
    {
        uint16_t id = 0;
        ctx.main->GetNextAvailablePlazaId(id);
		CPlaza.insert(id, Plaza(id, max_players));
        return id;
    }
    inline void TryCreateCustomPlaza(JoinPlazaCtx& ctx, uint16_t id, uint32_t max_players = 32)
    {
        CPlaza.insert(id, Plaza(id, max_players));
    }
    inline void PlazaJoin(SCallbackData& callback, CMainServer* main_server)
    {
        auto session = callback.session;
        auto message = callback.message;
        if (!session || !message) return;

        //std::shared_lock lock(session->GetMutex());
        CServer* server = callback.server;
        auto sid = session->GetSessionId();
        auto acc_cache = CAccount.get<unique_t>(sid);
        if (acc_cache->acc_info.Index == -1) return;
        auto req = reinterpret_cast<MainJoinPlazaReq*>(message->GetData());
        auto plaza_id = req->plaza_id;
        auto channel_id = req->channel_id;
        auto old_plaza_id = acc_cache->plaza_id;
        DEBUGLOG(dark_cyan, "player ({}) join plaza attempt -> plaza id: ({}), plaza server/channel id: ({}), mission: ({}),  extra: ({}), option: ({})", acc_cache->acc_info.Nickname.c_str(), plaza_id, channel_id, message->GetMission(), message->GetExtra(), message->GetOption());
        JoinPlazaCtx ctx{ main_server, session.get(), acc_cache };
        if (plaza_id == 0)
        {
            if (!main_server->IsPlazaAlready(plaza_id))
            {
                auto new_plaza_id = TryCreateAvailablePlaza(ctx);
                TryJoinPlaza(ctx, new_plaza_id, old_plaza_id);
                return;
            }
            TryJoinOrFindPlaza(ctx, plaza_id);
            return;
        }
        if (!main_server->IsPlazaAlready(plaza_id))
        {
            TryCreateCustomPlaza(ctx, plaza_id);
            TryJoinPlaza(ctx, plaza_id, old_plaza_id);
            return;
        }
        auto plaza = CPlaza.get<unique_t>(plaza_id);

        if (std::ranges::contains(plaza->session_ids, sid)) // here issue plaza->session_ids didn't clear idk how
        {
            DEBUGLOG(dark_cyan, "player ({}) join plaza -> plaza id: ({}), session already here, disconnect", acc_cache->acc_info.Nickname.c_str(), plaza_id);
            main_server->DisconnectPlayer(sid, Disconnect::Reason::Close);
            return;
        }
        if (main_server->IsPlazaFull(plaza)) session->SendMsg(173, 0, EPlazaJoin::Result::Full, 0);
        plaza.unlock();
        TryJoinOrFindPlaza(ctx, plaza_id);
    }
}