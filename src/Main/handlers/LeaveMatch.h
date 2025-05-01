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
            auto session = callback.session;
            auto message = callback.message;
            if (!session || !message) return;

            std::shared_lock lock(session->GetMutex());
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
            acc_cache.lock();
            acc_cache->acc_info.Loses += 1;
            if (room_cache->is_clan_room)
            {
                acc_cache->acc_info.ClanLoses += 1;
            }
            acc_cache.unlock();

            if (left_while_vote_kicked)
            {
                room_cache->voters_session_ids.clear();
                room_cache->vote_kick_target_session_id = 0;
                room_cache->is_kick_vote_running = false;
                if (!main_server->IsSessionIdAlready(acc_index, room_cache->kicked_index_ids))
                    room_cache->kicked_session_ids.push_back(acc_index);
            }

            for (const auto& id : players)
            {
                if (auto player_session = server->GetSessionById(id))
                    player_session->SendMsg(callback.message->GetOrder(), 0, 0, 0, reinterpret_cast<uint8_t*>(&my_unique_id), sizeof(my_unique_id)); // notify player leave room
            }
            const uint32_t& total_players_playing = static_cast<uint32_t>(std::count_if(players.begin(), players.end(),
                [main_server](const auto& id)
            {
                auto player_acc_cache = main_server->GetAccCacheSharedBySessionId(id);
                auto is_playing = player_acc_cache->playing;
                player_acc_cache.unlock();
                return is_playing;
            }));
            uint32_t total_players_room = static_cast<uint32_t>(players.size());
            if(total_players_room == 1) room_cache->is_playing = false;
            if (total_players_playing > 0)
            {
                if (room_cache->host_session_id == session_id)
                {
                    acc_cache.lock();
                    auto my_team_id = acc_cache->team_id;
                    acc_cache.unlock();
                    main_server->NewRemoveRoomPlayer(room_cache, session_id, my_team_id, left_while_vote_kicked ? NetEngine::Room::Leave::Ack::Result::KickedByKickVote : NetEngine::Room::Leave::Ack::Result::Leave, true);
                }
                else if (left_while_vote_kicked && room_cache->host_session_id != session_id)
                {
                    acc_cache.lock();
                    auto my_team_id = acc_cache->team_id;
                    acc_cache.unlock();
                    main_server->NewRemoveRoomPlayer(room_cache, session_id, my_team_id, NetEngine::Room::Leave::Ack::Result::KickedByKickVote, true);
                }
                else
                {
                    BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "player wasnt special case state: ({}), now room is playing: ({})", static_cast<uint32_t>(acc_cache->state), room_cache->is_playing);
                    acc_cache->playing = false;
                }
                if (in_party)
                {
                    acc_cache.lock();
                    BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "leave match while in party battle!");
                    if (acc_cache->in_room)
                    {
                        BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "leave party battle didnt assure leaving room");
                        auto my_team_id = acc_cache->team_id;
                        acc_cache.unlock();
                        main_server->NewRemoveRoomPlayer(room_cache, session_id, my_team_id, NetEngine::Room::Leave::Ack::Result::Leave, true);
                        acc_cache.lock();
                    }
                    session->SendMsg(120, 0, 45, 0);
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
                                    player_session->SendMsg(120, 0, 45, 0);
                            }
                        }
                        uint16_t new_leader_index = 0;
                        uint16_t new_leader = 0;
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
                                player_session->SendMsg(114, 0, 1, static_cast<uint8_t>(new_leader_index));
                        }
                        party_cache->party_host_session_id = new_leader;
                    }


                    auto remove_myself = std::remove(party_cache->members.begin(), party_cache->members.end(), session_id);
                    party_cache->members.erase(remove_myself, party_cache->members.end());

                    for (const auto& party_member_session_id : party_cache->members)
                    {
                        if (auto player_session = server->GetSessionById(party_member_session_id))
                            player_session->SendMsg(419, 0, 0, 0, reinterpret_cast<uint8_t*>(&my_unique_id), sizeof(my_unique_id));
                    }

                    acc_cache->party_id = 0;
                    acc_cache->in_party = false;

                    //auto leavePartyReq = reinterpret_cast<MainLeavePartyReq*>(callback.message->GetData());
                    session->SendMsg(111, 0, 1, 0);

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