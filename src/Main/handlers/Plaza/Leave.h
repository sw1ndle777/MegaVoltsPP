#pragma once
namespace Game::Handlers
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Main;
    inline void PlazaLeave(SCallbackData& callback, CMainServer* main_server)
    {
        auto session = callback.session;
        auto message = callback.message;
        if (!session || !message) return;

        //std::shared_lock lock(session->GetMutex());
        CServer* server = callback.server;
        auto session_id = session->GetSessionId();
        auto acc_cache = CAccount.get<unique_t>(session_id);
        if (acc_cache->acc_info.Index == -1) return;
        auto plaza_id = acc_cache->plaza_id;
        DatabaseUpdateCtx dctx{ .sid = session_id,.aid = acc_cache->acc_info.Index };
        if (main_server->IsPlazaAlready(plaza_id))
        {
            auto current_plaza = CPlaza.get<unique_t>(plaza_id);
            auto& session_ids = current_plaza->session_ids;
            if (std::ranges::contains(session_ids, session_id))
            {
                auto my_unique_id = NetEngine::Packets::Core::UniqueId(session_id, 1).data;
                for (const auto& plaza_player_session_id : session_ids)
                {
                    if (plaza_player_session_id == session_id) continue;
                    if (auto player_session = server->GetSessionById(plaza_player_session_id))
                        player_session->SendMsg(425, 0, 0, 1, reinterpret_cast<uint8_t*>(&my_unique_id), sizeof(my_unique_id));
                }
                DEBUGLOG(dark_cyan, "sid=({}) left plaza id: ({})", session_id, plaza_id);
                auto remove_myself = std::remove(current_plaza->session_ids.begin(), current_plaza->session_ids.end(), session_id);
                current_plaza->session_ids.erase(remove_myself, current_plaza->session_ids.end());
                acc_cache->plaza_id = 0;
                acc_cache->in_plaza = false;
            }
        }
        session->SendMsg(174, 0, 0, 0); // leave plaza success
        acc_cache.unlock();
        ResultLevelUpInfo level_up_info{};
        if (acc_cache->state == 0 && acc_cache->acc_info.GuideMission == 9) // Done guide mission "View roomlist"
        {
            auto acc_cache = CAccount.get<unique_t>(session_id);
            auto current_coll = CCollectionInfo.get<shared_t>(55);
            dctx.ops.emplace_back(AccountInfoPatch{ .guide_mission = 10 });
            if (current_coll->rewardPoint > 0)
            {
                using enum CurrencyType;
                dctx.ops.emplace_back(AccountCurrencyDelta{ .type = MP, .value = current_coll->rewardPoint, .is_reward = true });
            }

            auto level_up = main_server->ProcessLevelUp(acc_cache, current_coll->rewardExp, dctx);
            if (!level_up.has_value())
            {
                DEBUGLOG(red, "ProcessLevelUp failed for player [{}] [{}]: {}", acc_cache->acc_info.Index, acc_cache->acc_info.Nickname.c_str(), static_cast<int>(level_up.error()));
                return;
            }
            level_up_info = level_up.value();
            MainCompleteMissionReq mission_data;
            mission_data.collection_id = 55;
            session->SendMsg(168, 0, 2, 0, reinterpret_cast<uint8_t*>(&mission_data.collection_id), sizeof(mission_data.collection_id));
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