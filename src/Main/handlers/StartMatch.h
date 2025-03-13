#pragma once
namespace Game
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Main;

    namespace Handlers
    {
        inline void StartMatch(SCallbackData& callback, CMainServer* main_server)
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
            auto match_result = static_cast<NetEngine::Room::Match::Result>(callback.message->GetExtra());
            auto my_unique_id = NetEngine::Packets::Core::UniqueId(session_id, 1).data;

            if (acc_index == -1 || !acc_cache->in_room || !main_server->IsRoomAlready(acc_cache->room_id)) return;
            auto room_cache = main_server->GetRoomCacheUnique(acc_cache->room_id);
            acc_cache.unlock();
            auto players_ids = main_server->GetRoomSortedPlayerSessionIds(room_cache);
            acc_cache.lock();
            bool is_host = session_id == room_cache->host_session_id;

            BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "player ({}) start match, is host: ({})", acc_cache->acc_info.Nickname.c_str(), is_host);

            switch (match_result)
            {
                case NetEngine::Room::Match::Result::SingleWave:
                {
                    acc_cache->match_loaded_time = Utility::GetUtcTimeNowInSeconds();
                    break;
                }
                case NetEngine::Room::Match::Result::Started:
                {
                    if (room_cache->host_session_id == session_id && room_cache->MapIndex == NetEngine::Room::Map::Index::Random) //set a real map id to RandomMapIndex !
                    {
                        room_cache->RandomMapIndex = NetEngine::Room::Map::Index::HouseTop;
                    }
                    room_cache->is_playing = true;
                    acc_cache->playing = true;
                    BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "player normal started match and will broadcast to: ({}) players", players_ids.size());
                    for (const auto& room_player_session_id : players_ids)
                        if (auto player_session = server->GetSessionById(room_player_session_id))
                            send_msg(player_session.get(), callback.message->GetOrder(), 0, NetEngine::Room::Match::Result::Started, (room_cache->MapIndex == NetEngine::Room::Map::Index::Random ? room_cache->RandomMapIndex : room_cache->MapIndex), reinterpret_cast<uint8_t*>(&my_unique_id), sizeof(my_unique_id)); // broadcasted players that match is loading
                    break;
                }
                case NetEngine::Room::Match::Result::Loaded:
                {
                    acc_cache.unlock();
                    if (is_host)
                    {
                        
                        std::uint64_t sv_uptime_tick = Utility::GetUtcTimeNowInMilliseconds() - server->GetStartTime();
                        for (const auto& room_player_session_id : players_ids)
                        {
                            if (auto player_session = server->GetSessionById(room_player_session_id))
                            {
                                auto player_acc_cache = main_server->GetAccCacheUniqueBySessionId(room_player_session_id);
                                if (!player_acc_cache->playing) continue;
                                send_msg(player_session.get(), 258, 0, 1, 0, reinterpret_cast<uint8_t*>(&sv_uptime_tick), sizeof(sv_uptime_tick)); // broadcasted players room tick
                                player_acc_cache->state = PlayerInfo::State::Normal;
                                player_acc_cache->playing = true;
                                player_acc_cache->match_loaded_time = Utility::GetUtcTimeNowInSeconds();
                                player_acc_cache.unlock();
                            }
                        }
                            
                    }
                    else
                    {
                        for (const auto& room_player_session_id : players_ids)
                        {
                            if (auto player_session = server->GetSessionById(room_player_session_id))
                            {
                                auto player_acc_cache = main_server->GetAccCacheUniqueBySessionId(room_player_session_id);
                                if (!player_acc_cache->playing) continue;
                                send_msg(player_session.get(), 415, 0, 1, 0, reinterpret_cast<uint8_t*>(&my_unique_id), sizeof(my_unique_id)); // broadcasted players that match loaded
                                //player_acc_cache->state = PlayerInfo::State::Normal;
                                player_acc_cache->playing = true;
                                player_acc_cache->match_loaded_time = Utility::GetUtcTimeNowInSeconds();
                                player_acc_cache.unlock();
                            }  
                        }     
                    }
                    break;
                }
            }
        }
    } 
}