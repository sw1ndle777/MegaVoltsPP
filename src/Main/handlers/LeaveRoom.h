#pragma once
namespace Game
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Main;

    namespace Handlers
    {
        inline void LeaveRoom(SCallbackData& callback, CMainServer* main_server)
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
            auto extra = callback.message->GetExtra();
            auto session_id = session->GetSessionId();
            auto leaveRoomReq = reinterpret_cast<MainLeaveRoomReq*>(callback.message->GetData());
            auto target_unique_id = NetEngine::Packets::Core::UniqueId(leaveRoomReq->uniqueId);
            if (extra == 28) // kick player
            {
                auto acc_cache = main_server->GetAccCacheUniqueBySessionId(session_id);
                auto acc_index = acc_cache->acc_info.Index;
                
                if (acc_index == -1) return;
                if (!acc_cache->in_room || !main_server->IsRoomAlready(acc_cache->room_id)) return;
                auto room = main_server->GetRoomCacheUnique(acc_cache->room_id);
                if (room->host_session_id != session_id) return;
                if (target_unique_id.session == session_id) return;
                acc_cache.unlock();

                auto target_acc_cache = main_server->GetAccCacheUniqueBySessionId(target_unique_id.session);
                auto target_acc_index = target_acc_cache->acc_info.Index;
                if (target_acc_index == -1) return;
                if (!target_acc_cache->in_room || target_acc_cache->room_id != room->room_id) return;
                if (main_server->IsSessionIdAlready(target_unique_id.session, room->kicked_session_ids)) return;
                room->kicked_session_ids.push_back(target_unique_id.session);
                auto target_team_id = target_acc_cache->team_id;
                auto target_slot = target_acc_cache->slot_id;
                target_acc_cache->in_room = false;
                target_acc_cache->slot_id = 0;
                target_acc_cache->playing = false;
                target_acc_cache->state = PlayerInfo::State::Waiting;
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "player ({}) force kicked by host from room -> id: ({})", target_acc_cache->acc_info.Nickname.c_str(), room->room_id);
                target_acc_cache.unlock();

                main_server->RemoveRoomPlayerCache(room, target_unique_id.session, target_team_id);
                main_server->RoomPlayersSlotReorder(room);

                std::vector<std::uint32_t> players_ids;
                std::vector<std::pair<std::uint32_t, std::uint32_t>> player_slot_pairs;
                auto insert_player_slot_pair = [&](const auto& session_ids)
                {
                    for (const auto& id : session_ids)
                    {
                        auto player_cache = main_server->GetAccCacheSharedBySessionId(id);
                        if (player_cache->acc_info.Index == -1 || !player_cache->in_room || player_cache->room_id != room->room_id)
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
                if (main_server->IsModeTeamBased(room->ModeIndex))
                {
                    insert_player_slot_pair(room->blueteam_session_ids);
                    insert_player_slot_pair(room->redteam_session_ids);
                }
                else
                    insert_player_slot_pair(room->neutralteam_session_ids);
                insert_player_slot_pair(room->observers_session_ids);

                room.unlock();

                std::sort(player_slot_pairs.begin(), player_slot_pairs.end(), [](const std::pair<std::uint32_t, int>& a, const std::pair<std::uint32_t, int>& b) { return a.second < b.second; });
                for (const auto& pair : player_slot_pairs)  players_ids.push_back(pair.first);

                for (const auto& room_player_session_id : players_ids)
                {
                    if (auto player_session = server->GetSessionById(room_player_session_id))
                        send_msg(player_session.get(), 422, 0, 0, target_slot, reinterpret_cast<uint8_t*>(&target_unique_id), sizeof(target_unique_id));
                }
                
                if (auto target_session = server->GetSessionById(target_unique_id.session))
                    send_msg(target_session.get(), 141, 0, NetEngine::Room::Leave::Ack::Result::KickedByHost, 0); // leave room ack

                

            }
            else
            {
                auto acc_cache = main_server->GetAccCacheUniqueBySessionId(session_id);
                auto acc_index = acc_cache->acc_info.Index;
                auto my_unique_id = NetEngine::Packets::Core::UniqueId(session_id, 1).data;
                auto my_slot = acc_cache->slot_id;
                auto my_team_id = acc_cache->team_id;
                auto leave_result = static_cast<NetEngine::Room::Leave::Req::Result>(callback.message->GetExtra());
                if (acc_index == -1) return;
                auto leaveRoomReq = reinterpret_cast<MainLeaveRoomReq*>(callback.message->GetData());
                if (leave_result != NetEngine::Room::Leave::Req::Result::Leave || !acc_cache->in_room || !main_server->IsRoomAlready(acc_cache->room_id)) return;
                auto room = main_server->GetRoomCacheUnique(acc_cache->room_id);
                acc_cache->in_room = false;
                acc_cache->slot_id = 0;
                acc_cache->playing = false;
                acc_cache->state = PlayerInfo::State::Waiting;
                acc_cache.unlock();
                main_server->RemoveRoomPlayerCache(room, session_id, my_team_id);
                main_server->RoomPlayersSlotReorder(room);


                std::vector<std::uint32_t> players_ids;
                std::vector<std::pair<std::uint32_t, std::uint32_t>> player_slot_pairs;
                auto insert_player_slot_pair = [&](const auto& session_ids)
                {
                    for (const auto& id : session_ids)
                    {
                        auto player_cache = main_server->GetAccCacheSharedBySessionId(id);
                        if (player_cache->acc_info.Index == -1 || !player_cache->in_room || player_cache->room_id != room->room_id)
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
                if (main_server->IsModeTeamBased(room->ModeIndex))
                {
                    insert_player_slot_pair(room->blueteam_session_ids);
                    insert_player_slot_pair(room->redteam_session_ids);
                }
                else
                    insert_player_slot_pair(room->neutralteam_session_ids);
                insert_player_slot_pair(room->observers_session_ids);
                std::sort(player_slot_pairs.begin(), player_slot_pairs.end(), [](const std::pair<std::uint32_t, int>& a, const std::pair<std::uint32_t, int>& b) { return a.second < b.second; });
                for (const auto& pair : player_slot_pairs)  players_ids.push_back(pair.first);
                for (const auto& room_player_session_id : players_ids)
                {
                    if (room_player_session_id == session_id) continue;
                    if (auto player_session = server->GetSessionById(room_player_session_id))
                        send_msg(player_session.get(), 422, 0, 0, my_slot, reinterpret_cast<uint8_t*>(&my_unique_id), sizeof(my_unique_id));
                }
                if (!room->neutralteam_session_ids.empty() || !room->redteam_session_ids.empty() || !room->blueteam_session_ids.empty())
                {
                    if (room->host_session_id == session_id)
                    {
                        auto best_ping_session_id = main_server->GetBestPlayerPingSessionIdInRoom(room);
                        auto best_ping_acc_cache = main_server->GetAccCacheSharedBySessionId(best_ping_session_id);
                        if (best_ping_acc_cache->acc_info.Index != -1)
                        {
                            room->host_session_id = best_ping_session_id;
                            for (const auto& id : players_ids)
                                if (auto player_session = server->GetSessionById(id))
                                    send_msg(player_session.get(), 128, 0, 1, static_cast<std::uint8_t>(best_ping_acc_cache->slot_id)); // broadcast host change


                            struct RoomAuthData
                            {
                                std::uint16_t room_id;
                                std::uint64_t auth_key;
                            };
                            RoomAuthData new_host_data{ room->room_id, best_ping_acc_cache->acc_info.AuthKey };

                            main_server->SendCastIpc(PacketIds::Ipc::MainToCastHostChange, Utility::ToVector(new_host_data));
                        }
                        best_ping_acc_cache.unlock();
                    }
                }
                send_msg(session, 141, 0, NetEngine::Room::Leave::Ack::Result::Leave, 0); // leave room ack
                if (room->neutralteam_session_ids.empty() && room->redteam_session_ids.empty() && room->blueteam_session_ids.empty())
                {
                    if (!room->observers_session_ids.empty())
                    {
                        for (const auto& observer_id : room->observers_session_ids)
                        {
                            auto observer_cache = main_server->GetAccCacheUniqueBySessionId(observer_id);
                            if (observer_cache->acc_info.Index == -1 || !observer_cache->in_room || observer_cache->room_id != room->room_id) continue;
                            observer_cache->in_room = false;
                            observer_cache->slot_id = 0;
                            observer_cache->playing = false;
                            observer_cache->state = PlayerInfo::State::Waiting;
                            auto observer_cache_team_id = observer_cache->team_id;
                            observer_cache.unlock();
                            main_server->RemoveRoomPlayerCache(room, observer_id, observer_cache_team_id);
                            main_server->RoomPlayersSlotReorder(room);
                            if (auto observer_session = server->GetSessionById(observer_id))
                                send_msg(observer_session.get(), 141, 0, NetEngine::Room::Leave::Ack::Result::Leave, 0);
                        }
                    }
                    main_server->RemoveRoomCache(room->room_id);
                    server->SetRoomIdAvailable(room->room_id, true);
                }
                acc_cache.lock();
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "player ({}) left room -> id: ({})", acc_cache->acc_info.Nickname.c_str(), room->room_id);
            }        
        }
    }
}