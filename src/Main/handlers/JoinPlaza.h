#pragma once
namespace Game
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Main;

    namespace Handlers
    {
        inline std::vector<std::uint8_t> get_equipped_data(CMainServer* main_server, const std::uint16_t& sess_id)
        {
            auto cache_acc = main_server->GetAccCacheSharedBySessionId(sess_id);
            RoomUserPlayerInfo1 my_info1{}; RoomUserPlayerInfo2 my_info2{};
            my_info1.grade = cache_acc->acc_info.Grade;
            my_info1.vip_level = cache_acc->acc_info.VIPExperience;
            my_info1.character = cache_acc->acc_info.SelectedCharacter;
            my_info1.team = cache_acc->team_id;
        #if defined(RELEASE_1_0_3)
            my_info1.level = cache_acc->acc_info.Level;
        #else
            my_info1.level = cache_acc->acc_info.Level + 1;
        #endif
            my_info1.ping = 0;
            my_info2.player_state = cache_acc->state;
            my_info2.ping = cache_acc->ping;
            my_info2.fps_limit = cache_acc->fps_limit;
            const auto& my_unique_id = NetEngine::Packets::Core::UniqueId(sess_id, 1).data;

            std::vector<BaseLib::Item> my_equipped_items;
            for (const auto& item : cache_acc->inventory_items)
                if (item.is_equipped == 1 && item.character_id == static_cast<std::uint8_t>(cache_acc->acc_info.SelectedCharacter))
                    my_equipped_items.push_back(item);

           

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
            const auto& my_EquippedHairItemId = my_hair_id ? my_hair_id : (my_setitem_info->Hair > 0 ? my_setitem_info->Id : 0);
            const auto& my_EquippedFaceItemId = my_face_id ? my_face_id : (my_setitem_info->Face > 0 ? my_setitem_info->Id : 0);
            const auto& my_EquippedUpperItemId = my_upper_id ? my_upper_id : (my_setitem_info->Upper > 0 ? my_setitem_info->Id : 0);
            const auto& my_EquippedUnderItemId = my_under_id ? my_under_id : (my_setitem_info->Under > 0 ? my_setitem_info->Id : 0);
            const auto& my_EquippedPantsItemId = my_pants_id ? my_pants_id : (my_setitem_info->Pants > 0 ? my_setitem_info->Id : 0);
            const auto& my_EquippedShirtItemId = my_shirt_id ? my_shirt_id : (my_setitem_info->Arms > 0 ? my_setitem_info->Id : 0);
            const auto& my_EquippedBootsItemId = my_boots_id ? my_boots_id : (my_setitem_info->Boots > 0 ? my_setitem_info->Id : 0);
            const auto& my_EquippedGlassItemId = my_acc_head_id ? my_acc_head_id : (my_setitem_info->AccessoryA > 0 ? my_setitem_info->Id : 0);
            const auto& my_EquippedAccessoryWaistItemId = my_acc_waist_id ? my_acc_waist_id : (my_setitem_info->AccessoryB > 0 ? my_setitem_info->Id : 0);
            const auto& my_EquippedAccessoryBackItemId = my_acc_back_id ? my_acc_back_id : (my_setitem_info->AccessoryC > 0 ? my_setitem_info->Id : 0);
            const auto& my_EquippedMeleeItemId = main_server->GetItemByType(my_equipped_items, 10).item_info.item_number.item_id;
            const auto& my_EquippedRifleItemId = main_server->GetItemByType(my_equipped_items, 11).item_info.item_number.item_id;
            const auto& my_EquippedShotgunItemId = main_server->GetItemByType(my_equipped_items, 12).item_info.item_number.item_id;
            const auto& my_EquippedSniperItemId = main_server->GetItemByType(my_equipped_items, 13).item_info.item_number.item_id;
            const auto& my_EquippedGatlingItemId = main_server->GetItemByType(my_equipped_items, 14).item_info.item_number.item_id;
            const auto& my_EquippedGrenadeItemId = main_server->GetItemByType(my_equipped_items, 15).item_info.item_number.item_id;
            const auto& my_EquippedBazookaItemId = main_server->GetItemByType(my_equipped_items, 16).item_info.item_number.item_id;
            auto playerEnterInfoData = MainRoomPlayerEnterInfoAck(cache_acc->acc_info.Nickname.c_str(), my_unique_id, my_info1, my_info2,
                my_EquippedHairItemId, my_EquippedFaceItemId, my_EquippedUpperItemId,
                my_EquippedUnderItemId, my_EquippedPantsItemId, my_EquippedShirtItemId,
                my_EquippedBootsItemId, my_EquippedGlassItemId, my_EquippedAccessoryWaistItemId,
                my_EquippedAccessoryBackItemId, my_EquippedMeleeItemId, my_EquippedRifleItemId,
                my_EquippedShotgunItemId, my_EquippedSniperItemId, my_EquippedGatlingItemId,
                my_EquippedGrenadeItemId, my_EquippedBazookaItemId).Serialize();

            cache_acc.unlock();

            return playerEnterInfoData;
        };

        inline void JoinPlaza(SCallbackData& callback, CMainServer* main_server)
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
            if (acc_cache->acc_info.Index == -1) return;
            auto joinPlazaReq = reinterpret_cast<MainJoinPlazaReq*>(callback.message->GetData());
            auto plaza_id = joinPlazaReq->plaza_id;
            auto channel_id = joinPlazaReq->channel_id;
            BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "player ({}) join plaza attempt -> plaza id: ({}), plaza server/channel id: ({}), mission: ({}),  extra: ({}), option: ({})", acc_cache->acc_info.Nickname.c_str(), plaza_id, channel_id, callback.message->GetMission(), callback.message->GetExtra(), callback.message->GetOption());
            auto old_plaza_id = acc_cache->plaza_id;

            auto removeOldPlazaPlayer = [&](std::uint32_t plaza_id)
            {
                auto old_plaza = main_server->GetPlazaCacheUnique(plaza_id);
                auto& session_ids = old_plaza->session_ids;
                if (main_server->IsSessionIdAlready(session_id, session_ids))
                {
                    if (main_server->IsPlazaBroadcastable(old_plaza))
                    {
                        auto my_unique_id = NetEngine::Packets::Core::UniqueId(session_id, 1).data;
                        for (const auto& plaza_player_session_id : session_ids)
                        {
                            if (plaza_player_session_id == session_id) continue;
                            if (auto player_session = server->GetSessionById(plaza_player_session_id))
                                send_msg(player_session.get(), 425, 0, 0, 1, reinterpret_cast<uint8_t*>(&my_unique_id), sizeof(my_unique_id)); // disconnect
                        }
                    }
                    BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "session id: ({}) left plaza id: ({})", session_id, plaza_id);
                    auto remove_myself = std::remove(old_plaza->session_ids.begin(), old_plaza->session_ids.end(), session_id);
                    old_plaza->session_ids.erase(remove_myself, old_plaza->session_ids.end());
                }
                old_plaza.unlock();
            };

            auto updatePlayerEquipInfo = [&](PlazaCacheResource& plaza)
            {
                if (main_server->IsPlazaBroadcastable(plaza))
                {
                    auto playerEnterInfoData = get_equipped_data(main_server, session_id);
                    auto& players_ids = plaza->session_ids;
                    for (const auto& plaza_player_session_id : players_ids)
                    {
                        if (plaza_player_session_id == session_id) continue;
                        if (auto player_session = server->GetSessionById(plaza_player_session_id))
                        {
                            send_msg(player_session.get(), 424, 0, 0, 1, reinterpret_cast<uint8_t*>(playerEnterInfoData.data()), playerEnterInfoData.size());
                            auto other_player_equipped_data = get_equipped_data(main_server, plaza_player_session_id);
                            send_msg(session, 424, 0, 0, 1, reinterpret_cast<uint8_t*>(other_player_equipped_data.data()), other_player_equipped_data.size());
                        }
                    }
                }
            };

            if (plaza_id == 0)
            {
                if (main_server->IsPlazaAlready(0))
                {
                    auto plaza_id_0 = main_server->GetPlazaCacheShared(plaza_id);
                    if (main_server->IsPlazaFull(plaza_id_0))
                    {
                        plaza_id_0.unlock();
                        //send_msg(session, 173, 0, PlazaJoin::Result::Full, 0);
                        auto best_plaza_id = main_server->FindFirstNonFullPlaza();
                        if (best_plaza_id != old_plaza_id && main_server->IsPlazaAlready(old_plaza_id)) removeOldPlazaPlayer(old_plaza_id);
                        auto current_plaza = main_server->GetPlazaCacheUnique(best_plaza_id);
                        acc_cache->plaza_id = best_plaza_id;
                        acc_cache->in_plaza = true;
                        current_plaza->session_ids.push_back(session_id);
                        BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "player ({}) join first plaza -> plaza id: ({}) already exists, connecting player there", acc_cache->acc_info.Nickname.c_str(), best_plaza_id);
                        send_msg(session, 173, 0, PlazaJoin::Result::Success, 0, reinterpret_cast<uint8_t*>(&best_plaza_id), sizeof(best_plaza_id));
                        acc_cache.unlock();
                        updatePlayerEquipInfo(current_plaza);
                        current_plaza.unlock();
                            
                    }
                    else
                    {
                        plaza_id_0.unlock();
                       
                        if (plaza_id != old_plaza_id && main_server->IsPlazaAlready(old_plaza_id)) removeOldPlazaPlayer(old_plaza_id);
                        auto current_plaza = main_server->GetPlazaCacheUnique(plaza_id);
                        acc_cache->plaza_id = plaza_id;
                        acc_cache->in_plaza = true;
                        current_plaza->session_ids.push_back(session_id);
                        BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "player ({}) join first plaza -> plaza id: ({}) already exists, connecting player there", acc_cache->acc_info.Nickname.c_str(), plaza_id);
                        send_msg(session, 173, 0, PlazaJoin::Result::Success, 0, reinterpret_cast<uint8_t*>(&plaza_id), sizeof(plaza_id)); // join plaza success
                        acc_cache.unlock();
                        updatePlayerEquipInfo(current_plaza);
                        current_plaza.unlock();
                    }
                }
                else
                {
                    std::uint16_t current_plaza_id = 0;
                    server->GetNextAvailablePlazaId(current_plaza_id);
                    //server->SetPlazaIdAvailable(current_plaza_id);
                    auto new_plaza = Plaza(current_plaza_id, 2);
                    if (current_plaza_id != old_plaza_id && main_server->IsPlazaAlready(old_plaza_id)) removeOldPlazaPlayer(old_plaza_id);
                    acc_cache->plaza_id = current_plaza_id;
                    acc_cache->in_plaza = true;
                    new_plaza.session_ids.push_back(session_id);
                    main_server->AddPlazaCache(current_plaza_id, new_plaza);
                    BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "player ({}) join first plaza -> plaza id: ({}) doesn't exists, creating plaza then connecting player there", acc_cache->acc_info.Nickname.c_str(), plaza_id);
                    send_msg(session, 173, 0, PlazaJoin::Result::Success, 0, reinterpret_cast<uint8_t*>(&current_plaza_id), sizeof(current_plaza_id)); // join plaza success
                    auto current_plaza = main_server->GetPlazaCacheUnique(current_plaza_id);
                    acc_cache.unlock();
                    updatePlayerEquipInfo(current_plaza);
                }  
            }
            else
            {
                if (main_server->IsPlazaAlready(plaza_id))
                {
                    auto plaza = main_server->GetPlazaCacheUnique(plaza_id);

                    if (main_server->IsSessionIdAlready(session_id, plaza->session_ids))
                    {
                        BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "player ({}) join plaza -> plaza id: ({}), session already here, disconnect", acc_cache->acc_info.Nickname.c_str(), plaza_id);
                        main_server->DisconnectPlayer(server, session_id, Disconnect::Reason::Close);
                    }
                    else
                    {
                        if (main_server->IsPlazaFull(plaza))
                        {
                            send_msg(session, 173, 0, PlazaJoin::Result::Full, 0);
                            plaza.unlock();
                            auto best_plaza_id = main_server->FindFirstNonFullPlaza();
                            if (best_plaza_id != old_plaza_id && main_server->IsPlazaAlready(old_plaza_id)) removeOldPlazaPlayer(old_plaza_id);
                            auto current_plaza = main_server->GetPlazaCacheUnique(best_plaza_id);
                            acc_cache->plaza_id = best_plaza_id;
                            acc_cache->in_plaza = true;
                            current_plaza->session_ids.push_back(session_id);
                            BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "player ({}) join first plaza -> plaza id: ({}) already exists, connecting player there", acc_cache->acc_info.Nickname.c_str(), best_plaza_id);
                            send_msg(session, 173, 0, PlazaJoin::Result::Success, 0, reinterpret_cast<uint8_t*>(&best_plaza_id), sizeof(best_plaza_id));
                            acc_cache.unlock();
                            updatePlayerEquipInfo(current_plaza);
                            current_plaza.unlock();
                        }
                        else
                        {
                            if (plaza_id != old_plaza_id && main_server->IsPlazaAlready(old_plaza_id)) removeOldPlazaPlayer(old_plaza_id);
                            acc_cache->plaza_id = plaza_id;
                            acc_cache->in_plaza = true;
                            plaza->session_ids.push_back(session_id);
                            BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "player ({}) join plaza -> plaza id: ({}) already exists, connecting player there", acc_cache->acc_info.Nickname.c_str(), plaza_id);
                            send_msg(session, 173, 0, PlazaJoin::Result::Success, 0, reinterpret_cast<uint8_t*>(&plaza_id), sizeof(plaza_id)); // join plaza success
                            acc_cache.unlock();
                            updatePlayerEquipInfo(plaza);
                        }
                    }
                    plaza.unlock();
                }
                else
                {
                    std::uint16_t current_plaza_id = plaza_id;
                    //server->SetPlazaIdAvailable(current_plaza_id);
                    auto new_plaza = Plaza(current_plaza_id, 2);
                    if (current_plaza_id != old_plaza_id && main_server->IsPlazaAlready(old_plaza_id)) removeOldPlazaPlayer(old_plaza_id);
                    acc_cache->plaza_id = current_plaza_id;
                    acc_cache->in_plaza = true;
                    new_plaza.session_ids.push_back(session_id);
                    main_server->AddPlazaCache(current_plaza_id, new_plaza);
                    BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "player ({}) join plaza -> plaza id: ({}) doesn't exists, creating plaza then connecting player there", acc_cache->acc_info.Nickname.c_str(), plaza_id);
                    send_msg(session, 173, 0, PlazaJoin::Result::Success, 0, reinterpret_cast<uint8_t*>(&current_plaza_id), sizeof(current_plaza_id)); // join plaza success
                    auto current_plaza = main_server->GetPlazaCacheUnique(current_plaza_id);
                    acc_cache.unlock();
                    updatePlayerEquipInfo(current_plaza);
                    current_plaza.unlock();
                } 
            }
        }
    }
}