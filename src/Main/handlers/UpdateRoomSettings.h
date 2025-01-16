#pragma once
namespace Game
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Main;

    namespace Handlers
    {
        inline void ChangeHost(SCallbackData& callback, CMainServer* main_server)
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
            auto target_slot_id = callback.message->GetOption();
            std::uint16_t target_session_id = 0;      
            auto room_cache = main_server->GetRoomCacheUnique(acc_cache->room_id);
            if (acc_index == -1 || !acc_cache->in_room || !main_server->IsRoomAlready(acc_cache->room_id) || room_cache->is_playing)
            {
                send_msg(session, 128, 0, NetEngine::Room::ChangeHost::Result::Error, 0);
                return;
            }
            acc_cache.unlock();
            auto find_target_session = [&](const std::vector<std::uint16_t>& session_ids) -> std::uint16_t
            {
                for (const auto& id : session_ids)
                {
                    auto player_cache = main_server->GetAccCacheSharedBySessionId(id);
                    if (player_cache->acc_info.Index != -1 && player_cache->in_room && player_cache->room_id == room_cache->room_id && player_cache->slot_id == target_slot_id)
                    {
                        player_cache.unlock();
                        return id;
                    }
                    else
                        player_cache.unlock();
                        
                }
                return 0; 
            };
            if(main_server->IsModeTeamBased(room_cache->ModeIndex))
            {
                target_session_id = find_target_session(room_cache->blueteam_session_ids);
                if (target_session_id == 0)
                    target_session_id = find_target_session(room_cache->redteam_session_ids);
            }
            else
                target_session_id = find_target_session(room_cache->neutralteam_session_ids);

            if (target_session_id == 0)
            {
                send_msg(session, 128, 0, NetEngine::Room::ChangeHost::Result::NotInRoom, 0);
                return;
            }
            acc_cache.lock();
            auto target_acc_cache = main_server->GetAccCacheUniqueBySessionId(target_session_id);
            if (target_acc_cache->acc_info.Index == -1 || !target_acc_cache->in_room || target_acc_cache->room_id != room_cache->room_id)
            {
                send_msg(session, 128, 0, NetEngine::Room::ChangeHost::Result::NotInRoom, 0);
                return;
            }
            if (room_cache->host_session_id != session_id)
            {
                send_msg(session, 128, 0, NetEngine::Room::ChangeHost::Result::NotTheHost, 0);
                return;
            }
            room_cache->host_session_id = target_session_id;
           
            target_acc_cache->slot_id = acc_cache->slot_id;

            struct RoomAuthData
            {
                std::uint16_t room_id;
                std::uint64_t auth_key;
            };
            RoomAuthData new_host_data{ room_cache->room_id, target_acc_cache->acc_info.AuthKey };

            main_server->SendCastIpc(PacketIds::Ipc::MainToCastHostChange, Utility::ToVector(new_host_data));
            target_acc_cache.unlock();
            acc_cache->slot_id = target_slot_id;
            acc_cache.unlock();
            auto players_ids = main_server->GetRoomSortedPlayerSessionIds(room_cache);
            lock.unlock();
            for (const auto& room_player_session_id : players_ids)
                if (auto player_session = server->GetSessionById(room_player_session_id))
                    send_msg(player_session.get(), 128, 0, NetEngine::Room::ChangeHost::Result::Success, target_slot_id);
        }
        inline void MaxTimeLimit(SCallbackData& callback, CMainServer* main_server)
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
            auto acc_cache = main_server->GetAccCacheSharedBySessionId(session_id);
            auto acc_index = acc_cache->acc_info.Index;
            auto max_time = callback.message->GetOption();
            if (acc_index == -1 || !acc_cache->in_room || !main_server->IsRoomAlready(acc_cache->room_id)) return;
            auto room_cache = main_server->GetRoomCacheUnique(acc_cache->room_id);
            if (room_cache->is_playing || room_cache->host_session_id != session_id) return;
            room_cache->time_rule = max_time;
            acc_cache.unlock();
            auto players_ids = main_server->GetRoomSortedPlayerSessionIds(room_cache);
            lock.unlock();
            for (const auto& room_player_session_id : players_ids)
                if (auto player_session = server->GetSessionById(room_player_session_id))
                    send_msg(player_session.get(), 135, 0, 0, max_time);
        }
        inline void MaxPointsLimit(SCallbackData& callback, CMainServer* main_server)
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
            auto acc_cache = main_server->GetAccCacheSharedBySessionId(session_id);
            auto acc_index = acc_cache->acc_info.Index;
            auto max_points = callback.message->GetOption();
            if (acc_index == -1 || !acc_cache->in_room || !main_server->IsRoomAlready(acc_cache->room_id)) return;
            auto room_cache = main_server->GetRoomCacheUnique(acc_cache->room_id);
            if (room_cache->is_playing || room_cache->host_session_id != session_id) return;
            room_cache->score_rule = max_points;
            acc_cache.unlock();
            auto players_ids = main_server->GetRoomSortedPlayerSessionIds(room_cache);
            lock.unlock();
            for (const auto& room_player_session_id : players_ids)
                if (auto player_session = server->GetSessionById(room_player_session_id))
                    send_msg(player_session.get(), 134, 0, 0, max_points);
        }
        inline void MaxPlayersLimit(SCallbackData& callback, CMainServer* main_server)
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
            auto acc_cache = main_server->GetAccCacheSharedBySessionId(session_id);
            auto acc_index = acc_cache->acc_info.Index;
            auto max_players = callback.message->GetOption();
            if (acc_index == -1 || !acc_cache->in_room || !main_server->IsRoomAlready(acc_cache->room_id)) return;
            auto room_cache = main_server->GetRoomCacheUnique(acc_cache->room_id);
            if (room_cache->is_playing || room_cache->host_session_id != session_id) return;
            auto players_count = room_cache->blueteam_session_ids.size() + room_cache->redteam_session_ids.size() + room_cache->neutralteam_session_ids.size();
            if(players_count > max_players) return;
            room_cache->max_players = max_players;
            acc_cache.unlock();
            auto players_ids = main_server->GetRoomSortedPlayerSessionIds(room_cache);
            lock.unlock();
            for (const auto& room_player_session_id : players_ids)
                if (auto player_session = server->GetSessionById(room_player_session_id))
                    send_msg(player_session.get(), 132, 0, 0, max_players);
        }
        inline void MapState(SCallbackData& callback, CMainServer* main_server)
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
            auto acc_cache = main_server->GetAccCacheSharedBySessionId(session_id);
            auto acc_index = acc_cache->acc_info.Index;
            if (acc_index == -1 || !acc_cache->in_room || !main_server->IsRoomAlready(acc_cache->room_id)) return;
            auto room_cache = main_server->GetRoomCacheUnique(acc_cache->room_id);
            if (room_cache->is_playing || room_cache->host_session_id != session_id) return;
            room_cache->MapIndex = static_cast<NetEngine::Room::Map::Index>(callback.message->GetExtra());
            acc_cache.unlock();
            auto players_ids = main_server->GetRoomSortedPlayerSessionIds(room_cache);
            lock.unlock();
            for (const auto& room_player_session_id : players_ids)
                if (auto player_session = server->GetSessionById(room_player_session_id))
                    send_msg(player_session.get(), 131, 1, room_cache->MapIndex, room_cache->ModeIndex);
        }
        inline void ObserversState(SCallbackData& callback, CMainServer* main_server)
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
            auto acc_cache = main_server->GetAccCacheSharedBySessionId(session_id);
            auto acc_index = acc_cache->acc_info.Index;
            auto state = callback.message->GetOption();
            if (acc_index == -1 || !acc_cache->in_room || !main_server->IsRoomAlready(acc_cache->room_id)) return;
            auto room_cache = main_server->GetRoomCacheUnique(acc_cache->room_id);
            if (room_cache->is_playing || room_cache->host_session_id != session_id) return;
            room_cache->allow_observers = static_cast<bool>(state);
            const auto& observer_ids = room_cache->observers_session_ids;
            acc_cache.unlock();
            auto players_ids = main_server->GetRoomSortedPlayerSessionIds(room_cache);
            lock.unlock();
            for (const auto& room_player_session_id : players_ids)
                if (auto player_session = server->GetSessionById(room_player_session_id))
                    send_msg(player_session.get(), 133, 0, 0, state);

            if (observer_ids.empty() || room_cache->allow_observers) return;
            for (const auto& observer_id : observer_ids)
            {
                auto observer_cache = main_server->GetAccCacheUniqueBySessionId(observer_id);
                if (observer_cache->acc_info.Index == -1) continue;
                if (!observer_cache->in_room || observer_cache->room_id != room_cache->room_id) continue;

                observer_cache->in_room = false;
                observer_cache->slot_id = 0;
                observer_cache->playing = false;
                observer_cache->state = PlayerInfo::State::Waiting;
                

                main_server->RemoveRoomPlayerCache(room_cache, observer_id, observer_cache->team_id);
                observer_cache.unlock();
                main_server->RoomPlayersSlotReorder(room_cache);
                
                if (auto player_session = server->GetSessionById(observer_id))
                    send_msg(player_session.get(), 141, 0, NetEngine::Room::Leave::Ack::Result::Leave, 0);
            }
        }
        inline void IntrudersState(SCallbackData& callback, CMainServer* main_server)
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
            auto acc_cache = main_server->GetAccCacheSharedBySessionId(session_id);
            auto acc_index = acc_cache->acc_info.Index;
            auto state = callback.message->GetOption();
            if (acc_index == -1 || !acc_cache->in_room || !main_server->IsRoomAlready(acc_cache->room_id)) return;
            auto room_cache = main_server->GetRoomCacheUnique(acc_cache->room_id);
            if (room_cache->is_playing || room_cache->host_session_id != session_id) return;
            room_cache->allow_intruders = static_cast<bool>(state);
            acc_cache.unlock();
            auto players_ids = main_server->GetRoomSortedPlayerSessionIds(room_cache);
            lock.unlock();
            for (const auto& room_player_session_id : players_ids)
                if (auto player_session = server->GetSessionById(room_player_session_id))
                    send_msg(player_session.get(), 127, 0, 0, state);
        }
        inline void ObjectsState(SCallbackData& callback, CMainServer* main_server)
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
            auto acc_cache = main_server->GetAccCacheSharedBySessionId(session_id);
            auto acc_index = acc_cache->acc_info.Index;
            auto state = callback.message->GetOption();
            if (acc_index == -1 || !acc_cache->in_room || !main_server->IsRoomAlready(acc_cache->room_id)) return;
            auto room_cache = main_server->GetRoomCacheUnique(acc_cache->room_id);
            if (room_cache->is_playing || room_cache->host_session_id != session_id) return;
            room_cache->allow_drops = static_cast<bool>(state);
            acc_cache.unlock();
            auto players_ids = main_server->GetRoomSortedPlayerSessionIds(room_cache);
            lock.unlock();
            for (const auto& room_player_session_id : players_ids)
                if (auto player_session = server->GetSessionById(room_player_session_id))
                    send_msg(player_session.get(), 124, 0, 0, state);
        }
        inline void TitlePasswordSettings(SCallbackData& callback, CMainServer* main_server)
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
           
            auto broadcastToPlayers = [&](const std::uint32_t target_session_id, const std::uint8_t& order, const std::uint8_t& mission = 0, const std::uint8_t& extra = 0, const std::uint8_t& option = 0)
            {
                if (auto player_session = server->GetSessionById(target_session_id))
                    send_msg(player_session.get(), order, mission, extra, option, callback.message->GetData(), callback.message->GetDataSize());
            };
            
            auto session_id = session->GetSessionId();
            auto acc_cache = main_server->GetAccCacheSharedBySessionId(session_id);
            auto acc_index = acc_cache->acc_info.Index;
            auto order = callback.message->GetOrder();
            auto option = callback.message->GetOption();
            auto extra = callback.message->GetExtra();
            auto mission = callback.message->GetMission();
            auto mode_id = static_cast<NetEngine::Room::Mode::Index>(option);
            auto data_size = callback.message->GetDataSize();

            if (acc_index == -1 || !acc_cache->in_room || !main_server->IsRoomAlready(acc_cache->room_id)) return;
            
            auto room_cache = main_server->GetRoomCacheUnique(acc_cache->room_id);
            auto previous_mode = room_cache->ModeIndex;
            auto previous_is_team_based = main_server->IsModeTeamBased(previous_mode);
            if (room_cache->is_playing || room_cache->host_session_id != session_id) return;
            acc_cache.unlock();
            auto players_ids = main_server->GetRoomSortedPlayerSessionIds(room_cache);
            lock.unlock();
            if (mission == 2) // add password
            {
                char newPassword[8]{};
                std::memcpy(newPassword, callback.message->GetData(), 8);
                room_cache->password = newPassword;
                room_cache->has_password = true;
                for (const auto& room_player_session_id : players_ids)
                    broadcastToPlayers(room_player_session_id, order);

                return;
            }
            else if (mission == 3) // remove password
            {
                room_cache->password = "";
                room_cache->has_password = false;
                for (const auto& room_player_session_id : players_ids)
                    broadcastToPlayers(room_player_session_id, order);

                return;
            }
            if (data_size == sizeof(RoomSettingsUpdateInfo))
            {
                auto settings_info = reinterpret_cast<RoomSettingsUpdateInfo*>(callback.message->GetData());
                room_cache->max_players = settings_info->max_players;
                room_cache->time_rule = settings_info->time;
                room_cache->score_rule = settings_info->score_limit;
                room_cache->Restriction = static_cast<NetEngine::Room::Restriction::Type>(settings_info->restriction);
                room_cache->allow_drops = settings_info->allow_items;
                room_cache->allow_intruders = settings_info->allow_intruders;
                room_cache->MapIndex = static_cast<NetEngine::Room::Map::Index>(settings_info->map_index);
                room_cache->TeamBalance = static_cast<NetEngine::Room::Balance::State>(settings_info->team_balance);
                room_cache->ModeIndex = mode_id;
                for (const auto& room_player_session_id : players_ids)
                    broadcastToPlayers(room_player_session_id, order, mission, extra, option);
            }
            else if (data_size == sizeof(RoomSettingsUpdateTitle))
            {
                const auto& settings_info = reinterpret_cast<RoomSettingsUpdateTitle*>(callback.message->GetData());
                room_cache->max_players = settings_info->update_info.max_players;
                room_cache->time_rule = settings_info->update_info.time;
                room_cache->score_rule = settings_info->update_info.score_limit;
                room_cache->Restriction = static_cast<NetEngine::Room::Restriction::Type>(settings_info->update_info.restriction);
                room_cache->allow_drops = settings_info->update_info.allow_items;
                room_cache->allow_intruders = settings_info->update_info.allow_intruders;
                room_cache->MapIndex = static_cast<NetEngine::Room::Map::Index>(settings_info->update_info.map_index);
                room_cache->TeamBalance = static_cast<NetEngine::Room::Balance::State>(settings_info->update_info.team_balance);
                room_cache->ModeIndex = mode_id;
                room_cache->title = settings_info->title;
                for (const auto& room_player_session_id : players_ids)
                    broadcastToPlayers(room_player_session_id, order, mission, extra, option);
            }
            else if (data_size == sizeof(RoomSettingsUpdatePassword))
            {
                const auto& settings_info = reinterpret_cast<RoomSettingsUpdatePassword*>(callback.message->GetData());
                room_cache->max_players = settings_info->update_info.max_players;
                room_cache->time_rule = settings_info->update_info.time;
                room_cache->score_rule = settings_info->update_info.score_limit;
                room_cache->Restriction = static_cast<NetEngine::Room::Restriction::Type>(settings_info->update_info.restriction);
                room_cache->allow_drops = settings_info->update_info.allow_items;
                room_cache->allow_intruders = settings_info->update_info.allow_intruders;
                room_cache->MapIndex = static_cast<NetEngine::Room::Map::Index>(settings_info->update_info.map_index);
                room_cache->TeamBalance = static_cast<NetEngine::Room::Balance::State>(settings_info->update_info.team_balance);
                room_cache->ModeIndex = mode_id;
                room_cache->password = settings_info->password;
                room_cache->has_password = true;
                for (const auto& room_player_session_id : players_ids)
                    broadcastToPlayers(room_player_session_id, order, mission, extra, option);
            }
            else if (data_size == sizeof(RoomSettingsUpdateTitlePassword))
            {
                const auto& settings_info = reinterpret_cast<RoomSettingsUpdateTitlePassword*>(callback.message->GetData());
                room_cache->max_players = settings_info->update_info.max_players;
                room_cache->time_rule = settings_info->update_info.time;
                room_cache->score_rule = settings_info->update_info.score_limit;
                room_cache->Restriction = static_cast<NetEngine::Room::Restriction::Type>(settings_info->update_info.restriction);
                room_cache->allow_drops = settings_info->update_info.allow_items;
                room_cache->allow_intruders = settings_info->update_info.allow_intruders;
                room_cache->MapIndex = static_cast<NetEngine::Room::Map::Index>(settings_info->update_info.map_index);
                room_cache->TeamBalance = static_cast<NetEngine::Room::Balance::State>(settings_info->update_info.team_balance);
                room_cache->ModeIndex = mode_id;
                room_cache->title = settings_info->title;
                room_cache->password = settings_info->password;
                room_cache->has_password = true;
                for (const auto& room_player_session_id : players_ids)
                    broadcastToPlayers(room_player_session_id, order, mission, extra, option);
            }
            auto new_mode = room_cache->ModeIndex;
            auto is_team_based = main_server->IsModeTeamBased(new_mode);
            if (previous_mode != new_mode)
            {
                if (!previous_is_team_based && is_team_based)
                {
                    room_cache->redteam_session_ids.clear();
                    room_cache->blueteam_session_ids.clear();
                    for (const auto& id : room_cache->neutralteam_session_ids)
                    {
                        auto player_acc_cache = main_server->GetAccCacheUniqueBySessionId(id);
                        auto blue_team_size = room_cache->blueteam_session_ids.size();
                        auto red_team_size = room_cache->redteam_session_ids.size();
                        if (blue_team_size == 0 || blue_team_size < red_team_size)
                        {
                            room_cache->blueteam_session_ids.push_back(id);
                            player_acc_cache->team_id = Team::IdType::Blue;
                        }
                        else
                        {
                            room_cache->redteam_session_ids.push_back(id);
                            player_acc_cache->team_id = Team::IdType::Red;
                        }
                        player_acc_cache.unlock();
                    }
                    room_cache->neutralteam_session_ids.clear();
                }
                else if (previous_is_team_based && !is_team_based)
                {
                    room_cache->neutralteam_session_ids.clear();

                    std::vector<std::pair<std::uint16_t, std::uint32_t>> player_slot_pairs;

                    auto addPlayerToSlotPairs = [&](const std::vector<std::uint16_t>& team_session_ids) 
                    {
                        for (const auto& id : team_session_ids)
                        {
                            auto player_cache = main_server->GetAccCacheSharedBySessionId(id);
                            if (player_cache->acc_info.Index != -1 && player_cache->in_room && player_cache->room_id == room_cache->room_id)
                                player_slot_pairs.emplace_back(id, player_cache->slot_id);

                            player_cache.unlock();
                        }
                    };
                    addPlayerToSlotPairs(room_cache->blueteam_session_ids);
                    addPlayerToSlotPairs(room_cache->redteam_session_ids);

                    std::stable_sort(player_slot_pairs.begin(), player_slot_pairs.end(),
                        [](const std::pair<std::uint16_t, std::uint32_t>& a, const std::pair<std::uint16_t, std::uint32_t>& b) {
                        return a.second < b.second;
                    });

                    for (const auto& pair : player_slot_pairs)
                    {
                        room_cache->neutralteam_session_ids.push_back(pair.first);
                        auto player_acc_cache = main_server->GetAccCacheUniqueBySessionId(pair.first);
                        player_acc_cache->team_id = Team::IdType::Neutral;
                        player_acc_cache.unlock();
                    }
                }
            }
        }

        inline void VoteKick(SCallbackData& callback, CMainServer* main_server)
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
            auto acc_cache = main_server->GetAccCacheSharedBySessionId(session_id);
            auto acc_index = acc_cache->acc_info.Index;
            auto order = callback.message->GetOrder();
            auto option = callback.message->GetOption();
            auto extra = callback.message->GetExtra();
            auto mission = callback.message->GetMission();
            auto mode_id = static_cast<NetEngine::Room::Mode::Index>(option);
            auto data_size = callback.message->GetDataSize();

            if (acc_index == -1 || !acc_cache->in_room || !main_server->IsRoomAlready(acc_cache->room_id)) return;

            auto room_cache = main_server->GetRoomCacheUnique(acc_cache->room_id);
            
        }
        inline void UpdateRoomSettings(SCallbackData& callback, CMainServer* main_server)
        {
            const auto order = callback.message->GetOrder();
            const auto extra = callback.message->GetExtra();
            switch (order)
            {
                case 124: ObjectsState(callback, main_server); break;
                case 125: 
                    if (extra == 28)
                        VoteKick(callback, main_server);
                    else
                        TitlePasswordSettings(callback, main_server); // gamemode change
                    break;
                case 126: TitlePasswordSettings(callback, main_server); break;
                case 127: IntrudersState(callback, main_server); break;
                case 128: ChangeHost(callback, main_server); break;
                case 129: TitlePasswordSettings(callback, main_server); break;
                case 130: TitlePasswordSettings(callback, main_server); break;
                case 131:  MapState(callback, main_server); break;
                case 132:  MaxPlayersLimit(callback, main_server); break;
                case 133: ObserversState(callback, main_server); break;
                case 134: MaxPointsLimit(callback, main_server); break;
                case 135: MaxTimeLimit(callback, main_server); break;
            }
        }
    }
}