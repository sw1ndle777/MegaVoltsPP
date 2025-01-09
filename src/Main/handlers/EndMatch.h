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

        inline void EndMatch(SCallbackData& callback, CMainServer* main_server)
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
            auto my_unique_id = NetEngine::Packets::Core::UniqueId(session_id, 1).data;
            if (acc_index == -1 || !acc_cache->in_room || !main_server->IsRoomAlready(acc_cache->room_id)) return;
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
                                if (!no_rewards)
                                {
                                    auto my_unique_id = NetEngine::Packets::Core::UniqueId(player_session_id, 1).data;
                                    auto my_reward_id = get_random_boss_reward();
                                    boss_items.push_back({ my_unique_id, my_reward_id });
                                }
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

                        auto serial_index = main_server->FindLowestAvailableItemSerialInfoId(acc_cache->inventory_items);
                        auto item_info = main_server->GetItemInfoCache(player_won_item);
                        ShopItem new_item = { {player_won_item ,item_info->Stock } , ItemExpire::Type::Unused ,ItemSerialInfo(serial_index, 1, 1, Items::Origin::From_Game, Utility::GetUtcTimeNow()) };
                        
                        InventoryItemInfo inv_item_info = { {player_won_item , item_info->Stock} ,ItemExpire::Type::Unused, new_item.serial_info, item_info->Durability, 0 };
                        main_server->AddPlayerItemInventory(acc_cache, { inv_item_info, item_info->Stock, false, 0, false });

                        if (auto player_session = server->GetSessionById(player_session_id))
                            send_msg(player_session.get(), 254, 0, 41, 0, reinterpret_cast<uint8_t*>(boss_endmatch_ack.data()), boss_endmatch_ack.size());
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
    }
}