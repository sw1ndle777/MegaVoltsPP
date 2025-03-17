#pragma once
namespace Game
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Main;

    namespace Handlers
    {
        inline void JoinRoom(SCallbackData& callback, CMainServer* main_server)
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
            auto acc_index = acc_cache->acc_info.Index;
            std::int32_t current_team_id = -1;
            auto join_result = static_cast<NetEngine::Room::Join::ReqResult>(callback.message->GetExtra());
            if (acc_index == -1) return;
            const auto& joinRoomReq = reinterpret_cast<MainJoinRoomReq*>(callback.message->GetData());
            auto room_cache = main_server->GetRoomCacheUnique(joinRoomReq->room_id);
            BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "player ({}) attempt to join Room No. ({}), channel id: ({})", session->GetSessionId(), joinRoomReq->room_id, joinRoomReq->channel_id);          
            if (room_cache->title.empty())
            {
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "fail: no title");
                send_msg(session, 140, 0, NetEngine::Room::Join::Result::RoomDeleted, 0);
                return;
            }
            if (acc_cache->in_room)
            {
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "fail: already in room");
                send_msg(session, 140, 0, NetEngine::Room::Join::Result::GenericError, 0);
                return;
            }
            if (room_cache->has_password || join_result == NetEngine::Room::Join::ReqResult::Password)
            {
                auto room_pass_req = std::string(joinRoomReq->password);
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "player try to join with password: ({})", room_pass_req);
                if (room_pass_req.empty() || room_pass_req != room_cache->password)
                {
                    send_msg(session, 140, 0, NetEngine::Room::Join::Result::InvalidPassword, 0);
                    return;
                }
            }

            auto in_party = acc_cache->in_party;
            bool is_clan = false;
            bool is_my_party = false;
            bool is_vs_party = false;
            if (in_party) {
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "join in party room");
                if (room_cache->host_session_id == session_id) {
                    BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "host try to join room!");
                    return;
                }
                auto host_cache = main_server->GetAccCacheSharedBySessionId(room_cache->host_session_id);
                if (!host_cache->in_party) {
                    BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "player try to join party a room which isnt a party!");
                    return;
                }
                auto host_party_id = host_cache->party_id;
                host_cache.unlock();
                auto party_cache = main_server->GetPartyCacheShared(host_party_id);
                is_clan = party_cache->is_clan;
                party_cache.unlock();
                if (acc_cache->party_id == host_party_id) {
                    is_my_party = true;
                }
                else {
                    is_vs_party = true;
                }
            }
            BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "all checks passed");
            auto is_mode_teambased = main_server->IsModeTeamBased(static_cast<NetEngine::Room::Mode::Index>(room_cache->ModeIndex));
            auto observers_max_count = room_cache->allow_observers ? 10 : 0;
            auto room_players_max_count = room_cache->max_players;
            std::uint32_t players_count = is_mode_teambased ? room_cache->redteam_session_ids.size() + room_cache->blueteam_session_ids.size() : room_cache->neutralteam_session_ids.size();
            if (room_cache->allow_observers) players_count += static_cast<std::uint32_t>(room_cache->observers_session_ids.size());
            if (players_count >= room_players_max_count + observers_max_count)
            {
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "room was full");
                send_msg(session, 140, 0, NetEngine::Room::Join::Result::LobbyFull, 0);
                return;
            }
            if (main_server->IsSessionIdAlready(session_id, room_cache->neutralteam_session_ids) ||
                main_server->IsSessionIdAlready(session_id, room_cache->redteam_session_ids) ||
                main_server->IsSessionIdAlready(session_id, room_cache->blueteam_session_ids) ||
                main_server->IsSessionIdAlready(session_id, room_cache->observers_session_ids))
            {
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "already in room");
                send_msg(session, 140, 0, NetEngine::Room::Join::Error::AlreadyInRoom, 0);
                return;
            }
            if (main_server->IsSessionIdAlready(session_id, room_cache->kicked_session_ids))
            {
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "player was kicked");
                send_msg(session, 140, 0, NetEngine::Room::Join::Result::PreviouslyKicked, 0);
                return;
            }
            if (!is_mode_teambased)
            {
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "is not team based");
                if (room_cache->neutralteam_session_ids.size() < room_cache->max_players)
                {
                    room_cache->neutralteam_session_ids.push_back(session_id);
                    acc_cache->team_id = Team::IdType::Neutral;
                    current_team_id = Team::IdType::Neutral;
                }
                else
                {
                    if (room_cache->allow_observers)
                    {
                        if (room_cache->observers_session_ids.size() < 10)
                        {
                            room_cache->observers_session_ids.push_back(session_id);
                            acc_cache->team_id = Team::IdType::Observer;
                            current_team_id = Team::IdType::Observer;
                        }
                        else
                            send_msg(session, 140, 0, room_cache->allow_observers ? NetEngine::Room::Join::Error::RoomFull : NetEngine::Room::Join::Error::NoIntrusion, 0);
                    }
                    else
                        send_msg(session, 140, 0, room_cache->allow_observers ? NetEngine::Room::Join::Error::RoomFull : NetEngine::Room::Join::Error::NoIntrusion, 0);
                }
            }
            else if (in_party)
            {
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "player will join party battle with team case handling");
                if (is_my_party)
                {
                    room_cache->blueteam_session_ids.push_back(session_id);
                    acc_cache->team_id = Team::IdType::Blue;
                    current_team_id = Team::IdType::Blue;
                }
                else if (is_vs_party)
                {
                    room_cache->redteam_session_ids.push_back(session_id);
                    acc_cache->team_id = Team::IdType::Red;
                    current_team_id = Team::IdType::Red;
                }
                else
                {
                    BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "critical error: player dont match any case in party battle join");
                }
            }
            else
            {
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "is team based");
                auto blue_team_size = room_cache->blueteam_session_ids.size();
                auto red_team_size = room_cache->redteam_session_ids.size();
                auto blue_team_not_full = blue_team_size < room_cache->max_players / 2;
                auto red_team_not_full = red_team_size < room_cache->max_players / 2;
                if (blue_team_size <= red_team_size && blue_team_not_full)
                {
                    BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "added to team blue");
                    room_cache->blueteam_session_ids.push_back(session_id);
                    acc_cache->team_id = Team::IdType::Blue;
                    current_team_id = Team::IdType::Blue;
                }
                else if ((red_team_size < blue_team_size && red_team_not_full) || (red_team_size <= blue_team_size && !blue_team_not_full && red_team_not_full))
                {
                    BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "added to team red");
                    room_cache->redteam_session_ids.push_back(session_id);
                    acc_cache->team_id = Team::IdType::Red;
                    current_team_id = Team::IdType::Red;
                }
                else if (!blue_team_not_full && !red_team_not_full)
                {
                    if (room_cache->allow_observers)
                    {
                        if (room_cache->observers_session_ids.size() < 10)
                        {
                            room_cache->observers_session_ids.push_back(session_id);
                            acc_cache->team_id = Team::IdType::Observer;
                            current_team_id = Team::IdType::Observer;
                        }
                        else
                            send_msg(session, 140, 0, room_cache->allow_observers ? NetEngine::Room::Join::Error::RoomFull : NetEngine::Room::Join::Error::NoIntrusion, 0);
                    }
                    else
                        send_msg(session, 140, 0, room_cache->allow_observers ? NetEngine::Room::Join::Error::RoomFull : NetEngine::Room::Join::Error::NoIntrusion, 0);
                }
            }
            BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "now prepare settings");
            auto has_password = static_cast<std::uint8_t>(!room_cache->password.empty());
            RoomSettingsInfo2 settings_info{};
            settings_info.map_index = room_cache->MapIndex;
            settings_info.mode_index = room_cache->ModeIndex;
            settings_info.max_players = room_cache->max_players;
            settings_info.restriction = room_cache->Restriction;
            settings_info.allow_intruders = room_cache->allow_intruders;
            settings_info.allow_observers = room_cache->allow_observers;
            settings_info.team_balance = room_cache->TeamBalance;
            if (room_cache->ModeIndex == NetEngine::Room::Mode::Index::BombBattle)
                settings_info.team_balance = NetEngine::Room::Balance::State::Disabled;
            settings_info.has_password = has_password;
            settings_info.hide_password = false;
            settings_info.is_clan_room = (acc_cache->in_party ? (is_clan ? 2 : 1) : 0);
            auto settings_data = MainRoomSettingsInfoAck(room_cache->password.c_str(), settings_info).Serialize();
            std::uint8_t high_room_id_part = (room_cache->room_id >> 8) & 0xFF; // Extract the high 8 bits
            std::uint8_t low_room_id_part = room_cache->room_id & 0xFF;
            BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "sending settings");
            send_msg(session, 139, has_password, low_room_id_part, high_room_id_part, reinterpret_cast<uint8_t*>(settings_data.data()), settings_data.size());
            RoomSettingsModeInfo2 mode_settings_info;
            mode_settings_info.time_limit = room_cache->time_rule;
            mode_settings_info.score_limit = room_cache->score_rule;
            mode_settings_info.allow_items = room_cache->allow_drops;
            mode_settings_info.restriction = room_cache->Restriction;
            std::vector<PlayerRoomClanListInfo> players_clan_info;
            /*
            if (acc_cache->acc_info.ClanId)
            {
                if (main_server->IsClanAlready(acc_cache->acc_info.ClanId))
                {
                    auto clan_info = main_server->GetClanCacheShared(acc_cache->acc_info.ClanId);
                    auto info = PlayerRoomClanListInfo(0, clan_info->clan_name.c_str(), clan_info->logo_front, clan_info->logo_back, acc_cache->acc_info.ClanId, 0);
                    clan_info.unlock();
                    players_clan_info.push_back(info);
                }
            }
            */
            acc_cache.unlock();
            auto players_ids = main_server->GetRoomSortedPlayerSessionIds(room_cache);
            
            auto players_size = players_ids.size();
            auto equipBlocksCount = players_size == 0 ? 0 : (players_size / 16) + 1;
            constexpr std::size_t MAX_PACKET_SIZE = 1440;          
            for (std::uint32_t batch_id = 0; batch_id < equipBlocksCount; batch_id++)
            {
                const std::uint32_t max_batch_size = (MAX_PACKET_SIZE - 8) / sizeof(MainRoomPlayersInfoAck);
                const std::uint8_t extra = (batch_id == 0) ? 37 : 0;
                std::vector<std::uint8_t> new_info;
                std::uint32_t block_size = 0;
                const std::uint32_t start_index = batch_id * max_batch_size;
                const std::uint32_t end_index = std::min(start_index + max_batch_size, static_cast<std::uint32_t>(players_ids.size()));
                for (auto i = start_index; i < end_index; i++)
                {
                    auto player_id = players_ids[i];
                    if (player_id == session_id) continue;
                    auto player_cache = main_server->GetAccCacheSharedBySessionId(player_id);
                    RoomUserPlayerInfo1 info1; RoomUserPlayerInfo2 info2;
                    info1.grade = player_cache->acc_info.Grade;
                    info1.vip_level = player_cache->acc_info.PCRoom;
                    info1.character = player_cache->acc_info.SelectedCharacter;
                    info1.team = player_cache->team_id;
                #if defined(RELEASE_1_0_3)
                    info1.level = player_cache->acc_info.Level + 1;
                #else
                    info1.level = player_cache->acc_info.Level + 1;
                #endif
                    info1.ping = player_cache->ping;
                    info2.fps_limit = player_cache->fps_limit;
                    info2.player_state = player_cache->state;
                    info2.ping = player_cache->ping;
                    auto unique_id = NetEngine::Packets::Core::UniqueId(player_id, 1).data;
                    auto player_data = MainRoomPlayersInfoAck(player_cache->acc_info.Nickname.c_str(), unique_id, info1, info2).Serialize();
                    new_info.insert(new_info.end(), player_data.begin(), player_data.end());
                    block_size++;

                    if (player_cache->acc_info.ClanId)
                    {
                        if (main_server->IsClanAlready(player_cache->acc_info.ClanId))
                        {
                            auto clan_info = main_server->GetClanCacheShared(player_cache->acc_info.ClanId);
                            auto info = PlayerRoomClanListInfo(player_cache->slot_id, clan_info->clan_name.c_str(), clan_info->logo_front, clan_info->logo_back, acc_cache->acc_info.ClanId, 0);
                            clan_info.unlock();
                            players_clan_info.push_back(info);
                        }
                    }
                    else
                        players_clan_info.push_back(PlayerRoomClanListInfo(player_cache->slot_id, "", 0, 0, 0, 0));



                    player_cache.unlock();
                }
                send_msg(session, 406, 0, extra, block_size, reinterpret_cast<uint8_t*>(new_info.data()), new_info.size());
            }
            BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "sent clan info");
            for (std::uint32_t batch_id = 0; batch_id < equipBlocksCount; batch_id++)
            {
                const std::uint32_t max_batch_size = (MAX_PACKET_SIZE - 8) / sizeof(MainRoomPlayersEquipInfoAck);
                const std::uint8_t extra = (batch_id == 0) ? 37 : 0;
                std::vector<MainRoomPlayersEquipInfoAck> new_equipinfo;
                std::uint32_t block_size = 0;
                const std::uint32_t start_index = batch_id * max_batch_size;
                const std::uint32_t end_index = std::min(start_index + max_batch_size, static_cast<std::uint32_t>(players_ids.size()));
                for (auto i = start_index; i < end_index; i++)
                {
                    auto player_id = players_ids[i];
                    if (player_id == session_id) continue;
                    auto player_cache = main_server->GetAccCacheSharedBySessionId(player_id);
                    auto unique_id = NetEngine::Packets::Core::UniqueId(player_id, 1).data;
                    auto voice_id = player_cache->voice_id;
                    auto pcroom_tier = player_cache->acc_info.PCRoom;
                    std::vector<BaseLib::Item> equipped_items;
                    for (const auto& item : player_cache->inventory_items)
                        if (item.is_equipped == 1 && item.character_id == static_cast<std::uint8_t>(player_cache->acc_info.SelectedCharacter))
                            equipped_items.push_back(item);

                    const auto& set_item = main_server->GetItemByType(equipped_items, 25).item_info.item_number.item_id;
                    auto setitem_info = main_server->GetSetItemInfoCache(set_item);
                    const auto& hair_id = main_server->GetItemByType(equipped_items, 0).item_info.item_number.item_id;
                    const auto& face_id = main_server->GetItemByType(equipped_items, 1).item_info.item_number.item_id;
                    const auto& upper_id = main_server->GetItemByType(equipped_items, 2).item_info.item_number.item_id;
                    const auto& under_id = main_server->GetItemByType(equipped_items, 3).item_info.item_number.item_id;
                    const auto& pants_id = main_server->GetItemByType(equipped_items, 4).item_info.item_number.item_id;
                    const auto& shirt_id = main_server->GetItemByType(equipped_items, 5).item_info.item_number.item_id;
                    const auto& boots_id = main_server->GetItemByType(equipped_items, 6).item_info.item_number.item_id;
                    const auto& acc_head_id = main_server->GetItemByType(equipped_items, 7).item_info.item_number.item_id;
                    const auto& acc_waist_id = main_server->GetItemByType(equipped_items, 8).item_info.item_number.item_id;
                    const auto& acc_back_id = main_server->GetItemByType(equipped_items, 9).item_info.item_number.item_id;
                    const auto& EquippedHairItemId = hair_id ? hair_id : (setitem_info->Hair != UINT32_MAX ? setitem_info->Id : 0);
                    const auto& EquippedFaceItemId = face_id ? face_id : (setitem_info->Face != UINT32_MAX ? setitem_info->Id : 0);
                    const auto& EquippedUpperItemId = upper_id ? upper_id : (setitem_info->Upper != UINT32_MAX ? setitem_info->Id : 0);
                    const auto& EquippedUnderItemId = under_id ? under_id : (setitem_info->Under != UINT32_MAX ? setitem_info->Id : 0);
                    const auto& EquippedPantsItemId = pants_id ? pants_id : (setitem_info->Pants != UINT32_MAX ? setitem_info->Id : 0);
                    const auto& EquippedShirtItemId = shirt_id ? shirt_id : (setitem_info->Arms != UINT32_MAX ? setitem_info->Id : 0);
                    const auto& EquippedBootsItemId = boots_id ? boots_id : (setitem_info->Boots != UINT32_MAX ? setitem_info->Id : 0);
                    const auto& EquippedGlassItemId = acc_head_id ? acc_head_id : (setitem_info->AccessoryA != UINT32_MAX ? setitem_info->Id : 0);
                    const auto& EquippedAccessoryWaistItemId = acc_waist_id ? acc_waist_id : (setitem_info->AccessoryB != UINT32_MAX ? setitem_info->Id : 0);
                    const auto& EquippedAccessoryBackItemId = acc_back_id ? acc_back_id : (setitem_info->AccessoryC != UINT32_MAX ? setitem_info->Id : 0);
                    const auto& EquippedMeleeItemId = main_server->GetItemByType(equipped_items, 10).item_info.item_number.item_id;
                    const auto& EquippedRifleItemId = main_server->GetItemByType(equipped_items, 11).item_info.item_number.item_id;
                    const auto& EquippedShotgunItemId = main_server->GetItemByType(equipped_items, 12).item_info.item_number.item_id;
                    const auto& EquippedSniperItemId = main_server->GetItemByType(equipped_items, 13).item_info.item_number.item_id;
                    const auto& EquippedGatlingItemId = main_server->GetItemByType(equipped_items, 14).item_info.item_number.item_id;
                    const auto& EquippedGrenadeItemId = main_server->GetItemByType(equipped_items, 15).item_info.item_number.item_id;
                    const auto& EquippedBazookaItemId = main_server->GetItemByType(equipped_items, 16).item_info.item_number.item_id;
                    auto equip_data = MainRoomPlayersEquipInfoAck(unique_id,
                        EquippedHairItemId, EquippedFaceItemId, EquippedUpperItemId,
                        EquippedUnderItemId, EquippedPantsItemId, EquippedShirtItemId,
                        EquippedBootsItemId, EquippedGlassItemId, EquippedAccessoryWaistItemId,
                        EquippedAccessoryBackItemId, EquippedMeleeItemId, EquippedRifleItemId,
                        EquippedShotgunItemId, EquippedSniperItemId, EquippedGatlingItemId,
                        EquippedGrenadeItemId, EquippedBazookaItemId);

                    new_equipinfo.push_back(equip_data);
                    block_size++;
                    player_cache.unlock();

                    send_msg(session, 314, 0, 0, voice_id, reinterpret_cast<uint8_t*>(&unique_id), sizeof(unique_id));
                    //send_msg(session, 403, 0, 0, pcroom_tier, reinterpret_cast<uint8_t*>(&unique_id), sizeof(unique_id));
                }
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "sent player info for: ({}) players", new_equipinfo.size());
                send_msg(session, 303, 0, extra, block_size, reinterpret_cast<uint8_t*>(new_equipinfo.data()), new_equipinfo.size() * sizeof(MainRoomPlayersEquipInfoAck));
            }
            BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "sent players info");
            acc_cache.lock();

            acc_cache->in_room = true;
            acc_cache->room_id = room_cache->room_id;
            acc_cache->playing = false;

            /*leave plaza start*/
            if (acc_cache->in_plaza) {
                auto plaza_id = acc_cache->plaza_id;
                if (main_server->IsPlazaAlready(plaza_id))
                {
                    BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "player will leave plaza: ({})", plaza_id);
                    auto current_plaza = main_server->GetPlazaCacheUnique(plaza_id);
                    auto& session_ids = current_plaza->session_ids;
                    if (main_server->IsSessionIdAlready(session_id, session_ids))
                    {
                        if (main_server->IsPlazaBroadcastable(current_plaza))
                        {
                            auto my_unique_id = NetEngine::Packets::Core::UniqueId(session_id, 1).data;
                            for (const auto& plaza_player_session_id : session_ids)
                            {
                                if (plaza_player_session_id == session_id) continue;
                                if (auto player_session = server->GetSessionById(plaza_player_session_id))
                                    send_msg(player_session.get(), 425, 0, 0, 1, reinterpret_cast<uint8_t*>(&my_unique_id), sizeof(my_unique_id));
                            }
                        }
                        BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "session id: ({}) left plaza id: ({})", session_id, plaza_id);
                        auto remove_myself = std::remove(current_plaza->session_ids.begin(), current_plaza->session_ids.end(), session_id);
                        current_plaza->session_ids.erase(remove_myself, current_plaza->session_ids.end());
                        acc_cache->plaza_id = 0;
                        acc_cache->in_plaza = false;
                    }
                }
            }
            /*leave plaza end*/

            RoomUserPlayerInfo1 my_info1; RoomUserPlayerInfo2 my_info2;
            my_info1.grade = acc_cache->acc_info.Grade;
            my_info1.vip_level = acc_cache->acc_info.PCRoom;
            my_info1.character = acc_cache->acc_info.SelectedCharacter;
            my_info1.team = current_team_id;
            my_info1.level = acc_cache->acc_info.Level + 1;

            my_info2.fps_limit = acc_cache->fps_limit;
            my_info2.player_state = PlayerInfo::State::Waiting;
            my_info2.ping = acc_cache->ping;


            const auto& my_unique_id = NetEngine::Packets::Core::UniqueId(session_id, 1).data;
            std::vector<BaseLib::Item> my_equipped_items;
            for (const auto& item : acc_cache->inventory_items)
                if (item.is_equipped == 1 && item.character_id == static_cast<std::uint8_t>(acc_cache->acc_info.SelectedCharacter))
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
            const auto& my_EquippedHairItemId = my_hair_id ? my_hair_id : (my_setitem_info->Hair != UINT32_MAX ? my_setitem_info->Id : 0);
            const auto& my_EquippedFaceItemId = my_face_id ? my_face_id : (my_setitem_info->Face != UINT32_MAX ? my_setitem_info->Id : 0);
            const auto& my_EquippedUpperItemId = my_upper_id ? my_upper_id : (my_setitem_info->Upper != UINT32_MAX ? my_setitem_info->Id : 0);
            const auto& my_EquippedUnderItemId = my_under_id ? my_under_id : (my_setitem_info->Under != UINT32_MAX ? my_setitem_info->Id : 0);
            const auto& my_EquippedPantsItemId = my_pants_id ? my_pants_id : (my_setitem_info->Pants != UINT32_MAX ? my_setitem_info->Id : 0);
            const auto& my_EquippedShirtItemId = my_shirt_id ? my_shirt_id : (my_setitem_info->Arms != UINT32_MAX ? my_setitem_info->Id : 0);
            const auto& my_EquippedBootsItemId = my_boots_id ? my_boots_id : (my_setitem_info->Boots != UINT32_MAX ? my_setitem_info->Id : 0);
            const auto& my_EquippedGlassItemId = my_acc_head_id ? my_acc_head_id : (my_setitem_info->AccessoryA != UINT32_MAX ? my_setitem_info->Id : 0);
            const auto& my_EquippedAccessoryWaistItemId = my_acc_waist_id ? my_acc_waist_id : (my_setitem_info->AccessoryB != UINT32_MAX ? my_setitem_info->Id : 0);
            const auto& my_EquippedAccessoryBackItemId = my_acc_back_id ? my_acc_back_id : (my_setitem_info->AccessoryC != UINT32_MAX ? my_setitem_info->Id : 0);
            const auto& my_EquippedMeleeItemId = main_server->GetItemByType(my_equipped_items, 10).item_info.item_number.item_id;
            const auto& my_EquippedRifleItemId = main_server->GetItemByType(my_equipped_items, 11).item_info.item_number.item_id;
            const auto& my_EquippedShotgunItemId = main_server->GetItemByType(my_equipped_items, 12).item_info.item_number.item_id;
            const auto& my_EquippedSniperItemId = main_server->GetItemByType(my_equipped_items, 13).item_info.item_number.item_id;
            const auto& my_EquippedGatlingItemId = main_server->GetItemByType(my_equipped_items, 14).item_info.item_number.item_id;
            const auto& my_EquippedGrenadeItemId = main_server->GetItemByType(my_equipped_items, 15).item_info.item_number.item_id;
            const auto& my_EquippedBazookaItemId = main_server->GetItemByType(my_equipped_items, 16).item_info.item_number.item_id;
            auto playerEnterInfoData = MainRoomPlayerEnterInfoAck(acc_cache->acc_info.Nickname.c_str(), my_unique_id, my_info1, my_info2,
                my_EquippedHairItemId, my_EquippedFaceItemId, my_EquippedUpperItemId,
                my_EquippedUnderItemId, my_EquippedPantsItemId, my_EquippedShirtItemId,
                my_EquippedBootsItemId, my_EquippedGlassItemId, my_EquippedAccessoryWaistItemId,
                my_EquippedAccessoryBackItemId, my_EquippedMeleeItemId, my_EquippedRifleItemId,
                my_EquippedShotgunItemId, my_EquippedSniperItemId, my_EquippedGatlingItemId,
                my_EquippedGrenadeItemId, my_EquippedBazookaItemId).Serialize();


            send_msg(session, 409, 0, 37, players_clan_info.size(), reinterpret_cast<uint8_t*>(players_clan_info.data()), sizeof(PlayerRoomClanListInfo)  * players_clan_info.size());
            BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "sent players clan info for: ({}) players", players_clan_info.size());

            if (room_cache->ModeIndex == NetEngine::Room::Mode::Index::FreeForAll)
            {
                FFA_ModeInfo ffa_info{};
                ffa_info.state = room_cache->is_playing ? 3 : 0;
                ffa_info.timelimited = room_cache->time_rule;
                ffa_info.weaponlimited = room_cache->Restriction;
                ffa_info.winrule = room_cache->score_rule;
                ffa_info.kitdrop = room_cache->allow_drops;
                send_msg(session, 309, 0, 0, room_cache->ModeIndex, reinterpret_cast<uint8_t*>(&ffa_info), sizeof(ffa_info));
            }
            else if (room_cache->ModeIndex == NetEngine::Room::Mode::Index::Scrimmage)
            {
                Scrimmage_ModeInfo scrimmage_info{};
                scrimmage_info.winrule = room_cache->score_rule;
                scrimmage_info.state = room_cache->is_playing ? 3 : 0;
                scrimmage_info.timelimited = room_cache->time_rule;
                scrimmage_info.weaponlimited = room_cache->Restriction;
                send_msg(session, 309, 0, 0, room_cache->ModeIndex, reinterpret_cast<uint8_t*>(&scrimmage_info), sizeof(scrimmage_info));
            }
            else if (room_cache->ModeIndex == NetEngine::Room::Mode::Index::CaptureTheBattery ||
                room_cache->ModeIndex == NetEngine::Room::Mode::Index::CLAN_CaptureTheBattery)
            {
                CaptureTheBattery_ModeInfo ctb_info{};
                ctb_info.state = room_cache->is_playing ? 3 : 0;
                ctb_info.timelimited = room_cache->time_rule;
                ctb_info.weaponlimited = room_cache->Restriction;
                ctb_info.winrule = room_cache->score_rule;
                ctb_info.bluescore = 0;
                ctb_info.redscore = 0;
                ctb_info.kitdrop = room_cache->allow_drops;
                send_msg(session, 309, 0, 0, room_cache->ModeIndex, reinterpret_cast<uint8_t*>(&ctb_info), sizeof(ctb_info));
            }
            else if (room_cache->ModeIndex == NetEngine::Room::Mode::Index::Elimination ||
                room_cache->ModeIndex == NetEngine::Room::Mode::Index::CLAN_Elimination)
            {
                Elimination_ModeInfo sbt_info{};
                sbt_info.state = room_cache->is_playing ? 3 : 0;
                sbt_info.timelimited = room_cache->time_rule;
                sbt_info.weaponlimited = room_cache->Restriction;
                sbt_info.winrule = room_cache->score_rule;
                sbt_info.bluescore = 0;
                sbt_info.redscore = 0;
                //sbt_info.kitdrop = room_cache->allow_drops;
                send_msg(session, 309, 0, 0, room_cache->ModeIndex, reinterpret_cast<uint8_t*>(&sbt_info), sizeof(sbt_info));
            }
            else if (room_cache->ModeIndex == NetEngine::Room::Mode::Index::ZombieMode)
            {
                Zombie_ModeInfo zombie_info{};
                zombie_info.state = room_cache->is_playing ? 3 : 0;
                zombie_info.timelimited = room_cache->time_rule;
                zombie_info.weaponlimited = room_cache->Restriction;
                zombie_info.winrule = room_cache->score_rule;
                zombie_info.bluescore = 0;
                zombie_info.redscore = 0;
                //zombie_info.kitdrop = room_cache->allow_drops;
                send_msg(session, 309, 0, 0, room_cache->ModeIndex, reinterpret_cast<uint8_t*>(&zombie_info), sizeof(zombie_info));
            }
            else if (room_cache->ModeIndex == NetEngine::Room::Mode::Index::ArmsRace)
            {
                ArmsRace_ModeInfo arms_info{};
                arms_info.state = room_cache->is_playing ? 3 : 0;
                arms_info.timelimited = room_cache->time_rule;
                arms_info.weaponlimited = room_cache->Restriction;
                arms_info.winrule = room_cache->score_rule;
                //arms_info.kitdrop = room_cache->allow_drops;
                send_msg(session, 309, 0, 0, room_cache->ModeIndex, reinterpret_cast<uint8_t*>(&arms_info), sizeof(arms_info));
            }
            else if (room_cache->ModeIndex == NetEngine::Room::Mode::Index::BombBattle)
            {
                BombBattle_ModeInfo bmb_info{};
                bmb_info.bluescore = 0;
                bmb_info.redscore = 0;
                bmb_info.state = room_cache->is_playing ? 3 : 0;
                bmb_info.timelimited = room_cache->time_rule;
                bmb_info.weaponlimited = room_cache->Restriction;
                bmb_info.winrule = room_cache->score_rule;
                //arms_info.kitdrop = room_cache->allow_drops;
                send_msg(session, 309, 0, 0, room_cache->ModeIndex, reinterpret_cast<uint8_t*>(&bmb_info), sizeof(bmb_info));
            }
            else if (room_cache->ModeIndex == NetEngine::Room::Mode::Index::BossBattle)
            {
                BossBattle_ModeInfo boss_info{};
                boss_info.state = room_cache->is_playing ? 3 : 0;
                boss_info.timelimited = room_cache->time_rule;
                boss_info.weaponlimited = room_cache->Restriction;
                boss_info.winrule = room_cache->score_rule;
                //arms_info.kitdrop = room_cache->allow_drops;
                send_msg(session, 309, 0, 0, room_cache->ModeIndex, reinterpret_cast<uint8_t*>(&boss_info), sizeof(boss_info));
            }
            else if (room_cache->ModeIndex == NetEngine::Room::Mode::Index::TeamDeathMatch ||
                room_cache->ModeIndex == NetEngine::Room::Mode::Index::ItemMatch ||
                room_cache->ModeIndex == NetEngine::Room::Mode::Index::CloseCombat ||
                room_cache->ModeIndex == NetEngine::Room::Mode::Index::SuperItemMatch ||
                room_cache->ModeIndex == NetEngine::Room::Mode::Index::CLAN_TeamDeathMatch)
            {
                TDM_ModeInfo tdm_info{};
                tdm_info.state = room_cache->is_playing ? 3 : 0;
                tdm_info.timelimited = room_cache->time_rule;
                tdm_info.weaponlimited = room_cache->Restriction;
                tdm_info.winrule = room_cache->score_rule;
                tdm_info.bluescore = 0;
                tdm_info.redscore = 0;
                if (room_cache->ModeIndex != NetEngine::Room::Mode::Index::CloseCombat)
                    tdm_info.kitdrop = room_cache->allow_drops;

                send_msg(session, 309, 0, 0, room_cache->ModeIndex, reinterpret_cast<uint8_t*>(&tdm_info), sizeof(tdm_info));
            }
            std::vector<std::pair<std::uint32_t, uint32_t>> filtered_slots;
            acc_cache.unlock();
            for (const auto& player_id : players_ids)
            {
                auto player_cache = main_server->GetAccCacheSharedBySessionId(player_id);
                if (player_cache->slot_id != 0xFF)
                    filtered_slots.emplace_back(player_id, player_cache->slot_id);

                player_cache.unlock();
            }

            std::stable_sort(filtered_slots.begin(), filtered_slots.end(),
                [](const std::pair<std::uint32_t, uint32_t>& a, const std::pair<std::uint32_t, uint32_t>& b)
            {
                return a.second < b.second;
            });
            acc_cache.lock();
            auto voice_id = acc_cache->voice_id;
            auto pcroom_tier = acc_cache->acc_info.PCRoom;
            auto my_auto_unique_id = NetEngine::Packets::Core::UniqueId(session_id, 1).data;

            acc_cache->slot_id = (current_team_id != NetEngine::Team::IdType::Observer) ? static_cast<std::uint8_t>(filtered_slots.back().second + 1) : 0xFF;
            send_msg(session, 140, 0, (current_team_id != NetEngine::Team::IdType::Observer) ? NetEngine::Room::Join::Result::JoinAsPlayer : NetEngine::Room::Join::Result::JoinAsObserver, 1);
            PlayerRoomClanListInfo my_clan_info;
            if (acc_cache->acc_info.ClanId)
            {
                if (main_server->IsClanAlready(acc_cache->acc_info.ClanId))
                {
                    auto clan_info = main_server->GetClanCacheShared(acc_cache->acc_info.ClanId);
                    my_clan_info = PlayerRoomClanListInfo(acc_cache->slot_id, clan_info->clan_name.c_str(), clan_info->logo_front, clan_info->logo_back, acc_cache->acc_info.ClanId, 0);
                    clan_info.unlock();
                }
                else
                    my_clan_info = PlayerRoomClanListInfo(acc_cache->slot_id, "", 0, 0, 0, 0);
            }
            else
                my_clan_info = PlayerRoomClanListInfo(1, "", 0, 0, 0, 0);
            for (const auto& room_player_session_id : players_ids)
            {
                if (room_player_session_id == session_id) continue;
                if (auto player_session = server->GetSessionById(room_player_session_id))
                {
                    send_msg(player_session.get(), 421, 0, 0, 1, reinterpret_cast<uint8_t*>(playerEnterInfoData.data()), playerEnterInfoData.size());
                    send_msg(player_session.get(), 409, 0, 37, 1, reinterpret_cast<uint8_t*>(&my_clan_info), sizeof(PlayerRoomClanListInfo));
                    send_msg(player_session.get(), 314, 0, 0, voice_id, reinterpret_cast<uint8_t*>(&my_auto_unique_id), sizeof(my_auto_unique_id));
                    //send_msg(player_session.get(), 403, 0, 0, pcroom_tier, reinterpret_cast<uint8_t*>(&my_auto_unique_id), sizeof(my_auto_unique_id));
                }
                    
            }

            acc_cache->state = 7;
            
            BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "will broadcast to all player new state 7 (waiting) to avoid playing bug state");
            for (const auto& room_player_session_id : players_ids)
                if (auto player_session = server->GetSessionById(room_player_session_id))
                    send_msg(player_session.get(), 312, 0, 0, 7, reinterpret_cast<uint8_t*>(&my_auto_unique_id), sizeof(my_auto_unique_id));

            BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "player ({}) join room -> id: ({})", acc_cache->acc_info.Nickname.c_str(), room_cache->room_id);
        }
    }  
}