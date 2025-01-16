#pragma once
namespace Game
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Main;

    namespace Handlers
    {
        inline void PlayerChangeCharacter(SCallbackData& callback, CMainServer* main_server)
        {
            auto send_msg = [&](CSession* session, std::uint16_t order, std::uint8_t mission, std::uint8_t extra, std::uint8_t option, std::uint8_t* data = nullptr, std::uint16_t data_size = 0)
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
            auto selected_character = static_cast<Character::Type>(callback.message->GetOption());
            acc_cache->acc_info.SelectedCharacter = static_cast<std::uint32_t>(selected_character);
            auto auth_key = acc_cache->acc_info.AuthKey;
            auto my_unique_id = NetEngine::Packets::Core::UniqueId(session_id, 1).data;

            send_msg(session, 74, 0, CharacterSelectInfo::Result::Ok, static_cast<std::uint8_t>(selected_character));

            
            /*
            asio::post([selected_character, auth_key]()
            {
                
            });
            */

            if (!acc_cache->in_room || !main_server->IsRoomAlready(acc_cache->room_id)) return;
            auto room = main_server->GetRoomCacheShared(acc_cache->room_id);

            std::vector<BaseLib::Item> my_equipped_items;
            for (const auto& item : acc_cache->inventory_items)
                if (item.is_equipped == 1 && item.character_id == static_cast<std::uint8_t>(selected_character))
                    my_equipped_items.push_back(item);

            acc_cache.unlock();
            auto players_ids = main_server->GetRoomSortedPlayerSessionIds(room);

            const auto& my_set_item = main_server->GetItemByType(my_equipped_items, 25).item_info.item_number.item_id;
            auto my_setitem_info = main_server->GetSetItemInfoCache(my_set_item);
            const auto& my_hair_id = main_server->GetItemByType(my_equipped_items, 0).item_info.item_number.item_id;
            const auto& my_face_id = main_server->GetItemByType(my_equipped_items, 1).item_info.item_number.item_id;
            const auto& my_upper_id = main_server->GetItemByType(my_equipped_items, 2).item_info.item_number.item_id;
            const auto& my_under_id = main_server->GetItemByType(my_equipped_items, 3).item_info.item_number.item_id;
            const auto& my_pants_id = main_server->GetItemByType(my_equipped_items, 4).item_info.item_number.item_id;
            const auto& my_shirt_id = main_server->GetItemByType(my_equipped_items, 5).item_info.item_number.item_id;
            const auto& my_boots_id = main_server->GetItemByType(my_equipped_items, 6).item_info.item_number.item_id;
            const auto& my_acc_head_id = main_server->GetItemByType(my_equipped_items, 7).item_info.item_number.item_id;
            const auto& my_acc_waist_id = main_server->GetItemByType(my_equipped_items, 8).item_info.item_number.item_id;
            const auto& my_acc_back_id = main_server->GetItemByType(my_equipped_items, 9).item_info.item_number.item_id;
            auto my_EquippedHairItemId = EquipItemNumber(my_hair_id ? my_hair_id : (my_setitem_info->Hair > 0 ? my_setitem_info->Id : 0), 0);
            auto my_EquippedFaceItemId = EquipItemNumber(my_face_id ? my_face_id : (my_setitem_info->Face > 0 ? my_setitem_info->Id : 0), 1);
            auto my_EquippedUpperItemId = EquipItemNumber(my_upper_id ? my_upper_id : (my_setitem_info->Upper > 0 ? my_setitem_info->Id : 0), 2);
            auto my_EquippedUnderItemId = EquipItemNumber(my_under_id ? my_under_id : (my_setitem_info->Under > 0 ? my_setitem_info->Id : 0), 3);
            auto my_EquippedPantsItemId = EquipItemNumber(my_pants_id ? my_pants_id : (my_setitem_info->Pants > 0 ? my_setitem_info->Id : 0), 4);
            auto my_EquippedShirtItemId = EquipItemNumber(my_shirt_id ? my_shirt_id : (my_setitem_info->Arms > 0 ? my_setitem_info->Id : 0), 5);
            auto my_EquippedBootsItemId = EquipItemNumber(my_boots_id ? my_boots_id : (my_setitem_info->Boots > 0 ? my_setitem_info->Id : 0), 6);
            auto my_EquippedGlassItemId = EquipItemNumber(my_acc_head_id ? my_acc_head_id : (my_setitem_info->AccessoryA > 0 ? my_setitem_info->Id : 0), 7);
            auto my_EquippedAccessoryWaistItemId = EquipItemNumber(my_acc_waist_id ? my_acc_waist_id : (my_setitem_info->AccessoryB > 0 ? my_setitem_info->Id : 0), 8);
            auto my_EquippedAccessoryBackItemId = EquipItemNumber(my_acc_back_id ? my_acc_back_id : (my_setitem_info->AccessoryC > 0 ? my_setitem_info->Id : 0), 9);
            auto my_EquippedMeleeItemId = EquipItemNumber(main_server->GetItemByType(my_equipped_items, 10).item_info.item_number.item_id, 10);
            auto my_EquippedRifleItemId = EquipItemNumber(main_server->GetItemByType(my_equipped_items, 11).item_info.item_number.item_id, 11);
            auto my_EquippedShotgunItemId = EquipItemNumber(main_server->GetItemByType(my_equipped_items, 12).item_info.item_number.item_id, 12);
            auto my_EquippedSniperItemId = EquipItemNumber(main_server->GetItemByType(my_equipped_items, 13).item_info.item_number.item_id, 13);
            auto my_EquippedGatlingItemId = EquipItemNumber(main_server->GetItemByType(my_equipped_items, 14).item_info.item_number.item_id, 14);
            auto my_EquippedGrenadeItemId = EquipItemNumber(main_server->GetItemByType(my_equipped_items, 15).item_info.item_number.item_id, 15);
            auto my_EquippedBazookaItemId = EquipItemNumber(main_server->GetItemByType(my_equipped_items, 16).item_info.item_number.item_id, 16);

            auto equip_data = MainRoomPlayersEquipInfoUpdateRoomAck(my_unique_id,
                my_EquippedHairItemId.data, my_EquippedFaceItemId.data, my_EquippedUpperItemId.data,
                my_EquippedUnderItemId.data, my_EquippedPantsItemId.data, my_EquippedShirtItemId.data,
                my_EquippedBootsItemId.data, my_EquippedGlassItemId.data, my_EquippedAccessoryWaistItemId.data,
                my_EquippedAccessoryBackItemId.data, my_EquippedMeleeItemId.data, my_EquippedRifleItemId.data,
                my_EquippedShotgunItemId.data, my_EquippedSniperItemId.data, my_EquippedGatlingItemId.data,
                my_EquippedGrenadeItemId.data, my_EquippedBazookaItemId.data);


            for (const auto& room_player_session_id : players_ids)
            {
                if (room_player_session_id == session_id) continue;
                if (auto player_session = server->GetSessionById(room_player_session_id))
                    send_msg(player_session.get(), 414, 0, selected_character, 17, reinterpret_cast<uint8_t*>(&equip_data), sizeof(MainRoomPlayersEquipInfoUpdateRoomAck));
            }
            BaseLib::Database->UpdateSelectedCharacter(static_cast<std::uint32_t>(selected_character), auth_key);
        }
    }
    
}