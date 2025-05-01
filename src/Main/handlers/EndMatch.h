#pragma once
namespace Game
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Main;

    namespace Handlers
    {
        boost::unordered_flat_map<uint32_t, std::pair<uint32_t, uint32_t>> boss_rewards =
        {
            {0, {4801002, 50}}, // bronze 60%
            {1, {4801001, 35}}, // silver 25%
            {2, {4801000, 10}}, // gold 10%
            {3, {4801003, 5}}   // diamond 5%
        };
        uint32_t get_random_boss_reward() 
        {

            uint32_t total_weight = 0;
            for (const auto& [key, reward] : boss_rewards) 
                total_weight += reward.second;

            auto random_value = Utility::Random::CustomGen(1, total_weight);

            uint32_t cumulative_weight = 0;
            for (const auto& [key, reward] : boss_rewards)
            {
                cumulative_weight += reward.second;
                if (random_value <= cumulative_weight)
                    return reward.first;
            }
            return boss_rewards.begin()->second.first;
        }
        inline void SingleWaveEasy(uint32_t playtime_seconds, SingleWaveEndReq* endmatch_sw, AccCacheResource& acc_cache)
        {
            if (playtime_seconds >= 180) // 3 minutes
            {
                acc_cache->acc_info.SingleWaveHighScore = std::max(acc_cache->acc_info.SingleWaveHighScore, endmatch_sw->score);
                acc_cache->acc_info.SingleWaveHighestWave = std::max(acc_cache->acc_info.SingleWaveHighestWave, endmatch_sw->stage);
                acc_cache->acc_info.SingleWaveLastUpdate = Utility::GetUtcTimeNow64();
            }
            
        }
        inline void SingleWaveHard(uint32_t playtime_seconds, SingleWaveEndReq* endmatch_sw, AccCacheResource& acc_cache)
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
        inline void ProcessLevelUp(CMainServer* main_server, CServer* server, AccCacheResource& player_acc_cache, uint16_t my_id, const std::vector<uint16_t>& playing_players)
        {
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
                            other_session->SendMsg(311, 0, 0, static_cast<uint8_t>(player_acc_cache->acc_info.Level + 1), reinterpret_cast<uint8_t*>(&my_unique_id), sizeof(my_unique_id));
                    }
                }
            }
        }
        inline void ProcessUpdatePlayerAccCache( AccCacheResource& player_acc_cache, bool draw, bool blue_team_win, uint32_t melee_kills, uint32_t rifle_kills, uint32_t shotgun_kills, 
            uint32_t sniper_kills, uint32_t gatling_kills, uint32_t bazooka_kills, uint32_t grenade_kills, uint32_t kills, uint32_t deaths, uint32_t headshots, 
            uint32_t assists, uint32_t killstreak, uint32_t earnt_battery, uint32_t total_xp ,uint32_t total_mp, uint64_t playtime_seconds, bool is_clan_match)
        {

            auto safe_add_uint32 = [](uint32_t& target, uint32_t value_to_add)
            {
                (UINT32_MAX - target < value_to_add) ? target = UINT32_MAX : target += value_to_add;
            };
            auto safe_add_uint64 = [](uint64_t& target, uint64_t value_to_add)
            {
                (UINT32_MAX - target < value_to_add) ? target = UINT32_MAX : target += value_to_add;
            };
            auto safe_add_uint8 = [](uint32_t& target, uint32_t new_value)
            {
                target = std::min<uint8_t>(UINT8_MAX, std::max(target, new_value));
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
                safe_add_uint64(player_acc_cache->acc_info.ClanContribution, (static_cast<uint64_t>(kills) * 6 + assists));//TODO: own contribution math
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
        inline void ClearEndmMatchResponse(MainRoomEndMatchResponse& resp, uint32_t tmp, uint32_t txp)
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
        inline void AddBonusExpPoint(CMainServer *server, std::vector<BaseLib::Item> items, uint32_t selected_character, uint32_t &exp, uint32_t &point) {
            float extra_procent_exp = 0, extra_procent_point = 0;
            for (auto& item : items) {
                if (item.is_equipped == 1 && item.character_id == static_cast<uint8_t>(selected_character)) {
                    //BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "will check item id ({})", (uint32_t)item.item_info.item_number.item_id);
                    auto item_info = server->GetItemInfoCache(item.item_info.item_number.item_id);
                    auto bonus_ef_id = item_info->BonusEffectId;
                    //BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "found bonus effect id ({})", (uint32_t)bonus_ef_id);
                    if (bonus_ef_id) {
                        auto bonus_ef_info = server->GetEffectInfoCache(bonus_ef_id);
                        BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "found effect info key({}) value({}) for itemid({})", (uint32_t)bonus_ef_info->key, (uint32_t)bonus_ef_info->valueA, (uint32_t)item.item_info.item_number.item_id);
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
            exp += (uint32_t)(extra_procent_exp * exp);
            point += (uint32_t)(extra_procent_point * point);
            BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "player now will get exp: ({}), point: ({})", exp, point);
        }

        inline void EndMatch(SCallbackData& callback, CMainServer* main_server)
        {
            auto session = callback.session;
            auto message = callback.message;
            if (!session || !message) return;

            std::shared_lock lock(session->GetMutex());
            CServer* server = callback.server;
            auto session_id = session->GetSessionId();
            auto acc_cache = main_server->GetAccCacheUniqueBySessionId(session_id);
            auto acc_index = acc_cache->acc_info.Index;
            auto my_unique_id = NetEngine::Packets::Core::UniqueId(session_id, 1).data;
            if (acc_index == -1) return;
            auto extra = message->GetExtra();
            auto mission = message->GetMission();
            if (extra == 6)
            {
                if (mission == 2 && extra == 6) //Storymod
                {
                    struct StoryDoneStruct {
                        uint8_t done_episode;
                    };
                    auto story_done = reinterpret_cast<StoryDoneStruct*>(message->GetData());
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
                        session->SendMsg(66, 3, 51, 0, reinterpret_cast<uint8_t*>(&new_item), sizeof(ShopItem));
                    }
                    else
                    {
                        BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::red, "check failed and player will not reward");
                    }
                    return;
                }

                auto end_single_wave_time = Utility::GetUtcTimeNowInSeconds();
                auto playtime_seconds = end_single_wave_time - acc_cache->match_loaded_time;
                auto endmatch_sw = reinterpret_cast<SingleWaveEndReq*>(message->GetData());

                if (endmatch_sw->type == 1 || endmatch_sw->type == 2)
                {
                    BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::red, "single wave check player level up");
                    std::vector<uint16_t> empty_vec;
                    ProcessLevelUp(main_server, server, acc_cache, session_id, empty_vec);
                }

                switch (endmatch_sw->type)
                {
                    case 1: SingleWaveEasy(playtime_seconds, endmatch_sw, acc_cache);  break;
                    case 2:  SingleWaveHard(playtime_seconds, endmatch_sw, acc_cache); break;
                    default: Tutorial(main_server, session.get(), acc_cache);   break;
                }
                return;
            }
            if (!acc_cache->in_room || !main_server->IsRoomAlready(acc_cache->room_id)) return;
            auto room_cache = main_server->GetRoomCacheUnique(acc_cache->room_id);
            acc_cache->zombie_team = 0;
            acc_cache.unlock();
            //auto players = main_server->GetRoomSortedPlayerSessionIds(room_cache);
            auto is_pve = room_cache->ModeIndex == NetEngine::Room::Mode::Index::BossBattle;

            std::vector<uint32_t> playerIds;
            std::vector<BossItem> pve_rewards;

            if (is_pve)
            {
                auto self_session_id = session->GetSessionId();
                auto end_match_time = Utility::GetUtcTimeNowInSeconds();
                auto playing_players = main_server->GetRoomSortedPlayerPlayingWithoutObserverSessionIds(room_cache);

                for (const auto& id : playing_players)
                {
                    if (id == self_session_id) continue;
                    if (auto player_session = server->GetSessionById(id))
                        player_session->SendMsg(message->GetOrder(), message->GetMission(), message->GetExtra(), message->GetOption(), message->GetData(), message->GetDataSize());
                }

                switch (room_cache->ModeIndex)
                {
                    case NetEngine::Room::Mode::Index::BossBattle:
                    {

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
                                        player_session->SendMsg(254, 0, 6, 0); // no rewards
                                    else
                                    {
                                        auto my_unique_id = NetEngine::Packets::Core::UniqueId(id, 1).data;
                                        auto my_reward_id = get_random_boss_reward();
                                        pve_rewards.push_back({ my_unique_id, my_reward_id });
                                        auto my_mp = player_acc_cache->acc_info.MicroPoints;
                                        auto my_exp = player_acc_cache->acc_info.Experience;
                                        auto endmatchinfo_response = MainRoomEndMatchResponseBossBattle(my_mp, my_exp, my_reward_id);
                                        player_session->SendMsg(254, 0, 1, 0, reinterpret_cast<uint8_t*>(&endmatchinfo_response), sizeof(MainRoomEndMatchResponseBossBattle));
                                        main_server->SendInventoryItem(player_session.get(), player_acc_cache, { my_reward_id }, Items::Origin::From_Game);
                                    }
                                }
                                player_acc_cache->playing = false;
                                player_acc_cache->state = room_cache->host_session_id == id ? PlayerInfo::State::HostReady : PlayerInfo::State::Waiting;
                            }
                            player_acc_cache.unlock();
                        }
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
                        player_session->SendMsg(254, 0, 41, 0, reinterpret_cast<uint8_t*>(other_rewards_ack.data()), other_rewards_ack.size());
                        playerIds.push_back(id);
                    }
                }
            }
            else
            {
                auto self_session_id = session->GetSessionId();
                auto end_match_time = Utility::GetUtcTimeNowInSeconds();
                auto all_room_players = main_server->GetRoomSortedPlayerSessionIds(room_cache);
                auto playing_players = main_server->GetRoomSortedPlayerPlayingAndObserverSessionIds(room_cache);
                auto endmatch_score_header = reinterpret_cast<MainRoomEndMatchScoreClientInfo*>(message->GetData());
                auto blue_team_win = endmatch_score_header->blue_score > endmatch_score_header->red_score;
                auto draw = endmatch_score_header->blue_score == endmatch_score_header->red_score;
                bool is_clan_match = room_cache->is_clan_room;
                auto clan_id_1 = room_cache->clan_id_1;
                auto clan_id_2 = room_cache->clan_id_2;
                boost::unordered_flat_set<uint32_t> processed_unique_ids;
                boost::unordered_flat_map<uint32_t, MainRoomEndMatchResponse> end_match_infos;
                boost::unordered_flat_map<uint32_t, MainRoomEndMatchClientInfo> client_match_infos;

                for (const auto& id : playing_players)
                {
                    if (id == self_session_id) continue;
                    if (auto player_session = server->GetSessionById(id))
                        player_session->SendMsg(message->GetOrder(), message->GetMission(), message->GetExtra(), message->GetOption(), message->GetData(), message->GetDataSize());
                }

                for (size_t i = 0; i < message->GetOption(); i++)
                {
                    auto endmatch_info = reinterpret_cast<MainRoomEndMatchClientInfo*>(message->GetData() + sizeof(MainRoomEndMatchClientInfo) * i + sizeof(MainRoomEndMatchScoreClientInfo));
                    if (processed_unique_ids.find(endmatch_info->unique_id) != processed_unique_ids.end())
                        continue;

                    auto client_unique_id = NetEngine::Packets::Core::UniqueId(endmatch_info->unique_id);
                    auto client_session_id = client_unique_id.session;
                    client_match_infos.insert({ client_session_id, *endmatch_info });
                    playerIds.push_back(client_session_id);
                    processed_unique_ids.insert(endmatch_info->unique_id);
                }

                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "was clan fight: ({}) between ({}) and ({})", is_clan_match, clan_id_1, clan_id_2);
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "now handle mod id: ({})", static_cast<uint32_t>(room_cache->ModeIndex));
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
                                uint32_t exp_earn = 0;
                                uint32_t point_earn = 0;
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
                                        uint32_t calc_exp = ri->ExpBase;
                                        uint32_t calc_point = ri->PointBase;

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

                                        uint32_t calc_exp = ri->ExpBase;
                                        uint32_t calc_point = ri->PointBase;

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

                                        uint32_t calc_exp = ri->ExpBase;
                                        uint32_t calc_point = ri->PointBase;

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

                                        uint32_t calc_exp = ri->ExpBase;
                                        uint32_t calc_point = ri->PointBase;

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

                                        uint32_t calc_exp = ri->ExpBase;
                                        uint32_t calc_point = ri->PointBase;

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
                        player_session->SendMsg(254, 0, 1, 0, reinterpret_cast<uint8_t*>(&endmatchinfo_response), sizeof(MainRoomEndMatchResponse));
                    }
                }
            }


            room_cache->is_playing = false;
            room_cache->kick_voters_session_ids.clear();

            auto players = main_server->GetRoomSortedPlayerSessionIds(room_cache);
            for (const auto& room_player_session_id : players)
            {
                if (auto player_session = server->GetSessionById(room_player_session_id))
                    player_session->SendMsg(256, 0, 33, 0); // notify leave match
            }

            std::vector<EndMatchUpdateDatabaseInfo> playerUpdates;
            playerUpdates.resize(playerIds.size());
            for (auto& id : playerIds) 
            {
                auto player_acc_cache = main_server->GetAccCacheSharedBySessionId(id);
                EndMatchUpdateDatabaseInfo new_endmatch_info = {
                    player_acc_cache->acc_info.Index,
                    player_acc_cache->acc_info.ClanKills,
                    player_acc_cache->acc_info.ClanDeaths,
                    player_acc_cache->acc_info.ClanAssists,
                    player_acc_cache->acc_info.ClanContribution,
                    player_acc_cache->acc_info.ClanWins,
                    player_acc_cache->acc_info.ClanLoses,
                    player_acc_cache->acc_info.ClanDraws,
                    player_acc_cache->acc_info.Level,
                    player_acc_cache->acc_info.Experience,
                    player_acc_cache->acc_info.PlayTime,
                    player_acc_cache->acc_info.SelectedCharacter,
                    player_acc_cache->acc_info.Energy,
                    player_acc_cache->acc_info.MicroPoints,
                    player_acc_cache->acc_info.Wins,
                    player_acc_cache->acc_info.Loses,
                    player_acc_cache->acc_info.Draws,
                    player_acc_cache->acc_info.Kills,
                    player_acc_cache->acc_info.Deaths,
                    player_acc_cache->acc_info.Assists,
                    player_acc_cache->acc_info.Headshots,
                    player_acc_cache->acc_info.HighestKillStreak,
                    player_acc_cache->acc_info.MeleeKills,
                    player_acc_cache->acc_info.RifleKills,
                    player_acc_cache->acc_info.ShotgunKills,
                    player_acc_cache->acc_info.SniperKills,
                    player_acc_cache->acc_info.GatlingKills,
                    player_acc_cache->acc_info.BazookaKills,
                    player_acc_cache->acc_info.GrenadeKills,
                    player_acc_cache->acc_info.ZombieKills,
                    player_acc_cache->acc_info.Infections
                };
                player_acc_cache.unlock();
                playerUpdates.push_back(new_endmatch_info);
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::red, "will update match info for player id ({})", id);
            }
            BaseLib::DbPool->submit_task([=]() mutable
            {
                if (!BaseLib::Database->UpdateEndMatchInfo(playerUpdates))
                {
                    BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan,
                        "failed to update player end match info");
                    return;
                }
            });
        }
    }
}