#pragma once
namespace Game::Handlers
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Main;
    inline void ItemEquip(SCallbackData& callback, CMainServer* main_server)
    {
        auto session = callback.session;
        auto message = callback.message;
        if (!session || !message) return;

        //std::shared_lock lock(session->GetMutex());
        auto session_id = session->GetSessionId();
        auto acc_cache = CAccount.get<unique_t>(session_id);
        if (acc_cache->acc_info.Index == -1) return;

        auto order = message->GetOrder();
        auto equip_remove_type = message->GetExtra();
        auto equip_items_count = message->GetOption();
        auto equip_update_type = message->GetMission();

        const auto& charEquipUpdateReq = reinterpret_cast<MainCharacterEquipUpdateReq*>(message->GetData());

        auto current_character = static_cast<uint8_t>(acc_cache->acc_info.SelectedCharacter);
        DEBUGLOG(dark_cyan, "player ({}) equip update type: ({}), remove type: ({}), items count: ({}) on character ({})", acc_cache->acc_info.Nickname.c_str(), equip_update_type, equip_remove_type, equip_items_count, current_character);
        DatabaseUpdateCtx dctx{ .sid = session_id, .aid = acc_cache->acc_info.Index };

        auto AddEquip = [&](std::optional<ItemSerialInfo> serial, std::optional<uint32_t> type, bool equipped, std::optional<uint8_t> char_id)
            {
                ItemPatchCtx p
                {
                    .sel = ItemSelector{.serial = serial, .item_type = type, .character_id = char_id},
                    .is_equipped = equipped,
                    .character_id = char_id,
                };
                dctx.ops.push_back(p);
            };

        auto AddExpire = [&](const ItemInfo& info, const ItemSerialInfo& serial)
            {
                ItemPatchCtx p
                {
                    .sel = ItemSelector{.serial = serial},
                    .expire_date = info.LimitedTime == 0 ? ItemExpire::Type::Unlimited : Utility::GetUtcTimeNowPlusSeconds(info.LimitedTime)
                };
                dctx.ops.push_back(p);
            };


        if (equip_remove_type == 17) equip_remove_type = 25;
        if (equip_remove_type == 19) equip_remove_type = 22;
        if (equip_remove_type == 20) equip_remove_type = 23;
        for (uint32_t i = 0; i < equip_items_count; i++)
            DEBUGLOG(dark_cyan, "player ({}) sent item data: ({})", acc_cache->acc_info.Nickname.c_str(), charEquipUpdateReq->item[i].data);

        if (equip_remove_type == 51)
        {
            DEBUGLOG(dark_cyan, "player ({}) unsafe equip change type 51 for {} items", acc_cache->acc_info.Nickname.c_str(), equip_items_count);
            const auto& charEquipSwitchReq = reinterpret_cast<MainCharacterEquipSwitchReq*>(message->GetData());
            for (uint32_t i = 0; i < equip_items_count; i++)
            {
                auto character_id = charEquipSwitchReq->item[i].character_id;
                auto item_id = charEquipSwitchReq->item[i].item_id;
                auto item_inv = main_server->GetPlayerItemInventory(acc_cache, item_id);
                if (!item_inv.has_value()) continue;
                const auto& item = item_inv.value();
                auto item_info = CItemsInfo.get<shared_t>(item.item_info.item_number.item_id);
                if (main_server->IsItemSet(item_id))
                {
                    const auto& set_item_types = main_server->GetSetItemTypes(item_id);
                    for (const auto& item_type : set_item_types)
                    {
                        auto curr_item = main_server->GetPlayerItemInventory(acc_cache, item_type, character_id);
                        if (curr_item.has_value())
                        {
                            AddEquip(std::nullopt, item_type, false, character_id);
                            DEBUGLOG(dark_cyan, "player ({}) had an item but set replaced part type ({})", acc_cache->acc_info.Nickname.c_str(), item_type);
                        }
                    }
                }
                AddEquip(std::nullopt, item_info->Type, false, character_id);
                AddEquip(item.item_info.serial_info, std::nullopt, true, character_id);
            }
        }

        if (equip_update_type == 1)
        {
            AddEquip(std::nullopt, equip_remove_type, false, current_character);
            if (equip_items_count == 0) { // UNEQUIP ITEM TYPE BASED ON REMOVETYPE !
                DEBUGLOG(dark_cyan, "player ({}) singlehand unequip item type ({})", acc_cache->acc_info.Nickname.c_str(), equip_remove_type);
            }
            else {
                DEBUGLOG(dark_cyan, "player ({}) unknown action on update_type 1: remove type {}, item count {}", acc_cache->acc_info.Nickname.c_str(), equip_remove_type, equip_items_count);
            }
        }

        if (equip_update_type == 0)
        {
            for (uint32_t i = 0; i < equip_items_count; i++)
            {
                if (charEquipUpdateReq->item[i].data <= 25) // UNEQUIP ITEM TYPE !
                {
                    auto unquip_item_type = charEquipUpdateReq->item[i].data;
                    if (unquip_item_type == 17) unquip_item_type = 25;
                    if (unquip_item_type == 19) unquip_item_type = 22;
                    if (unquip_item_type == 20) unquip_item_type = 23;

                    AddEquip(std::nullopt, unquip_item_type, false, current_character);
                    DEBUGLOG(dark_cyan, "player ({}) unequip item type ({})", acc_cache->acc_info.Nickname.c_str(), charEquipUpdateReq->item[i].data);
                    continue;
                }
                auto item_inv = main_server->GetPlayerItemInventory(acc_cache, charEquipUpdateReq->item[i]);
                if (!item_inv.has_value())
                {
                    DEBUGLOG(dark_cyan, "player ({}) attempt equip but can't find item inventory", acc_cache->acc_info.Nickname.c_str());
                    continue;
                }
                const auto& item = item_inv.value();
                auto item_info = CItemsInfo.get<shared_t>(item.item_info.item_number.item_id);
                if (!item_info->Id)
                {
                    DEBUGLOG(dark_cyan, "player ({}) attempt equip but can't find item in item info cache or it has no namme item id: ({})", acc_cache->acc_info.Nickname.c_str(), item.item_info.item_number.item_id);
                    continue;
                }
                if (main_server->IsItemSet(item.item_info.item_number.item_id))
                {
                    const auto& set_item_types = main_server->GetSetItemTypes(item.item_info.item_number.item_id);
                    for (const auto& item_type : set_item_types)
                    {
                        auto curr_item = main_server->GetPlayerItemInventory(acc_cache, item_type, current_character);
                        if (curr_item.has_value())
                        {
                            AddEquip(std::nullopt, item_type, false, current_character);
                            DEBUGLOG(dark_cyan, "player ({}) had an item but set replaced part type ({})", acc_cache->acc_info.Nickname.c_str(), item_type);
                        }
                    }
                }
                else
                {
                    auto item_set = main_server->GetPlayerItemInventory(acc_cache, 25, current_character);
                    if (item_set.has_value())
                    {
                        const auto set_item_types = main_server->GetSetItemTypes(item_set->item_info.item_number.item_id);
                        for (const auto& item_type : set_item_types)
                        {
                            if (item_type == item_info->Type)
                            {
                                DEBUGLOG(dark_cyan, "player ({}) had an set but part replaced it type {}", acc_cache->acc_info.Nickname.c_str(), item_type);
                                AddEquip(std::nullopt, 25, false, current_character);
                                break;
                            }
                        }
                    }
                }
                auto curr_item = main_server->GetPlayerItemInventory(acc_cache, item_info->Type, current_character);
                if (curr_item.has_value())
                {
                    AddEquip(curr_item.value().item_info.serial_info, std::nullopt, false, current_character);
                    DEBUGLOG(dark_cyan, "player ({}) had an same type ({}) but replaced", acc_cache->acc_info.Nickname.c_str(), item_info->Type);
                }
                DEBUGLOG(dark_cyan, "player ({}) have equipped item type ({})", acc_cache->acc_info.Nickname.c_str(), item_info->Type);
                AddEquip(item.item_info.serial_info, std::nullopt, true, current_character);

                if (item.item_info.expire_date == ItemExpire::Type::Unused)
                    AddExpire(*item_info, item.item_info.serial_info);
            }
        }
        auto validated = main_server->ValidateDatabaseUpdates(acc_cache, dctx);
        if (!validated.has_value())
        {
            DEBUGLOG(red, "ValidateDatabaseUpdates failed for [{}] [{}]: {}", acc_cache->acc_info.Index, acc_cache->acc_info.Nickname.c_str(), static_cast<int>(validated.error()));
            return;
        }
        acc_cache.unlock();

        [[maybe_unused]] auto ignored_result = BaseLib::DbPool->submit_task([main_server,
            session = std::move(callback.session),
            session_id = session_id,
            p_order = std::move(order), items_count = std::move(equip_items_count),
            v = std::move(validated.value())
        ]() mutable
            {
                if (!session) return;

                auto new_acc_cache = CAccount.get<unique_t>(session->GetSessionId());
                ResultDbUpdateInfo dbres;

                if (!BaseLib::Database->UpdateAccount(v, dbres).has_value()) return;

                auto applied = main_server->ApplyDatabaseUpdates(new_acc_cache, v);
                if (!applied.has_value())
                {
                    DEBUGLOG(red, "ApplyDatabaseUpdates failed for [{}] [{}]: {}", new_acc_cache->acc_info.Index, new_acc_cache->acc_info.Nickname.c_str(), static_cast<int>(applied.error()));
                    return;
                }
                session->SendMsg(p_order, 0, 51, items_count);
                auto uid = new_acc_cache->uid.data;
                if (new_acc_cache->in_room)
                {
                    if (CRoom.contains(new_acc_cache->room_id))
                    {
                        auto room = CRoom.get<shared_t>(new_acc_cache->room_id);
                        auto selected_character = new_acc_cache->acc_info.SelectedCharacter;
                        auto voice_id = new_acc_cache->voice_id;
                        auto equipped_items = main_server->GetEquippedItems(new_acc_cache);
                        auto equip_data = EquipInfoAck(new_acc_cache->uid, equipped_items);
                        new_acc_cache.unlock();
                        
                        auto players_ids = main_server->GetRoomSortedPlayerSessionIds(room);
                        for (const auto& room_player_session_id : players_ids)
                        {
                            if (room_player_session_id == session_id) continue;
                            if (auto player_session = main_server->GetSessionById(room_player_session_id))
                            {
                                player_session->SendMsg(414, 0, selected_character, 17, reinterpret_cast<uint8_t*>(&equip_data), sizeof(EquipInfoAck));
                                player_session->SendMsg(314, 0, 0, voice_id, reinterpret_cast<uint8_t*>(&uid), sizeof(uid));
                                DEBUGLOG(dark_cyan, "player ({}) sent equip update to player session id ({}) in room ({})", new_acc_cache->acc_info.Nickname.c_str(), room_player_session_id, room->room_id);
                            }
                        }
                        new_acc_cache.lock();
                    }
                }
                if (new_acc_cache->in_plaza)
                {
                    if (main_server->IsPlazaAlready(new_acc_cache->plaza_id))
                    {
                        auto plaza = CPlaza.get<shared_t>(new_acc_cache->plaza_id);
                        auto selected_character = new_acc_cache->acc_info.SelectedCharacter;
                        auto voice_id = new_acc_cache->voice_id;
                        auto equipped_items = main_server->GetEquippedItems(new_acc_cache);
                        auto equip_data = EquipInfoAck(new_acc_cache->uid, equipped_items);
                        new_acc_cache.unlock();
                        auto& players_ids = plaza->session_ids;
                        for (const auto& plaza_player_session_id : players_ids)
                        {
                            if (plaza_player_session_id == session_id) continue;
                            if (auto player_session = main_server->GetSessionById(plaza_player_session_id))
                            {
                                player_session->SendMsg(414, 0, selected_character, 17, reinterpret_cast<uint8_t*>(&equip_data), sizeof(EquipInfoAck));
                                player_session->SendMsg(314, 0, 0, voice_id, reinterpret_cast<uint8_t*>(&uid), sizeof(uid));
                            }
                        }
                        new_acc_cache.lock();
                    }
                }
            }
        );
    }
}