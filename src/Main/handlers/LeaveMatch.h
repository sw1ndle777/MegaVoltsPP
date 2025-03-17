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
           
            auto left_while_vote_kicked = room_cache->vote_kick_target_session_id == session_id;
            auto in_party = acc_cache->in_party;

            BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "player leave match so will apply penality of 1 lose and 1 clan lose if clan room");
            acc_cache->acc_info.Loses += 1;
            if (room_cache->is_clan_room)
            {
                acc_cache->acc_info.ClanLoses += 1;
            }

            if (left_while_vote_kicked)
            {
                room_cache->voters_session_ids.clear();
                room_cache->vote_kick_target_session_id = 0;
                room_cache->is_kick_vote_running = false;
                if (!main_server->IsSessionIdAlready(session_id, room_cache->kicked_session_ids))
                    room_cache->kicked_session_ids.push_back(session_id);
            }

            for (const auto& id : players)
            {
                if (auto player_session = server->GetSessionById(id))
                    send_msg(player_session.get(), callback.message->GetOrder(), 0, 0, 0, reinterpret_cast<uint8_t*>(&my_unique_id), sizeof(my_unique_id)); // notify player leave room
            }
            const std::uint32_t& total_players_playing = static_cast<std::uint32_t>(std::count_if(players.begin(), players.end(),
                [main_server](const auto& id)
            {
                auto player_acc_cache = main_server->GetAccCacheSharedBySessionId(id);
                auto is_playing = player_acc_cache->playing;
                player_acc_cache.unlock();
                return is_playing;
            }));
            std::uint32_t total_players_room = static_cast<std::uint32_t>(players.size());
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
                            
                            for (const auto& id : players)
                            {
                                if (auto player_session = server->GetSessionById(id))
                                {

                                    if (id != session_id && !left_while_vote_kicked)
                                        send_msg(player_session.get(), callback.message->GetOrder(), 0, 0, 0, reinterpret_cast<uint8_t*>(&my_unique_id), sizeof(my_unique_id)); // notify player leave room

                                    send_msg(player_session.get(), 422, 0, 0, my_slot_id, reinterpret_cast<uint8_t*>(&my_unique_id), sizeof(my_unique_id));
                                }   
                            }

                            send_msg(session, 141, 0, left_while_vote_kicked ? NetEngine::Room::Leave::Ack::Result::KickedByKickVote : NetEngine::Room::Leave::Ack::Result::Leave, 0); // leave room ack
                            //party send_msg(session, 120, 0, 45, 0);
                            //send_msg(session, 111, 0, 1, 0);
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
                else if (left_while_vote_kicked && room_cache->host_session_id != session_id)
                {
                    send_msg(session, 141, 0, NetEngine::Room::Leave::Ack::Result::KickedByKickVote, 0); // leave room ack

                    for (const auto& id : players)
                        if (auto player_session = server->GetSessionById(id))
                            send_msg(player_session.get(), 422, 0, 0, my_slot_id, reinterpret_cast<uint8_t*>(&my_unique_id), sizeof(my_unique_id));

                    acc_cache.lock();
                    acc_cache->room_id = 0;
                    acc_cache->in_room = false;
                    acc_cache->playing = false;
                    auto my_team_id = acc_cache->team_id;
                    acc_cache.unlock();
                    main_server->RemoveRoomPlayerCache(room_cache, session_id, my_team_id);
                    main_server->RoomPlayersSlotReorder(room_cache);
                }
                else
                {
                    BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "player wasnt special case state: ({}), now room is playing: ({})", static_cast<std::uint32_t>(acc_cache->state), room_cache->is_playing);
                    acc_cache->playing = false;
                    /*
                    std::uint32_t my_team_id = static_cast<std::uint32_t>(acc_cache->team_id);
                    std::uint32_t my_team_left = 0;
                    //std::uint32_t my_team_left = (my_team_id == 0 ? (room_cache->neutralteam_session_ids.size() - 1) : (my_team_id == 1 ? room_cache->redteam_session_ids.size() : room_cache->blueteam_session_ids.size()));
                    const std::vector<uint16_t> &my_team = (my_team_id == 0 ? (room_cache->neutralteam_session_ids) : (my_team_id == 1 ? room_cache->redteam_session_ids : room_cache->blueteam_session_ids));
                    for (int i = 0, j = my_team.size(); i < j; i++)
                    {
                        auto player_cache = main_server->GetAccCacheSharedBySessionId(my_team[i]);
                        if (player_cache->playing) my_team_left++;
                        player_cache.unlock();
                    }
                    BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "after player leave remaining team playing: ({})", my_team_left);
                    if (my_team_left <= 1 && acc_cache->in_room)//match ended, but current dont know
                    {
                        BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "will broadcast to player that match end!");
                        for (const auto& id : players)
                        {
                            auto player_unique_id = NetEngine::Packets::Core::UniqueId(id, 1).data;
                            send_msg(session, 256, 0, 0, 0, reinterpret_cast<uint8_t*>(&player_unique_id), sizeof(player_unique_id));
                        }
                    }
                    */
                }
                if (in_party)
                {
                    acc_cache.lock();
                    BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "leave match while in party battle!");
                    if (acc_cache->in_room)
                    {
                        BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "leave party battle didnt assure leaving room");
                        acc_cache->room_id = 0;
                        acc_cache->in_room = false;
                        acc_cache->playing = false;
                        auto my_team_id = acc_cache->team_id;
                        acc_cache.unlock();
                        for (const auto& id : players)
                        {
                            if (auto player_session = server->GetSessionById(id))
                            {

                                if (id != session_id)
                                    send_msg(player_session.get(), callback.message->GetOrder(), 0, 0, 0, reinterpret_cast<uint8_t*>(&my_unique_id), sizeof(my_unique_id)); // notify player leave room

                                send_msg(player_session.get(), 422, 0, 0, my_slot_id, reinterpret_cast<uint8_t*>(&my_unique_id), sizeof(my_unique_id));
                            }
                        }
                        main_server->RemoveRoomPlayerCache(room_cache, session_id, my_team_id);
                        main_server->RoomPlayersSlotReorder(room_cache);
                        acc_cache.lock();
                    }
                    send_msg(session, 120, 0, 45, 0);
                    auto party_id = acc_cache->party_id;
                    auto party_cache = main_server->GetPartyCacheUnique(party_id);

                    if (party_cache->party_host_session_id == session_id) {
                        BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "party will need change host");
                        if (!acc_cache->in_room)
                        {
                            party_cache->is_registered = false;
                            party_cache->is_queueing = false;
                            for (const auto& party_member_session_id : party_cache->members)
                            {
                                if (auto player_session = server->GetSessionById(party_member_session_id))
                                    send_msg(player_session.get(), 120, 0, 45, 0);
                            }
                        }
                        std::uint16_t new_leader_index = 0;
                        std::uint16_t new_leader = 0;
                        for (const auto& member : party_cache->members)
                        {
                            if (member != party_cache->party_host_session_id) {
                                new_leader = member;
                                break;
                            }
                            new_leader_index++;
                        }
                        for (const auto& party_member_session_id : party_cache->members)
                        {
                            if (party_member_session_id == party_cache->party_host_session_id) continue;
                            if (auto player_session = server->GetSessionById(party_member_session_id))
                                send_msg(player_session.get(), 114, 0, 1, static_cast<std::uint8_t>(new_leader_index));
                        }
                        party_cache->party_host_session_id = new_leader;
                    }


                    auto remove_myself = std::remove(party_cache->members.begin(), party_cache->members.end(), session_id);
                    party_cache->members.erase(remove_myself, party_cache->members.end());

                    for (const auto& party_member_session_id : party_cache->members)
                    {
                        if (auto player_session = server->GetSessionById(party_member_session_id))
                            send_msg(player_session.get(), 419, 0, 0, 0, reinterpret_cast<uint8_t*>(&my_unique_id), sizeof(my_unique_id));
                    }

                    acc_cache->party_id = 0;
                    acc_cache->in_party = false;

                    //auto leavePartyReq = reinterpret_cast<MainLeavePartyReq*>(callback.message->GetData());
                    send_msg(session, 111, 0, 1, 0);

                    BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "player ({}) left party id: ({})", acc_cache->acc_info.Nickname.c_str(), party_id);
                    BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "now party have member count: ({})", party_cache->members.size());

                    if (party_cache->members.size() == 0) {
                        BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "party is empty so will be deleted id: ({})", party_id);
                        main_server->RemovePartyCache(party_id);
                        main_server->SetQueuePartyIdAvailable(party_id);
                    }
                    else party_cache.unlock();
                    acc_cache.unlock();
                }
            }
            BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "player ({}) left room match -> id: ({})", acc_cache->acc_info.Nickname.c_str(), room_cache->room_id);
        }
    }
}