#pragma once
namespace Game
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Main;

    namespace Handlers
    {
        boost::unordered_flat_map<std::uint32_t, std::pair<std::uint32_t, std::uint32_t>> boss_rewards =
        {
            {0, {4801002, 50}}, // bronze 60%
            {1, {4801001, 35}}, // silver 25%
            {2, {4801000, 10}}, // gold 10%
            {3, {4801003, 5}}   // diamond 5%
        };
        std::uint32_t get_random_boss_reward() 
        {

            std::uint32_t total_weight = 0;
            for (const auto& [key, reward] : boss_rewards) 
                total_weight += reward.second;

            auto random_value = Utility::Random::CustomGen(1, total_weight);

            std::uint32_t cumulative_weight = 0;
            for (const auto& [key, reward] : boss_rewards)
            {
                cumulative_weight += reward.second;
                if (random_value <= cumulative_weight)
                    return reward.first;
            }
            return boss_rewards.begin()->second.first;
        }
        inline void SingleWaveEasy(std::uint32_t playtime_seconds, SingleWaveEndReq* endmatch_sw, AccCacheResource& acc_cache)
        {
            if (playtime_seconds >= 180) // 3 minutes
            {
                acc_cache->acc_info.SingleWaveHighScore = std::max(acc_cache->acc_info.SingleWaveHighScore, endmatch_sw->score);
                acc_cache->acc_info.SingleWaveHighestWave = std::max(acc_cache->acc_info.SingleWaveHighestWave, endmatch_sw->stage);
                acc_cache->acc_info.SingleWaveLastUpdate = Utility::GetUtcTimeNow64();
            }
            
        }
        inline void SingleWaveHard(std::uint32_t playtime_seconds, SingleWaveEndReq* endmatch_sw, AccCacheResource& acc_cache)
        {
            if (playtime_seconds >= 180) // 3 minutes
            {
                acc_cache->acc_info.SingleWaveHighScore = std::max(acc_cache->acc_info.SingleWaveHighScore, endmatch_sw->score);
                acc_cache->acc_info.SingleWaveHighestWave = std::max(acc_cache->acc_info.SingleWaveHighestWave, endmatch_sw->stage);
                acc_cache->acc_info.SingleWaveLastUpdate = Utility::GetUtcTimeNow64();

            } 
        }
        inline void Tutorial(CMainServer* main_server, CSession* session, AccCacheResource& acc_cache)
        {
            if (!acc_cache->acc_info.Tutorial)
            {
                main_server->SendInventoryItem(session, acc_cache, { 4500000 });
                acc_cache->acc_info.Tutorial = true;
            }
        }
        inline void ProcessLevelUp(CMainServer* main_server, CServer* server, AccCacheResource& player_acc_cache, std::uint16_t my_id, const std::vector<std::uint16_t>& playing_players)
        {
            auto send_msg = [&](CSession* session, std::uint16_t order, std::uint8_t mission, std::uint8_t extra, std::uint8_t option, std::uint8_t* data = nullptr, std::uint16_t data_size = 0)
            {
                CMessage message(session->GetEncryptionKey());
                message.SetSession(session->GetSessionId());
                message.SetCommand(order, mission, extra, option);
                if (data_size > 0 && data != nullptr) message.SetData(data, data_size);
                session->Send(message);
            };
            auto my_unique_id = NetEngine::Packets::Core::UniqueId(my_id, 1).data;
            auto old_level = player_acc_cache->acc_info.Level;
            BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "will check if level up, current level: ({})", old_level);
            auto gi = main_server->GetGradeInfoCache(old_level + 2);
            if (gi->Grade)
            {
                if (player_acc_cache->acc_info.Experience >= gi->Exp)
                {
                    BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "will level up: ({})", gi->Grade - 1);
                    player_acc_cache->acc_info.Level = old_level + 1;
                    if (gi->RewardPoint)
                    {
                        BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "will get point reward: ({})", gi->RewardPoint);
                        player_acc_cache->acc_info.MicroPoints += gi->RewardPoint;
                        /*
                        MainCurrencyUpdateAck currency_update_data = { player_acc_cache->acc_info.RockTokens, player_acc_cache->acc_info.MicroPoints, player_acc_cache->acc_info.Coins };
                        if (auto my_session = server->GetSessionById(my_id))
                        {
                            send_msg(my_session.get(), 307, 0x0, 0, 0, reinterpret_cast<uint8_t*>(&currency_update_data), sizeof(currency_update_data)); // currency update ack
                        }
                        */
                    }
                    if (gi->RewardItem)
                    {
                        BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "will get item reward: ({})", gi->RewardItem);
                        if (auto my_session = server->GetSessionById(my_id))
                        {
                            main_server->SendInventoryItem(my_session.get(), player_acc_cache, { gi->RewardItem });
                        }
                    }
                    for (const auto& others_id : playing_players)
                    {
                        if (others_id == my_id) continue;
                        if (auto other_session = server->GetSessionById(others_id))
                            send_msg(other_session.get(), 311, 0, 0, static_cast<std::uint8_t>(player_acc_cache->acc_info.Level + 1), reinterpret_cast<uint8_t*>(&my_unique_id), sizeof(my_unique_id));
                    }
                }
            }

            return;//OLD CODE BUT CLIENT DONT UNDERSTAND MULTI LEVEL UP
            /*
            auto gi = main_server->GetGradeInfoLevelForExp(old_level + 1, player_acc_cache->acc_info.Experience);
            BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "got level to check level: ({})", gi->Grade - 1);
            if (gi->Grade)
            {
                if (gi->Grade - 1 > old_level)
                {
                    BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "will check next level eligible: ({})", gi->Grade - 1);
                    if (player_acc_cache->acc_info.Experience >= gi->Exp)
                    {
                        BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "next level match exp and will level up");
                        player_acc_cache->acc_info.Level = gi->Grade - 1;
                        for (const auto& others_id : playing_players)
                        {
                            if (others_id == my_id) continue;
                            if (auto other_session = server->GetSessionById(others_id))
                                send_msg(other_session.get(), 311, 0, 0, static_cast<std::uint8_t>(player_acc_cache->acc_info.Level + 1), reinterpret_cast<uint8_t*>(&my_unique_id), sizeof(my_unique_id));
                        }
                    }
                }
            }
            */
        }
        inline void ProcessUpdatePlayerAccCache( AccCacheResource& player_acc_cache, bool draw, bool blue_team_win, std::uint32_t melee_kills, std::uint32_t rifle_kills, std::uint32_t shotgun_kills, 
            std::uint32_t sniper_kills, std::uint32_t gatling_kills, std::uint32_t bazooka_kills, std::uint32_t grenade_kills, std::uint32_t kills, std::uint32_t deaths, std::uint32_t headshots, 
            std::uint32_t assists, std::uint32_t killstreak, std::uint32_t earnt_battery, std::uint32_t total_xp ,std::uint32_t total_mp, std::uint64_t playtime_seconds, bool is_clan_match)
        {

            auto safe_add_uint32 = [](std::uint32_t& target, std::uint32_t value_to_add)
            {
                (UINT32_MAX - target < value_to_add) ? target = UINT32_MAX : target += value_to_add;
            };
            auto safe_add_uint64 = [](std::uint64_t& target, std::uint64_t value_to_add)
            {
                (UINT32_MAX - target < value_to_add) ? target = UINT32_MAX : target += value_to_add;
            };
            auto safe_add_uint8 = [](std::uint32_t& target, std::uint32_t new_value)
            {
                target = std::min<std::uint8_t>(UINT8_MAX, std::max(target, new_value));
            };

            safe_add_uint32(player_acc_cache->acc_info.MeleeKills, melee_kills);
            safe_add_uint32(player_acc_cache->acc_info.RifleKills, rifle_kills);
            safe_add_uint32(player_acc_cache->acc_info.ShotgunKills, shotgun_kills);
            safe_add_uint32(player_acc_cache->acc_info.SniperKills, sniper_kills);
            safe_add_uint32(player_acc_cache->acc_info.GatlingKills, gatling_kills);
            safe_add_uint32(player_acc_cache->acc_info.BazookaKills, bazooka_kills);
            safe_add_uint32(player_acc_cache->acc_info.GrenadeKills, grenade_kills);
            safe_add_uint32(player_acc_cache->acc_info.Kills, kills);
            safe_add_uint32(player_acc_cache->acc_info.Deaths, deaths);
            safe_add_uint32(player_acc_cache->acc_info.Headshots, headshots);
            safe_add_uint32(player_acc_cache->acc_info.Assists, assists);
            safe_add_uint8(player_acc_cache->acc_info.HighestKillStreak, killstreak);

            if (is_clan_match)
            {
                safe_add_uint32(player_acc_cache->acc_info.ClanKills, kills);
                safe_add_uint32(player_acc_cache->acc_info.ClanDeaths, deaths);
                safe_add_uint32(player_acc_cache->acc_info.ClanAssists, assists);
                safe_add_uint64(player_acc_cache->acc_info.ClanContribution, (static_cast<std::uint64_t>(kills) * 6 + assists));//TODO: own contribution math
            }
          
            if (player_acc_cache->acc_info.Energy + earnt_battery <= player_acc_cache->acc_info.MaximumEnergy)
                safe_add_uint32(player_acc_cache->acc_info.Energy, earnt_battery);

            player_acc_cache->acc_info.Experience = total_xp;
            player_acc_cache->acc_info.MicroPoints = total_mp;

            safe_add_uint64(player_acc_cache->acc_info.PlayTime, playtime_seconds);

            if (draw)
            {
                safe_add_uint32(player_acc_cache->acc_info.Draws, 1);
                if (is_clan_match)
                    safe_add_uint64(player_acc_cache->acc_info.ClanDraws, 1);
            }
            else if (blue_team_win)
            {
                if (player_acc_cache->team_id == Team::IdType::Blue)
                {
                    safe_add_uint32(player_acc_cache->acc_info.Wins, 1);
                    if (is_clan_match)
                        safe_add_uint64(player_acc_cache->acc_info.ClanWins, 1);
                }
                else
                {
                    safe_add_uint32(player_acc_cache->acc_info.Loses, 1);
                    if (is_clan_match)
                        safe_add_uint64(player_acc_cache->acc_info.ClanLoses, 1);
                }
            }
            else
            {
                if (player_acc_cache->team_id == Team::IdType::Blue)
                {
                    safe_add_uint32(player_acc_cache->acc_info.Loses, 1);
                    if (is_clan_match)
                        safe_add_uint64(player_acc_cache->acc_info.ClanLoses, 1);
                }
                else
                {
                    safe_add_uint32(player_acc_cache->acc_info.Wins, 1);
                    if (is_clan_match)
                        safe_add_uint64(player_acc_cache->acc_info.ClanWins, 1);
                }
            }
        }
        inline MainRoomEndMatchResponse GetEndMatchResponse(MainRoomEndMatchClientInfo& cl_info)
        {
            return { cl_info.melee_kills, cl_info.rifle_kills, cl_info.shotgun_kills, cl_info.sniper_kills, cl_info.gatling_kills, cl_info.bazooka_kills, cl_info.grenade_kills, cl_info.killstreak, cl_info.total_kills, cl_info.deaths, cl_info.headshots, cl_info.assists, 0, 0, 0, 0 };
        }
        inline void ClearEndmMatchResponse(MainRoomEndMatchResponse& resp, std::uint32_t tmp, std::uint32_t txp)
        {
            resp.melee_kills = 0;
            resp.rifle_kills = 0;
            resp.shotgun_kills = 0;
            resp.sniper_kills = 0;
            resp.gatling_kills = 0;
            resp.bazooka_kills = 0;
            resp.grenade_kills = 0;
            resp.killstreak = 0;
            resp.total_kills = 0;
            resp.deaths = 0;
            resp.headshots = 0;
            resp.assists = 0;
            resp.total_mp = tmp;
            resp.total_xp = txp;
            //resp.unknown = 0;
            //resp.unknown2 = 0;
        }
        inline void AddBonusExpPoint(CMainServer *server, std::vector<BaseLib::Item> items, std::uint32_t selected_character, std::uint32_t &exp, std::uint32_t &point) {
            float extra_procent_exp = 0, extra_procent_point = 0;
            for (auto& item : items) {
                if (item.is_equipped == 1 && item.character_id == static_cast<std::uint8_t>(selected_character)) {
                    //BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "will check item id ({})", (std::uint32_t)item.item_info.item_number.item_id);
                    auto item_info = server->GetItemInfoCache(item.item_info.item_number.item_id);
                    auto bonus_ef_id = item_info->BonusEffectId;
                    //BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "found bonus effect id ({})", (std::uint32_t)bonus_ef_id);
                    if (bonus_ef_id) {
                        auto bonus_ef_info = server->GetEffectInfoCache(bonus_ef_id);
                        BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "found effect info key({}) value({}) for itemid({})", (std::uint32_t)bonus_ef_info->key, (std::uint32_t)bonus_ef_info->valueA, (std::uint32_t)item.item_info.item_number.item_id);
                        if (bonus_ef_info->key == 122) {//bonus exp
                            extra_procent_exp += bonus_ef_info->valueA / 10.0;
                        }
                        else if (bonus_ef_info->key == 123) {//bonus point
                            extra_procent_point += bonus_ef_info->valueA / 10.0;
                        }
                    }
                }
            }
            extra_procent_exp = extra_procent_exp / 100;
            extra_procent_point = extra_procent_point / 100;
            BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "player before have exp: ({}), point: ({})", exp, point);
            BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "player benefit of bonus exp procent: ({}) and bonus point procent: ({})", extra_procent_exp, extra_procent_point);
            exp += (std::uint32_t)(extra_procent_exp * exp);
            point += (std::uint32_t)(extra_procent_point * point);
            BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "player now will get exp: ({}), point: ({})", exp, point);
        }

        inline void TeamDeathMatch(CMainServer* main_server, CServer* server, RoomCacheResource& room_cache, 
            boost::unordered_flat_map<std::uint32_t, MainRoomEndMatchClientInfo>& client_match_infos,
            boost::unordered_flat_map<std::uint32_t, MainRoomEndMatchResponse>& end_match_infos,
            std::vector<std::uint16_t>& playing_players, bool draw, bool blue_team_win, std::uint16_t id, std::uint64_t end_match_time) 
        {
            for (const auto& id : playing_players)
            {
                if (client_match_infos.find(id) == client_match_infos.end())  continue;
                auto player_acc_cache = main_server->GetAccCacheUniqueBySessionId(id);
                if (player_acc_cache->acc_info.Index != -1)
                {
                    auto playtime_seconds = end_match_time - player_acc_cache->match_loaded_time;
                    if (auto player_session = server->GetSessionById(id))
                    {
                        auto& match_info = client_match_infos[id];
                        auto my_unique_id = match_info.unique_id;
                        auto unknown1 = match_info.unknown1, unknown2 = match_info.unknown2, unknown3 = match_info.unknown3, unknown4 = match_info.unknown4;
                        auto rsp = GetEndMatchResponse(match_info);
                        auto no_rewards = (rsp.total_kills == 0 && rsp.deaths == 0 && rsp.assists == 0);
                        auto earnt_battery = player_acc_cache->earnt_battery;
                        player_acc_cache->earnt_battery = 0;
                        auto playtime_min_seconds = main_server->GetPlaytimeMinSeconds();
                        if (playtime_seconds < playtime_min_seconds)
                        {
                            no_rewards = true, earnt_battery = 0;
                            ClearEndmMatchResponse(rsp, player_acc_cache->acc_info.MicroPoints, player_acc_cache->acc_info.Experience); end_match_infos.insert({ id,rsp });
                        }
                        else
                        {
                            auto ri = main_server->GetRewardInfoCache(room_cache->ModeIndex);
                            auto exp_earnt = no_rewards ? 0 : std::min(ri->ExpBase + (2 * rsp.total_kills * ri->ExpKill) + ((rsp.total_kills ? 2 : 1) * rsp.deaths * ri->ExpDeath) + (rsp.assists * ri->ExpAssist), ri->ExpMax);
                            BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "calc point: {} {} {} {} {}", ri->PointBase , (rsp.total_kills * ri->PointKill) , (rsp.total_kills ? (rsp.deaths * ri->PointDeath) : 0) , (rsp.assists * ri->PointAssist), ri->PointMax);
                            auto points_earnt = no_rewards ? 0 : std::min(ri->PointBase + (rsp.total_kills * ri->PointKill) + (rsp.total_kills ? (rsp.deaths * ri->PointDeath) : 0) + (rsp.assists * ri->PointAssist), ri->PointMax);
                            AddBonusExpPoint(main_server, player_acc_cache->inventory_items, player_acc_cache->acc_info.SelectedCharacter, exp_earnt, points_earnt);
                            ri.unlock();
                            auto old_exp = player_acc_cache->acc_info.Experience;
                            rsp.total_mp = player_acc_cache->acc_info.MicroPoints + points_earnt;
                            rsp.total_xp = old_exp + exp_earnt;
                            end_match_infos.insert({ id, rsp });
                            ProcessLevelUp(main_server, server, player_acc_cache, id, playing_players);
                            if (!no_rewards)
                            {
                                ProcessUpdatePlayerAccCache(player_acc_cache, draw, blue_team_win, rsp.melee_kills, rsp.rifle_kills, rsp.shotgun_kills, rsp.sniper_kills, rsp.gatling_kills, rsp.bazooka_kills, rsp.grenade_kills,
                                    rsp.total_kills, rsp.deaths, rsp.headshots, rsp.assists, rsp.killstreak, earnt_battery, rsp.total_xp, rsp.total_mp, playtime_seconds, false);
                            }
                        }
                    }
                    player_acc_cache->playing = false;
                    player_acc_cache->state = room_cache->host_session_id == id ? PlayerInfo::State::HostReady : PlayerInfo::State::Waiting;
                }
                player_acc_cache.unlock();
            }
        }

        inline void FreeForAll(CMainServer* main_server, CServer* server, RoomCacheResource& room_cache,
            boost::unordered_flat_map<std::uint32_t, MainRoomEndMatchClientInfo>& client_match_infos,
            boost::unordered_flat_map<std::uint32_t, MainRoomEndMatchResponse>& end_match_infos,
            std::vector<std::uint16_t>& playing_players, bool draw, bool blue_team_win, std::uint16_t id, std::uint64_t end_match_time)
        {
            for (const auto& id : playing_players)
            {
                if (client_match_infos.find(id) == client_match_infos.end())  continue;
                auto player_acc_cache = main_server->GetAccCacheUniqueBySessionId(id);
                if (player_acc_cache->acc_info.Index != -1)
                {
                    auto playtime_seconds = end_match_time - player_acc_cache->match_loaded_time;

                    if (auto player_session = server->GetSessionById(id))
                    {
                        auto& match_info = client_match_infos[id];
                        auto my_unique_id = match_info.unique_id;
                        auto unknown1 = match_info.unknown1, unknown2 = match_info.unknown2, unknown3 = match_info.unknown3, unknown4 = match_info.unknown4;
                        auto rsp = GetEndMatchResponse(match_info);
                        auto no_rewards = (rsp.total_kills == 0 && rsp.deaths == 0 && rsp.assists == 0);
                        auto earnt_battery = player_acc_cache->earnt_battery;
                        player_acc_cache->earnt_battery = 0;
                        auto playtime_min_seconds = main_server->GetPlaytimeMinSeconds();
                        if (playtime_seconds < playtime_min_seconds)
                        {
                            no_rewards = true, earnt_battery = 0;
                            ClearEndmMatchResponse(rsp, player_acc_cache->acc_info.MicroPoints, player_acc_cache->acc_info.Experience); end_match_infos.insert({ id,rsp });
                        }
                        else
                        {
                            auto ri = main_server->GetRewardInfoCache(room_cache->ModeIndex);
                            auto exp_earnt = no_rewards ? 0 : std::min(ri->ExpBase + (2 * rsp.total_kills * ri->ExpKill) + (2 * rsp.deaths * ri->ExpDeath) + (rsp.assists * ri->ExpAssist), ri->ExpMax);
                            auto points_earnt = no_rewards ? 0 : std::min(ri->PointBase + (rsp.total_kills * ri->PointKill) + (rsp.deaths * ri->PointDeath) + (rsp.assists * ri->PointAssist), ri->PointMax);
                            ri.unlock();
                            auto old_exp = player_acc_cache->acc_info.Experience;
                            rsp.total_mp = player_acc_cache->acc_info.MicroPoints + points_earnt;
                            rsp.total_xp = old_exp + exp_earnt;
                            end_match_infos.insert({ id, rsp });
                            ProcessLevelUp(main_server, server, player_acc_cache, id, playing_players);
                            if (!no_rewards)
                            {
                                ProcessUpdatePlayerAccCache(player_acc_cache, draw, blue_team_win, rsp.melee_kills, rsp.rifle_kills, rsp.shotgun_kills, rsp.sniper_kills, rsp.gatling_kills, rsp.bazooka_kills, rsp.grenade_kills,
                                    rsp.total_kills, rsp.deaths, rsp.headshots, rsp.assists, rsp.killstreak, earnt_battery, rsp.total_xp, rsp.total_mp, playtime_seconds, false);
                            }
                        }
                    }
                    player_acc_cache->playing = false;
                    player_acc_cache->state = room_cache->host_session_id == id ? PlayerInfo::State::HostReady : PlayerInfo::State::Waiting;
                }
                player_acc_cache.unlock();
            }
        }

        inline void ItemMatch(CMainServer* main_server, CServer* server, RoomCacheResource& room_cache,
            boost::unordered_flat_map<std::uint32_t, MainRoomEndMatchClientInfo>& client_match_infos,
            boost::unordered_flat_map<std::uint32_t, MainRoomEndMatchResponse>& end_match_infos,
            std::vector<std::uint16_t>& playing_players, bool draw, bool blue_team_win, std::uint16_t id, std::uint64_t end_match_time)
        {
            for (const auto& id : playing_players)
            {
                if (client_match_infos.find(id) == client_match_infos.end())  continue;
                auto player_acc_cache = main_server->GetAccCacheUniqueBySessionId(id);
                if (player_acc_cache->acc_info.Index != -1)
                {
                    auto playtime_seconds = end_match_time - player_acc_cache->match_loaded_time;

                    if (auto player_session = server->GetSessionById(id))
                    {
                        auto& match_info = client_match_infos[id];
                        auto my_unique_id = match_info.unique_id;
                        auto unknown1 = match_info.unknown1, unknown2 = match_info.unknown2, unknown3 = match_info.unknown3, unknown4 = match_info.unknown4;
                        auto rsp = GetEndMatchResponse(match_info);
                        auto no_rewards = (rsp.total_kills == 0 && rsp.deaths == 0 && rsp.assists == 0);
                        auto earnt_battery = player_acc_cache->earnt_battery;
                        player_acc_cache->earnt_battery = 0;
                        auto playtime_min_seconds = main_server->GetPlaytimeMinSeconds();
                        if (playtime_seconds < playtime_min_seconds)
                        {
                            no_rewards = true, earnt_battery = 0;
                            ClearEndmMatchResponse(rsp, player_acc_cache->acc_info.MicroPoints, player_acc_cache->acc_info.Experience); end_match_infos.insert({ id,rsp });
                        }
                        else
                        {
                            auto ri = main_server->GetRewardInfoCache(room_cache->ModeIndex);
                            auto exp_earnt = no_rewards ? 0 : std::min(ri->ExpBase + (2 * rsp.total_kills * ri->ExpKill) + (2 * rsp.deaths * ri->ExpDeath) + (rsp.assists * ri->ExpAssist), ri->ExpMax);
                            auto points_earnt = no_rewards ? 0 : std::min(ri->PointBase + (rsp.total_kills * ri->PointKill) + (rsp.deaths * ri->PointDeath) + (rsp.assists * ri->PointAssist), ri->PointMax);
                            ri.unlock();
                            auto old_exp = player_acc_cache->acc_info.Experience;
                            rsp.total_mp = player_acc_cache->acc_info.MicroPoints + points_earnt;
                            rsp.total_xp = old_exp + exp_earnt;
                            end_match_infos.insert({ id, rsp });
                            ProcessLevelUp(main_server, server, player_acc_cache, id, playing_players);
                            if (!no_rewards)
                            {
                                ProcessUpdatePlayerAccCache(player_acc_cache, draw, blue_team_win, rsp.melee_kills, rsp.rifle_kills, rsp.shotgun_kills, rsp.sniper_kills, rsp.gatling_kills, rsp.bazooka_kills, rsp.grenade_kills,
                                    rsp.total_kills, rsp.deaths, rsp.headshots, rsp.assists, rsp.killstreak, earnt_battery, rsp.total_xp, rsp.total_mp, playtime_seconds, false);
                            }   
                        }
                    }
                    player_acc_cache->playing = false;
                    player_acc_cache->state = room_cache->host_session_id == id ? PlayerInfo::State::HostReady : PlayerInfo::State::Waiting;
                }
                player_acc_cache.unlock();
            }
        }

        inline void CaptureTheBattery(CMainServer* main_server, CServer* server, RoomCacheResource& room_cache,
            boost::unordered_flat_map<std::uint32_t, MainRoomEndMatchClientInfo>& client_match_infos,
            boost::unordered_flat_map<std::uint32_t, MainRoomEndMatchResponse>& end_match_infos,
            std::vector<std::uint16_t>& playing_players, bool draw, bool blue_team_win, std::uint16_t id, std::uint64_t end_match_time)
        {
            for (const auto& id : playing_players)
            {
                if (client_match_infos.find(id) == client_match_infos.end())  continue;
                auto player_acc_cache = main_server->GetAccCacheUniqueBySessionId(id);
                if (player_acc_cache->acc_info.Index != -1)
                {
                    auto playtime_seconds = end_match_time - player_acc_cache->match_loaded_time;

                    if (auto player_session = server->GetSessionById(id))
                    {
                        auto& match_info = client_match_infos[id];
                        auto my_unique_id = match_info.unique_id;
                        auto unknown1 = match_info.unknown1, unknown2 = match_info.unknown2, unknown3 = match_info.unknown3, unknown4 = match_info.unknown4;
                        auto rsp = GetEndMatchResponse(match_info);
                        auto no_rewards = (rsp.total_kills == 0 && rsp.deaths == 0 && rsp.assists == 0);
                        auto earnt_battery = player_acc_cache->earnt_battery;
                        player_acc_cache->earnt_battery = 0;
                        auto playtime_min_seconds = main_server->GetPlaytimeMinSeconds();
                        if (playtime_seconds < playtime_min_seconds)
                        {
                            no_rewards = true, earnt_battery = 0;
                            ClearEndmMatchResponse(rsp, player_acc_cache->acc_info.MicroPoints, player_acc_cache->acc_info.Experience); end_match_infos.insert({ id,rsp });
                        }
                        else
                        {
                            auto ri = main_server->GetRewardInfoCache(room_cache->ModeIndex);
                            auto TeamBatteryCaptures = unknown2;
                            auto MyBatteryCaptures = unknown1;

                            auto exp_earnt = no_rewards ? 0 : std::min(ri->ExpBase + (rsp.total_kills * ri->ExpKill) + (rsp.deaths * ri->ExpDeath) + (rsp.assists * ri->ExpAssist) + (TeamBatteryCaptures * ri->ExpMission + 1) + (MyBatteryCaptures * ri->ExpMissionWin + 2), ri->ExpMax);
                            auto points_earnt = no_rewards ? 0 : std::min(ri->PointBase + (rsp.total_kills * ri->PointKill) - (rsp.deaths * ri->PointDeath) + (rsp.assists * ri->PointAssist) + (TeamBatteryCaptures * ri->PointMission + 3) + (MyBatteryCaptures * ri->PointMissionWin + 1), ri->PointMax);
                            ri.unlock();
                            auto old_exp = player_acc_cache->acc_info.Experience;
                            rsp.total_mp = player_acc_cache->acc_info.MicroPoints + points_earnt;
                            rsp.total_xp = old_exp + exp_earnt;
                            end_match_infos.insert({ id, rsp });
                            ProcessLevelUp(main_server, server, player_acc_cache, id, playing_players);
                            if (!no_rewards)
                            {
                                ProcessUpdatePlayerAccCache(player_acc_cache, draw, blue_team_win, rsp.melee_kills, rsp.rifle_kills, rsp.shotgun_kills, rsp.sniper_kills, rsp.gatling_kills, rsp.bazooka_kills, rsp.grenade_kills,
                                    rsp.total_kills, rsp.deaths, rsp.headshots, rsp.assists, rsp.killstreak, earnt_battery, rsp.total_xp, rsp.total_mp, playtime_seconds, false);
                            }
                        }
                    }
                    player_acc_cache->playing = false;
                    player_acc_cache->state = room_cache->host_session_id == id ? PlayerInfo::State::HostReady : PlayerInfo::State::Waiting;
                }
                player_acc_cache.unlock();
            }
        }

        inline void Elimination(CMainServer* main_server, CServer* server, RoomCacheResource& room_cache,
            boost::unordered_flat_map<std::uint32_t, MainRoomEndMatchClientInfo>& client_match_infos,
            boost::unordered_flat_map<std::uint32_t, MainRoomEndMatchResponse>& end_match_infos,
            std::vector<std::uint16_t>& playing_players, bool draw, bool blue_team_win, std::uint16_t id, std::uint64_t end_match_time)
        {
            for (const auto& id : playing_players)
            {
                if (client_match_infos.find(id) == client_match_infos.end())  continue;
                auto player_acc_cache = main_server->GetAccCacheUniqueBySessionId(id);
                if (player_acc_cache->acc_info.Index != -1)
                {
                    auto playtime_seconds = end_match_time - player_acc_cache->match_loaded_time;

                    if (auto player_session = server->GetSessionById(id))
                    {
                        auto& match_info = client_match_infos[id];
                        auto my_unique_id = match_info.unique_id;
                        auto unknown1 = match_info.unknown1, unknown2 = match_info.unknown2, unknown3 = match_info.unknown3, unknown4 = match_info.unknown4;
                        auto rsp = GetEndMatchResponse(match_info);
                        auto no_rewards = (rsp.total_kills == 0 && rsp.deaths == 0 && rsp.assists == 0);
                        auto earnt_battery = player_acc_cache->earnt_battery;
                        player_acc_cache->earnt_battery = 0;
                        auto playtime_min_seconds = main_server->GetPlaytimeMinSeconds();
                        if (playtime_seconds < playtime_min_seconds)
                        {
                            no_rewards = true, earnt_battery = 0;
                            ClearEndmMatchResponse(rsp, player_acc_cache->acc_info.MicroPoints, player_acc_cache->acc_info.Experience); end_match_infos.insert({ id,rsp });
                        }
                        else
                        {
                            auto ri = main_server->GetRewardInfoCache(room_cache->ModeIndex);
                            auto RoundWin = unknown2;
                            auto exp_earnt = no_rewards ? 0 : std::min(ri->ExpBase + (rsp.total_kills * ri->ExpKill) + (rsp.deaths * ri->ExpDeath) + (rsp.assists * ri->ExpAssist) + (RoundWin * ri->ExpMissionWin), ri->ExpMax);
                            auto points_earnt = no_rewards ? 0 : std::min(ri->PointBase + (rsp.total_kills * ri->PointKill) - (rsp.deaths * ri->PointDeath) + (rsp.assists * ri->PointAssist) + (RoundWin * ri->PointMissionWin), ri->PointMax);

                            ri.unlock();
                            auto old_exp = player_acc_cache->acc_info.Experience;
                            rsp.total_mp = player_acc_cache->acc_info.MicroPoints + points_earnt;
                            rsp.total_xp = old_exp + exp_earnt;
                            end_match_infos.insert({ id, rsp });
                            ProcessLevelUp(main_server, server, player_acc_cache, id, playing_players);
                            if (!no_rewards)
                            {
                                ProcessUpdatePlayerAccCache(player_acc_cache, draw, blue_team_win, rsp.melee_kills, rsp.rifle_kills, rsp.shotgun_kills, rsp.sniper_kills, rsp.gatling_kills, rsp.bazooka_kills, rsp.grenade_kills,
                                    rsp.total_kills, rsp.deaths, rsp.headshots, rsp.assists, rsp.killstreak, earnt_battery, rsp.total_xp, rsp.total_mp, playtime_seconds, false);
                            }
                        }
                    }
                    player_acc_cache->playing = false;
                    player_acc_cache->state = room_cache->host_session_id == id ? PlayerInfo::State::HostReady : PlayerInfo::State::Waiting;
                }
                player_acc_cache.unlock();
            }
        }

        inline void SuperItemMatch(CMainServer* main_server, CServer* server, RoomCacheResource& room_cache,
            boost::unordered_flat_map<std::uint32_t, MainRoomEndMatchClientInfo>& client_match_infos,
            boost::unordered_flat_map<std::uint32_t, MainRoomEndMatchResponse>& end_match_infos,
            std::vector<std::uint16_t>& playing_players, bool draw, bool blue_team_win, std::uint16_t id, std::uint64_t end_match_time)
        {
            for (const auto& id : playing_players)
            {
                if (client_match_infos.find(id) == client_match_infos.end())  continue;
                auto player_acc_cache = main_server->GetAccCacheUniqueBySessionId(id);
                if (player_acc_cache->acc_info.Index != -1)
                {
                    auto playtime_seconds = end_match_time - player_acc_cache->match_loaded_time;

                    if (auto player_session = server->GetSessionById(id))
                    {
                        auto& match_info = client_match_infos[id];
                        auto my_unique_id = match_info.unique_id;
                        auto unknown1 = match_info.unknown1, unknown2 = match_info.unknown2, unknown3 = match_info.unknown3, unknown4 = match_info.unknown4;
                        auto rsp = GetEndMatchResponse(match_info);
                        auto no_rewards = (rsp.total_kills == 0 && rsp.deaths == 0 && rsp.assists == 0);
                        auto earnt_battery = player_acc_cache->earnt_battery;
                        player_acc_cache->earnt_battery = 0;
                        auto playtime_min_seconds = main_server->GetPlaytimeMinSeconds();
                        if (playtime_seconds < playtime_min_seconds)
                        {
                            no_rewards = true, earnt_battery = 0;
                            ClearEndmMatchResponse(rsp, player_acc_cache->acc_info.MicroPoints, player_acc_cache->acc_info.Experience); end_match_infos.insert({ id,rsp });
                        }
                        else
                        {
                            auto ri = main_server->GetRewardInfoCache(room_cache->ModeIndex);
                            auto exp_earnt = no_rewards ? 0 : std::min(ri->ExpBase + (2 * rsp.total_kills * ri->ExpKill) + (2 * rsp.deaths * ri->ExpDeath) + (rsp.assists * ri->ExpAssist), ri->ExpMax);
                            auto points_earnt = no_rewards ? 0 : std::min(ri->PointBase + (rsp.total_kills * ri->PointKill) + (rsp.deaths * ri->PointDeath) + (rsp.assists * ri->PointAssist), ri->PointMax);
                            ri.unlock();
                            auto old_exp = player_acc_cache->acc_info.Experience;
                            rsp.total_mp = player_acc_cache->acc_info.MicroPoints + points_earnt;
                            rsp.total_xp = old_exp + exp_earnt;
                            end_match_infos.insert({ id, rsp });
                            ProcessLevelUp(main_server, server, player_acc_cache, id, playing_players);
                            if (!no_rewards)
                            {
                                ProcessUpdatePlayerAccCache(player_acc_cache, draw, blue_team_win, rsp.melee_kills, rsp.rifle_kills, rsp.shotgun_kills, rsp.sniper_kills, rsp.gatling_kills, rsp.bazooka_kills, rsp.grenade_kills,
                                    rsp.total_kills, rsp.deaths, rsp.headshots, rsp.assists, rsp.killstreak, earnt_battery, rsp.total_xp, rsp.total_mp, playtime_seconds, false);
                            }
                        }
                    }
                    player_acc_cache->playing = false;
                    player_acc_cache->state = room_cache->host_session_id == id ? PlayerInfo::State::HostReady : PlayerInfo::State::Waiting;
                }
                player_acc_cache.unlock();
            }
        }

        inline void ZombieMode(CMainServer* main_server, CServer* server, RoomCacheResource& room_cache,
            boost::unordered_flat_map<std::uint32_t, MainRoomEndMatchClientInfo>& client_match_infos,
            boost::unordered_flat_map<std::uint32_t, MainRoomEndMatchResponse>& end_match_infos,
            std::vector<std::uint16_t>& playing_players, bool draw, bool blue_team_win, std::uint16_t id, std::uint64_t end_match_time)
        {
            for (const auto& id : playing_players)
            {
                if (client_match_infos.find(id) == client_match_infos.end())  continue;
                auto player_acc_cache = main_server->GetAccCacheUniqueBySessionId(id);
                if (player_acc_cache->acc_info.Index != -1)
                {
                    auto playtime_seconds = end_match_time - player_acc_cache->match_loaded_time;

                    if (auto player_session = server->GetSessionById(id))
                    {
                        auto& match_info = client_match_infos[id];
                        auto my_unique_id = match_info.unique_id;
                        auto unknown1 = match_info.unknown1, unknown2 = match_info.unknown2, unknown3 = match_info.unknown3, unknown4 = match_info.unknown4;
                        auto rsp = GetEndMatchResponse(match_info);
                        auto no_rewards = (rsp.total_kills == 0 && rsp.deaths == 0 && rsp.assists == 0);
                        auto earnt_battery = player_acc_cache->earnt_battery;
                        player_acc_cache->earnt_battery = 0;
                        auto playtime_min_seconds = main_server->GetPlaytimeMinSeconds();
                        if (playtime_seconds < playtime_min_seconds)
                        {
                            no_rewards = true, earnt_battery = 0;
                            ClearEndmMatchResponse(rsp, player_acc_cache->acc_info.MicroPoints, player_acc_cache->acc_info.Experience); end_match_infos.insert({ id,rsp });
                        }
                        else
                        {
                            auto ri = main_server->GetRewardInfoCache(room_cache->ModeIndex);
                            auto ZombiKill = rsp.killstreak;
                            auto Infected = rsp.melee_kills;
                            auto Survived = unknown2;
                            auto exp_earnt = no_rewards ? 0 : std::min(ri->ExpBase + (ZombiKill * ri->ExpModeKill) + (Infected * ri->ExpKill) + (rsp.deaths * ri->ExpDeath) + (Survived * ri->ExpMissionWin), ri->ExpMax);
                            auto points_earnt = no_rewards ? 0 : std::min(ri->PointBase + (ZombiKill * ri->PointModeKill) + (Infected * ri->PointKill) - (rsp.deaths * ri->PointDeath) + (Survived * ri->PointMissionWin), ri->PointMax);
                            ri.unlock();
                            auto old_exp = player_acc_cache->acc_info.Experience;
                            rsp.total_mp = player_acc_cache->acc_info.MicroPoints + points_earnt;
                            rsp.total_xp = old_exp + exp_earnt;
                            end_match_infos.insert({ id, rsp });
                            ProcessLevelUp(main_server, server, player_acc_cache, id, playing_players);
                            if (!no_rewards)
                            {
                                ProcessUpdatePlayerAccCache(player_acc_cache, draw, blue_team_win, rsp.melee_kills, rsp.rifle_kills, rsp.shotgun_kills, rsp.sniper_kills, rsp.gatling_kills, rsp.bazooka_kills, rsp.grenade_kills,
                                    rsp.total_kills, rsp.deaths, rsp.headshots, rsp.assists, rsp.killstreak, earnt_battery, rsp.total_xp, rsp.total_mp, playtime_seconds, false);
                            }
                        }
                    }
                    player_acc_cache->playing = false;
                    player_acc_cache->state = room_cache->host_session_id == id ? PlayerInfo::State::HostReady : PlayerInfo::State::Waiting;
                }
                player_acc_cache.unlock();
            }
        }

        inline void ArmsRace(CMainServer* main_server, CServer* server, RoomCacheResource& room_cache,
            boost::unordered_flat_map<std::uint32_t, MainRoomEndMatchClientInfo>& client_match_infos,
            boost::unordered_flat_map<std::uint32_t, MainRoomEndMatchResponse>& end_match_infos,
            std::vector<std::uint16_t>& playing_players, bool draw, bool blue_team_win, std::uint16_t id, std::uint64_t end_match_time)
        {
            for (const auto& id : playing_players)
            {
                if (client_match_infos.find(id) == client_match_infos.end())  continue;
                auto player_acc_cache = main_server->GetAccCacheUniqueBySessionId(id);
                if (player_acc_cache->acc_info.Index != -1)
                {
                    auto playtime_seconds = end_match_time - player_acc_cache->match_loaded_time;

                    if (auto player_session = server->GetSessionById(id))
                    {
                        auto& match_info = client_match_infos[id];
                        auto my_unique_id = match_info.unique_id;
                        auto unknown1 = match_info.unknown1, unknown2 = match_info.unknown2, unknown3 = match_info.unknown3, unknown4 = match_info.unknown4;
                        auto rsp = GetEndMatchResponse(match_info);
                        auto no_rewards = (rsp.total_kills == 0 && rsp.deaths == 0 && rsp.assists == 0);
                        auto earnt_battery = player_acc_cache->earnt_battery;
                        player_acc_cache->earnt_battery = 0;
                        auto playtime_min_seconds = main_server->GetPlaytimeMinSeconds();
                        if (playtime_seconds < playtime_min_seconds)
                        {
                            no_rewards = true, earnt_battery = 0;
                            ClearEndmMatchResponse(rsp, player_acc_cache->acc_info.MicroPoints, player_acc_cache->acc_info.Experience); end_match_infos.insert({ id,rsp });
                        }
                        else
                        {
                            auto ri = main_server->GetRewardInfoCache(room_cache->ModeIndex);
                            auto Mission = unknown1;
                            auto exp_earnt = no_rewards ? 0 : std::min(ri->ExpBase + (2 * rsp.total_kills * ri->ExpKill) + (2 * rsp.deaths * ri->ExpDeath) + (rsp.assists * ri->ExpAssist) + (Mission * ri->ExpMission), ri->ExpMax);
                            auto points_earnt = no_rewards ? 0 : std::min(ri->PointBase + (rsp.total_kills * ri->PointKill) + (rsp.deaths * ri->PointDeath) + (rsp.assists * ri->PointAssist) + (Mission * ri->PointMission), ri->PointMax);
                            ri.unlock();
                            auto old_exp = player_acc_cache->acc_info.Experience;
                            rsp.total_mp = player_acc_cache->acc_info.MicroPoints + points_earnt;
                            rsp.total_xp = old_exp + exp_earnt;
                            end_match_infos.insert({ id, rsp });
                            ProcessLevelUp(main_server, server, player_acc_cache, id, playing_players);
                            if (!no_rewards)
                            {
                                ProcessUpdatePlayerAccCache(player_acc_cache, draw, blue_team_win, rsp.melee_kills, rsp.rifle_kills, rsp.shotgun_kills, rsp.sniper_kills, rsp.gatling_kills, rsp.bazooka_kills, rsp.grenade_kills,
                                    rsp.total_kills, rsp.deaths, rsp.headshots, rsp.assists, rsp.killstreak, earnt_battery, rsp.total_xp, rsp.total_mp, playtime_seconds, false);
                            }
                        }
                    }
                    player_acc_cache->playing = false;
                    player_acc_cache->state = room_cache->host_session_id == id ? PlayerInfo::State::HostReady : PlayerInfo::State::Waiting;
                }
                player_acc_cache.unlock();
            }
        }

        inline void BombBattle(CMainServer* main_server, CServer* server, RoomCacheResource& room_cache,
            boost::unordered_flat_map<std::uint32_t, MainRoomEndMatchClientInfo>& client_match_infos,
            boost::unordered_flat_map<std::uint32_t, MainRoomEndMatchResponse>& end_match_infos,
            std::vector<std::uint16_t>& playing_players, bool draw, bool blue_team_win, std::uint16_t id, std::uint64_t end_match_time)
        {
            for (const auto& id : playing_players)
            {
                if (client_match_infos.find(id) == client_match_infos.end())  continue;
                auto player_acc_cache = main_server->GetAccCacheUniqueBySessionId(id);
                if (player_acc_cache->acc_info.Index != -1)
                {
                    auto playtime_seconds = end_match_time - player_acc_cache->match_loaded_time;

                    if (auto player_session = server->GetSessionById(id))
                    {
                        auto& match_info = client_match_infos[id];
                        auto my_unique_id = match_info.unique_id;
                        auto unknown1 = match_info.unknown1, unknown2 = match_info.unknown2, unknown3 = match_info.unknown3, unknown4 = match_info.unknown4;
                        auto rsp = GetEndMatchResponse(match_info);
                        auto no_rewards = (rsp.total_kills == 0 && rsp.deaths == 0 && rsp.assists == 0);
                        auto earnt_battery = player_acc_cache->earnt_battery;
                        player_acc_cache->earnt_battery = 0;
                        auto playtime_min_seconds = main_server->GetPlaytimeMinSeconds();
                        if (playtime_seconds < playtime_min_seconds)
                        {
                            no_rewards = true, earnt_battery = 0;
                            ClearEndmMatchResponse(rsp, player_acc_cache->acc_info.MicroPoints, player_acc_cache->acc_info.Experience); end_match_infos.insert({ id,rsp });
                        }
                        else
                        {
                            auto ri = main_server->GetRewardInfoCache(room_cache->ModeIndex);
                            auto RoundWin = unknown2;
                            auto exp_earnt = no_rewards ? 0 : std::min(ri->ExpBase + (rsp.total_kills * ri->ExpKill) + (rsp.deaths * ri->ExpDeath) + (rsp.assists * ri->ExpAssist) + (RoundWin * ri->ExpMissionWin), ri->ExpMax);
                            auto points_earnt = no_rewards ? 0 : std::min(ri->PointBase + (rsp.total_kills * ri->PointKill) - (rsp.deaths * ri->PointDeath) + (rsp.assists * ri->PointAssist) + (RoundWin * ri->PointMissionWin), ri->PointMax);

                            ri.unlock();
                            auto old_exp = player_acc_cache->acc_info.Experience;
                            rsp.total_mp = player_acc_cache->acc_info.MicroPoints + points_earnt;
                            rsp.total_xp = old_exp + exp_earnt;
                            end_match_infos.insert({ id, rsp });
                            ProcessLevelUp(main_server, server, player_acc_cache, id, playing_players);
                            if (!no_rewards)
                            {
                                ProcessUpdatePlayerAccCache(player_acc_cache, draw, blue_team_win, rsp.melee_kills, rsp.rifle_kills, rsp.shotgun_kills, rsp.sniper_kills, rsp.gatling_kills, rsp.bazooka_kills, rsp.grenade_kills,
                                    rsp.total_kills, rsp.deaths, rsp.headshots, rsp.assists, rsp.killstreak, earnt_battery, rsp.total_xp, rsp.total_mp, playtime_seconds, false);
                            }
                        }
                    }
                    player_acc_cache->playing = false;
                    player_acc_cache->state = room_cache->host_session_id == id ? PlayerInfo::State::HostReady : PlayerInfo::State::Waiting;
                }
                player_acc_cache.unlock();
            }
        }

        inline void ClanCaptureTheBattery(CMainServer* main_server, CServer* server, RoomCacheResource& room_cache,
            boost::unordered_flat_map<std::uint32_t, MainRoomEndMatchClientInfo>& client_match_infos,
            boost::unordered_flat_map<std::uint32_t, MainRoomEndMatchResponse>& end_match_infos,
            std::vector<std::uint16_t>& playing_players, bool draw, bool blue_team_win, std::uint16_t id, std::uint64_t end_match_time)
        {
            for (const auto& id : playing_players)
            {
                if (client_match_infos.find(id) == client_match_infos.end())  continue;
                auto player_acc_cache = main_server->GetAccCacheUniqueBySessionId(id);
                if (player_acc_cache->acc_info.Index != -1)
                {
                    auto playtime_seconds = end_match_time - player_acc_cache->match_loaded_time;

                    if (auto player_session = server->GetSessionById(id))
                    {
                        auto& match_info = client_match_infos[id];
                        auto my_unique_id = match_info.unique_id;
                        auto unknown1 = match_info.unknown1, unknown2 = match_info.unknown2, unknown3 = match_info.unknown3, unknown4 = match_info.unknown4;
                        auto rsp = GetEndMatchResponse(match_info);
                        auto no_rewards = (rsp.total_kills == 0 && rsp.deaths == 0 && rsp.assists == 0);
                        auto earnt_battery = player_acc_cache->earnt_battery;
                        player_acc_cache->earnt_battery = 0;
                        auto playtime_min_seconds = main_server->GetPlaytimeMinSeconds();
                        if (playtime_seconds < playtime_min_seconds)
                        {
                            no_rewards = true, earnt_battery = 0;
                            ClearEndmMatchResponse(rsp, player_acc_cache->acc_info.MicroPoints, player_acc_cache->acc_info.Experience); end_match_infos.insert({ id,rsp });
                        }
                        else
                        {
                            auto ri = main_server->GetRewardInfoCache(room_cache->ModeIndex);
                            auto TeamBatteryCaptures = unknown2;
                            auto MyBatteryCaptures = unknown1;

                            auto exp_earnt = no_rewards ? 0 : std::min(ri->ExpBase + (rsp.total_kills * ri->ExpKill) + (rsp.deaths * ri->ExpDeath) + (rsp.assists * ri->ExpAssist) + (TeamBatteryCaptures * ri->ExpMission + 1) + (MyBatteryCaptures * ri->ExpMissionWin + 2), ri->ExpMax);
                            auto points_earnt = no_rewards ? 0 : std::min(ri->PointBase + (rsp.total_kills * ri->PointKill) - (rsp.deaths * ri->PointDeath) + (rsp.assists * ri->PointAssist) + (TeamBatteryCaptures * ri->PointMission + 3) + (MyBatteryCaptures * ri->PointMissionWin + 1), ri->PointMax);
                            ri.unlock();
                            auto old_exp = player_acc_cache->acc_info.Experience;
                            rsp.total_mp = player_acc_cache->acc_info.MicroPoints + points_earnt;
                            rsp.total_xp = old_exp + exp_earnt;
                            end_match_infos.insert({ id, rsp });
                            ProcessLevelUp(main_server, server, player_acc_cache, id, playing_players);
                            if (!no_rewards)
                            {
                                ProcessUpdatePlayerAccCache(player_acc_cache, draw, blue_team_win, rsp.melee_kills, rsp.rifle_kills, rsp.shotgun_kills, rsp.sniper_kills, rsp.gatling_kills, rsp.bazooka_kills, rsp.grenade_kills,
                                    rsp.total_kills, rsp.deaths, rsp.headshots, rsp.assists, rsp.killstreak, earnt_battery, rsp.total_xp, rsp.total_mp, playtime_seconds, false);
                            }
                        }
                    }
                    player_acc_cache->playing = false;
                    player_acc_cache->state = room_cache->host_session_id == id ? PlayerInfo::State::HostReady : PlayerInfo::State::Waiting;
                }
                player_acc_cache.unlock();
            }
        }

        inline void ClanElimination(CMainServer* main_server, CServer* server, RoomCacheResource& room_cache,
            boost::unordered_flat_map<std::uint32_t, MainRoomEndMatchClientInfo>& client_match_infos,
            boost::unordered_flat_map<std::uint32_t, MainRoomEndMatchResponse>& end_match_infos,
            std::vector<std::uint16_t>& playing_players, bool draw, bool blue_team_win, std::uint16_t id, std::uint64_t end_match_time)
        {
            for (const auto& id : playing_players)
            {
                if (client_match_infos.find(id) == client_match_infos.end())  continue;
                auto player_acc_cache = main_server->GetAccCacheUniqueBySessionId(id);
                if (player_acc_cache->acc_info.Index != -1)
                {
                    auto playtime_seconds = end_match_time - player_acc_cache->match_loaded_time;

                    if (auto player_session = server->GetSessionById(id))
                    {
                        auto& match_info = client_match_infos[id];
                        auto my_unique_id = match_info.unique_id;
                        auto unknown1 = match_info.unknown1, unknown2 = match_info.unknown2, unknown3 = match_info.unknown3, unknown4 = match_info.unknown4;
                        auto rsp = GetEndMatchResponse(match_info);
                        auto no_rewards = (rsp.total_kills == 0 && rsp.deaths == 0 && rsp.assists == 0);
                        auto earnt_battery = player_acc_cache->earnt_battery;
                        player_acc_cache->earnt_battery = 0;
                        auto playtime_min_seconds = main_server->GetPlaytimeMinSeconds();
                        if (playtime_seconds < playtime_min_seconds)
                        {
                            no_rewards = true, earnt_battery = 0;
                            ClearEndmMatchResponse(rsp, player_acc_cache->acc_info.MicroPoints, player_acc_cache->acc_info.Experience); end_match_infos.insert({ id,rsp });
                        }
                        else
                        {
                            auto ri = main_server->GetRewardInfoCache(room_cache->ModeIndex);
                            auto RoundWin = unknown2;
                            auto exp_earnt = no_rewards ? 0 : std::min(ri->ExpBase + (rsp.total_kills * ri->ExpKill) + (rsp.deaths * ri->ExpDeath) + (rsp.assists * ri->ExpAssist) + (RoundWin * ri->ExpMissionWin), ri->ExpMax);
                            auto points_earnt = no_rewards ? 0 : std::min(ri->PointBase + (rsp.total_kills * ri->PointKill) - (rsp.deaths * ri->PointDeath) + (rsp.assists * ri->PointAssist) + (RoundWin * ri->PointMissionWin), ri->PointMax);

                            ri.unlock();
                            auto old_exp = player_acc_cache->acc_info.Experience;
                            rsp.total_mp = player_acc_cache->acc_info.MicroPoints + points_earnt;
                            rsp.total_xp = old_exp + exp_earnt;
                            end_match_infos.insert({ id, rsp });
                            ProcessLevelUp(main_server, server, player_acc_cache, id, playing_players);
                            if (!no_rewards)
                            {
                                ProcessUpdatePlayerAccCache(player_acc_cache, draw, blue_team_win, rsp.melee_kills, rsp.rifle_kills, rsp.shotgun_kills, rsp.sniper_kills, rsp.gatling_kills, rsp.bazooka_kills, rsp.grenade_kills,
                                    rsp.total_kills, rsp.deaths, rsp.headshots, rsp.assists, rsp.killstreak, earnt_battery, rsp.total_xp, rsp.total_mp, playtime_seconds, false);
                            }
                        }
                    }
                    player_acc_cache->playing = false;
                    player_acc_cache->state = room_cache->host_session_id == id ? PlayerInfo::State::HostReady : PlayerInfo::State::Waiting;
                }
                player_acc_cache.unlock();
            }
        }

        inline void ClanTeamDeathMatch(CMainServer* main_server, CServer* server, RoomCacheResource& room_cache,
            boost::unordered_flat_map<std::uint32_t, MainRoomEndMatchClientInfo>& client_match_infos,
            boost::unordered_flat_map<std::uint32_t, MainRoomEndMatchResponse>& end_match_infos,
            std::vector<std::uint16_t>& playing_players, bool draw, bool blue_team_win, std::uint16_t id, std::uint64_t end_match_time)
        {
            for (const auto& id : playing_players)
            {
                if (client_match_infos.find(id) == client_match_infos.end())  continue;
                auto player_acc_cache = main_server->GetAccCacheUniqueBySessionId(id);
                if (player_acc_cache->acc_info.Index != -1)
                {
                    auto playtime_seconds = end_match_time - player_acc_cache->match_loaded_time;

                    if (auto player_session = server->GetSessionById(id))
                    {
                        auto& match_info = client_match_infos[id];
                        auto my_unique_id = match_info.unique_id;
                        auto unknown1 = match_info.unknown1, unknown2 = match_info.unknown2, unknown3 = match_info.unknown3, unknown4 = match_info.unknown4;
                        auto rsp = GetEndMatchResponse(match_info);
                        auto no_rewards = (rsp.total_kills == 0 && rsp.deaths == 0 && rsp.assists == 0);
                        auto earnt_battery = player_acc_cache->earnt_battery;
                        player_acc_cache->earnt_battery = 0;
                        auto playtime_min_seconds = main_server->GetPlaytimeMinSeconds();
                        if (playtime_seconds < playtime_min_seconds)
                        {
                            no_rewards = true, earnt_battery = 0;
                            ClearEndmMatchResponse(rsp, player_acc_cache->acc_info.MicroPoints, player_acc_cache->acc_info.Experience); end_match_infos.insert({ id,rsp });
                        }
                        else
                        {
                            auto ri = main_server->GetRewardInfoCache(room_cache->ModeIndex);
                            auto exp_earnt = no_rewards ? 0 : std::min(ri->ExpBase + (2 * rsp.total_kills * ri->ExpKill) + (2 * rsp.deaths * ri->ExpDeath) + (rsp.assists * ri->ExpAssist), ri->ExpMax);
                            auto points_earnt = no_rewards ? 0 : std::min(ri->PointBase + (rsp.total_kills * ri->PointKill) + (rsp.deaths * ri->PointDeath) + (rsp.assists * ri->PointAssist), ri->PointMax);
                            ri.unlock();
                            auto old_exp = player_acc_cache->acc_info.Experience;
                            rsp.total_mp = player_acc_cache->acc_info.MicroPoints + points_earnt;
                            rsp.total_xp = old_exp + exp_earnt;
                            end_match_infos.insert({ id, rsp });
                            ProcessLevelUp(main_server, server, player_acc_cache, id, playing_players);
                            if (!no_rewards)
                            {
                                ProcessUpdatePlayerAccCache(player_acc_cache, draw, blue_team_win, rsp.melee_kills, rsp.rifle_kills, rsp.shotgun_kills, rsp.sniper_kills, rsp.gatling_kills, rsp.bazooka_kills, rsp.grenade_kills,
                                    rsp.total_kills, rsp.deaths, rsp.headshots, rsp.assists, rsp.killstreak, earnt_battery, rsp.total_xp, rsp.total_mp, playtime_seconds, false);
                            }
                        }
                    }
                    player_acc_cache->playing = false;
                    player_acc_cache->state = room_cache->host_session_id == id ? PlayerInfo::State::HostReady : PlayerInfo::State::Waiting;
                }
                player_acc_cache.unlock();
            }
        }

        inline void BossBattle(CMainServer* main_server, CServer* server, RoomCacheResource& room_cache, std::vector<BossItem>& pve_rewards,
            std::vector<std::uint16_t>& playing_players, std::uint16_t id, std::uint64_t end_match_time)
        {
            auto send_msg = [&](CSession* session, std::uint16_t order, std::uint8_t mission, std::uint8_t extra, std::uint8_t option, std::uint8_t* data = nullptr, std::uint16_t data_size = 0)
            {
                CMessage message(session->GetEncryptionKey());
                message.SetSession(session->GetSessionId());
                message.SetCommand(order, mission, extra, option);
                if (data_size > 0 && data != nullptr) message.SetData(data, data_size);
                session->Send(message);
            };

            for (const auto& id : playing_players)
            {
                auto player_acc_cache = main_server->GetAccCacheUniqueBySessionId(id);
                if (player_acc_cache->acc_info.Index != -1)
                {
                    auto playtime_seconds = end_match_time - player_acc_cache->match_loaded_time;

                    if (auto player_session = server->GetSessionById(id))
                    {
                        auto playtime_min_seconds = main_server->GetPlaytimeMinSeconds();
                        if (playtime_seconds < playtime_min_seconds)
                            send_msg(player_session.get(), 254, 0, 6, 0); // no rewards
                        else
                        {
                            auto my_unique_id = NetEngine::Packets::Core::UniqueId(id, 1).data;
                            auto my_reward_id = get_random_boss_reward();
                            pve_rewards.push_back({ my_unique_id, my_reward_id });
                            auto my_mp = player_acc_cache->acc_info.MicroPoints;
                            auto my_exp = player_acc_cache->acc_info.Experience;
                            auto endmatchinfo_response = MainRoomEndMatchResponseBossBattle(my_mp, my_exp, my_reward_id);
                            send_msg(player_session.get(), 254, 0, 1, 0, reinterpret_cast<uint8_t*>(&endmatchinfo_response), sizeof(MainRoomEndMatchResponseBossBattle));
                            main_server->SendInventoryItem(player_session.get(), player_acc_cache, { my_reward_id }, Items::Origin::From_Game);
                        }
                    }
                    player_acc_cache->playing = false;
                    player_acc_cache->state = room_cache->host_session_id == id ? PlayerInfo::State::HostReady : PlayerInfo::State::Waiting;
                }
                player_acc_cache.unlock();
            }
        }

        inline void NewProcessPvpModes(SCallbackData& callback, CMainServer* main_server, RoomCacheResource& room_cache)
        {
            auto send_msg = [&](CSession* session, std::uint16_t order, std::uint8_t mission, std::uint8_t extra, std::uint8_t option, std::uint8_t* data = nullptr, std::uint16_t data_size = 0)
            {
                CMessage message(session->GetEncryptionKey());
                message.SetSession(session->GetSessionId());
                message.SetCommand(order, mission, extra, option);
                if (data_size > 0 && data != nullptr) message.SetData(data, data_size);
                session->Send(message);
            };
            CSession* session = callback.session;
            CServer* server = callback.server;
            auto self_session_id = session->GetSessionId();
            auto end_match_time = Utility::GetUtcTimeNowInSeconds();
            auto all_room_players = main_server->GetRoomSortedPlayerSessionIds(room_cache);
            auto playing_players = main_server->GetRoomSortedPlayerPlayingAndObserverSessionIds(room_cache);
            auto endmatch_score_header = reinterpret_cast<MainRoomEndMatchScoreClientInfo*>(callback.message->GetData());
            auto blue_team_win = endmatch_score_header->blue_score > endmatch_score_header->red_score;
            auto draw = endmatch_score_header->blue_score == endmatch_score_header->red_score;
            bool is_clan_match = room_cache->is_clan_room;
            auto clan_id_1 = room_cache->clan_id_1;
            auto clan_id_2 = room_cache->clan_id_2;
            boost::unordered_flat_set<std::uint32_t> processed_unique_ids;
            boost::unordered_flat_map<std::uint32_t, MainRoomEndMatchResponse> end_match_infos;
            boost::unordered_flat_map<std::uint32_t, MainRoomEndMatchClientInfo> client_match_infos;

            for (const auto& id : playing_players)
            {
                if (id == self_session_id) continue;
                if (auto player_session = server->GetSessionById(id))
                    send_msg(player_session.get(), callback.message->GetOrder(), callback.message->GetMission(), callback.message->GetExtra(), callback.message->GetOption(), callback.message->GetData(), callback.message->GetDataSize());
            }

            for (std::size_t i = 0; i < callback.message->GetOption(); i++)
            {
                auto endmatch_info = reinterpret_cast<MainRoomEndMatchClientInfo*>(callback.message->GetData() + sizeof(MainRoomEndMatchClientInfo) * i + sizeof(MainRoomEndMatchScoreClientInfo));
                if (processed_unique_ids.find(endmatch_info->unique_id) != processed_unique_ids.end())
                    continue;

                auto client_unique_id = NetEngine::Packets::Core::UniqueId(endmatch_info->unique_id);
                auto client_session_id = client_unique_id.session;
                client_match_infos.insert({ client_session_id, *endmatch_info });
                processed_unique_ids.insert(endmatch_info->unique_id);
            }

            BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "was clan fight: ({}) between ({}) and ({})", is_clan_match, clan_id_1, clan_id_2);
            BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "now handle mod id: ({})", static_cast<std::uint32_t>(room_cache->ModeIndex));
            for (const auto& id : playing_players)
            {
                if (client_match_infos.find(id) == client_match_infos.end())  continue;
                auto player_acc_cache = main_server->GetAccCacheUniqueBySessionId(id);
                if (player_acc_cache->acc_info.Index != -1)
                {
                    auto playtime_seconds = end_match_time - player_acc_cache->match_loaded_time;
                    if (auto player_session = server->GetSessionById(id))
                    {
                        auto& match_info = client_match_infos[id];
                        auto my_unique_id = match_info.unique_id;
                        auto unknown1 = match_info.unknown1, unknown2 = match_info.unknown2, unknown3 = match_info.unknown3, unknown4 = match_info.unknown4;
                        auto rsp = GetEndMatchResponse(match_info);
                        auto playtime_min_seconds = main_server->GetPlaytimeMinSeconds();
                        auto no_rewards = (rsp.total_kills == 0 && rsp.deaths == 0 && rsp.assists == 0) || (playtime_seconds < playtime_min_seconds) || (player_acc_cache->team_id == NetEngine::Team::IdType::Observer);
                        auto earnt_battery = player_acc_cache->earnt_battery;
                        player_acc_cache->earnt_battery = 0;
                        if (no_rewards)
                        {
                            BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "player ({}) dont get endmatch reward", player_acc_cache->acc_info.Nickname);
                            earnt_battery = 0;
                            ClearEndmMatchResponse(rsp, player_acc_cache->acc_info.MicroPoints, player_acc_cache->acc_info.Experience); end_match_infos.insert({ id,rsp });
                        }
                        else
                        {
                            BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "player id:({}) nick:({}) will possible get endmatch reward", id, player_acc_cache->acc_info.Nickname);
                            auto kills = match_info.total_kills;
                            auto deaths = match_info.deaths;
                            auto assists = match_info.assists;
                            auto melee_kills = match_info.melee_kills;
                            auto rifle_kills = match_info.rifle_kills;
                            auto shotgun_kills = match_info.shotgun_kills;
                            auto sniper_kills = match_info.sniper_kills;
                            auto gatling_kills = match_info.gatling_kills;
                            auto bazooka_kills = match_info.bazooka_kills;
                            auto grenade_kills = match_info.grenade_kills;
                            auto killstreak = match_info.killstreak;
                            auto headshots = match_info.headshots;
                            auto ri = main_server->GetRewardInfoCache(room_cache->ModeIndex);
                            std::uint32_t exp_earn = 0;
                            std::uint32_t point_earn = 0;
                            bool isClan = false;
                            switch (room_cache->ModeIndex)
                            {
                                case NetEngine::Room::Mode::Index::CLAN_Elimination:
                                case NetEngine::Room::Mode::Index::CLAN_TeamDeathMatch:
                                    isClan = true;
                                case NetEngine::Room::Mode::Index::TeamDeathMatch:
                                case NetEngine::Room::Mode::Index::FreeForAll:
                                case NetEngine::Room::Mode::Index::ItemMatch:
                                case NetEngine::Room::Mode::Index::Elimination:
                                case NetEngine::Room::Mode::Index::SuperItemMatch:
                                {
                                    std::uint32_t calc_exp = ri->ExpBase;
                                    std::uint32_t calc_point = ri->PointBase;

                                    calc_exp += (kills * ri->ExpKill) + (deaths * ri->ExpDeath) + (assists * ri->ExpAssist);
                                    calc_point += (kills * ri->PointKill) - (deaths * ri->PointDeath) + (assists * ri->PointAssist);

                                    exp_earn = std::max(ri->ExpBase, calc_exp);
                                    point_earn = std::max(ri->PointBase, calc_point);

                                    if (isClan)
                                    {
                                        BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "is clan mod");
                                    }

                                    exp_earn = std::min(ri->ExpMax, exp_earn);
                                    point_earn = std::min(ri->ExpMax, point_earn);
                                    break;
                                }
                                case NetEngine::Room::Mode::Index::CLAN_CaptureTheBattery:
                                    isClan = true;
                                case NetEngine::Room::Mode::Index::CaptureTheBattery:
                                {
                                    auto TeamBatteryCaptures = unknown2;
                                    auto MyBatteryCaptures = unknown1;

                                    std::uint32_t calc_exp = ri->ExpBase;
                                    std::uint32_t calc_point = ri->PointBase;

                                    calc_exp += (kills * ri->ExpKill) + (deaths * ri->ExpDeath) + (assists * ri->ExpAssist) + (TeamBatteryCaptures * ri->ExpMission) + (MyBatteryCaptures * ri->ExpMissionWin);
                                    calc_point += (kills * ri->PointKill) - (deaths * ri->PointDeath) + (assists * ri->PointAssist) + (TeamBatteryCaptures * ri->PointMission) + (MyBatteryCaptures * ri->PointMissionWin);

                                    exp_earn = std::max(ri->ExpBase, calc_exp);
                                    point_earn = std::max(ri->PointBase, calc_point);

                                    if (isClan)
                                    {
                                        BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "is clan mod");
                                    }

                                    exp_earn = std::min(ri->ExpMax, exp_earn);
                                    point_earn = std::min(ri->ExpMax, point_earn);
                                    break;
                                }
                                case NetEngine::Room::Mode::Index::BombBattle:
                                {
                                    auto RoundWin = unknown2;

                                    std::uint32_t calc_exp = ri->ExpBase;
                                    std::uint32_t calc_point = ri->PointBase;

                                    calc_exp += (kills * ri->ExpKill) + (deaths * ri->ExpDeath) + (assists * ri->ExpAssist) + (RoundWin * ri->ExpMissionWin);
                                    calc_point += (kills * ri->PointKill) - (deaths * ri->PointDeath) + (assists * ri->PointAssist) + (RoundWin * ri->PointMissionWin);

                                    exp_earn = std::max(ri->ExpBase, calc_exp);
                                    point_earn = std::max(ri->PointBase, calc_point);

                                    exp_earn = std::min(ri->ExpMax, exp_earn);
                                    point_earn = std::min(ri->ExpMax, point_earn);
                                    break;
                                }
                                case NetEngine::Room::Mode::Index::ZombieMode:
                                {
                                    auto ZombiKill = killstreak;
                                    auto Infected = melee_kills;
                                    auto Survived = unknown2;

                                    std::uint32_t calc_exp = ri->ExpBase;
                                    std::uint32_t calc_point = ri->PointBase;

                                    calc_exp += (ZombiKill * ri->ExpModeKill) + (Infected * ri->ExpKill) + (deaths * ri->ExpDeath) + (Survived * ri->ExpMissionWin);
                                    calc_point += (ZombiKill * ri->PointModeKill) + (Infected * ri->PointKill) - (deaths * ri->PointDeath) + (Survived * ri->PointMissionWin);

                                    exp_earn = std::max(ri->ExpBase, calc_exp);
                                    point_earn = std::max(ri->PointBase, calc_point);

                                    exp_earn = std::min(ri->ExpMax, exp_earn);
                                    point_earn = std::min(ri->ExpMax, point_earn);
                                    break;
                                }
                                case NetEngine::Room::Mode::Index::ArmsRace:
                                {
                                    auto Mission = unknown1;

                                    std::uint32_t calc_exp = ri->ExpBase;
                                    std::uint32_t calc_point = ri->PointBase;

                                    calc_exp += (kills * ri->ExpKill) + (deaths * ri->ExpDeath) + (assists * ri->ExpAssist) + (Mission * ri->ExpMission);
                                    calc_point += (kills * ri->PointKill) - (deaths * ri->PointDeath) + (assists * ri->PointAssist) + (Mission * ri->PointMission);

                                    exp_earn = std::max(ri->ExpBase, calc_exp);
                                    point_earn = std::max(ri->PointBase, calc_point);

                                    exp_earn = std::min(ri->ExpMax, exp_earn);
                                    point_earn = std::min(ri->ExpMax, point_earn);
                                    break;
                                }
                                default:
                                {
                                    BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "unknown mod id, no reward");
                                    no_rewards = true, earnt_battery = 0;
                                    ClearEndmMatchResponse(rsp, player_acc_cache->acc_info.MicroPoints, player_acc_cache->acc_info.Experience); end_match_infos.insert({ id,rsp });
                                }
                            }
                            auto pcroom_state = player_acc_cache->acc_info.PCRoom;
                            AddBonusExpPoint(main_server, player_acc_cache->inventory_items, player_acc_cache->acc_info.SelectedCharacter, exp_earn, point_earn);
                            if (pcroom_state > 1)
                            {
                                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "pcroom state with bonus exp and point enabled");
                                exp_earn += std::ceil(0.28 * exp_earn);
                                point_earn += std::floor(0.47 * point_earn);
                            }
                            BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "player final have exp: ({}), point: ({})", exp_earn, point_earn);
                            ri.unlock();
                            rsp.unknown1 = unknown1;
                            rsp.unknown2 = unknown2;
                            rsp.unknown3 = unknown3;
                            rsp.unknown4 = unknown4;
                            rsp.unique_id = my_unique_id;
                            rsp.total_mp = player_acc_cache->acc_info.MicroPoints + point_earn;
                            rsp.total_xp = player_acc_cache->acc_info.Experience + exp_earn;
                            end_match_infos.insert({ id, rsp });
                            ProcessUpdatePlayerAccCache(player_acc_cache, draw, blue_team_win, rsp.melee_kills, rsp.rifle_kills, rsp.shotgun_kills, rsp.sniper_kills, rsp.gatling_kills, rsp.bazooka_kills, rsp.grenade_kills,
                                rsp.total_kills, rsp.deaths, rsp.headshots, rsp.assists, rsp.killstreak, earnt_battery, rsp.total_xp, rsp.total_mp, playtime_seconds, is_clan_match);
                        }
                        ProcessLevelUp(main_server, server, player_acc_cache, id, all_room_players);
                    }
                    player_acc_cache->playing = false;
                    player_acc_cache->state = room_cache->host_session_id == id ? PlayerInfo::State::HostReady : PlayerInfo::State::Waiting;
                }
                player_acc_cache.unlock();
            }

            for (const auto& id : playing_players)
            {
                if (end_match_infos.find(id) == end_match_infos.end())
                {
                    BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "dont find endmatch info for player: ({})", id);
                    continue;
                }

                if (auto player_session = server->GetSessionById(id))
                {
                    auto& endmatchinfo_response = end_match_infos[id];
                    send_msg(player_session.get(), 254, 0, 1, 0, reinterpret_cast<uint8_t*>(&endmatchinfo_response), sizeof(MainRoomEndMatchResponse));
                }
            }
        }

        inline void ProcessPvpModes(SCallbackData& callback, CMainServer* main_server, RoomCacheResource& room_cache)
        {
            NewProcessPvpModes(callback, main_server, room_cache);
            return;

            auto send_msg = [&](CSession* session, std::uint16_t order, std::uint8_t mission, std::uint8_t extra, std::uint8_t option, std::uint8_t* data = nullptr, std::uint16_t data_size = 0)
            {
                CMessage message(session->GetEncryptionKey());
                message.SetSession(session->GetSessionId());
                message.SetCommand(order, mission, extra, option);
                if (data_size > 0 && data != nullptr) message.SetData(data, data_size);
                session->Send(message);
            };

            CSession* session = callback.session;
            CServer* server = callback.server;
            auto self_session_id = session->GetSessionId();
            auto end_match_time = Utility::GetUtcTimeNowInSeconds();
            auto playing_players = main_server->GetRoomSortedPlayerPlayingWithoutObserverSessionIds(room_cache);
            auto endmatch_score_header = reinterpret_cast<MainRoomEndMatchScoreClientInfo*>(callback.message->GetData());
            auto blue_team_win = endmatch_score_header->blue_score > endmatch_score_header->red_score;
            auto draw = endmatch_score_header->blue_score == endmatch_score_header->red_score;
            boost::unordered_flat_set<std::uint32_t> processed_unique_ids;
            boost::unordered_flat_map<std::uint32_t, MainRoomEndMatchResponse> end_match_infos;
            boost::unordered_flat_map<std::uint32_t, MainRoomEndMatchClientInfo> client_match_infos;

            for (const auto& id : playing_players)
            {
                if (id == self_session_id) continue;
                if (auto player_session = server->GetSessionById(id))
                    send_msg(player_session.get(), callback.message->GetOrder(), callback.message->GetMission(), callback.message->GetExtra(), callback.message->GetOption(), callback.message->GetData(), callback.message->GetDataSize());
            }

            for (std::size_t i = 0; i < callback.message->GetOption(); i++)
            {
                auto endmatch_info = reinterpret_cast<MainRoomEndMatchClientInfo*>(callback.message->GetData() + sizeof(MainRoomEndMatchClientInfo) * i + sizeof(MainRoomEndMatchScoreClientInfo));
                if (processed_unique_ids.find(endmatch_info->unique_id) != processed_unique_ids.end())
                    continue;
                
                auto client_unique_id = NetEngine::Packets::Core::UniqueId(endmatch_info->unique_id);
                auto client_session_id = client_unique_id.session;
                client_match_infos.insert({ client_session_id, *endmatch_info });
                processed_unique_ids.insert(endmatch_info->unique_id);
            }
            switch (room_cache->ModeIndex)
            {
                case NetEngine::Room::Mode::Index::TeamDeathMatch:
                {
                    TeamDeathMatch(main_server, server, room_cache, client_match_infos, end_match_infos, playing_players, draw, blue_team_win, self_session_id, end_match_time);
                    break;
                }
                case NetEngine::Room::Mode::Index::FreeForAll:
                {
                    FreeForAll(main_server, server, room_cache, client_match_infos, end_match_infos, playing_players, draw, blue_team_win, self_session_id, end_match_time);
                    break;
                }
                case NetEngine::Room::Mode::Index::ItemMatch:
                {
                    ItemMatch(main_server, server, room_cache, client_match_infos, end_match_infos, playing_players, draw, blue_team_win, self_session_id, end_match_time);
                    break;
                }
                case NetEngine::Room::Mode::Index::CaptureTheBattery:
                {
                    CaptureTheBattery(main_server, server, room_cache, client_match_infos, end_match_infos, playing_players, draw, blue_team_win, self_session_id, end_match_time);
                    break;
                }
                case NetEngine::Room::Mode::Index::Elimination:
                {
                    Elimination(main_server, server, room_cache, client_match_infos, end_match_infos, playing_players, draw, blue_team_win, self_session_id, end_match_time);
                    break;
                }
                case NetEngine::Room::Mode::Index::SuperItemMatch:
                {
                    SuperItemMatch(main_server, server, room_cache, client_match_infos, end_match_infos, playing_players, draw, blue_team_win, self_session_id, end_match_time);
                    break;
                }
                case NetEngine::Room::Mode::Index::ZombieMode:
                {
                    ZombieMode(main_server, server, room_cache, client_match_infos, end_match_infos, playing_players, draw, blue_team_win, self_session_id, end_match_time);
                    break;
                }
                case NetEngine::Room::Mode::Index::ArmsRace:
                {
                    ArmsRace(main_server, server, room_cache, client_match_infos, end_match_infos, playing_players, draw, blue_team_win, self_session_id, end_match_time);
                    break;
                }
                case NetEngine::Room::Mode::Index::BombBattle:
                {
                    BombBattle(main_server, server, room_cache, client_match_infos, end_match_infos, playing_players, draw, blue_team_win, self_session_id, end_match_time);
                    break;
                }
                case NetEngine::Room::Mode::Index::CLAN_CaptureTheBattery:
                {
                    ClanCaptureTheBattery(main_server, server, room_cache, client_match_infos, end_match_infos, playing_players, draw, blue_team_win, self_session_id, end_match_time);
                    break;
                }
                case NetEngine::Room::Mode::Index::CLAN_Elimination:
                {
                    ClanElimination(main_server, server, room_cache, client_match_infos, end_match_infos, playing_players, draw, blue_team_win, self_session_id, end_match_time);
                    break;
                }
                case NetEngine::Room::Mode::Index::CLAN_TeamDeathMatch:
                {
                    ClanTeamDeathMatch(main_server, server, room_cache, client_match_infos, end_match_infos, playing_players, draw, blue_team_win, self_session_id, end_match_time);
                    break;
                }
            }

            for (const auto& id : playing_players)
            {
                if (end_match_infos.find(id) == end_match_infos.end())
                    continue;

                if (auto player_session = server->GetSessionById(id))
                {
                    auto& endmatchinfo_response = end_match_infos[id];
                    send_msg(player_session.get(), 254, 0, 1, 0, reinterpret_cast<uint8_t*>(&endmatchinfo_response), sizeof(MainRoomEndMatchResponse));
                }
            }
        }

        inline void ProcessPveModes(SCallbackData& callback, CMainServer* main_server, RoomCacheResource& room_cache)
        {
            auto send_msg = [&](CSession* session, std::uint16_t order, std::uint8_t mission, std::uint8_t extra, std::uint8_t option, std::uint8_t* data = nullptr, std::uint16_t data_size = 0)
            {
                CMessage message(session->GetEncryptionKey());
                message.SetSession(session->GetSessionId());
                message.SetCommand(order, mission, extra, option);
                if (data_size > 0 && data != nullptr) message.SetData(data, data_size);
                session->Send(message);
            };

            CSession* session = callback.session;
            CServer* server = callback.server;
            auto self_session_id = session->GetSessionId();
            auto end_match_time = Utility::GetUtcTimeNowInSeconds();
            auto playing_players = main_server->GetRoomSortedPlayerPlayingWithoutObserverSessionIds(room_cache);

            for (const auto& id : playing_players)
            {
                if (id == self_session_id) continue;
                if (auto player_session = server->GetSessionById(id))
                    send_msg(player_session.get(), callback.message->GetOrder(), callback.message->GetMission(), callback.message->GetExtra(), callback.message->GetOption(), callback.message->GetData(), callback.message->GetDataSize());
            }

            std::vector<BossItem> pve_rewards;

            switch (room_cache->ModeIndex)
            {
                case NetEngine::Room::Mode::Index::BossBattle:
                {
                    BossBattle(main_server, server, room_cache, pve_rewards, playing_players, self_session_id, end_match_time);
                    break;
                }
            }

            for (const auto& id : playing_players)
            {
                if (auto player_session = server->GetSessionById(id))
                {
                    std::vector<BossItem> others_rewards;
                    auto my_unique_id = NetEngine::Packets::Core::UniqueId(id, 1).data;
                    std::copy_if(pve_rewards.begin(), pve_rewards.end(), std::back_inserter(others_rewards), [my_unique_id](const BossItem& item) {  return item.unique_id != my_unique_id;  });
                    auto other_rewards_ack = MainBossBattleEndMatchResultAck(others_rewards).Serialize();
                    send_msg(player_session.get(), 254, 0, 41, 0, reinterpret_cast<uint8_t*>(other_rewards_ack.data()), other_rewards_ack.size());
                }
            }
        }

        inline void EndMatch(SCallbackData& callback, CMainServer* main_server)
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
            auto my_unique_id = NetEngine::Packets::Core::UniqueId(session_id, 1).data;
            if (acc_index == -1) return;
            auto extra = callback.message->GetExtra();
            auto mission = callback.message->GetMission();
            if (extra == 6)
            {
                if (mission == 2 && extra == 6) //Storymod
                {
                    struct StoryDoneStruct {
                        std::uint8_t done_episode;
                    };
                    auto story_done = reinterpret_cast<StoryDoneStruct*>(callback.message->GetData());
                    auto story_episode = story_done->done_episode;
                    auto current_story = acc_cache->acc_info.Story;
                    BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::red, "player current story: ({}), done story episode: ({})", current_story, story_episode);
                    if (current_story == story_episode - 1)
                    {
                        acc_cache->acc_info.Story = story_episode;
                        uint32_t item_got_id = 4600100;
                        auto item_info = main_server->GetItemInfoCache(item_got_id);
                        auto serial_index = main_server->FindLowestAvailableItemSerialInfoId(acc_cache->inventory_items);
                        ShopItem new_item = { {item_got_id , item_info->Stock} , ItemExpire::Type::Unused,  ItemSerialInfo(serial_index, 1, 1, Items::Origin::From_Game, Utility::GetUtcTimeNow()) };
                        const InventoryItemInfo& inv_item_info = { {item_got_id , item_info->Stock } ,ItemExpire::Type::Unused, new_item.serial_info, item_info->Durability, 0 };
                        main_server->AddPlayerItemInventory(acc_cache, { inv_item_info,item_info->Stock, false, 0, false });
                        send_msg(session, 66, 3, 51, 0, reinterpret_cast<uint8_t*>(&new_item), sizeof(ShopItem));
                        //main_server->SendInventoryItem(session, acc_cache, { 4600100 });//make it better
                    }
                    else
                    {
                        BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::red, "check failed and player will not reward");
                    }
                    return;
                }

                auto end_single_wave_time = Utility::GetUtcTimeNowInSeconds();
                auto playtime_seconds = end_single_wave_time - acc_cache->match_loaded_time;
                auto endmatch_sw = reinterpret_cast<SingleWaveEndReq*>(callback.message->GetData());

                if (endmatch_sw->type == 1 || endmatch_sw->type == 2)
                {
                    BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::red, "single wave check player level up");
                    std::vector<std::uint16_t> empty_vec;
                    ProcessLevelUp(main_server, server, acc_cache, session_id, empty_vec);
                }

                switch (endmatch_sw->type)
                {
                    case 1: SingleWaveEasy(playtime_seconds, endmatch_sw, acc_cache);  break;
                    case 2:  SingleWaveHard(playtime_seconds, endmatch_sw, acc_cache); break;
                    default: Tutorial(main_server, session, acc_cache);   break;
                }
                return;
            }
            if (!acc_cache->in_room || !main_server->IsRoomAlready(acc_cache->room_id)) return;
            auto room_cache = main_server->GetRoomCacheUnique(acc_cache->room_id);
            acc_cache->zombie_team = 0;
            acc_cache.unlock();
            //auto players = main_server->GetRoomSortedPlayerSessionIds(room_cache);
            auto is_pve = room_cache->ModeIndex == NetEngine::Room::Mode::Index::BossBattle;

            if (is_pve)
                ProcessPveModes(callback, main_server, room_cache);
            else
                ProcessPvpModes(callback, main_server, room_cache);


            room_cache->is_playing = false;
            room_cache->kick_voters_session_ids.clear();

            auto players = main_server->GetRoomSortedPlayerSessionIds(room_cache);
            for (const auto& room_player_session_id : players)
            {
                if (auto player_session = server->GetSessionById(room_player_session_id))
                    send_msg(player_session.get(), 256, 0, 33, 0); // notify leave match
            }
        }

        /*
        inline void EndMatch(SCallbackData& callback, CMainServer* main_server)
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
            auto my_unique_id = NetEngine::Packets::Core::UniqueId(session_id, 1).data;
            if (acc_index == -1) return;
            auto extra = callback.message->GetExtra();
            auto mission = callback.message->GetMission();
            if (extra == 6)
            {
                auto end_single_wave_time = Utility::GetUtcTimeNowInSeconds();
                auto playtime_seconds = end_single_wave_time - acc_cache->match_loaded_time;
                auto endmatch_sw = reinterpret_cast<SingleWaveEndReq*>(callback.message->GetData());
                
                switch (endmatch_sw->type)
                {
                    case 1:
                    {
                        SingleWaveEasy(playtime_seconds, endmatch_sw, acc_cache);
                        break;
                    }
                    case 2:
                    {
                        SingleWaveHard(playtime_seconds, endmatch_sw, acc_cache);
                        break;
                    }
                    default:
                    {
                        Tutorial(main_server, session, acc_cache);
                        break;
                    }
                }
            }


            if (!acc_cache->in_room || !main_server->IsRoomAlready(acc_cache->room_id)) return;
            auto room_cache = main_server->GetRoomCacheUnique(acc_cache->room_id);
            auto ri = main_server->GetRewardInfoCache(room_cache->ModeIndex);
            acc_cache->playing = false;
            acc_cache->state = PlayerInfo::State::HostReady;
            acc_cache.unlock();
            auto players = main_server->GetRoomSortedPlayerSessionIds(room_cache);

            auto safe_add_micropoints = [](std::uint32_t& target, std::uint32_t value_to_add)
            {
                (0x7FFFFFFF - target < value_to_add) ? target = 0x7FFFFFFF : target += value_to_add;
            };
           
            auto safe_add_uint32 = [](std::uint32_t& target, std::uint32_t value_to_add)
            {
                (UINT32_MAX - target < value_to_add) ? target = UINT32_MAX : target += value_to_add;
            };
            auto safe_add_uint64 = [](std::uint64_t& target, std::uint64_t value_to_add)
            {
                (UINT32_MAX - target < value_to_add) ? target = UINT32_MAX : target += value_to_add;
            };
            auto safe_add_uint8 = [](std::uint32_t& target, std::uint32_t new_value)
            {
                target = std::min<std::uint8_t>(UINT8_MAX, std::max(target, new_value));
            };

            auto endmatch_score_header = reinterpret_cast<MainRoomEndMatchScoreClientInfo*>(callback.message->GetData());
            auto boss_endmatch_score_header = reinterpret_cast<MainRoomEndMatchScoreClientBossBattleInfo*>(callback.message->GetData());
            auto blue_team_win = endmatch_score_header->blue_score > endmatch_score_header->red_score;
            auto draw = endmatch_score_header->blue_score == endmatch_score_header->red_score;
            for (const auto& id : players)
            {
                if (id == session_id) continue;
                if (auto player_session = server->GetSessionById(id))
                    send_msg(player_session.get(), callback.message->GetOrder(), callback.message->GetMission(), callback.message->GetExtra(), callback.message->GetOption(), callback.message->GetData(), callback.message->GetDataSize());
            }
            boost::unordered_flat_set<std::uint32_t> processed_unique_ids;
            boost::unordered_flat_map<std::uint32_t, MainRoomEndMatchResponse> end_match_infos;
            boost::unordered_flat_map<std::uint32_t, MainRoomEndMatchClientInfo> client_match_infos;


            auto end_match_time = Utility::GetUtcTimeNowInSeconds();
            std::vector<BossItem> boss_items;
            auto is_boss_battle = room_cache->ModeIndex == NetEngine::Room::Mode::Index::BossBattle;
            for (std::size_t i = 0; i < callback.message->GetOption(); i++)
            {
                auto endmatch_info = reinterpret_cast<MainRoomEndMatchClientInfo*>(callback.message->GetData() + sizeof(MainRoomEndMatchClientInfo) * i + sizeof(MainRoomEndMatchScoreClientInfo));
                auto boss_endmatch_info = reinterpret_cast<MainRoomEndMatchClientBossBattleInfo*>(callback.message->GetData() + sizeof(MainRoomEndMatchClientBossBattleInfo) * i + sizeof(MainRoomEndMatchScoreClientBossBattleInfo));
                if (is_boss_battle)
                    if (processed_unique_ids.find(boss_endmatch_info->unique_id) != processed_unique_ids.end()) 
                        continue;
                else
                    if (processed_unique_ids.find(endmatch_info->unique_id) != processed_unique_ids.end()) 
                        continue;

                

                auto unique_id = NetEngine::Packets::Core::UniqueId(is_boss_battle ? boss_endmatch_info->unique_id : endmatch_info->unique_id);
                auto player_session_id = static_cast<std::uint16_t>(unique_id.session);
                auto player_acc_cache = main_server->GetAccCacheUniqueBySessionId(player_session_id);
                if (player_acc_cache->acc_info.Index != -1)
                {
                    auto playtime_seconds = end_match_time - player_acc_cache->match_loaded_time;

                    if (auto player_session = server->GetSessionById(player_session_id))
                    {
                        auto kills = endmatch_info->total_kills;
                        auto deaths = endmatch_info->deaths;
                        auto assists = endmatch_info->assists;
                        auto melee_kills = endmatch_info->melee_kills;
                        auto rifle_kills = endmatch_info->rifle_kills;
                        auto shotgun_kills = endmatch_info->shotgun_kills;
                        auto sniper_kills = endmatch_info->sniper_kills;
                        auto gatling_kills = endmatch_info->gatling_kills;
                        auto bazooka_kills = endmatch_info->bazooka_kills;
                        auto grenade_kills = endmatch_info->grenade_kills;
                        auto killstreak = endmatch_info->killstreak;
                        auto headshots = endmatch_info->headshots;
                        auto unknown1 = endmatch_info->unknown1;
                        auto unknown2 = endmatch_info->unknown2;
                        auto unknown3 = endmatch_info->unknown3;
                        auto unknown4 = endmatch_info->unknown4;


                        
                        auto no_rewards = (room_cache->ModeIndex == NetEngine::Room::Mode::Index::BossBattle) ? false : (kills == 0 && deaths == 0 && assists == 0);
                        auto earnt_battery = player_acc_cache->earnt_battery;
                        
                        if (playtime_seconds < 90)
                        {
                            no_rewards = true;
                            kills = 0;
                            deaths = 0;
                            assists = 0;
                            melee_kills = 0;
                            rifle_kills = 0;
                            shotgun_kills = 0;
                            sniper_kills = 0;
                            gatling_kills = 0;
                            bazooka_kills = 0;
                            grenade_kills = 0;
                            killstreak = 0;
                            headshots = 0;
                            unknown1 = 0;
                            unknown2 = 0;
                            unknown3 = 0;
                            unknown4 = 0;
                            earnt_battery = 0;
                        }
                        else
                        {
                            safe_add_uint64(player_acc_cache->acc_info.PlayTime, playtime_seconds);
                            if (player_acc_cache->acc_info.Energy + earnt_battery <= player_acc_cache->acc_info.MaximumEnergy)
                                safe_add_uint32(player_acc_cache->acc_info.Energy, earnt_battery);
                        }
                        player_acc_cache->earnt_battery = 0;

                        auto exp_earnt = 0;
                        auto points_earnt = 0;
                        switch (room_cache->ModeIndex)
                        {
                            case NetEngine::Room::Mode::Index::TeamDeathMatch:
                            {
                                exp_earnt = no_rewards ? 0 : std::min(ri->ExpBase + (2 * kills * ri->ExpKill) + (2 * deaths * ri->ExpDeath) + (assists * ri->ExpAssist), ri->ExpMax);
                                points_earnt = no_rewards ? 0 : std::min(ri->PointBase + (kills * ri->PointKill) + (deaths * ri->PointDeath) + (assists * ri->PointAssist), ri->PointMax);
                                break;
                            }
                            case NetEngine::Room::Mode::Index::FreeForAll:
                            {
                                exp_earnt = no_rewards ? 0 : std::min(ri->ExpBase + (2 * kills * ri->ExpKill) + (2 * deaths * ri->ExpDeath) + (assists * ri->ExpAssist), ri->ExpMax);
                                points_earnt = no_rewards ? 0 : std::min(ri->PointBase + (kills * ri->PointKill) + (deaths * ri->PointDeath) + (assists * ri->PointAssist), ri->PointMax);
                                break;
                            }
                            case NetEngine::Room::Mode::Index::ItemMatch:
                            {
                                exp_earnt = no_rewards ? 0 : std::min(ri->ExpBase + (2 * kills * ri->ExpKill) + (2 * deaths * ri->ExpDeath) + (assists * ri->ExpAssist), ri->ExpMax);
                                points_earnt = no_rewards ? 0 : std::min(ri->PointBase + (kills * ri->PointKill) + (deaths * ri->PointDeath) + (assists * ri->PointAssist), ri->PointMax);
                                break;
                            }
                            case NetEngine::Room::Mode::Index::CaptureTheBattery:
                            {
                                auto TeamBatteryCaptures = unknown2;
                                auto MyBatteryCaptures = unknown1;

                                exp_earnt = no_rewards ? 0 : std::min(ri->ExpBase + (kills * ri->ExpKill) + (deaths * ri->ExpDeath) + (assists * ri->ExpAssist) + (TeamBatteryCaptures * ri->ExpMission + 1) + (MyBatteryCaptures * ri->ExpMissionWin + 2), ri->ExpMax);
                                points_earnt = no_rewards ? 0 : std::min(ri->PointBase + (kills * ri->PointKill) - (deaths * ri->PointDeath) + (assists * ri->PointAssist) + (TeamBatteryCaptures * ri->PointMission + 3) + (MyBatteryCaptures * ri->PointMissionWin + 1), ri->PointMax);
                                break;
                            }
                            case NetEngine::Room::Mode::Index::Elimination:
                            {
                                auto RoundWin = unknown2;

                                exp_earnt = no_rewards ? 0 : std::min(ri->ExpBase + (kills * ri->ExpKill) + (deaths * ri->ExpDeath) + (assists * ri->ExpAssist) + (RoundWin * ri->ExpMissionWin), ri->ExpMax);
                                points_earnt = no_rewards ? 0 : std::min(ri->PointBase + (kills * ri->PointKill) - (deaths * ri->PointDeath) + (assists * ri->PointAssist) + (RoundWin * ri->PointMissionWin), ri->PointMax);
                                break;
                            }
                            case NetEngine::Room::Mode::Index::SuperItemMatch:
                            {
                                exp_earnt = no_rewards ? 0 : std::min(ri->ExpBase + (2 * kills * ri->ExpKill) + (2 * deaths * ri->ExpDeath) + (assists * ri->ExpAssist), ri->ExpMax);
                                points_earnt = no_rewards ? 0 : std::min(ri->PointBase + (kills * ri->PointKill) + (deaths * ri->PointDeath) + (assists * ri->PointAssist), ri->PointMax);
                                break;
                            }
                            case NetEngine::Room::Mode::Index::ZombieMode:
                            {
                                auto ZombiKill = killstreak;
                                auto Infected = melee_kills;
                                auto Survived = unknown2;

                                exp_earnt = no_rewards ? 0 : std::min(ri->ExpBase + (ZombiKill * ri->ExpModeKill) + (Infected * ri->ExpKill) + (deaths * ri->ExpDeath) + (Survived * ri->ExpMissionWin), ri->ExpMax);
                                points_earnt = no_rewards ? 0 : std::min(ri->PointBase + (ZombiKill * ri->PointModeKill) + (Infected * ri->PointKill) - (deaths * ri->PointDeath) + (Survived * ri->PointMissionWin), ri->PointMax);
                                break;
                            }
                            case NetEngine::Room::Mode::Index::ArmsRace:
                            {
                                auto Mission = unknown1;

                                exp_earnt = no_rewards ? 0 : std::min(ri->ExpBase + (2 * kills * ri->ExpKill) + (2 * deaths * ri->ExpDeath) + (assists * ri->ExpAssist) + (Mission * ri->ExpMission), ri->ExpMax);
                                points_earnt = no_rewards ? 0 : std::min(ri->PointBase + (kills * ri->PointKill) + (deaths * ri->PointDeath) + (assists * ri->PointAssist) + (Mission * ri->PointMission), ri->PointMax);
                                break;
                            }
                            case NetEngine::Room::Mode::Index::BombBattle:
                            {
                                auto RoundWin = unknown2;

                                exp_earnt = no_rewards ? 0 : std::min(ri->ExpBase + (kills * ri->ExpKill) + (deaths * ri->ExpDeath) + (assists * ri->ExpAssist) + (RoundWin * ri->ExpMissionWin), ri->ExpMax);
                                points_earnt = no_rewards ? 0 : std::min(ri->PointBase + (kills * ri->PointKill) - (deaths * ri->PointDeath) + (assists * ri->PointAssist) + (RoundWin * ri->PointMissionWin), ri->PointMax);
                                break;
                            }
                            case NetEngine::Room::Mode::Index::CLAN_CaptureTheBattery:
                            {
                                auto TeamBatteryCaptures = unknown2;
                                auto MyBatteryCaptures = unknown1;

                                exp_earnt = no_rewards ? 0 : std::min(ri->ExpBase + (kills * ri->ExpKill) + (deaths * ri->ExpDeath) + (assists * ri->ExpAssist) + (TeamBatteryCaptures * ri->ExpMission + 1) + (MyBatteryCaptures * ri->ExpMissionWin + 2), ri->ExpMax);
                                points_earnt = no_rewards ? 0 : std::min(ri->PointBase + (kills * ri->PointKill) - (deaths * ri->PointDeath) + (assists * ri->PointAssist) + (TeamBatteryCaptures * ri->PointMission + 3) + (MyBatteryCaptures * ri->PointMissionWin + 1), ri->PointMax);
                                break;
                            }
                            case NetEngine::Room::Mode::Index::CLAN_Elimination:
                            {
                                auto RoundWin = unknown2;

                                exp_earnt = no_rewards ? 0 : std::min(ri->ExpBase + (kills * ri->ExpKill) + (deaths * ri->ExpDeath) + (assists * ri->ExpAssist) + (RoundWin * ri->ExpMissionWin), ri->ExpMax);
                                points_earnt = no_rewards ? 0 : std::min(ri->PointBase + (kills * ri->PointKill) - (deaths * ri->PointDeath) + (assists * ri->PointAssist) + (RoundWin * ri->PointMissionWin), ri->PointMax);
                                break;
                            }
                            case NetEngine::Room::Mode::Index::CLAN_TeamDeathMatch:
                            {
                                exp_earnt = no_rewards ? 0 : std::min(ri->ExpBase + (2 * kills * ri->ExpKill) + (2 * deaths * ri->ExpDeath) + (assists * ri->ExpAssist), ri->ExpMax);
                                points_earnt = no_rewards ? 0 : std::min(ri->PointBase + (kills * ri->PointKill) + (deaths * ri->PointDeath) + (assists * ri->PointAssist), ri->PointMax);
                                break;
                            }
                            case  NetEngine::Room::Mode::Index::BossBattle:
                            {
                                exp_earnt = 0;
                                points_earnt = 0;
                                //if (!no_rewards)
                               //{
                                    auto my_unique_id = NetEngine::Packets::Core::UniqueId(player_session_id, 1).data;
                                    auto my_reward_id = get_random_boss_reward();
                                    boss_items.push_back({ my_unique_id, my_reward_id });
                                //}
                                break;
                            }
                            default:
                            {
                                no_rewards = true;
                                exp_earnt = no_rewards ? 0 : std::min(ri->ExpBase + (2 * kills * ri->ExpKill) + (2 * deaths * ri->ExpDeath) + (assists * ri->ExpAssist), ri->ExpMax);
                                points_earnt = no_rewards ? 0 : std::min(ri->PointBase + (kills * ri->PointKill) + (deaths * ri->PointDeath) + (assists * ri->PointAssist), ri->PointMax);
                                break;
                            }
                        }
                        

                        MainRoomEndMatchResponse endmatch_response;
                        endmatch_response.melee_kills = melee_kills;
                        endmatch_response.rifle_kills = rifle_kills;
                        endmatch_response.shotgun_kills = shotgun_kills;
                        endmatch_response.sniper_kills = sniper_kills;
                        endmatch_response.gatling_kills = gatling_kills;
                        endmatch_response.bazooka_kills = bazooka_kills;
                        endmatch_response.grenade_kills = grenade_kills;
                        endmatch_response.killstreak = killstreak;
                        endmatch_response.total_kills = kills;
                        endmatch_response.deaths = deaths;
                        endmatch_response.headshots = headshots;
                        endmatch_response.assists = assists;
                        endmatch_response.unknown = 0;
                        endmatch_response.unknown2 = 0;
                        endmatch_response.total_mp = player_acc_cache->acc_info.MicroPoints + points_earnt;
                        player_acc_cache->acc_info.MicroPoints = endmatch_response.total_mp;
                        auto old_exp = player_acc_cache->acc_info.Experience;
                        endmatch_response.total_xp = old_exp + exp_earnt;
                        BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "player ({}) received endmatch_info unknown1: ({}), unknown2: ({}), unknown3: ({}), unknown4: ({}), exp_earnt: ({}), points_earnt: ({}), no_rewards: ({}), kills: ({}), deaths: ({}), assists: ({})", player_acc_cache->acc_info.Nickname.c_str(), unknown1, unknown2, unknown3, unknown4, exp_earnt, points_earnt, no_rewards ? "true" : "false", kills, deaths, assists);

                        auto old_level = player_acc_cache->acc_info.Level;
                        auto gi = main_server->GetGradeInfoCache(old_level + 2);
                        if (gi->Grade)
                        {
                            if (player_acc_cache->acc_info.Experience >= gi->Exp)
                            {
                                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "player ({}) level up: ({}) -> ({}), required xp: ({}), current xp: ({})", player_acc_cache->acc_info.Nickname.c_str(), old_level, old_level + 1, gi->Exp, player_acc_cache->acc_info.Experience);
                                player_acc_cache->acc_info.Level = old_level + 1;
                                for (const auto& id : players)
                                {
                                    if (id == player_session_id) continue;
                                    if (auto other_session = server->GetSessionById(id))
                                        send_msg(other_session.get(), 311, 0, 0, static_cast<std::uint8_t>(player_acc_cache->acc_info.Level + 1), reinterpret_cast<uint8_t*>(&unique_id), sizeof(unique_id)); // broadcast to others that player leveled up
                                }
                            }
                            else
                                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "player ({}) couldn't level up: ({}) -> ({}), required xp: ({}), current xp: ({})", player_acc_cache->acc_info.Nickname.c_str(), old_level, old_level + 1, gi->Exp, player_acc_cache->acc_info.Experience);
                        }
                        if (!no_rewards && room_cache->ModeIndex != NetEngine::Room::Mode::Index::BossBattle)
                        {
                            safe_add_uint32(player_acc_cache->acc_info.MeleeKills, melee_kills);
                            safe_add_uint32(player_acc_cache->acc_info.RifleKills, rifle_kills);
                            safe_add_uint32(player_acc_cache->acc_info.ShotgunKills, shotgun_kills);
                            safe_add_uint32(player_acc_cache->acc_info.SniperKills, sniper_kills);
                            safe_add_uint32(player_acc_cache->acc_info.GatlingKills, gatling_kills);
                            safe_add_uint32(player_acc_cache->acc_info.BazookaKills, bazooka_kills);
                            safe_add_uint32(player_acc_cache->acc_info.GrenadeKills, grenade_kills);
                            safe_add_uint32(player_acc_cache->acc_info.Kills, kills);
                            safe_add_uint32(player_acc_cache->acc_info.Deaths, deaths);
                            safe_add_uint32(player_acc_cache->acc_info.Headshots, headshots);
                            safe_add_uint32(player_acc_cache->acc_info.Assists, assists);
                            safe_add_uint8(player_acc_cache->acc_info.HighestKillStreak, killstreak);
                            player_acc_cache->acc_info.Experience = endmatch_response.total_xp;

                            if (draw)
                                safe_add_uint32(player_acc_cache->acc_info.Draws, 1);
                            else if (blue_team_win)
                            {
                                if (player_acc_cache->team_id == Team::IdType::Blue)
                                    safe_add_uint32(player_acc_cache->acc_info.Wins, 1);
                                else
                                    safe_add_uint32(player_acc_cache->acc_info.Loses, 1);
                            }
                            else
                            {
                                if (player_acc_cache->team_id == Team::IdType::Blue)
                                    safe_add_uint32(player_acc_cache->acc_info.Loses, 1);
                                else
                                    safe_add_uint32(player_acc_cache->acc_info.Wins, 1);
                            }
                        }
                        player_acc_cache->playing = false;
                        player_acc_cache->state = PlayerInfo::State::Waiting;
                        //send_msg(player_session.get(), 254, 0, 1, 0, reinterpret_cast<uint8_t*>(&endmatch_response), sizeof(MainRoomEndMatchResponse));
                        end_match_infos.insert({ player_session_id, endmatch_response });
                        processed_unique_ids.insert(is_boss_battle ? boss_endmatch_info->unique_id : endmatch_info->unique_id);
                    }
                }
                player_acc_cache.unlock();
            }
            if (room_cache->ModeIndex == NetEngine::Room::Mode::Index::BossBattle)
            {
                auto boss_endmatch_ack = MainBossBattleEndMatchResultAck(boss_items).Serialize();
                if (boss_items.empty())
                {
                    for (const auto& [player_session_id, endmatch_response] : end_match_infos)
                    {
                        auto endmatchinfo_response = endmatch_response;
                        if (auto player_session = server->GetSessionById(player_session_id))
                            send_msg(player_session.get(), 254, 0, 6, 0);
                    }
                }
                else
                {
                    acc_cache.lock();
                    for (const auto& boss_reward : boss_items)
                    {
                        auto player_unique_id = NetEngine::Packets::Core::UniqueId(boss_reward.unique_id);
                        auto player_session_id = static_cast<std::uint16_t>(player_unique_id.session);
                        auto player_won_item = boss_reward.item_id;

                        

                        if (auto player_session = server->GetSessionById(player_session_id))
                        {
                            send_msg(player_session.get(), 254, 0, 41, 0, reinterpret_cast<uint8_t*>(boss_endmatch_ack.data()), boss_endmatch_ack.size());

                            main_server->SendInventoryItem(session, acc_cache, { player_won_item }, Items::Origin::From_Game);

                            BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "player ({}) won item: ({}) from boss battle", acc_cache->acc_info.Nickname.c_str(), player_won_item);
                        }
                            
                    }
                }
                
            }
            else
            {
                for (const auto& [player_session_id, endmatch_response] : end_match_infos)
                {
                    auto endmatchinfo_response = endmatch_response;
                    if (auto player_session = server->GetSessionById(player_session_id))
                        send_msg(player_session.get(), 254, 0, 1, 0, reinterpret_cast<uint8_t*>(&endmatchinfo_response), sizeof(MainRoomEndMatchResponse));
                }
            }

            

            room_cache->is_playing = false;
           
            send_msg(session, 256, 0, 33, 0); // notify leave match
        }

        */
    }
}