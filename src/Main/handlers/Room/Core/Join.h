#pragma once
namespace Game::Handlers
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Main;
    inline void RoomJoin(SCallbackData& callback, CMainServer* main_server)
    {
        auto session = callback.session;
        auto message = callback.message;
        if (!session || !message) return;

        //std::shared_lock lock(session->GetMutex());
        CServer* server = callback.server;
        auto session_id = session->GetSessionId();
        auto acc_cache = CAccount.get<unique_t>(session_id);
        auto acc_index = acc_cache->acc_info.Index;
        int32_t current_team_id = -1;
        auto join_result = static_cast<NetEngine::Room::Join::ReqResult>(message->GetExtra());
        if (acc_index == -1) return;
        const auto& joinRoomReq = reinterpret_cast<MainJoinRoomReq*>(message->GetData());

        if (acc_cache->in_room)
        {
            if (joinRoomReq->room_id == acc_cache->room_id)
            {
                DEBUGLOG(dark_cyan, "fail: already in room");
                session->SendMsg(140, 0, NetEngine::Room::Join::Result::GenericError, 0);
                return;
            }
            DEBUGLOG(dark_cyan, "is in room previously and now will be removed");
            auto old_room_cache = CRoom.get<unique_t>(acc_cache->room_id);
            auto old_team_id = acc_cache->team_id;
            acc_cache.unlock();
            main_server->NewRemoveRoomPlayer(old_room_cache, session_id, old_team_id, NetEngine::Room::Leave::Ack::Result::Leave, false);
            old_room_cache.unlock();
            acc_cache.lock();
        }

        auto room_cache = CRoom.get<unique_t>(joinRoomReq->room_id);
        DEBUGLOG(dark_cyan, "player ({}) attempt to join Room No. ({}), channel id: ({})", session->GetSessionId(), joinRoomReq->room_id, joinRoomReq->channel_id);
        if (room_cache->title.empty())
        {
            DEBUGLOG(dark_cyan, "fail: no title");
            session->SendMsg(140, 0, NetEngine::Room::Join::Result::RoomDeleted, 0);
            return;
        }
        if (room_cache->has_password || join_result == NetEngine::Room::Join::ReqResult::Password)
        {
            auto room_pass_req = std::string(joinRoomReq->password);
            DEBUGLOG(dark_cyan, "player try to join with password: ({})", room_pass_req);
            if (room_pass_req.empty() || room_pass_req != room_cache->password)
            {
                session->SendMsg(140, 0, NetEngine::Room::Join::Result::InvalidPassword, 0);
                return;
            }
        }

        acc_cache->zombie_team = 0;

        auto in_party = acc_cache->in_party;
        bool is_clan = false;
        bool is_my_party = false;
        bool is_vs_party = false;
        if (in_party) {
            DEBUGLOG(dark_cyan, "join in party room");
            if (room_cache->host_session_id == session_id) {
                DEBUGLOG(dark_cyan, "host try to join room!");
                return;
            }
            auto host_cache = CAccount.get<shared_t>(room_cache->host_session_id);
            if (!host_cache->in_party) {
                DEBUGLOG(dark_cyan, "player try to join party a room which isnt a party!");
                return;
            }
            auto host_party_id = host_cache->party_id;
            host_cache.unlock();
            auto party_cache = CParty.get<shared_t>(host_party_id);
            is_clan = party_cache->is_clan;
            party_cache.unlock();
            if (acc_cache->party_id == host_party_id) {
                is_my_party = true;
            }
            else {
                is_vs_party = true;
            }
        }
        DEBUGLOG(dark_cyan, "all checks passed");
        auto is_mode_teambased = main_server->IsModeTeamBased(static_cast<NetEngine::Room::Mode::Index>(room_cache->ModeIndex));
        auto observers_max_count = room_cache->allow_observers ? 10 : 0;
        auto room_players_max_count = room_cache->max_players;
        uint32_t players_count = is_mode_teambased ? room_cache->redteam_session_ids.size() + room_cache->blueteam_session_ids.size() : room_cache->neutralteam_session_ids.size();
        if (room_cache->allow_observers) players_count += static_cast<uint32_t>(room_cache->observers_session_ids.size());
        if (players_count >= room_players_max_count + observers_max_count)
        {
            DEBUGLOG(dark_cyan, "room was full");
            session->SendMsg(140, 0, NetEngine::Room::Join::Result::LobbyFull, 0);
            return;
        }
        if (std::ranges::contains(room_cache->neutralteam_session_ids, session_id) ||
            std::ranges::contains(room_cache->redteam_session_ids, session_id) ||
            std::ranges::contains(room_cache->blueteam_session_ids, session_id) ||
            std::ranges::contains(room_cache->observers_session_ids, session_id))
        {
            DEBUGLOG(dark_cyan, "already in room");
            session->SendMsg(140, 0, NetEngine::Room::Join::Result::GenericError, 0);
            return;
        }
        
        if (room_cache->kicked.contains(acc_index))
        {
            DEBUGLOG(dark_cyan, "player was kicked");
            session->SendMsg(140, 0, NetEngine::Room::Join::Result::PreviouslyKicked, 0);
            return;
        }
        if (!is_mode_teambased)
        {
            DEBUGLOG(dark_cyan, "is not team based");
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
                        session->SendMsg(140, 0, room_cache->allow_observers ? NetEngine::Room::Join::Error::RoomFull : NetEngine::Room::Join::Error::NoIntrusion, 0);
                }
                else
                    session->SendMsg(140, 0, room_cache->allow_observers ? NetEngine::Room::Join::Error::RoomFull : NetEngine::Room::Join::Error::NoIntrusion, 0);
            }
        }
        else if (in_party)
        {
            DEBUGLOG(dark_cyan, "player will join party battle with team case handling");
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
                DEBUGLOG(dark_cyan, "critical error: player dont match any case in party battle join");
            }
        }
        else
        {
            DEBUGLOG(dark_cyan, "is team based");
            auto blue_team_size = room_cache->blueteam_session_ids.size();
            auto red_team_size = room_cache->redteam_session_ids.size();
            auto blue_team_not_full = blue_team_size < room_cache->max_players / 2;
            auto red_team_not_full = red_team_size < room_cache->max_players / 2;
            if (blue_team_size <= red_team_size && blue_team_not_full)
            {
                DEBUGLOG(dark_cyan, "({}) added to team blue", acc_cache->acc_info.Nickname.c_str());
                room_cache->blueteam_session_ids.push_back(session_id);
                acc_cache->team_id = Team::IdType::Blue;
                current_team_id = Team::IdType::Blue;
            }
            else if ((red_team_size < blue_team_size && red_team_not_full) || (red_team_size <= blue_team_size && !blue_team_not_full && red_team_not_full))
            {
                DEBUGLOG(dark_cyan, "({}) added to team red", acc_cache->acc_info.Nickname.c_str());
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
                        session->SendMsg(140, 0, room_cache->allow_observers ? NetEngine::Room::Join::Error::RoomFull : NetEngine::Room::Join::Error::NoIntrusion, 0);
                }
                else
                    session->SendMsg(140, 0, room_cache->allow_observers ? NetEngine::Room::Join::Error::RoomFull : NetEngine::Room::Join::Error::NoIntrusion, 0);
            }
        }
        DEBUGLOG(dark_cyan, "now prepare settings");
        auto has_password = static_cast<uint8_t>(!room_cache->password.empty());
        RoomSettingsInfo2 settings_info{};
        settings_info.map_index = room_cache->MapIndex;
        settings_info.mode_index = room_cache->ModeIndex;
        settings_info.max_players = room_cache->max_players;
        settings_info.restriction = room_cache->Restriction;
        settings_info.allow_intruders = room_cache->allow_intruders;
        settings_info.allow_observers = room_cache->allow_observers;
        settings_info.team_balance = NetEngine::Room::Balance::State::Disabled;//room_cache->TeamBalance;
        if (room_cache->ModeIndex == NetEngine::Room::Mode::Index::BombBattle)
            settings_info.team_balance = NetEngine::Room::Balance::State::Disabled;
        settings_info.has_password = has_password;
        settings_info.hide_password = false;
        settings_info.is_clan_room = (acc_cache->in_party ? (is_clan ? 2 : 1) : 0);
        auto settings_data = MainRoomSettingsInfoAck(room_cache->password.c_str(), settings_info).Serialize();
        uint8_t high_room_id_part = (room_cache->room_id >> 8) & 0xFF; // Extract the high 8 bits
        uint8_t low_room_id_part = room_cache->room_id & 0xFF;
        DEBUGLOG(dark_cyan, "sending settings");
        session->SendMsg(139, has_password, low_room_id_part, high_room_id_part, reinterpret_cast<uint8_t*>(settings_data.data()), settings_data.size());
        RoomSettingsModeInfo2 mode_settings_info;
        mode_settings_info.time_limit = room_cache->time_rule;
        mode_settings_info.score_limit = room_cache->score_rule;
        mode_settings_info.allow_items = room_cache->allow_drops;
        mode_settings_info.restriction = room_cache->Restriction;
        std::vector<PlayerRoomClanListInfo> players_clan_info;

        acc_cache.unlock();
        auto players_ids = main_server->GetRoomSortedPlayerSessionIds(room_cache);
        auto observer_ids = main_server->GetRoomSortedObserversSessionIds(room_cache);

        auto players_size = players_ids.size();
        auto equipBlocksCount = players_size == 0 ? 0 : (players_size / 16) + 1;
        constexpr size_t MAX_PACKET_SIZE = 1440;
        for (uint32_t batch_id = 0; batch_id < equipBlocksCount; batch_id++)
        {
            const uint32_t max_batch_size = (MAX_PACKET_SIZE - 8) / sizeof(MainRoomPlayersInfoAck);
            const uint8_t extra = (batch_id == 0) ? 37 : 0;
            std::vector<uint8_t> new_info;
            uint32_t block_size = 0;
            const uint32_t start_index = batch_id * max_batch_size;
            const uint32_t end_index = std::min(start_index + max_batch_size, static_cast<uint32_t>(players_ids.size()));
            for (auto i = start_index; i < end_index; i++)
            {
                auto player_id = players_ids[i];
                if (player_id == session_id) continue;
                auto player_cache = CAccount.get<shared_t>(player_id);
				auto info1 = main_server->GetRoomUserPlayerInfo1(player_cache);
				auto info2 = main_server->GetRoomUserPlayerInfo2(player_cache);    
                if (player_cache->zombie_team)
                {
                    DEBUGLOG(red, "player ({}) ({}) is zombie", player_id, player_cache->acc_info.Nickname.c_str());
                    info1.team = player_cache->zombie_team;
                }
                DEBUGLOG(dark_cyan, "send ({}) as team ({})", player_cache->acc_info.Nickname.c_str(), static_cast<uint32_t>(info1.team));

                auto player_data = MainRoomPlayersInfoAck(player_cache->acc_info.Nickname.c_str(), player_cache->uid, info1, info2).Serialize();
                new_info.insert(new_info.end(), player_data.begin(), player_data.end());
                block_size++;

                if (player_cache->acc_info.ClanId)
                {
                    if (CClan.contains(player_cache->acc_info.ClanId))
                    {
                        auto clan_info = CClan.get<shared_t>(player_cache->acc_info.ClanId);
                        auto info = PlayerRoomClanListInfo(player_cache->slot_id, clan_info->clan_name.c_str(), clan_info->logo_front, clan_info->logo_back, acc_cache->acc_info.ClanId, 0);
                        clan_info.unlock();
                        players_clan_info.push_back(info);
                    }
                }
                else
                    players_clan_info.push_back(PlayerRoomClanListInfo(player_cache->slot_id, "", 0, 0, 0, 0));



                player_cache.unlock();
            }
            session->SendMsg(406, 0, extra, block_size, reinterpret_cast<uint8_t*>(new_info.data()), new_info.size());
        }
        DEBUGLOG(dark_cyan, "sent clan info");
        for (uint32_t batch_id = 0; batch_id < equipBlocksCount; batch_id++)
        {
            const uint32_t max_batch_size = (MAX_PACKET_SIZE - 8) / sizeof(PartyEquipInfoAck);
            const uint8_t extra = (batch_id == 0) ? 37 : 0;
            std::vector<PartyEquipInfoAck> new_equipinfo;
            uint32_t block_size = 0;
            const uint32_t start_index = batch_id * max_batch_size;
            const uint32_t end_index = std::min(start_index + max_batch_size, static_cast<uint32_t>(players_ids.size()));
            for (auto i = start_index; i < end_index; i++)
            {
                auto player_id = players_ids[i];
                if (player_id == session_id) continue;
                auto player_cache = CAccount.get<shared_t>(player_id);
                auto unique_id = NetEngine::Packets::Core::UniqueId(player_id, 1).data;
                auto voice_id = player_cache->voice_id;
                auto pcroom_tier = player_cache->acc_info.PCRoom;
                auto equipped_items = main_server->GetEquippedItems(player_cache);

                auto equip_data = PartyEquipInfoAck(player_cache->uid, equipped_items);

                new_equipinfo.push_back(equip_data);
                block_size++;
                player_cache.unlock();

                session->SendMsg(314, 0, 0, voice_id, reinterpret_cast<uint8_t*>(&unique_id), sizeof(unique_id));
                session->SendMsg(403, 0, 0, pcroom_tier, reinterpret_cast<uint8_t*>(&unique_id), sizeof(unique_id));
            }
            DEBUGLOG(dark_cyan, "sent player info for: ({}) players", new_equipinfo.size());
            session->SendMsg(303, 0, extra, block_size, reinterpret_cast<uint8_t*>(new_equipinfo.data()), new_equipinfo.size() * sizeof(PartyEquipInfoAck));
        }
        DEBUGLOG(dark_cyan, "sent players info");
        acc_cache.lock();

        acc_cache->in_room = true;
        acc_cache->room_id = room_cache->room_id;
        acc_cache->playing = false;

        /*leave plaza start*/
        if (acc_cache->in_plaza) {
            auto plaza_id = acc_cache->plaza_id;
            if (main_server->IsPlazaAlready(plaza_id))
            {
                DEBUGLOG(dark_cyan, "player will leave plaza: ({})", plaza_id);
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
        }
        /*leave plaza end*/

		auto my_info1 = main_server->GetRoomUserPlayerInfo1(acc_cache);
		auto my_info2 = main_server->GetRoomUserPlayerInfo2(acc_cache);
		auto my_equipped_items = main_server->GetEquippedItems(acc_cache);

        auto playerEnterInfoData = PlazaEquipInfoAck(acc_cache->acc_info.Nickname.c_str(),
            acc_cache->uid, my_equipped_items, my_info1, my_info2);


        session->SendMsg(409, 0, 37, players_clan_info.size(), reinterpret_cast<uint8_t*>(players_clan_info.data()), sizeof(PlayerRoomClanListInfo) * players_clan_info.size());
        DEBUGLOG(dark_cyan, "sent players clan info for: ({}) players", players_clan_info.size());

        if (room_cache->ModeIndex == NetEngine::Room::Mode::Index::FreeForAll)
        {
            FFA_ModeInfo ffa_info{};
            ffa_info.state = room_cache->is_playing ? 3 : 0;
            ffa_info.timelimited = room_cache->time_rule;
            ffa_info.weaponlimited = room_cache->Restriction;
            ffa_info.winrule = room_cache->score_rule;
            ffa_info.kitdrop = room_cache->allow_drops;
            session->SendMsg(309, 0, 0, room_cache->ModeIndex, reinterpret_cast<uint8_t*>(&ffa_info), sizeof(ffa_info));
        }
        else if (room_cache->ModeIndex == NetEngine::Room::Mode::Index::Scrimmage)
        {
            Scrimmage_ModeInfo scrimmage_info{};
            scrimmage_info.winrule = room_cache->score_rule;
            scrimmage_info.state = room_cache->is_playing ? 3 : 0;
            scrimmage_info.timelimited = room_cache->time_rule;
            scrimmage_info.weaponlimited = room_cache->Restriction;
            session->SendMsg(309, 0, 0, room_cache->ModeIndex, reinterpret_cast<uint8_t*>(&scrimmage_info), sizeof(scrimmage_info));
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
            session->SendMsg(309, 0, 0, room_cache->ModeIndex, reinterpret_cast<uint8_t*>(&ctb_info), sizeof(ctb_info));
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
            session->SendMsg(309, 0, 0, room_cache->ModeIndex, reinterpret_cast<uint8_t*>(&sbt_info), sizeof(sbt_info));
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
            session->SendMsg(309, 0, 0, room_cache->ModeIndex, reinterpret_cast<uint8_t*>(&zombie_info), sizeof(zombie_info));
        }
        else if (room_cache->ModeIndex == NetEngine::Room::Mode::Index::ArmsRace)
        {
            ArmsRace_ModeInfo arms_info{};
            arms_info.state = room_cache->is_playing ? 3 : 0;
            arms_info.timelimited = room_cache->time_rule;
            arms_info.weaponlimited = room_cache->Restriction;
            arms_info.winrule = room_cache->score_rule;
            //arms_info.kitdrop = room_cache->allow_drops;
            session->SendMsg(309, 0, 0, room_cache->ModeIndex, reinterpret_cast<uint8_t*>(&arms_info), sizeof(arms_info));
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
            session->SendMsg(309, 0, 0, room_cache->ModeIndex, reinterpret_cast<uint8_t*>(&bmb_info), sizeof(bmb_info));
        }
        else if (room_cache->ModeIndex == NetEngine::Room::Mode::Index::BossBattle)
        {
            BossBattle_ModeInfo boss_info{};
            boss_info.state = room_cache->is_playing ? 3 : 0;
            boss_info.timelimited = room_cache->time_rule;
            boss_info.weaponlimited = room_cache->Restriction;
            boss_info.winrule = room_cache->score_rule;
            //arms_info.kitdrop = room_cache->allow_drops;
            session->SendMsg(309, 0, 0, room_cache->ModeIndex, reinterpret_cast<uint8_t*>(&boss_info), sizeof(boss_info));
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

            session->SendMsg(309, 0, 0, room_cache->ModeIndex, reinterpret_cast<uint8_t*>(&tdm_info), sizeof(tdm_info));
        }
        std::vector<std::pair<uint32_t, uint32_t>> filtered_slots;
        filtered_slots.reserve(players_ids.size());
        std::vector<std::pair<uint32_t, uint32_t>> observer_slots;
        observer_slots.reserve(observer_ids.size());
        acc_cache.unlock();
        for (const auto& player_id : players_ids)
        {
            auto player_cache = CAccount.get<shared_t>(player_id);
            filtered_slots.emplace_back(player_id, player_cache->slot_id);
            player_cache.unlock();
        }
        for (const auto& observer_id : observer_ids)
        {
            auto observer_cache = CAccount.get<shared_t>(observer_id);
            observer_slots.emplace_back(observer_id, observer_cache->slot_id);
            observer_cache.unlock();
        }

        std::stable_sort(filtered_slots.begin(), filtered_slots.end(),
            [](const std::pair<uint32_t, uint32_t>& a, const std::pair<uint32_t, uint32_t>& b)
            {
                return a.second < b.second;
            });
        std::stable_sort(observer_slots.begin(), observer_slots.end(),
            [](const std::pair<uint32_t, uint32_t>& a, const std::pair<uint32_t, uint32_t>& b)
            {
                return a.second < b.second;
            });
        acc_cache.lock();
        auto voice_id = acc_cache->voice_id;
        auto pcroom_tier = acc_cache->acc_info.PCRoom;
        auto my_auto_unique_id = NetEngine::Packets::Core::UniqueId(session_id, 1).data;

        constexpr uint32_t observer_slot_base = 16;
        uint32_t next_player_slot = filtered_slots.empty() ? 0 : (filtered_slots.back().second + 1u);
        uint32_t next_observer_slot = observer_slots.empty() ? observer_slot_base : (observer_slots.back().second + 1u);

        acc_cache->slot_id = (current_team_id != NetEngine::Team::IdType::Observer) ? next_player_slot : next_observer_slot;

        session->SendMsg(140, 0, (current_team_id != NetEngine::Team::IdType::Observer) ? NetEngine::Room::Join::Result::JoinAsPlayer : NetEngine::Room::Join::Result::JoinAsObserver, 1);
        PlayerRoomClanListInfo my_clan_info;
        if (acc_cache->acc_info.ClanId)
        {
            if (CClan.contains(acc_cache->acc_info.ClanId))
            {
                auto clan_info = CClan.get<shared_t>(acc_cache->acc_info.ClanId);
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
                player_session->SendMsg(421, 0, 0, 1, reinterpret_cast<uint8_t*>(&playerEnterInfoData), sizeof(playerEnterInfoData));
                player_session->SendMsg(409, 0, 37, 1, reinterpret_cast<uint8_t*>(&my_clan_info), sizeof(PlayerRoomClanListInfo));
                player_session->SendMsg(314, 0, 0, voice_id, reinterpret_cast<uint8_t*>(&my_auto_unique_id), sizeof(my_auto_unique_id));
                player_session->SendMsg(403, 0, 0, pcroom_tier, reinterpret_cast<uint8_t*>(&my_auto_unique_id), sizeof(my_auto_unique_id));
            }

        }

        if (!is_my_party && !is_vs_party)
        {
            acc_cache.unlock();

            DEBUGLOG(dark_cyan, "will re-broadcast clan info of existing players to each other to assure correct view");
            players_ids = main_server->GetRoomSortedPlayerSessionIds(room_cache);
            std::vector<PlayerRoomClanListInfo> players_clan_info_assure;
            for (const auto& room_player_session_id : players_ids)
            {
                auto player_cache = CAccount.get<shared_t>(room_player_session_id);
                if (player_cache->acc_info.ClanId)
                {
                    if (CClan.contains(player_cache->acc_info.ClanId))
                    {
                        auto clan_info = CClan.get<shared_t>(player_cache->acc_info.ClanId);
                        auto info = PlayerRoomClanListInfo(player_cache->slot_id, clan_info->clan_name.c_str(), clan_info->logo_front, clan_info->logo_back, acc_cache->acc_info.ClanId, 0);
                        clan_info.unlock();
                        players_clan_info_assure.push_back(info);
                    }
                }
                else
                    players_clan_info_assure.push_back(PlayerRoomClanListInfo(player_cache->slot_id, "", 0, 0, 0, 0));
            }
            for (const auto& room_player_session_id : players_ids)
                if (auto player_session = server->GetSessionById(room_player_session_id))
                    player_session->SendMsg(409, 0, 37, players_clan_info_assure.size(), reinterpret_cast<uint8_t*>(players_clan_info_assure.data()), sizeof(PlayerRoomClanListInfo) * players_clan_info_assure.size());

            acc_cache.lock();
        }

        acc_cache->state = PlayerInfo::State::Waiting;

        DEBUGLOG(dark_cyan, "will broadcast to all player new state 7 (waiting) to avoid playing bug state");
        for (const auto& room_player_session_id : players_ids)
            if (auto player_session = server->GetSessionById(room_player_session_id))
                player_session->SendMsg(312, 0, 0, 7, reinterpret_cast<uint8_t*>(&my_auto_unique_id), sizeof(my_auto_unique_id));

        DEBUGLOG(dark_cyan, "player ({}) join room -> id: ({})", acc_cache->acc_info.Nickname.c_str(), room_cache->room_id);
    }
}