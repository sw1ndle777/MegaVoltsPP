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
                observer_cache->slot_id = 0xFF;
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
        inline void VoteKickAgree(SCallbackData& callback, CMainServer* main_server)
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

            if (!room_cache->is_playing)
            {
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan,
                    "player: ({}) tried to agree to vote kick while match is not playing",
                    acc_cache->acc_info.Nickname.c_str());
                return;
            }

            if (main_server->IsSessionIdAlready(session_id, room_cache->voters_session_ids))
            {
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan,
                    "player: ({}) tried to agree to vote kick more than once",
                    acc_cache->acc_info.Nickname.c_str());
                return;
            }
            BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan,
                "player: ({}) pressed Y in room id: ({})'s vote kick",
                acc_cache->acc_info.Nickname.c_str(), acc_cache->room_id);
            room_cache->voters_session_ids.push_back(session_id);
        }
        inline void VoteKickCheckVotes(SCallbackData& callback, CMainServer* main_server)
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

            if (acc_index == -1)
            {
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan,
                    "acc_index == -1");
                return;
            } 
            if (!acc_cache->in_room)
            {
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan,
                    "player: ({}) is not in room id: ({})",
                    acc_cache->acc_info.Nickname.c_str(), acc_cache->room_id);
                return;
            }
            
            if (!main_server->IsRoomAlready(acc_cache->room_id))
            {
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan,
                    "room id: ({}) doesn't exist",
                    acc_cache->room_id);
                return;
            }



            
            auto room_id = acc_cache->room_id;
            auto room_cache = main_server->GetRoomCacheUnique(room_id);

            if (!room_cache->is_playing)
            {
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan,
                    "player: ({}) tried to count votes from vote kick while match is not playing",
                    acc_cache->acc_info.Nickname.c_str());
                return;
            }

            if (!room_cache->is_kick_vote_running)
            {
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan,
                    "room id: ({})'s vote kick is no longer running",
                    room_id);
                return;
            }
            BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan,
                "player: ({}) is vote kick initiator and started counting votes for vote kick",
                acc_cache->acc_info.Nickname.c_str());

            acc_cache.unlock();

            auto target_session_id = room_cache->vote_kick_target_session_id;
            auto room_playing_players = main_server->GetRoomSortedPlayerPlayingWithoutObserverSessionIds(room_cache);
            auto total_y_voters = room_cache->voters_session_ids.size();
            auto total_n_voters = room_playing_players.size() - total_y_voters + 1;
            auto target_left = !main_server->IsSessionIdAlready(target_session_id, room_playing_players);
            auto getting_kicked = false;
            if (total_y_voters > total_n_voters)
                getting_kicked = true;
                
            if (target_left)
                getting_kicked = true;


            if(getting_kicked)
                room_cache->kicked_session_ids.push_back(target_session_id);

            if (!getting_kicked)
            {
                room_cache->voters_session_ids.clear();
                room_cache->vote_kick_target_session_id = 0;
                room_cache->is_kick_vote_running = false;
                return;
            }

            for (const auto& room_player_session_id : room_playing_players)
            {
                if (auto player_session = main_server->GetSessionById(room_player_session_id))
                    send_msg(player_session.get(), 125, 0, 42, static_cast<std::uint8_t>(total_y_voters));
            }

            room_cache->voters_session_ids.clear();
            room_cache->vote_kick_target_session_id = 0;
            room_cache->is_kick_vote_running = false;

            BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan,
                "vote kicked player session id: ({}) gonna get acc cache", target_session_id);
            auto player = main_server->GetAccCacheUniqueBySessionId(target_session_id);
            auto player_acc_index = player->acc_info.Index;
            auto player_session_id = player->session_id;
            auto player_nickname = player->acc_info.Nickname;
            auto player_unique_id = NetEngine::Packets::Core::UniqueId(player_session_id, 1);
            auto player_in_room = player->in_room;
            auto player_room_id = player->room_id;
            auto player_slot_id = player->slot_id;
            auto player_team_id = player->team_id;
            if (player_acc_index == -1)   return;
              
            if (!player_in_room || player_room_id != room_id) return;

            BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan,
                "player: ({}) got vote kicked out with Y: ({}) : N: ({}) from room id: ({})",
                player_nickname.c_str(), total_y_voters, total_n_voters, player_room_id);

            player->in_room = false;
            player->slot_id = 0xFF;
            player->playing = false;
            player->state = PlayerInfo::State::Waiting;
            player.unlock();
           
            main_server->RemoveRoomPlayerCache(room_cache, player_session_id, player_team_id);
            main_server->RoomPlayersSlotReorder(room_cache);

            std::vector<std::uint32_t> players_ids;
            std::vector<std::pair<std::uint32_t, std::uint32_t>> player_slot_pairs;
            auto insert_player_slot_pair = [&](const auto& session_ids)
            {
                for (const auto& id : session_ids)
                {
                    auto player_cache = main_server->GetAccCacheSharedBySessionId(id);
                    if (player_cache->acc_info.Index == -1 || !player_cache->in_room || player_cache->room_id != room_cache->room_id)
                    {
                        player_cache.unlock();
                        continue;
                    }
                    else
                    {
                        player_slot_pairs.emplace_back(id, player_cache->slot_id);
                        player_cache.unlock();
                    }

                }
            };
            if (main_server->IsModeTeamBased(room_cache->ModeIndex))
            {
                insert_player_slot_pair(room_cache->blueteam_session_ids);
                insert_player_slot_pair(room_cache->redteam_session_ids);
            }
            else
                insert_player_slot_pair(room_cache->neutralteam_session_ids);
            insert_player_slot_pair(room_cache->observers_session_ids);
            std::sort(player_slot_pairs.begin(), player_slot_pairs.end(), [](const std::pair<std::uint32_t, int>& a, const std::pair<std::uint32_t, int>& b) { return a.second < b.second; });
            for (const auto& pair : player_slot_pairs)  players_ids.push_back(pair.first);
           

            if (!room_cache->neutralteam_session_ids.empty() || !room_cache->redteam_session_ids.empty() || !room_cache->blueteam_session_ids.empty())
            {
                if (room_cache->host_session_id == player_session_id)
                {
                    auto best_ping_session_id = main_server->GetBestPlayerPingSessionIdInRoom(room_cache);
                    auto best_ping_acc_cache = main_server->GetAccCacheSharedBySessionId(best_ping_session_id);
                    if (best_ping_acc_cache->acc_info.Index != -1)
                    {
                        room_cache->host_session_id = best_ping_session_id;
                        for (const auto& id : players_ids)
                            if (auto player_session = main_server->GetSessionById(id))
                                send_msg(player_session.get(), 128, 0, 1, static_cast<std::uint8_t>(best_ping_acc_cache->slot_id)); // broadcast host change

                        BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "room No. ({}) changed host ({}) -> ({}) due to leaving while playing. ", room_cache->room_id, player_nickname.c_str(), best_ping_acc_cache->acc_info.Nickname.c_str());
                    }
                    best_ping_acc_cache.unlock();
                }
            }



            if (auto player_session = main_server->GetSessionById(player_session_id))
                send_msg(player_session.get(), 141, 0, NetEngine::Room::Leave::Ack::Result::KickedByKickVote, 0);// leave room ack

            for (const auto& room_player_session_id : players_ids)
            {
                if (room_player_session_id == player_session_id) continue;
                if (auto player_session = main_server->GetSessionById(room_player_session_id))
                    send_msg(player_session.get(), 422, 0, 0, player_slot_id, reinterpret_cast<uint8_t*>(&player_unique_id), sizeof(player_unique_id));
            }

            if (room_cache->neutralteam_session_ids.empty() && room_cache->redteam_session_ids.empty() && room_cache->blueteam_session_ids.empty())
            {
                if (!room_cache->observers_session_ids.empty())
                {
                    for (const auto& observer_id : room_cache->observers_session_ids)
                    {
                        auto observer_cache = main_server->GetAccCacheUniqueBySessionId(observer_id);
                        if (observer_cache->acc_info.Index == -1 || !observer_cache->in_room || observer_cache->room_id != room_cache->room_id) continue;
                        observer_cache->in_room = false;
                        observer_cache->slot_id = 0xFF;
                        observer_cache->playing = false;
                        observer_cache->state = PlayerInfo::State::Waiting;
                        auto observer_cache_team_id = observer_cache->team_id;
                        observer_cache.unlock();
                        main_server->RemoveRoomPlayerCache(room_cache, observer_id, observer_cache_team_id);
                        main_server->RoomPlayersSlotReorder(room_cache);
                        if (auto observer_session = main_server->GetSessionById(observer_id))
                            send_msg(observer_session.get(), 141, 0, NetEngine::Room::Leave::Ack::Result::Leave, 0);
                    }
                }
                main_server->RemoveRoomCache(room_cache->room_id);
                main_server->SetRoomIdAvailable(room_cache->room_id);
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
            auto room_id = acc_cache->room_id;

            if (acc_index == -1 || !acc_cache->in_room || !main_server->IsRoomAlready(room_id)) return;
            auto acc_cache_nickname = acc_cache->acc_info.Nickname;
            auto room_cache = main_server->GetRoomCacheUnique(room_id);
            if (!room_cache->is_playing)
            {
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan,
                    "player: ({}) tried to vote kick while match is not playing",
                    acc_cache_nickname.c_str());
                return;
            }
            
            if (main_server->IsSessionIdAlready(session_id, room_cache->kick_voters_session_ids))
            {
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan,
                    "player: ({}) tried to vote kick more than once",
                    acc_cache_nickname.c_str());
                return;
            }
            room_cache->kick_voters_session_ids.push_back(session_id);


            const auto& voteKickReq = reinterpret_cast<MainVoteKickReq*>(callback.message->GetData());

            auto target_unique_id = NetEngine::Packets::Core::UniqueId(voteKickReq->target_unique_id);
            if (target_unique_id.session == session_id)
            {
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan,
                    "player: ({}) tried to vote kick himself",
                    acc_cache_nickname.c_str());
                return;
            }

            auto my_unique_id = NetEngine::Packets::Core::UniqueId(session_id, 1).data;
            if (acc_cache->acc_info.MicroPoints < 100) // 100mp cost for kick check
            {
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan,
                    "player: ({}) tried to vote kick without enough mp",
                    acc_cache_nickname.c_str());
                send_msg(session, 125, 0, 14, 0); // KICK_VOTE_ERROR_5
                return;
            }
            acc_cache->acc_info.MicroPoints -= 100;

            acc_cache.unlock();
            auto room_playing_players = main_server->GetRoomSortedPlayerPlayingWithoutObserverSessionIds(room_cache);
            if (!main_server->IsSessionIdAlready(target_unique_id.session, room_playing_players))
            {
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan,
                    "player: ({}) tried to vote kick non playing player session: ({})",
                    acc_cache_nickname.c_str(), static_cast<std::uint16_t>(target_unique_id.session));
                send_msg(session, 125, 0, 13, 0); // KICK_VOTE_ERROR_3
                return;
            }
            
            auto target_acc_cache = main_server->GetAccCacheSharedBySessionId(target_unique_id.session);
            auto server_time = Utility::GetUtcTimeNowInMilliseconds() - server->GetStartTime();
            auto vote_kick_tick = (server_time + 30000) / 10;
            auto voteKickAck = MainVoteKickAck(target_unique_id.data, my_unique_id, voteKickReq->reason_id, vote_kick_tick);
            BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan,
                "player: ({}) vote kicked other player: ({}) for reason id: ({})",
                acc_cache_nickname.c_str(), target_acc_cache->acc_info.Nickname.c_str(), voteKickReq->reason_id);
           
            room_cache->is_kick_vote_running = true;
            room_cache->voters_session_ids.push_back(session_id);
            room_cache->voters_session_ids.push_back(target_unique_id.session);
            room_cache->vote_kick_target_session_id = target_unique_id.session;
            lock.unlock();
            for (const auto& room_player_session_id : room_playing_players)
            {
                if (auto player_session = server->GetSessionById(room_player_session_id))
                    send_msg(player_session.get(), 125, 0, 28, 1, reinterpret_cast<std::uint8_t*>(&voteKickAck), sizeof(MainVoteKickAck));
            }
        }
        inline void UpdateRoomSettings(SCallbackData& callback, CMainServer* main_server)
        {
            const auto order = callback.message->GetOrder();
            const auto extra = callback.message->GetExtra();
            const auto mission = callback.message->GetMission();
            switch (order)
            {
                case 124: ObjectsState(callback, main_server); break;
                case 125: 
                    if (extra == 28)
                        VoteKick(callback, main_server);
                    else if (extra == 29)
                        VoteKickAgree(callback, main_server);
                    else if (extra == 42)
                        VoteKickCheckVotes(callback, main_server);
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