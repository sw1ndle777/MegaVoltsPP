#pragma once
namespace Game::Handlers
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Main;
    inline void Missions(SCallbackData& callback, CMainServer* main_server)
    {
        auto session = callback.session;
        auto message = callback.message;
        if (!session || !message) return;
        //std::shared_lock lock(session->GetMutex());
        auto session_id = session->GetSessionId();
        auto acc_cache = CAccount.get<unique_t>(session_id);

        auto acc_index = acc_cache->acc_info.Index;
        if (acc_index == -1) return;

        auto extra = message->GetExtra();
        DatabaseUpdateCtx dctx{ .sid = session_id,.aid = acc_index };
        ResultLevelUpInfo level_up_info{};
        auto req = reinterpret_cast<MainCompleteMissionReq*>(message->GetData());
        const auto is_guide_mission = (req->mission_type == 1 && req->set_index == 9);
        const auto is_complete_goal = extra == 2;
        const auto is_reset_mission = extra == 3;
        const auto collection_id = req->collection_id;
        std::uint32_t new_mission{ 0 };
        if (is_complete_goal && is_guide_mission)
        {
            DEBUGLOG(dark_cyan, "player did guide mission: ({})", collection_id);

            if (collection_id <= 45 || collection_id >= 58 || collection_id < acc_cache->acc_info.GuideMission) return;

            auto current_coll = CCollectionInfo.get<shared_t>(collection_id);
            dctx.ops.emplace_back(AccountInfoPatch{ .guide_mission = collection_id - 45 });

            auto level_up = main_server->ProcessLevelUp(acc_cache, current_coll->rewardExp, dctx);
            if (!level_up.has_value())
            {
                DEBUGLOG(red, "ProcessLevelUp failed for player [{}] [{}]: {}", acc_cache->acc_info.Index, acc_cache->acc_info.Nickname.c_str(), static_cast<int>(level_up.error()));
                return;
            }
            level_up_info = level_up.value();
            if (current_coll->rewardPoint > 0)
            {
                using enum CurrencyType;
                dctx.ops.emplace_back(AccountCurrencyDelta{ .type = MP, .value = current_coll->rewardPoint, .is_reward = true });
            }
        }

        if ((is_complete_goal || is_reset_mission) && !is_guide_mission)
        {
            DEBUGLOG(dark_cyan, "daily mission request: ({}) ({}) ({}) ({})", collection_id, req->set_index, req->idk1, req->mission_type);
            if (is_complete_goal)
                DEBUGLOG(dark_cyan, "player did goal of daily mission: ({})", collection_id);
            if (is_reset_mission)
                DEBUGLOG(dark_cyan, "player want to reset daily mission: ({})", collection_id);

            uint32_t* curr_goal = nullptr;
            uint8_t   slot = 0;
            if (acc_cache->daily_mission_info.mission1 == collection_id) { curr_goal = &acc_cache->daily_mission_info.goal_mission1; slot = 1; }
            else if (acc_cache->daily_mission_info.mission2 == collection_id) { curr_goal = &acc_cache->daily_mission_info.goal_mission2; slot = 2; }
            else if (acc_cache->daily_mission_info.mission3 == collection_id) { curr_goal = &acc_cache->daily_mission_info.goal_mission3; slot = 3; }
            else
            {
                DEBUGLOG(dark_cyan, "mission is not correct for player");
                return;
            }
            auto info = CDailyMissionInfo.get<shared_t>(req->collection_id);
            if (*curr_goal >= info->goal) return;
            PlayerMissionsPatch pm{ .update_time = Utility::GetUtcTimeNow() };
            if (is_complete_goal)
            {
                const uint32_t new_goal = *curr_goal + 1;
                DEBUGLOG(dark_cyan, "will increase done goal count - current: ({}) max: ({})", new_goal, info->goal);
                if (slot == 1) pm.goal1 = new_goal;
                else if (slot == 2) pm.goal2 = new_goal;
                else pm.goal3 = new_goal;
                dctx.ops.emplace_back(pm);
                const bool completes = (new_goal == info->goal);
                if (completes)
                {
                    if (info->rewardPoint > 0)
                    {
                        using enum CurrencyType;
                        dctx.ops.emplace_back(AccountCurrencyDelta{ .type = MP, .value = info->rewardPoint, .is_reward = true });
                        DEBUGLOG(dark_cyan, "player get reward point: ({})", info->rewardPoint);
                    }
                    if (info->rewardItem)
                    {
                        DEBUGLOG(dark_cyan, "player get reward item: ({})", info->rewardItem);
                        auto crafted_item = main_server->CraftInventoryItems(acc_cache, { info->rewardItem }, NetEngine::Items::Origin::From_Game);
                        if (!crafted_item.has_value())
                        {
                            DEBUGLOG(red, "CraftInventoryItems failed for player [{}] [{}]: {}", acc_cache->acc_info.Index, acc_cache->acc_info.Nickname.c_str(), static_cast<int>(crafted_item.error()));
                            return;
                        }
                        dctx.ops.push_back(crafted_item.value());
                    }
                }
            }
            if (is_reset_mission)
            {
                if (acc_cache->acc_info.MicroPoints < 10000) //Value to check is in constantinfo!
                {
                    DEBUGLOG(dark_cyan, "player {} does not have enough MicroPoints to reset mission", acc_cache->acc_info.Nickname.c_str());
                    session->SendMsg(168, 0, 3, 0);
                    return;
                }
                using enum CurrencyType;
                dctx.ops.emplace_back(AccountCurrencyDelta{ .type = MP, .value = 10000, .is_reward = false });
                auto new_ids = main_server->GetRandomDailyMissionIds(1, acc_cache->daily_mission_info.mission1, acc_cache->daily_mission_info.mission2, acc_cache->daily_mission_info.mission3);
                new_mission = new_ids[0];
                if (slot == 1) { pm.mission1 = new_mission; pm.goal1 = 0; }
                else if (slot == 2) { pm.mission2 = new_mission; pm.goal2 = 0; }
                else { pm.mission3 = new_mission; pm.goal3 = 0; }

                dctx.ops.push_back(pm);
            }

        }

        auto validated = main_server->ValidateDatabaseUpdates(acc_cache, dctx);
        if (!validated.has_value())
        {
            DEBUGLOG(red, "ValidateDatabaseUpdates failed for [{}] [{}]: {}", acc_cache->acc_info.Index, acc_cache->acc_info.Nickname.c_str(), static_cast<int>(validated.error()));
            return;
        }
        acc_cache.unlock();

        [[maybe_unused]] auto ignored = BaseLib::DbPool->submit_task([main_server, session = std::move(callback.session),
            s_id = session_id,
            collection_id = collection_id,
            new_mission = new_mission,
            is_guide_mission = is_guide_mission,
            is_complete_goal = is_complete_goal,
            is_reset_mission = is_reset_mission,
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
                if (is_complete_goal)
                {
                    if (!is_guide_mission)
                    {
                        std::vector<MainCompleteMissionReq> missions;
                        missions.push_back(MainCompleteMissionReq{ new_acc_cache->daily_mission_info.mission1, 0, new_acc_cache->daily_mission_info.goal_mission1, 4 });
                        missions.push_back(MainCompleteMissionReq{ new_acc_cache->daily_mission_info.mission2, 0, new_acc_cache->daily_mission_info.goal_mission2, 4 });
                        missions.push_back(MainCompleteMissionReq{ new_acc_cache->daily_mission_info.mission3, 0, new_acc_cache->daily_mission_info.goal_mission3, 4 });
                        session->SendMsg(168, 0, 1, missions.size(), reinterpret_cast<uint8_t*>(missions.data()), missions.size() * sizeof(MainCompleteMissionReq));
                    }
                    session->SendMsg(168, 0, 2, 0, reinterpret_cast<uint8_t*>(&collection_id), sizeof(collection_id));
                }
                if (is_reset_mission)
                {
                    std::vector<MainCompleteMissionReq> missions;
                    missions.push_back(MainCompleteMissionReq{ new_mission, 0, 0, 4 });
                    session->SendMsg(168, 0, 3, missions.size(), reinterpret_cast<uint8_t*>(missions.data()), missions.size() * sizeof(MainCompleteMissionReq));
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