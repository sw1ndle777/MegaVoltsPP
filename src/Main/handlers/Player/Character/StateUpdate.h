#pragma once
namespace Game::Handlers
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Main;
    inline void StateUpdate(SCallbackData& callback, CMainServer* main_server)
    {
        auto session = callback.session;
        auto message = callback.message;
        if (!session || !message) return;

        //std::shared_lock lock(session->GetMutex());
        CServer* server = callback.server;
        auto player_state = static_cast<PlayerInfo::State>(message->GetOption());
        auto acc_cache = CAccount.get<unique_t>(session->GetSessionId());
        auto session_id = session->GetSessionId();
        auto my_unique_id = NetEngine::Packets::Core::UniqueId(session_id, 1).data;
        if (acc_cache->acc_info.Index == -1) return;

        DEBUGLOG(dark_cyan, "player update state: before ({}) now ({})", static_cast<uint32_t>(acc_cache->state), static_cast<uint32_t>(player_state));
        DatabaseUpdateCtx dctx{ .sid = session_id,.aid = acc_cache->acc_info.Index };
        ResultLevelUpInfo level_up_info{};
        acc_cache->state = player_state;

        if (player_state == PlayerInfo::State::GachaponMachine)
        {
            if (acc_cache->in_plaza && acc_cache->acc_info.GuideMission == 7) // Finish Guide Mission "go to gachapon"
            {
                auto current_coll = CCollectionInfo.get<shared_t>(53);
                dctx.ops.emplace_back(AccountInfoPatch{ .guide_mission = 8 });
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
                mission_data.collection_id = 53;
                session->SendMsg(168, 0, 2, 0, reinterpret_cast<uint8_t*>(&mission_data.collection_id), sizeof(mission_data.collection_id));
            }

            std::vector<MainGachaponSaleInfo> gs_info;
            auto sales_db = CGachaponSale.get_all(shared);
            for (auto& sale_id : *sales_db)
            {
                auto gsi = CGachaponSaleInfo.get<shared_t>(sale_id);
                auto gsd = MainGachaponSaleInfo(gsi->gachapon_id, gsi->sale_price, gsi->start_date, gsi->end_date);
                gs_info.push_back(gsd);
                auto id = gsd.data.id;
                auto price = gsd.data.sale_price;
                DEBUGLOG(dark_cyan, "gachapon sale info: id: {} price: {} start: {} end: {}", id, price, gsd.start_date, gsd.end_date);
            }
            auto gsi_ack = MainGachaponSalesInfoAck(gs_info).Serialize();
            session->SendMsg(83, 0, 0, gs_info.size(), reinterpret_cast<uint8_t*>(gsi_ack.data()), gsi_ack.size());
            acc_cache->state = player_state;
            MainCurrencyUpdateAck currency_update_data = { acc_cache->acc_info.RockTokens, acc_cache->acc_info.MicroPoints, acc_cache->acc_info.Coins };
            session->SendMsg(307, 0x0, 0, 0, reinterpret_cast<uint8_t*>(&currency_update_data), sizeof(currency_update_data)); // currency update ack
        }

        if (acc_cache->in_room)
        {
            if (CRoom.contains(acc_cache->room_id))
            {
                auto room = CRoom.get<shared_t>(acc_cache->room_id);
                auto selected_character = acc_cache->acc_info.SelectedCharacter;
                auto voice_id = acc_cache->voice_id;

                
                auto equipped_items = main_server->GetEquippedItems(acc_cache);
                auto equip_data = EquipInfoAck(acc_cache->uid, equipped_items);
                acc_cache.unlock();
                auto players_ids = main_server->GetRoomSortedPlayerSessionIds(room);

                for (const auto& room_player_session_id : players_ids)
                    if (auto player_session = server->GetSessionById(room_player_session_id))
                        player_session->SendMsg(312, 0, 0, player_state, reinterpret_cast<uint8_t*>(&my_unique_id), sizeof(my_unique_id));

                for (const auto& room_player_session_id : players_ids)
                {
                    if (room_player_session_id == session_id) continue;
                    if (auto player_session = server->GetSessionById(room_player_session_id))
                    {
                        player_session->SendMsg(414, 0, selected_character, 17, reinterpret_cast<uint8_t*>(&equip_data), sizeof(EquipInfoAck));
                        player_session->SendMsg(314, 0, 0, voice_id, reinterpret_cast<uint8_t*>(&my_unique_id), sizeof(my_unique_id));
                    }
                }
                acc_cache.lock();
            }
        }
        if (acc_cache->in_plaza)
        {
            if (main_server->IsPlazaAlready(acc_cache->plaza_id))
            {
                auto plaza = CPlaza.get<shared_t>(acc_cache->plaza_id);
                auto selected_character = acc_cache->acc_info.SelectedCharacter;
                auto voice_id = acc_cache->voice_id;
                auto equipped_items = main_server->GetEquippedItems(acc_cache);
                auto equip_data = EquipInfoAck(acc_cache->uid, equipped_items);
                acc_cache.unlock();
                auto& players_ids = plaza->session_ids;
                for (const auto& plaza_player_session_id : players_ids)
                {
                    if (plaza_player_session_id == session_id) continue;
                    if (auto player_session = server->GetSessionById(plaza_player_session_id))
                    {
                        player_session->SendMsg(414, 0, selected_character, 17, reinterpret_cast<uint8_t*>(&equip_data), sizeof(EquipInfoAck));
                        player_session->SendMsg(314, 0, 0, voice_id, reinterpret_cast<uint8_t*>(&my_unique_id), sizeof(my_unique_id));
                    }
                }
                acc_cache.lock();
            }
        }
        if (acc_cache->in_party && acc_cache->in_room)
        {
            bool is_going_to_waiting = (static_cast<uint32_t>(player_state) == 7);
            auto room_cache = CRoom.get<unique_t>(acc_cache->room_id);
            auto party_cache = CParty.get<unique_t>(acc_cache->party_id);
            bool is_ruined = (room_cache->blueteam_session_ids.size() == 0 || room_cache->redteam_session_ids.size() == 0);
            if (is_ruined && is_going_to_waiting)
            {
                auto target_room_id = room_cache->room_id;
                DEBUGLOG(dark_cyan, "player want to switch state to waiting in a party battle that need to be dismembered");
                session->SendMsg(141, 0, NetEngine::Room::Leave::Ack::Result::ClosedByGm, 0);
                session->SendMsg(120, 0, 45, 0);
                auto my_team_id = acc_cache->team_id;
                party_cache->is_registered = false;
                party_cache->is_queueing = false;
                acc_cache.unlock();
                main_server->NewRemoveRoomPlayer(room_cache, session_id, my_team_id, NetEngine::Room::Leave::Ack::Result::Leave, false);
                uint32_t player_count = room_cache->blueteam_session_ids.size() + room_cache->redteam_session_ids.size();
                if (player_count == 0)
                {
                    DEBUGLOG(dark_cyan, "last player remaining in endmatch screen leave so now room will be removed");
                    party_cache->is_playing = false;
                }
                acc_cache.lock();
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