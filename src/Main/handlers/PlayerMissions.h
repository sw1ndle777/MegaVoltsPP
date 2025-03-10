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
            auto mission_data = reinterpret_cast<MainCompleteMissionReq*>(callback.message->GetData());
            if (mission_data->mission_type == 1 && mission_data->set_index == 9)
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