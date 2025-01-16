#pragma once
namespace Game
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Main;

    namespace Handlers
    {
        inline void LeaveMatch(SCallbackData& callback, CMainServer* main_server)
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
            auto my_unique_id = NetEngine::Packets::Core::UniqueId(session_id, 1).data;
            if (acc_index == -1 || !acc_cache->in_room || !main_server->IsRoomAlready(acc_cache->room_id)) return;
            auto room_cache = main_server->GetRoomCacheUnique(acc_cache->room_id);
            auto my_slot_id = acc_cache->slot_id;
            acc_cache->playing = false;
            acc_cache->state = (room_cache->host_session_id == session_id) ? PlayerInfo::State::HostReady : PlayerInfo::State::Waiting;
            acc_cache->earnt_battery = 0;
            acc_cache.unlock();
            auto players = main_server->GetRoomSortedPlayerSessionIds(room_cache);
           
            for (const auto& id : players)
            {
                if (auto player_session = server->GetSessionById(id))
                    send_msg(player_session.get(), callback.message->GetOrder(), 0, 0, 0, reinterpret_cast<uint8_t*>(&my_unique_id), sizeof(my_unique_id)); // notify player leave room
            }
            const std::uint32_t& total_players_playing = std::count_if(players.begin(), players.end(),
                [main_server](const auto& id)
            {
                auto player_acc_cache = main_server->GetAccCacheSharedBySessionId(id);
                auto is_playing = player_acc_cache->playing;
                player_acc_cache.unlock();
                return is_playing;
            });
            std::uint32_t total_players_room = players.size();
            if(total_players_room == 1) room_cache->is_playing = false;
            if (total_players_playing > 0)
            {
                if (room_cache->host_session_id == session_id)
                {
                    auto best_ping_session_id = main_server->GetBestPlayerPingSessionIdInMatch(room_cache);
                    if (best_ping_session_id != 0)
                    {
                        auto best_ping_acc_cache = main_server->GetAccCacheUniqueBySessionId(best_ping_session_id);
                        if (best_ping_acc_cache->acc_info.Index != -1)
                        {
                            auto best_ping_slot_id = best_ping_acc_cache->slot_id;
                            acc_cache.lock();
                            best_ping_acc_cache->slot_id = acc_cache->slot_id;
                            acc_cache->slot_id = best_ping_slot_id;
                            room_cache->host_session_id = best_ping_session_id;
                            for (const auto& id : players)
                                if (auto player_session = server->GetSessionById(id))
                                    send_msg(player_session.get(), 128, 0, 1, static_cast<std::uint8_t>(best_ping_slot_id)); // host change


                            struct RoomAuthData
                            {
                                std::uint16_t room_id;
                                std::uint64_t auth_key;
                            };
                            RoomAuthData new_host_data{ room_cache->room_id, best_ping_acc_cache->acc_info.AuthKey };

                            main_server->SendCastIpc(PacketIds::Ipc::MainToCastHostChange, Utility::ToVector(new_host_data));

                            BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "room No. ({}) changed host ({}) -> ({}) due to leaving while playing. ", room_cache->room_id, acc_cache->acc_info.Nickname.c_str(), best_ping_acc_cache->acc_info.Nickname.c_str());
                            send_msg(session, 141, 0, NetEngine::Room::Leave::Ack::Result::Leave, 0); // leave room ack
                            for (const auto& id : players)
                            {
                                if (auto player_session = server->GetSessionById(id))
                                {
                                    if (id != session_id)
                                        send_msg(player_session.get(), callback.message->GetOrder(), 0, 0, 0, reinterpret_cast<uint8_t*>(&my_unique_id), sizeof(my_unique_id)); // notify player leave room
                                    send_msg(player_session.get(), 422, 0, 0, my_slot_id, reinterpret_cast<uint8_t*>(&my_unique_id), sizeof(my_unique_id));
                                }   
                            }
                            acc_cache->room_id = 0;
                            acc_cache->in_room = false;
                            acc_cache->playing = false;
                            auto my_team_id = acc_cache->team_id;
                            acc_cache.unlock();
                            best_ping_acc_cache.unlock();
                            main_server->RemoveRoomPlayerCache(room_cache, session_id, my_team_id);
                            main_server->RoomPlayersSlotReorder(room_cache); 
                        }
                    }
                }
            }
            BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "player ({}) left room match -> id: ({})", acc_cache->acc_info.Nickname.c_str(), room_cache->room_id);
        }
    }
}