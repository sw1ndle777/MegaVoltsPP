#pragma once
namespace Game::Handlers
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Main;
    inline void PartyCreate(SCallbackData& callback, CMainServer* main_server)
    {
        auto session = callback.session;
        auto message = callback.message;
        if (!session || !message) return;

        std::shared_lock lock(session->GetMutex());
        CServer* server = callback.server;
        auto sid = session->GetSessionId();
        auto acc = CAccount.get<unique_t>(sid);
        auto aid = acc->acc_info.Index;
        const auto nick = acc->acc_info.Nickname; // local copy: safe across the acc.unlock()/lock() below
        auto teamId = acc->team_id;
        auto uid = NetEngine::Packets::Core::UniqueId(sid, 1).data;
        auto leave_result = static_cast<NetEngine::Room::Leave::Req::Result>(message->GetExtra());
        if (aid == -1) return;
        auto createPartyReq = reinterpret_cast<MainCreatePartyReq*>(message->GetData());
        DatabaseUpdateCtx dctx{ .sid = sid,.aid = aid };
        ResultLevelUpInfo level_up_info{};
        uint16_t party_id = 0;
        if (server->GetNextAvailableQueuePartyId(party_id))
        {
            Party newParty;
            newParty.party_id = (uint32_t)party_id;
            newParty.is_playing = false;
            newParty.is_queueing = false;
            newParty.is_clan = false;
            newParty.is_registered = false;
            newParty.has_password = false;
            newParty.clan_id = acc->acc_info.ClanId;
            newParty.max_members = 4;
            newParty.party_host_session_id = sid;
            newParty.mod_id = 15;
            newParty.map_id = 14;
            newParty.members.push_back(sid);
			CParty.insert(party_id, newParty);
			CPartyId.emplace_back(party_id);
            //main_server->AddPartyCache(party_id, newParty);

            //party_ids.push_back(party_id);

            acc->in_party = true;
            acc->party_id = party_id;

            /*leave room*/
            if (acc->in_room)
            {
                auto room_id = acc->room_id;
                acc.unlock();
                auto room_cache = CRoom.get<unique_t>(room_id);
                main_server->NewRemoveRoomPlayer(room_cache, sid, teamId, NetEngine::Room::Leave::Ack::Result::Leave, false);
                room_cache.unlock();
                acc.lock();
            }

            /*leave plaza start*/
            if (acc->in_plaza)
            {
                auto plaza_id = acc->plaza_id;
                if (main_server->IsPlazaAlready(plaza_id))
                {
                    DEBUGLOG(dark_cyan, "player will leave plaza: ({})", plaza_id);
                    auto plaza = CPlaza.get<unique_t>(plaza_id);
                    auto& session_ids = plaza->session_ids;
                    
                    if (std::ranges::contains(session_ids, sid))
                    {
                        for (const auto& id : session_ids)
                        {
                            if (id == sid) continue;
                            if (auto pss = server->GetSessionById(id))
                                pss->SendMsg(425, 0, 0, 1, reinterpret_cast<uint8_t*>(&uid), sizeof(uid));
                        }
                        DEBUGLOG(dark_cyan, "user=({}) sid=({}) left plaza id: ({})", nick.c_str(), sid, plaza_id);
                        std::erase(plaza->session_ids, sid);
                        acc->plaza_id = 0;
                        acc->in_plaza = false;
                    }
                }
            }
            /*leave plaza end*/

            DEBUGLOG(dark_cyan, "player ({}) created a new party entity id: ({})", acc->acc_info.Nickname.c_str(), party_id);
        }
        else
            DEBUGLOG(red, "party pool is full");

        session->SendMsg(109, 0, 1, 0, reinterpret_cast<uint8_t*>(createPartyReq), sizeof(MainCreatePartyReq));

        DEBUGLOG(dark_cyan, "player ({}) create party -> unknown: ({})", acc->acc_info.Nickname.c_str(), createPartyReq->unknown);

        if (acc->acc_info.GuideMission != 10) return; // Done guide mission "Create a party"
        auto current_coll = CCollectionInfo.get<shared_t>(56);//CCollectionInfo.get<shared_t>(56);
        dctx.ops.emplace_back(AccountInfoPatch{ .guide_mission = 11 });
        if (current_coll->rewardPoint > 0)
        {
            using enum CurrencyType;
            dctx.ops.emplace_back(AccountCurrencyDelta{ .type = MP, .value = current_coll->rewardPoint, .is_reward = true });
        }

        auto level_up = main_server->ProcessLevelUp(acc, current_coll->rewardExp, dctx);
        if (!level_up.has_value())
        {
            DEBUGLOG(red, "ProcessLevelUp failed for player [{}] [{}]: {}", acc->acc_info.Index, acc->acc_info.Nickname.c_str(), static_cast<int>(level_up.error()));
            return;
        }
        level_up_info = level_up.value();
        MainCompleteMissionReq mission_data;
        mission_data.collection_id = 56;
        session->SendMsg(168, 0, 2, 0, reinterpret_cast<uint8_t*>(&mission_data.collection_id), sizeof(mission_data.collection_id));

        auto validated = main_server->ValidateDatabaseUpdates(acc, dctx);
        if (!validated.has_value())
        {
            DEBUGLOG(red, "ValidateDatabaseUpdates failed for [{}] [{}]: {}", acc->acc_info.Index, acc->acc_info.Nickname.c_str(), static_cast<int>(validated.error()));
            return;
        }
        acc.unlock();
        [[maybe_unused]] auto ignored = BaseLib::DbPool->submit_task([main_server, session = std::move(callback.session),
            s_id = sid,
            level_up_result = std::move(level_up_info),
            v = std::move(validated.value())
        ]() mutable
            {
                if (!session) return;
                ResultDbUpdateInfo dbres;
                if (!BaseLib::Database->UpdateAccount(v, dbres).has_value()) return;
                auto new_acc_cache = CAccount.get<unique_t>(s_id);
                auto applied = main_server->ApplyDatabaseUpdates(new_acc_cache, v);
                if (!applied.has_value())
                {
                    DEBUGLOG(red, "ApplyDatabaseUpdates failed for [{}] [{}]: {}", new_acc_cache->acc_info.Index, new_acc_cache->acc_info.Nickname.c_str(), static_cast<int>(applied.error()));
                    return;
                }

                if (!v.items_added.empty())
                {
                    std::vector<ShopItem> shop_items;
                    for (const auto& item : v.items_added)
                    {
                        auto item_info = CItemsInfo.get<shared_t>(item.item_info.item_number.item_id);
                        ShopItem new_item = { {item.item_info.item_number.item_id , item_info->Stock} , ItemExpire::Type::Unused,  item.item_info.serial_info };
                        shop_items.push_back(new_item);
                    }
                    if (!shop_items.empty())
                        session->SendMsg(99, 0, 37, static_cast<uint8_t>(shop_items.size()), reinterpret_cast<uint8_t*>(shop_items.data()), static_cast<uint16_t>(shop_items.size() * sizeof(ShopItem)));
                }
                if (level_up_result.level_up)
                {
                    std::vector<uint16_t> playing_players;
                    if (new_acc_cache->in_room)
                    {
                        auto room_cache = CRoom.get<shared_t>(new_acc_cache->room_id);
                        playing_players = main_server->GetRoomSortedPlayerSessionIds(room_cache);
                        room_cache.unlock();
                    }
                    else if (new_acc_cache->in_party)
                    {
                        auto party_cache = CParty.get<shared_t>(new_acc_cache->party_id);
                        playing_players = party_cache->members;
                        party_cache.unlock();
                    }
                    auto my_unique_id = NetEngine::Packets::Core::UniqueId(s_id, 1).data;
                    for (const auto& others_id : playing_players)
                    {
                        if (others_id == s_id) continue;
                        if (auto other_session = main_server->GetSessionById(others_id))
                            other_session->SendMsg(311, 0, 0, static_cast<uint8_t>(level_up_result.new_level + 1), reinterpret_cast<uint8_t*>(&my_unique_id), sizeof(my_unique_id));
                    }
                }
            });
    }
}