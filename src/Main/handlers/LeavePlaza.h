#pragma once
namespace Game
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Main;

    namespace Handlers
    {
        inline void LeavePlaza(SCallbackData& callback, CMainServer* main_server)
        {
            auto session = callback.session;
            auto message = callback.message;
            if (!session || !message) return;

            std::shared_lock lock(session->GetMutex());
            CServer* server = callback.server;
            auto session_id = session->GetSessionId();
            auto acc_cache = main_server->GetAccCacheSharedBySessionId(session_id);
            if (acc_cache->acc_info.Index == -1) return;
            auto plaza_id = acc_cache->plaza_id;
            if (main_server->IsPlazaAlready(plaza_id))
            {
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
                                player_session->SendMsg(425, 0, 0, 1, reinterpret_cast<uint8_t*>(&my_unique_id), sizeof(my_unique_id));
                        }
                    }
                    BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "session id: ({}) left plaza id: ({})", session_id, plaza_id);
                    auto remove_myself = std::remove(current_plaza->session_ids.begin(), current_plaza->session_ids.end(), session_id);
                    current_plaza->session_ids.erase(remove_myself, current_plaza->session_ids.end());
                    acc_cache->plaza_id = 0;
                    acc_cache->in_plaza = false;           
                }
            }
            session->SendMsg(174, 0, 0, 0); // leave plaza success
            acc_cache.unlock();

            if (acc_cache->state == 0 && acc_cache->acc_info.GuideMission == 9) // Done guide mission "View roomlist"
            {
                auto acc_cache = main_server->GetAccCacheUniqueBySessionId(session_id);
                auto current_coll = main_server->GetCollectionInfoCache(55);
                acc_cache->acc_info.GuideMission = 10;
                if (current_coll->rewardExp > 0)
                {
                    acc_cache->acc_info.Experience += current_coll->rewardExp;
                }
                if (current_coll->rewardPoint > 0)
                {
                    acc_cache->acc_info.MicroPoints += current_coll->rewardPoint;
                }
                MainCompleteMissionReq mission_data;
                mission_data.collection_id = 55;
                session->SendMsg(168, 0, 2, 0, reinterpret_cast<uint8_t*>(&mission_data.collection_id), sizeof(mission_data.collection_id));
                std::vector<uint16_t> empty_vec;
                ProcessLevelUp(main_server, callback.server, acc_cache, session_id, empty_vec);
            }
        }
    }
}