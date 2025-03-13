#pragma once
namespace Game
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Main;

    namespace Handlers
    {
        bool checkAchievement(std::uint64_t& tier, int achievementId) 
        {
            assert(achievementId >= 0 && achievementId < 64 && "Achievement ID must be between 0 and 63.");
            return (tier & (1ULL << achievementId)) != 0;
        }

        void doAchievement(std::uint64_t& tier, int achievementId) 
        {
            assert(achievementId >= 0 && achievementId < 64 && "Achievement ID must be between 0 and 63.");
            tier |= (1ULL << achievementId);
        }
        inline void PlayerCompleteAchievement(SCallbackData& callback, CMainServer* main_server)
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
            auto session_id = session->GetSessionId();
            auto acc_cache = main_server->GetAccCacheUniqueBySessionId(session_id);

            auto acc_index = acc_cache->acc_info.Index;
            if (acc_index == -1) return;

            auto achievement_done = acc_cache->acc_info.Achievement;
            auto desired_achiv = callback.message->GetOption();
            BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "player want to complete achievement: ({})", desired_achiv);
            if (desired_achiv > 0 && desired_achiv < 64)
            {
                auto current_coll = main_server->GetCollectionInfoCache(desired_achiv);
                if (current_coll->missionType == 3 && !checkAchievement(achievement_done, desired_achiv))
                {
                    BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "all checks passed");
                    doAchievement(achievement_done, desired_achiv);
                    acc_cache->acc_info.Achievement = achievement_done;
                    if (current_coll->rewardPoint > 0)
                    {
                        acc_cache->acc_info.MicroPoints += current_coll->rewardPoint;
                    }
                }
            }
        }
        inline void PlayerCompleteGuideMission(SCallbackData& callback, CMainServer* main_server)
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
            auto session_id = session->GetSessionId();
            auto acc_cache = main_server->GetAccCacheUniqueBySessionId(session_id);
            
            auto acc_index = acc_cache->acc_info.Index;
            if (acc_index == -1) return;

            if (callback.message->GetExtra() == 2) // Do a goal of Daily mission
            {
                struct daily_mission_req {
                    std::uint32_t id;
                    std::uint32_t idk1;
                    std::uint32_t idk2;
                    std::uint32_t idk3;
                };
                auto mission_data = reinterpret_cast<daily_mission_req*>(callback.message->GetData());
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "daily mission request: ({}) ({}) ({}) ({})", mission_data->id, mission_data->idk1, mission_data->idk2, mission_data->idk3);
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "player did goal of daily mission: ({})", mission_data->id);
                std::uint32_t* current_goal = nullptr;
                if (acc_cache->daily_mission_info.mission1 == mission_data->id)
                {
                    current_goal = &(acc_cache->daily_mission_info.goal_mission1);
                }
                else if (acc_cache->daily_mission_info.mission2 == mission_data->id)
                {
                    current_goal = &(acc_cache->daily_mission_info.goal_mission2);
                }
                else if (acc_cache->daily_mission_info.mission3 == mission_data->id)
                {
                    current_goal = &(acc_cache->daily_mission_info.goal_mission3);
                }
                else
                {
                    BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "mission is NOT correct for player");
                    return;
                }
                auto daily_mission_info = main_server->GetDailyMissionInfoCache(mission_data->id);
                if (*current_goal < daily_mission_info->goal)
                {
                    BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "will increase done goal count - current: ({}) max: ({})", *current_goal, daily_mission_info->goal);
                    *current_goal = *current_goal + 1;
                    if (*current_goal == daily_mission_info->goal)
                    {
                        BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "player finished daily mission");
                        std::vector<MainCompleteMissionReq> missions;
                        missions.push_back(MainCompleteMissionReq{ acc_cache->daily_mission_info.mission1, 0, acc_cache->daily_mission_info.goal_mission1, 4 });
                        missions.push_back(MainCompleteMissionReq{ acc_cache->daily_mission_info.mission2, 0, acc_cache->daily_mission_info.goal_mission2, 4 });
                        missions.push_back(MainCompleteMissionReq{ acc_cache->daily_mission_info.mission3, 0, acc_cache->daily_mission_info.goal_mission3, 4 });
                        send_msg(session, 168, 0, 1, missions.size(), reinterpret_cast<uint8_t*>(missions.data()), missions.size() * sizeof(MainCompleteMissionReq));

                        if (daily_mission_info->rewardPoint)
                        {
                            acc_cache->acc_info.MicroPoints += daily_mission_info->rewardPoint;
                        }
                        if (daily_mission_info->rewardItem)
                        {
                            BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "player get reward item: ({})", daily_mission_info->rewardItem);
                            main_server->SendInventoryItem(callback.session, acc_cache, { daily_mission_info->rewardItem });
                        }
                        send_msg(session, 168, 0, 2, 0, reinterpret_cast<uint8_t*>(&mission_data->id), sizeof(mission_data->id));
                    }
                }
                return;
            }

            if (callback.message->GetExtra() == 3) // Reset a Daily mission
            {
                struct daily_mission_reset_req {
                    std::uint32_t id;
                };
                auto mission_data = reinterpret_cast<daily_mission_reset_req*>(callback.message->GetData());
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "player want to reset daily mission: ({})", mission_data->id);
                std::uint32_t* current_goal = nullptr;
                std::uint32_t* current_mission = nullptr;
                if (acc_cache->daily_mission_info.mission1 == mission_data->id)
                {
                    current_mission = &(acc_cache->daily_mission_info.mission1);
                    current_goal = &(acc_cache->daily_mission_info.goal_mission1);
                }
                else if (acc_cache->daily_mission_info.mission2 == mission_data->id)
                {
                    current_mission = &(acc_cache->daily_mission_info.mission2);
                    current_goal = &(acc_cache->daily_mission_info.goal_mission2);
                }
                else if (acc_cache->daily_mission_info.mission3 == mission_data->id)
                {
                    current_mission = &(acc_cache->daily_mission_info.mission3);
                    current_goal = &(acc_cache->daily_mission_info.goal_mission3);
                }
                else
                {
                    BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "mission is NOT correct for player");
                    return;
                }
                auto daily_mission_info = main_server->GetDailyMissionInfoCache(mission_data->id);
                if (*current_goal < daily_mission_info->goal)
                {
                    if (acc_cache->acc_info.MicroPoints < 10000) //Value to check is in constantinfo!
                    {
                        send_msg(session, 168, 0, 3, 0);
                        return;
                    }
                    BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "checks passed and will reset mission");
                    acc_cache->acc_info.MicroPoints -= 10000;
                    auto new_ids = main_server->GetRandomDailyMissionIds(1, acc_cache->daily_mission_info.mission1, acc_cache->daily_mission_info.mission2, acc_cache->daily_mission_info.mission3);
                    *current_mission = new_ids[0];
                    *current_goal = 0;
                    std::vector<MainCompleteMissionReq> missions;
                    missions.push_back(MainCompleteMissionReq{ *current_mission, 0, *current_goal, 4 });
                    //missions.push_back(MainCompleteMissionReq{ acc_cache->daily_mission_info.mission2, 0, acc_cache->daily_mission_info.goal_mission2, 4 });
                    //missions.push_back(MainCompleteMissionReq{ acc_cache->daily_mission_info.mission3, 0, acc_cache->daily_mission_info.goal_mission3, 4 });
                    //MainCurrencyUpdateAck currency_update_data = { acc_cache->acc_info.RockTokens, acc_cache->acc_info.MicroPoints, acc_cache->acc_info.Coins };
                    //send_msg(session, 307, 0x0, 0, 0, reinterpret_cast<uint8_t*>(&currency_update_data), sizeof(currency_update_data)); // currency update ack
                    send_msg(session, 168, 0, 3, missions.size(), reinterpret_cast<uint8_t*>(missions.data()), missions.size() * sizeof(MainCompleteMissionReq));
                }
                return;
            }

            auto mission_data = reinterpret_cast<MainCompleteMissionReq*>(callback.message->GetData());
            if (mission_data->mission_type == 1 && mission_data->set_index == 9) //Guide mission
            {
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "player did guide mission: ({})", mission_data->collection_id);
                if (mission_data->collection_id > 45 && mission_data->collection_id < 58 && mission_data->collection_id >= acc_cache->acc_info.GuideMission) {
                    auto current_coll = main_server->GetCollectionInfoCache(mission_data->collection_id);
                    acc_cache->acc_info.GuideMission = (mission_data->collection_id - 45);
                    if (current_coll->rewardExp > 0)
                    {
                        acc_cache->acc_info.Experience += current_coll->rewardExp;
                    }
                    if (current_coll->rewardPoint > 0)
                    {
                        acc_cache->acc_info.MicroPoints += current_coll->rewardPoint;
                    }
                    send_msg(session, 168, 0, 2, 0, reinterpret_cast<uint8_t*>(&mission_data->collection_id), sizeof(mission_data->collection_id));
                    std::vector<std::uint16_t> playing_players;
                    if (acc_cache->in_room)
                    {
                        auto room_cache = main_server->GetRoomCacheShared(acc_cache->room_id);
                        playing_players = main_server->GetRoomSortedPlayerSessionIds(room_cache);
                        room_cache.unlock();
                    }
                    else if (acc_cache->in_party)
                    {
                        auto party_cache = main_server->GetPartyCacheShared(acc_cache->party_id);
                        playing_players = party_cache->members;
                        party_cache.unlock();
                    }
                    ProcessLevelUp(main_server, callback.server, acc_cache, session_id, playing_players);
                }
            }

        }

        inline void PlayerMissions(SCallbackData& callback, CMainServer* main_server)
        {
            const auto& order = callback.message->GetOrder();
            switch (order)
            {
                case 168: PlayerCompleteGuideMission(callback, main_server); break;
            }
        }
    }
    
}