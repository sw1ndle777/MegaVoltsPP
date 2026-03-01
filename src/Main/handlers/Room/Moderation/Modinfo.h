#pragma once
namespace Game::Handlers
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Main;

    void BalanceTeams(CMainServer* main_server, NetEngine::Room::Mode::Index previous_mode, RoomCacheResource& room_cache, std::vector<uint16_t>& players)
    {
        auto new_mode = room_cache->ModeIndex;
        auto is_team_based = main_server->IsModeTeamBased(new_mode);
		auto previous_is_team_based = main_server->IsModeTeamBased(previous_mode);
        if (previous_mode != new_mode)
        {
            if (!previous_is_team_based && is_team_based)
            {
                DEBUGLOG(dark_cyan, "previous wasnt team based and now is");
                room_cache->redteam_session_ids.clear();
                room_cache->blueteam_session_ids.clear();
                for (const auto& id : room_cache->neutralteam_session_ids)
                {
                    auto other_acc = CAccount.get<unique_t>(id);
                    auto blue_team_size = room_cache->blueteam_session_ids.size();
                    auto red_team_size = room_cache->redteam_session_ids.size();
                    if (blue_team_size == 0 || blue_team_size <= red_team_size)
                    {
                        DEBUGLOG(dark_cyan, "player ({}) is now team blue", other_acc->acc_info.Nickname.c_str());
                        room_cache->blueteam_session_ids.push_back(id);
                        other_acc->team_id = Team::IdType::Blue;
                    }
                    else
                    {
                        DEBUGLOG(dark_cyan, "player ({}) is now team red", other_acc->acc_info.Nickname.c_str());
                        room_cache->redteam_session_ids.push_back(id);
                        other_acc->team_id = Team::IdType::Red;
                    }
                    other_acc.unlock();
                }
                room_cache->neutralteam_session_ids.clear();
            }
            else if (previous_is_team_based && !is_team_based)
            {
                room_cache->neutralteam_session_ids.clear();

                std::vector<std::pair<uint16_t, uint32_t>> player_slot_pairs;

                auto addPlayerToSlotPairs = [&](const std::vector<uint16_t>& team_session_ids)
                    {
                        for (const auto& id : team_session_ids)
                        {
                            auto other_acc = CAccount.get<shared_t>(id);
                            if (other_acc->acc_info.Index != -1 && other_acc->in_room && other_acc->room_id == room_cache->room_id)
                            {
                                DEBUGLOG(dark_cyan, "player ({}) at index ({}) is now team white", other_acc->acc_info.Nickname.c_str(), id);
                                player_slot_pairs.emplace_back(id, other_acc->slot_id);
                            }

                            other_acc.unlock();
                        }
                    };
                addPlayerToSlotPairs(room_cache->blueteam_session_ids);
                addPlayerToSlotPairs(room_cache->redteam_session_ids);
                std::stable_sort(player_slot_pairs.begin(), player_slot_pairs.end(),
                    [](const std::pair<uint16_t, uint32_t>& a, const std::pair<uint16_t, uint32_t>& b) {
                        return a.second < b.second;
                    });
                for (const auto& pair : player_slot_pairs)
                {
                    room_cache->neutralteam_session_ids.push_back(pair.first);
                    auto player_acc_cache = CAccount.get<unique_t>(pair.first);
                    player_acc_cache->team_id = Team::IdType::Neutral;
                    player_acc_cache.unlock();
                }
                room_cache->redteam_session_ids.clear();
                room_cache->blueteam_session_ids.clear();
            }
            std::vector<PlayerRoomClanListInfo> players_clan_info;
            for (const auto& id : players)
            {
                auto other_acc = CAccount.get<shared_t>(id);
                if (other_acc->acc_info.ClanId)
                {
                    if (CClan.contains(other_acc->acc_info.ClanId))
                    {
                        auto clan_info = CClan.get<shared_t>(other_acc->acc_info.ClanId);
                        auto info = PlayerRoomClanListInfo(other_acc->slot_id, clan_info->clan_name.c_str(), clan_info->logo_front, clan_info->logo_back, other_acc->acc_info.ClanId, 0);
                        clan_info.unlock();
                        players_clan_info.push_back(info);
                    }
                }
                else
                    players_clan_info.push_back(PlayerRoomClanListInfo(other_acc->slot_id, "", 0, 0, 0, 0));
            }
            for (const auto& id : players)
                if (auto pss = main_server->GetSessionById(id))
                    pss->SendMsg(409, 0, 37, players_clan_info.size(), reinterpret_cast<uint8_t*>(players_clan_info.data()), sizeof(PlayerRoomClanListInfo) * players_clan_info.size());
        }
    }

    inline void Modinfo(SCallbackData& callback, CMainServer* main_server)
    {
        auto session = callback.session;
        auto message = callback.message;
        if (!session || !message) return;
        //std::shared_lock lock(session->GetMutex());
        CServer* server = callback.server;
        auto broadcast = [&](std::vector<uint16_t>& ids, const uint8_t& order, const uint8_t& mission = 0, const uint8_t& extra = 0, const uint8_t& option = 0)
            {
                for (const auto& id : ids)
                    if (auto pss = server->GetSessionById(id))
                        pss->SendMsg(order, mission, extra, option, message->GetData(), message->GetDataSize());
            };

        auto session_id = session->GetSessionId();
        auto acc_cache = CAccount.get<shared_t>(session_id);
        auto acc_index = acc_cache->acc_info.Index;
        auto order = message->GetOrder();
        auto option = message->GetOption();
        auto extra = message->GetExtra();
        auto mission = message->GetMission();
        auto mode_id = static_cast<NetEngine::Room::Mode::Index>(option);
        auto data_size = message->GetDataSize();

        if (acc_index == -1 || !acc_cache->in_room || !CRoom.contains(acc_cache->room_id)) return;

        auto room_cache = CRoom.get<unique_t>(acc_cache->room_id);
        auto previous_mode = room_cache->ModeIndex;
        if (room_cache->is_playing || room_cache->host_session_id != session_id) return;
        acc_cache.unlock();
        auto players_ids = main_server->GetRoomSortedPlayerSessionIds(room_cache);

        auto settings_info = reinterpret_cast<RoomSettingsUpdateInfo*>(message->GetData());
        room_cache->max_players = settings_info->max_players;
        room_cache->time_rule = settings_info->time;
        room_cache->score_rule = settings_info->score_limit;
        room_cache->Restriction = static_cast<NetEngine::Room::Restriction::Type>(settings_info->restriction);
        room_cache->allow_drops = settings_info->allow_items;
        room_cache->allow_intruders = settings_info->allow_intruders;
        room_cache->MapIndex = static_cast<NetEngine::Room::Map::Index>(settings_info->map_index);
        room_cache->TeamBalance = NetEngine::Room::Balance::State::Disabled;//static_cast<NetEngine::Room::Balance::State>(settings_info->team_balance);
        settings_info->team_balance = NetEngine::Room::Balance::State::Disabled;
        room_cache->ModeIndex = mode_id;
        message->SetData(reinterpret_cast<uint8_t*>(settings_info), data_size);
        broadcast(players_ids, order, mission, extra, option);
		BalanceTeams(main_server, previous_mode, room_cache, players_ids);
    }

    inline void RoomModInfo(SCallbackData& callback, CMainServer* main_server)
    {
        const auto extra = callback.message->GetExtra();
        if (extra == 28)
            VotekickStart(callback, main_server);
        else if (extra == 29)
            VotekickAgree(callback, main_server);
        else if (extra == 42)
            VotekickVerify(callback, main_server);
        else
            Modinfo(callback, main_server);
    }
}