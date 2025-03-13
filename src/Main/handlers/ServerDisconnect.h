#pragma once

#include "LeaveParty.h"

namespace Game
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Main;

    namespace Handlers
    {
        inline void ServerDisconnect(std::shared_ptr<CSession> session, CMainServer* main_server)
        {
            auto send_msg = [&](CSession* session, std::uint16_t order, std::uint8_t mission, std::uint8_t extra, std::uint8_t option, std::uint8_t* data = nullptr, std::uint16_t data_size = 0)
            {
                CMessage message(session->GetEncryptionKey());
                message.SetSession(session->GetSessionId());
                message.SetCommand(order, mission, extra, option);
                if (data_size > 0 && data != nullptr) message.SetData(data, data_size);
                session->Send(message);
            };
            
            std::shared_lock lock(session->GetMutex());
            auto session_id = session->GetSessionId();
           
            auto acc_cache = main_server->GetAccCacheSharedBySessionId(session_id);
            auto acc_index = acc_cache->acc_info.Index;
            if (acc_index == -1)
            {
                main_server->RemoveSession(session_id);
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "session id: ({}) disconnected", session_id);
                acc_cache.unlock();
                return;
            }
            auto my_slot = acc_cache->slot_id;
            auto auth_key = acc_cache->acc_info.AuthKey;
            auto clan_id = acc_cache->acc_info.ClanId;
            auto in_room = acc_cache->in_room;
            auto in_party = acc_cache->in_party;
            auto party_id = acc_cache->party_id;
            auto room_id = acc_cache->room_id;
            auto team_id = acc_cache->team_id;
            auto plaza_id = acc_cache->plaza_id;
            auto friends = main_server->GetFriendsList(session_id);
            auto my_unique_id = NetEngine::Packets::Core::UniqueId(session_id, 1).data;
            auto acc_info = acc_cache->acc_info;
            auto inventory_items = acc_cache->inventory_items;
            auto items_added = acc_cache->items_added;
            auto items_deleted = acc_cache->items_deleted;
            auto items_updated = acc_cache->items_updated;
            auto friends_accepted = acc_cache->friends_accepted;
            auto friends_deleted = acc_cache->friends_deleted;
            auto blockeds_added = acc_cache->blockeds_added;
            auto blockeds_deleted = acc_cache->blockeds_deleted;
            acc_cache.unlock();

            CServer* server = session->GetServer();
            

            main_server->SendFrontIpc(PacketIds::Ipc::MainToFrontDisconnectPlayer, Utility::ToVector(auth_key));
            main_server->SendCastIpc(PacketIds::Ipc::MainToCastDisconnectPlayer, Utility::ToVector(auth_key));

            if (clan_id)
            {
                if (main_server->IsClanAlready(clan_id))
                {
                    auto clan = main_server->GetClanCacheUnique(clan_id);
                    if (main_server->IsSessionIdAlready(session_id, clan->online_members))
                    {
                        auto remove_myself = std::remove(clan->online_members.begin(), clan->online_members.end(), session_id);
                        clan->online_members.erase(remove_myself, clan->online_members.end());
                    }
                    if (clan->online_members.empty())
                        main_server->RemoveClanCache(clan_id);
                }
            }


            if (in_room && main_server->IsRoomAlready(room_id))
            {
                auto room = main_server->GetRoomCacheUnique(room_id);

                auto left_while_vote_kicked = room->vote_kick_target_session_id == session_id;

                if (left_while_vote_kicked)
                {
                    room->voters_session_ids.clear();
                    room->vote_kick_target_session_id = 0;
                    room->is_kick_vote_running = false;
                    if (!main_server->IsSessionIdAlready(session_id, room->kicked_session_ids))
                        room->kicked_session_ids.push_back(session_id);
                }

                //acc_cache.unlock();
                main_server->RemoveRoomPlayerCache(room, session_id, team_id);
                main_server->RoomPlayersSlotReorder(room);



                auto isFreeForAllOrSimilarMode = !main_server->IsModeTeamBased(room->ModeIndex);
                std::vector<std::pair<std::uint16_t, std::uint32_t>> player_slot_pairs;


                auto process_team = [&](const std::vector<std::uint16_t>& session_ids)
                {
                    for (const auto& id : session_ids)
                    {
                        auto player_cache = main_server->GetAccCacheSharedBySessionId(id);
                        if (player_cache->acc_info.Index != -1 && player_cache->in_room && player_cache->room_id == room->room_id)
                            player_slot_pairs.emplace_back(id, player_cache->slot_id);

                        player_cache.unlock();
                    }
                };

                if (isFreeForAllOrSimilarMode)
                    process_team(room->neutralteam_session_ids);
                else
                {
                    process_team(room->blueteam_session_ids);
                    process_team(room->redteam_session_ids);
                }
                process_team(room->observers_session_ids);

                std::sort(player_slot_pairs.begin(), player_slot_pairs.end(),
                    [](const std::pair<std::uint32_t, int>& a, const std::pair<std::uint32_t, int>& b) {
                    return a.second < b.second;
                });


                auto notify_player_leave = [&](std::uint32_t room_player_session_id)
                {
                    if (room_player_session_id == session_id) return;;
                    if (auto player_session = server->GetSessionById(room_player_session_id))
                    {
                        CMessage playerLeaveInfoAck(player_session->GetEncryptionKey());
                        playerLeaveInfoAck.SetSession(room_player_session_id);
                        playerLeaveInfoAck.SetCommand(422, 0, 0, my_slot);
                        playerLeaveInfoAck.SetData(reinterpret_cast<uint8_t*>(&my_unique_id), sizeof(my_unique_id));
                        player_session->Send(playerLeaveInfoAck);
                    }
                };

                //acc_cache.lock();


                CMessage leaveRoomAck = CMessage(session->GetEncryptionKey());
                leaveRoomAck.SetSession(session_id);
                leaveRoomAck.SetCommand(141, 0, NetEngine::Room::Leave::Ack::Result::Leave, 0);
                session->Send(leaveRoomAck);

                for (const auto& [room_player_session_id, _] : player_slot_pairs)
                    notify_player_leave(room_player_session_id);

                if (!room->neutralteam_session_ids.empty() || !room->redteam_session_ids.empty() || !room->blueteam_session_ids.empty())
                {
                    if (room->host_session_id == session_id)
                    {

                        auto best_ping_session_id = main_server->GetBestPlayerPingSessionIdInMatch(room);
                        if (best_ping_session_id != 0)
                        {
                            auto best_ping_acc_cache = main_server->GetAccCacheUniqueBySessionId(best_ping_session_id);
                            if (best_ping_acc_cache->acc_info.Index != -1)
                            {
                                auto best_ping_slot_id = best_ping_acc_cache->slot_id;
                                acc_cache.lock();
                                best_ping_acc_cache->slot_id = acc_cache->slot_id;
                                acc_cache->slot_id = 0xFF;
                                room->host_session_id = best_ping_session_id;
                                for (const auto& [room_player_session_id, _] : player_slot_pairs)
                                    if (auto player_session = server->GetSessionById(room_player_session_id))
                                        send_msg(player_session.get(), 128, 0, 1, static_cast<std::uint8_t>(best_ping_slot_id)); // host change

                                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "room No. ({}) changed host ({}) -> ({}) due to leaving while playing. ", room->room_id, acc_cache->acc_info.Nickname.c_str(), best_ping_acc_cache->acc_info.Nickname.c_str());

                                struct RoomAuthData
                                {
                                    std::uint16_t room_id;
                                    std::uint64_t auth_key;
                                };
                                RoomAuthData new_host_data{ room->room_id, best_ping_acc_cache->acc_info.AuthKey };

                                main_server->SendCastIpc(PacketIds::Ipc::MainToCastHostChange, Utility::ToVector(new_host_data));
                            }
                        }
                    }
                }

                

                if (room->neutralteam_session_ids.empty() && room->redteam_session_ids.empty() && room->blueteam_session_ids.empty() && room->observers_session_ids.empty())
                {
                    main_server->RemoveRoomCache(room_id);
                    server->SetRoomIdAvailable(room_id);
                }
            }

            if (in_party && main_server->IsPartyAlready(party_id))
            {
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "now leave party on disconnect");
                acc_cache.lock();
                auto party_id = acc_cache->party_id;
                auto party_cache = main_server->GetPartyCacheUnique(party_id);

                if (party_cache->party_host_session_id == session_id) {
                    BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "party will need change host");
                    if (!in_room)
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

                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "player ({}) left party id: ({})", acc_cache->acc_info.Nickname.c_str(), party_id);
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "now party have member count: ({})", party_cache->members.size());

                if (party_cache->members.size() == 0) {
                    BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "party is empty so will be deleted id: ({})", party_id);
                    main_server->RemovePartyCache(party_id);
                    main_server->SetQueuePartyIdAvailable(party_id);
                }
                acc_cache.unlock();
            }

            if (main_server->IsPlazaAlready(plaza_id))
            {
                auto current_plaza = main_server->GetPlazaCacheUnique(plaza_id);
                if (main_server->IsSessionIdAlready(session_id, current_plaza->session_ids))
                {
                    if (main_server->IsPlazaBroadcastable(current_plaza))
                    {
                        auto my_unique_id = NetEngine::Packets::Core::UniqueId(session_id, 1).data;
                        const auto& players_ids = current_plaza->session_ids;
                        for (const auto& plaza_player_session_id : players_ids)
                        {
                            if (plaza_player_session_id == session_id) continue;
                            if (auto player_session = server->GetSessionById(plaza_player_session_id))
                                send_msg(player_session.get(), 425, 0, 0, 1, reinterpret_cast<uint8_t*>(&my_unique_id), sizeof(my_unique_id));
                        }
                    }
                    BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "session id: ({}) left plaza id: ({})", session_id, plaza_id);
                    auto remove_myself = std::remove(current_plaza->session_ids.begin(), current_plaza->session_ids.end(), session_id);
                    current_plaza->session_ids.erase(remove_myself, current_plaza->session_ids.end());
                }
            }


            BaseLib::Database->UpdateFrontAccount(acc_info);
            const auto& [transform_items_added, transform_items_deleted] = main_server->TransformAddedAndDeletedItems(inventory_items, items_added, items_deleted);
            BaseLib::Database->InsertInventoryItems(acc_index, transform_items_added);
            BaseLib::Database->DeleteInventoryItems(acc_index, transform_items_deleted);
            const auto& transform_items_updated = main_server->TransformUpdatedItems(inventory_items, items_updated, items_deleted);
            BaseLib::Database->UpdateInventoryItems(acc_index, transform_items_updated);
            BaseLib::Database->InsertPlayerFriends(friends_accepted);
            BaseLib::Database->DeletePlayerFriends(friends_deleted);
            BaseLib::Database->InsertPlayerBlockeds(blockeds_added);
            BaseLib::Database->DeletePlayerBlockeds(blockeds_deleted);

            BaseLib::Database->UpdatePlayerDailyMission(acc_info.Index, acc_cache->daily_mission_info);

            BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "database updated account info ({})", acc_info.Nickname.c_str());

            auto notify_friend_logout = [&](const auto& friend_info)
            {
                if (auto friend_session = server->GetSessionById(friend_info.friend_session_id))
                {
                    CMessage friendsStatusUpdateAck(friend_session->GetEncryptionKey());
                    friendsStatusUpdateAck.SetSession(friend_session->GetSessionId());
                    friendsStatusUpdateAck.SetCommand(85, 0x0, Userlist::Friends::DetailsType::FriendState, Userlist::FriendsState::Logout);
                    friendsStatusUpdateAck.SetData(reinterpret_cast<uint8_t*>(&acc_index), sizeof(acc_index));
                    friend_session->Send(friendsStatusUpdateAck);
                }
            };
            std::vector<FriendInfo> current_friends_accepted;

            for (const auto& friend_info : *friends)
            {
                if (friend_info.state != Userlist::Friends::State::Accepted) continue;
                friends_accepted.push_back(friend_info); 
            }
            for (const auto& friend_info : friends_accepted)
            {
                // Check if `friend_info` is already in `current_friends_accepted`
                auto it = std::find_if(
                    current_friends_accepted.begin(),
                    current_friends_accepted.end(),
                    [&friend_info](const FriendInfo& existing_friend)
                {
                    return existing_friend.friend_account_id == friend_info.friend_account_id;
                });

                // Add only if not found
                if (it == current_friends_accepted.end())
                {
                    current_friends_accepted.push_back(friend_info);
                }
            }

            for (const auto& friend_info : friends_deleted)
            {
                current_friends_accepted.erase(
                    std::remove_if(
                        current_friends_accepted.begin(),
                        current_friends_accepted.end(),
                        [&friend_info](const FriendInfo& accepted_friend_info)
                {
                    return accepted_friend_info.friend_account_id == friend_info.friend_account_id;
                }
                    ),
                    current_friends_accepted.end()
                );
            }

            for (const auto& blocked_info : blockeds_added)
            {
                current_friends_accepted.erase(
                    std::remove_if(
                        current_friends_accepted.begin(),
                        current_friends_accepted.end(),
                        [&blocked_info](const FriendInfo& accepted_friend_info)
                {
                    return accepted_friend_info.friend_account_id == blocked_info.blocked_account_id;
                }
                    ),
                    current_friends_accepted.end()
                );
            }

            friends.unlock();

            for (const auto& friend_info : current_friends_accepted)
            {
                notify_friend_logout(friend_info);
                main_server->RemovePlayerFriends(friend_info.friend_session_id, acc_index);
                main_server->AddPlayerFriends(friend_info.friend_session_id, { friend_info.friend_account_id, acc_index, Userlist::Friends::State::Accepted, 0,  acc_info.Nickname.c_str() });
            }

            main_server->RemoveAccCache(session_id);
            BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "removed acc cache for session id: ({})", session_id);
            main_server->RemoveFriendsCache(session_id);
            BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "removed friends cache for session id: ({})", session_id);
            main_server->RemoveBlockedsCache(session_id);
            BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "removed blockeds cache for session id: ({})", session_id);
            main_server->RemoveSession(session_id);
            BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "session id: ({}) disconnected", session_id);


            rapidjson::Document doc;
            doc.SetObject();
            rapidjson::Document::AllocatorType& allocator = doc.GetAllocator();

            rapidjson::Value main_object(rapidjson::kObjectType);

            main_object.AddMember("Index", acc_info.Index, allocator);
            main_object.AddMember("ClanId", acc_info.ClanId, allocator);
            main_object.AddMember("ClanKills", acc_info.ClanKills, allocator);
            main_object.AddMember("ClanDeaths", acc_info.ClanDeaths, allocator);
            main_object.AddMember("ClanAssists", acc_info.ClanAssists, allocator);
            main_object.AddMember("ClanContribution", acc_info.ClanContribution, allocator);
            main_object.AddMember("ClanWins", acc_info.ClanWins, allocator);
            main_object.AddMember("ClanLoses", acc_info.ClanLoses, allocator);
            main_object.AddMember("ClanDraws", acc_info.ClanDraws, allocator);
            main_object.AddMember("Nickname", rapidjson::Value(acc_info.Nickname.c_str(), allocator), allocator);
            main_object.AddMember("Level", acc_info.Level, allocator);
            main_object.AddMember("Experience", acc_info.Experience, allocator);
            main_object.AddMember("PlayTime", acc_info.PlayTime, allocator);
            main_object.AddMember("MutedUntil", acc_info.MutedUntil, allocator);
            main_object.AddMember("Coins", acc_info.Coins, allocator);
            main_object.AddMember("Energy", acc_info.Energy, allocator);
            main_object.AddMember("MicroPoints", acc_info.MicroPoints, allocator);
            main_object.AddMember("RockTokens", acc_info.RockTokens, allocator);
            main_object.AddMember("Coupons", acc_info.Coupons, allocator);
            main_object.AddMember("Wins", acc_info.Wins, allocator);
            main_object.AddMember("Loses", acc_info.Loses, allocator);
            main_object.AddMember("Draws", acc_info.Draws, allocator);
            main_object.AddMember("Kills", acc_info.Kills, allocator);
            main_object.AddMember("Deaths", acc_info.Deaths, allocator);
            main_object.AddMember("Assists", acc_info.Assists, allocator);
            main_object.AddMember("Headshots", acc_info.Headshots, allocator);
            main_object.AddMember("HighestKillStreak", acc_info.HighestKillStreak, allocator);
            main_object.AddMember("MeleeKills", acc_info.MeleeKills, allocator);
            main_object.AddMember("RifleKills", acc_info.RifleKills, allocator);
            main_object.AddMember("ShotgunKills", acc_info.ShotgunKills, allocator);
            main_object.AddMember("SniperKills", acc_info.SniperKills, allocator);
            main_object.AddMember("GatlingKills", acc_info.GatlingKills, allocator);
            main_object.AddMember("BazookaKills", acc_info.BazookaKills, allocator);
            main_object.AddMember("GrenadeKills", acc_info.GrenadeKills, allocator);
            main_object.AddMember("ZombieKills", acc_info.ZombieKills, allocator);
            main_object.AddMember("Infections", acc_info.Infections, allocator);
            main_object.AddMember("SingleWaveHighestWave", acc_info.SingleWaveHighestWave, allocator);
            main_object.AddMember("SingleWaveHighScore", acc_info.SingleWaveHighScore, allocator);

            doc.AddMember("mainServerIpc", main_object, allocator);

            rapidjson::StringBuffer payload;
            rapidjson::Writer<rapidjson::StringBuffer> writer(payload);
            doc.Accept(writer);

            main_server->WebsitePost("/", payload.GetString());

            /*
            asio::post([session, main_server, send_msg]()
            {
                
            });
            */
        }
    }  
}