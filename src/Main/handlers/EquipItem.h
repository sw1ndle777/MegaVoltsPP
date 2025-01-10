#pragma once
namespace Game
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Main;

    namespace Handlers
    {
        inline void EquipItem(SCallbackData& callback, CMainServer* main_server)
        {
            auto send_msg = [&](CSession* session, std::uint16_t order, std::uint8_t mission, std::uint8_t extra, std::uint8_t option, std::uint8_t* data = nullptr, std::size_t data_size = 0)
            {
                CMessage message(session->GetEncryptionKey());
                message.SetSession(session->GetSessionId());
                message.SetCommand(order, mission, extra, option);
                if (data_size > 0 && data != nullptr) message.SetData(data, data_size);
                session->Send(message);
            };
            std::shared_lock lock(callback.session->GetMutex());
            CSession* session = callback.session;
            CServer* server = callback.server;

            auto session_id = session->GetSessionId();
            auto acc_cache = main_server->GetAccCacheUniqueBySessionId(session_id);
            auto acc_index = acc_cache->acc_info.Index;
            auto equip_remove_type = callback.message->GetExtra();
            auto equip_items_count = callback.message->GetOption();
            auto equip_update_type = callback.message->GetMission();
            if (acc_index == -1) return;
            const auto& charEquipUpdateReq = reinterpret_cast<MainCharacterEquipUpdateReq*>(callback.message->GetData());

            auto current_character = static_cast<std::uint8_t>(acc_cache->acc_info.SelectedCharacter);
            BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "player ({}) equip update type: ({}), remove type: ({}), items count: ({}) on character ({})", acc_cache->acc_info.Nickname.c_str(), equip_update_type, equip_remove_type, equip_items_count, current_character);

            //al meu
            if (equip_remove_type == 17) equip_remove_type = 25;
           
            for (std::uint32_t i = 0; i < equip_items_count; i++)
            {
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "player ({}) sent item data: ({})", acc_cache->acc_info.Nickname.c_str(), charEquipUpdateReq->item[i].data);
            }

            if (equip_remove_type == 51)
            {
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "player ({}) unsafe equip change type 51 for {} items", acc_cache->acc_info.Nickname.c_str(), equip_items_count);
                const auto& charEquipSwitchReq = reinterpret_cast<MainCharacterEquipSwitchReq*>(callback.message->GetData());
                for (std::uint32_t i = 0; i < equip_items_count; i++)
                {
                    auto character_id = charEquipSwitchReq->item[i].character_id;
                    auto item_id = charEquipSwitchReq->item[i].item_id;
                    auto item_inv = main_server->GetPlayerItemInventory(acc_cache, item_id);
                    if (!item_inv.has_value()) continue;
                    const auto& item = item_inv.value();
                    auto item_info = main_server->GetItemInfoCache(item.item_info.item_number.item_id);
                    if (main_server->IsItemSet(item_id))
                    {
                        const auto& set_item_types = main_server->GetSetItemTypes(item_id);
                        for (const auto& item_type : set_item_types)
                        {
                            auto curr_item = main_server->GetPlayerItemInventory(acc_cache, item_type, character_id);
                            if (curr_item.has_value())
                            {
                                main_server->UpdatePlayerItemEquip(acc_cache, item_type, character_id, false);
                                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "player ({}) had an item but set replaced part type ({})", acc_cache->acc_info.Nickname.c_str(), item_type);
                            }
                        }
                    }
                    main_server->UpdatePlayerItemEquip(acc_cache, item_info->Type, character_id, false);

                    main_server->UpdatePlayerItemEquip(acc_cache, item.item_info.serial_info, character_id, true);
                }
            }

            if (equip_update_type == 1) {
                main_server->UpdatePlayerItemEquip(acc_cache, equip_remove_type, current_character, false);
                if (equip_items_count == 0) { // UNEQUIP ITEM TYPE BASED ON REMOVETYPE !
                    BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "player ({}) singlehand unequip item type ({})", acc_cache->acc_info.Nickname.c_str(), equip_remove_type);
                }
                else {
                    BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "player ({}) unknown action on update_type 1: remove type {}, item count {}", acc_cache->acc_info.Nickname.c_str(), equip_remove_type, equip_items_count);
                }
            }

            if (equip_update_type == 0) {
                for (std::uint32_t i = 0; i < equip_items_count; i++)
                {
                    if (charEquipUpdateReq->item[i].data <= 25) // UNEQUIP ITEM TYPE !
                    {
                        main_server->UpdatePlayerItemEquip(acc_cache, charEquipUpdateReq->item[i].data, current_character, false);
                        continue;
                    }
                    // ELSE EQUIP SERIALINFO !
                    auto item_inv = main_server->GetPlayerItemInventory(acc_cache, charEquipUpdateReq->item[i]);
                    if (!item_inv.has_value())
                    {
                        BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "player ({}) attempt equip but can't find item inventory", acc_cache->acc_info.Nickname.c_str());
                        continue;
                    }
                    const auto& item = item_inv.value();
                    auto item_info = main_server->GetItemInfoCache(item.item_info.item_number.item_id);
                    if (item_info->Name.empty())
                    {
                        BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "player ({}) attempt equip but can't find item in item info cache or it has no namme item id: ({})", acc_cache->acc_info.Nickname.c_str(), item.item_info.item_number.item_id);
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
                                main_server->UpdatePlayerItemEquip(acc_cache, item_type, current_character, false);
                                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "player ({}) had an item but set replaced part type ({})", acc_cache->acc_info.Nickname.c_str(), item_type);
                            }
                        }
                    }
                    else {
                        auto item_set = main_server->GetPlayerItemInventory(acc_cache, 25, current_character);
                        if (item_set.has_value()) {
                            const auto& set_item_types = main_server->GetSetItemTypes(item_set->item_info.item_number.item_id);
                            for (const auto& item_type : set_item_types)
                            {
                                if (item_type == item_info->Type) {
                                    BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "player ({}) had an set but part replaced it type {}", acc_cache->acc_info.Nickname.c_str(), item_type);
                                    main_server->UpdatePlayerItemEquip(acc_cache, 25, current_character, false);
                                    break;
                                }
                            }
                        }
                    }
                    auto curr_item = main_server->GetPlayerItemInventory(acc_cache, item_info->Type, current_character);
                    if (curr_item.has_value())
                    {
                        main_server->UpdatePlayerItemEquip(acc_cache, item_info->Type, current_character, false);
                        BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "player ({}) had an same type ({}) but replaced", acc_cache->acc_info.Nickname.c_str(), item_info->Type);
                    }
                    BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "player ({}) have equipped item type ({})", acc_cache->acc_info.Nickname.c_str(), item_info->Type);
                    main_server->UpdatePlayerItemEquip(acc_cache, item.item_info.serial_info, current_character, true);

                    if (item.item_info.expire_date == ItemExpire::Type::Unused)
                        main_server->UpdatePlayerItemExpireDate(acc_cache, item.item_info.serial_info, item_info->LimitedTime == 0 ? ItemExpire::Type::Unlimited : Utility::GetUtcTimeNowPlusSeconds(item_info->LimitedTime));
                }
            }



            send_msg(session, callback.message->GetOrder(), 0, 51, equip_items_count);

            return;//OLD CODE BELOW
            if (equip_remove_type == 51)
            {
                const auto& charEquipSwitchReq = reinterpret_cast<MainCharacterEquipSwitchReq*>(callback.message->GetData());
                for (std::uint32_t i = 0; i < equip_items_count; i++)
                {
                    auto character_id = charEquipSwitchReq->item[i].character_id;
                    auto item_id = charEquipSwitchReq->item[i].item_id;

                    auto item_inv = main_server->GetPlayerItemInventory(acc_cache, item_id);
                    if (!item_inv.has_value())
                    {
                        BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "player ({}) attempt switch but can't find item inventory", acc_cache->acc_info.Nickname.c_str());
                        continue;
                    }

                    const auto& item = item_inv.value();
                    auto item_info = main_server->GetItemInfoCache(item.item_info.item_number.item_id);
                    if (item_info->Name.empty())
                    {
                        BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "player ({}) attempt switch but can't find item in item info cache or it has no namme item id: ({})", acc_cache->acc_info.Nickname.c_str(), item.item_info.item_number.item_id);
                        continue;
                    }
                    if (main_server->IsItemSet(item_id))
                    {

                        const auto& set_item_types = main_server->GetSetItemTypes(item_id);
                        for (const auto& item_type : set_item_types)
                        {
                            const auto& uneqipped_items = main_server->UpdatePlayerItemEquip(acc_cache, item_type, character_id, false);
                            for (const auto& uneqipped_item : uneqipped_items)
                            {
                                auto uneqipped_item_info = main_server->GetItemInfoCache(uneqipped_item.item_info.item_number.item_id);
                                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "player ({}) switch unequipped item ({}) ({}) from character ({})",
                                    acc_cache->acc_info.Nickname.c_str(), uneqipped_item_info->Name.c_str(), uneqipped_item_info->NameTime.c_str(), main_server->GetCharacterStr(character_id).c_str());
                            }
                        }
                    }
                    else
                    {
                        auto item_set = main_server->GetPlayerItemInventory(acc_cache, 25, character_id);
                        if(item_set.has_value())
                        {
                            const auto& set_item_types = main_server->GetSetItemTypes(item_set->item_info.item_number.item_id);
                            for (const auto& item_type : set_item_types)
                            {
                                if (item_info->Type == item_type)
                                {
                                    const auto& uneqipped_items = main_server->UpdatePlayerItemEquip(acc_cache, 25, character_id, false);
                                    for (const auto& uneqipped_item : uneqipped_items)
                                    {
                                        auto uneqipped_item_info = main_server->GetItemInfoCache(uneqipped_item.item_info.item_number.item_id);
                                        BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "player ({}) unequipped item ({}) ({}) from character ({})",
                                            acc_cache->acc_info.Nickname.c_str(), uneqipped_item_info->Name.c_str(), uneqipped_item_info->NameTime.c_str(), main_server->GetCharacterStr(character_id).c_str());
                                    }
                                    break;
                                }
                            }
                        }
                        else
                        {
                            const auto& uneqipped_items = main_server->UpdatePlayerItemEquip(acc_cache, item_info->Type, character_id, false);
                            for (const auto& uneqipped_item : uneqipped_items)
                            {
                                auto uneqipped_item_info = main_server->GetItemInfoCache(uneqipped_item.item_info.item_number.item_id);
                                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "player ({}) switch unequipped item ({}) ({}) from character ({})",
                                    acc_cache->acc_info.Nickname.c_str(), uneqipped_item_info->Name.c_str(), uneqipped_item_info->NameTime.c_str(), main_server->GetCharacterStr(character_id).c_str());
                            }
                        }
                    }
                    


                    main_server->UpdatePlayerItemEquip(acc_cache, item.item_info.serial_info, character_id, true);

                    BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "player ({}) switch {} item ({}) ({}) to character ({})",
                        acc_cache->acc_info.Nickname.c_str(), item.is_equipped ? "unequipped" : "equipped", item_info->Name.c_str(), item_info->NameTime.c_str(), main_server->GetCharacterStr(character_id).c_str());

                    if (item.item_info.expire_date == ItemExpire::Type::Unused)
                        main_server->UpdatePlayerItemExpireDate(acc_cache, item.item_info.serial_info, item_info->LimitedTime == 0 ? ItemExpire::Type::Unlimited : Utility::GetUtcTimeNowPlusSeconds(item_info->LimitedTime));
                }
            }
            else if (equip_update_type == EquipUpdate::Type::Sigle) // unequip item
            {

                const auto& updated_items = main_server->UpdatePlayerItemEquip(acc_cache, equip_remove_type, current_character, false);
                for (const auto& item : updated_items)
                {
                    auto item_info = main_server->GetItemInfoCache(item.item_info.item_number.item_id);
                    BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "player ({}) single {} item ({}) ({}) to character ({})", 
                        acc_cache->acc_info.Nickname.c_str(), item.is_equipped ? "equipped" : "unequipped", item_info->Name.c_str(), item_info->NameTime.c_str(), main_server->GetCharacterStr(current_character).c_str());
                }
            }
            else if (equip_update_type == EquipUpdate::Type::Multiple) // needs rework
            {
                for (std::uint32_t i = 0; i < equip_items_count; i++)
                {
                    if (charEquipUpdateReq->item[i].data <= 25) // unequip item type
                    {
                        auto item_type = static_cast<std::uint32_t>(charEquipUpdateReq->item[i].data);
                        const auto& uneqipped_items = main_server->UpdatePlayerItemEquip(acc_cache, item_type, current_character, false);
                        for (const auto& uneqipped_item : uneqipped_items)
                        {
                            auto uneqipped_item_info = main_server->GetItemInfoCache(uneqipped_item.item_info.item_number.item_id);
                            BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "player ({}) multiple unequipped item ({}) ({}) from character ({})",
                                acc_cache->acc_info.Nickname.c_str(), uneqipped_item_info->Name.c_str(), uneqipped_item_info->NameTime.c_str(), main_server->GetCharacterStr(current_character).c_str());
                        }
                    }
                    else // equip item serial
                    {
                        auto item_inv = main_server->GetPlayerItemInventory(acc_cache, charEquipUpdateReq->item[i]);
                        if (!item_inv.has_value())
                        {
                            BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "player ({}) attempt equip but can't find item inventory", acc_cache->acc_info.Nickname.c_str());
                            continue;
                        }
                        const auto& item = item_inv.value();
                        auto item_info = main_server->GetItemInfoCache(item.item_info.item_number.item_id);
                        if (item_info->Name.empty())
                        {
                            BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "player ({}) attempt equip but can't find item in item info cache or it has no namme item id: ({})", acc_cache->acc_info.Nickname.c_str(), item.item_info.item_number.item_id);
                            continue;
                        }

                        //de ieri
                        if (main_server->IsItemSet(item.item_info.item_number.item_id)) {
                            const auto& set_item_types = main_server->GetSetItemTypes(item.item_info.item_number.item_id);
                            for (const auto& item_type : set_item_types) {
                                auto curr_item = main_server->GetPlayerItemInventory(acc_cache, item_type, current_character);
                                if (curr_item.has_value()) {
                                    main_server->UpdatePlayerItemEquip(acc_cache, item_type, current_character, false);
                                    BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "player ({}) had an item but set replaced part", acc_cache->acc_info.Nickname.c_str());
                                }
                            }
                        }
                        else {
                            auto curr_item = main_server->GetPlayerItemInventory(acc_cache, item_info->Type, current_character);
                            if (curr_item.has_value()) {
                                main_server->UpdatePlayerItemEquip(acc_cache, item_info->Type, current_character, false);
                                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "player ({}) had an part but replaced", acc_cache->acc_info.Nickname.c_str());
                            }
                        }

                        main_server->UpdatePlayerItemEquip(acc_cache, item.item_info.serial_info, current_character, true);

                        BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "player ({}) multiple {} item ({}) ({}) to character ({})",
                            acc_cache->acc_info.Nickname.c_str(), item.is_equipped ? "unequipped" : "equipped", item_info->Name.c_str(), item_info->NameTime.c_str(), main_server->GetCharacterStr(current_character).c_str());

                        if (item.item_info.expire_date == ItemExpire::Type::Unused)
                            main_server->UpdatePlayerItemExpireDate(acc_cache, item.item_info.serial_info, item_info->LimitedTime == 0 ? ItemExpire::Type::Unlimited : Utility::GetUtcTimeNowPlusSeconds(item_info->LimitedTime));

                        /*
                        if (main_server->IsItemSet(item.item_info.item_number.item_id))
                        {

                            const auto& set_item_types = main_server->GetSetItemTypes(item.item_info.item_number.item_id);
                            for (const auto& item_type : set_item_types)
                            {
                                const auto& uneqipped_items = main_server->UpdatePlayerItemEquip(acc_cache, item_type, current_character, false);
                                for (const auto& uneqipped_item : uneqipped_items)
                                {
                                    auto uneqipped_item_info = main_server->GetItemInfoCache(uneqipped_item.item_info.item_number.item_id);
                                    BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "player ({}) switch unequipped item ({}) ({}) from character ({})",
                                        acc_cache->acc_info.Nickname.c_str(), uneqipped_item_info->Name.c_str(), uneqipped_item_info->NameTime.c_str(), main_server->GetCharacterStr(current_character).c_str());
                                }
                            }
                        }
                        else
                        {
                            auto item_set = main_server->GetPlayerItemInventory(acc_cache, 25, current_character);
                            if (item_set.has_value())
                            {
                                const auto& set_item_types = main_server->GetSetItemTypes(item_set->item_info.item_number.item_id);
                                for (const auto& item_type : set_item_types)
                                {
                                    if (item_info->Type == item_type)
                                    {
                                        const auto& uneqipped_items = main_server->UpdatePlayerItemEquip(acc_cache, 25, current_character, false);
                                        for (const auto& uneqipped_item : uneqipped_items)
                                        {
                                            auto uneqipped_item_info = main_server->GetItemInfoCache(uneqipped_item.item_info.item_number.item_id);
                                            BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "player ({}) unequipped item ({}) ({}) from character ({})",
                                                acc_cache->acc_info.Nickname.c_str(), uneqipped_item_info->Name.c_str(), uneqipped_item_info->NameTime.c_str(), main_server->GetCharacterStr(current_character).c_str());
                                        }
                                        break;
                                    }
                                }
                            }
                            else
                            {
                                const auto& uneqipped_items = main_server->UpdatePlayerItemEquip(acc_cache, item_info->Type, current_character, false);
                                for (const auto& uneqipped_item : uneqipped_items)
                                {
                                    auto uneqipped_item_info = main_server->GetItemInfoCache(uneqipped_item.item_info.item_number.item_id);
                                    BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "player ({}) switch unequipped item ({}) ({}) from character ({})",
                                        acc_cache->acc_info.Nickname.c_str(), uneqipped_item_info->Name.c_str(), uneqipped_item_info->NameTime.c_str(), main_server->GetCharacterStr(current_character).c_str());
                                }
                            }
                        }


                        main_server->UpdatePlayerItemEquip(acc_cache, item.item_info.serial_info, current_character, true);

                        BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "player ({}) multiple {} item ({}) ({}) to character ({})",
                            acc_cache->acc_info.Nickname.c_str(), item.is_equipped ? "unequipped" : "equipped", item_info->Name.c_str(), item_info->NameTime.c_str(), main_server->GetCharacterStr(current_character).c_str());

                        if (item.item_info.expire_date == ItemExpire::Type::Unused)
                            main_server->UpdatePlayerItemExpireDate(acc_cache, item.item_info.serial_info, item_info->LimitedTime == 0 ? ItemExpire::Type::Unlimited : Utility::GetUtcTimeNowPlusSeconds(item_info->LimitedTime));
                        */
                    }

                }
            }
            send_msg(session, callback.message->GetOrder(), 0, 51, equip_items_count);
        }
    } 
}