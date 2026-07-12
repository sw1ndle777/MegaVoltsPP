#pragma once
namespace Game::Handlers
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Main;
    inline void MatchStart(SCallbackData& callback, CMainServer* main_server)
    {
        auto session = callback.session;
        auto message = callback.message;
        if (!session || !message) return;
        //std::shared_lock lock(session->GetMutex());

        CServer* server = callback.server;
        auto session_id = session->GetSessionId();
        auto acc_cache = CAccount.get<unique_t>(session_id);
        auto acc_index = acc_cache->acc_info.Index;
        auto acc_server_id = acc_cache->server_id;
        auto acc_team_id = acc_cache->team_id;
        auto match_result = static_cast<NetEngine::Room::Match::Result>(message->GetExtra());
        auto my_unique_id = NetEngine::Packets::Core::UniqueId(session_id, 1).data;

        if (acc_index == -1 || !acc_cache->in_room || !CRoom.contains(acc_cache->room_id)) return;

        auto nick_copy = acc_cache->acc_info.Nickname.c_str();
        auto room_cache = CRoom.get<unique_t>(acc_cache->room_id);

        if (match_result == NetEngine::Room::Match::Result::Started)
        {
            acc_cache->playing = true;
        }
        acc_cache.unlock();

        auto players_ids = main_server->GetRoomSortedPlayerSessionIds(room_cache);
        bool is_host = session_id == room_cache->host_session_id;

        DEBUGLOG(dark_cyan, "player ({}) start match, is host: ({})", nick_copy, is_host);

        switch (match_result)
        {
        case NetEngine::Room::Match::Result::SingleWave:
        {
            acc_cache.lock();
            acc_cache->match_loaded_time = Utility::GetUtcTimeNowInSeconds();
            acc_cache.unlock();
            break;
        }
        case NetEngine::Room::Match::Result::Started:
        {
            if (room_cache->host_session_id == session_id && room_cache->MapIndex == NetEngine::Room::Map::Index::Random) //set a real map id to RandomMapIndex !
            {
                if (const auto random_map = main_server->GetRandomMapIndexForMode(room_cache->ModeIndex, room_cache->max_players); random_map.has_value())
                {
                    room_cache->RandomMapIndex = random_map.value();
                    DEBUGLOG(dark_cyan,
                        "room ({}) random map resolved to ({}) for mode ({})",
                        room_cache->room_id,
                        main_server->GetMapName(static_cast<uint32_t>(room_cache->RandomMapIndex)),
                        main_server->GetModeName(static_cast<uint32_t>(room_cache->ModeIndex)));
                }
                else
                {
                    room_cache->RandomMapIndex = NetEngine::Room::Map::Index::HouseTop;
                    DEBUGLOG(dark_cyan,
                        "room ({}) random map pool empty for mode ({}) - fallback to ({})",
                        room_cache->room_id,
                        main_server->GetModeName(static_cast<uint32_t>(room_cache->ModeIndex)),
                        main_server->GetMapName(static_cast<uint32_t>(room_cache->RandomMapIndex)));
                }
            }
            room_cache->is_playing = true;
            room_cache->start_time = Utility::GetUtcTimeNowInMilliseconds();
            main_server->SendCastRoomMatchStateSync(room_cache->room_id, room_cache->host_session_id, true);
            if (is_host)
            {
                thread_local Utility::SecureRandomBlake2b::Generator rng;
                room_cache->match_instance_id = rng.GenerateAuthKey();

                // Room log: match started (one event per match, by the host).
                RoomLogEntry mlog;
                mlog.aid = acc_index;
                mlog.event_type = RoomLog::EventType::MatchStarted;
                mlog.server_id = acc_server_id;
                mlog.room_id = room_cache->room_id;
                mlog.host_aid = acc_index;
                [[maybe_unused]] auto ig = BaseLib::DbPool->submit_task([mlog]() mutable { BaseLib::Database->PersistRoomLogs({ mlog }); });
            }
            room_cache->team_rounds_started = 0;
            room_cache->match_combat_stats.clear();
            room_cache->left_sessions.clear(); // fresh match: drop any stale leaver stash
            room_cache->combat_events.clear(); // fresh match: drop any stale combat-hit stash
            room_cache->match_events.clear();  // fresh match: drop any stale timeline-event stash
            room_cache->combat_open = true;    // fresh match: combat is live
            room_cache->round_seq = 0;         // fresh match: round numbering restarts at 1
            //acc_cache->playing = true;
            DEBUGLOG(dark_cyan, "player normal started match and will broadcast to: ({}) players", players_ids.size());
            for (const auto& room_player_session_id : players_ids)
                if (auto player_session = server->GetSessionById(room_player_session_id))
                    player_session->SendMsg(message->GetOrder(), 0, NetEngine::Room::Match::Result::Started, (room_cache->MapIndex == NetEngine::Room::Map::Index::Random ? room_cache->RandomMapIndex : room_cache->MapIndex), reinterpret_cast<uint8_t*>(&my_unique_id), sizeof(my_unique_id)); // broadcasted players that match is loading
            break;
        }
        case NetEngine::Room::Match::Result::Loaded:
        {
            //acc_cache.unlock();
            // Room log: this player loaded/entered the match (one per loaded packet).
            {
                RoomLogEntry mlog;
                mlog.aid = acc_index;
                mlog.event_type = RoomLog::EventType::MatchEntered;
                mlog.server_id = acc_server_id;
                mlog.room_id = room_cache->room_id;
                mlog.team_id = static_cast<uint8_t>(acc_team_id);
                [[maybe_unused]] auto ig = BaseLib::DbPool->submit_task([mlog]() mutable { BaseLib::Database->PersistRoomLogs({ mlog }); });
            }
            if (is_host)
            {

                uint64_t uptime_tick = Utility::GetUtcTimeNowInMilliseconds() - server->GetStartTime();
                for (const auto& room_player_session_id : players_ids)
                {
                    if (auto player_session = server->GetSessionById(room_player_session_id))
                    {
                        auto player_acc_cache = CAccount.get<unique_t>(room_player_session_id);
                        if (!player_acc_cache->playing)
                        {
                            player_acc_cache.unlock();
                            continue;
                        }
                        //extra might need to be 5
                        player_session->SendMsg(258, 0, 1, 0, reinterpret_cast<uint8_t*>(&uptime_tick), sizeof(uptime_tick)); // broadcasted players room tick
                        player_acc_cache->state = PlayerInfo::State::Normal;
                        player_acc_cache->playing = true;
                        player_acc_cache->match_loaded_time = Utility::GetUtcTimeNowInSeconds();
                        // Match (round 1) start: everyone begins alive at full health on both servers.
                        main_server->RefreshPlayerHealthCache(player_acc_cache, true);
                        main_server->SendCastPlayerHealthSync(player_acc_cache);
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
                        auto player_acc_cache = CAccount.get<unique_t>(room_player_session_id);
                        if (!player_acc_cache->playing)
                        {
                            player_acc_cache.unlock();
                            continue;
                        }
                        player_session->SendMsg(415, 0, 1, 0, reinterpret_cast<uint8_t*>(&my_unique_id), sizeof(my_unique_id)); // broadcasted players that match loaded
                        player_acc_cache->state = PlayerInfo::State::Normal;
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
