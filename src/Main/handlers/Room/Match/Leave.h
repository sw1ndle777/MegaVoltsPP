#pragma once
namespace Game::Handlers
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Main;
    inline void MatchLeave(SCallbackData& callback, CMainServer* main_server)
    {
        auto session = callback.session;
        auto message = callback.message;
        if (!session || !message) return;

        //std::shared_lock lock(session->GetMutex());
        CServer* server = callback.server;
        auto session_id = session->GetSessionId();
        auto acc_cache = CAccount.get<unique_t>(session_id);
        auto acc_index = acc_cache->acc_info.Index;
        auto my_unique_id = NetEngine::Packets::Core::UniqueId(session_id, 1).data;
        if (acc_index == -1 || !acc_cache->in_room || !CRoom.contains(acc_cache->room_id)) return;
        auto room_cache = CRoom.get<unique_t>(acc_cache->room_id);
        auto my_slot_id = acc_cache->slot_id;
        auto nick_copy = acc_cache->acc_info.Nickname.c_str();
        acc_cache->playing = false;
#if defined(RELEASE_1_0_3)
        acc_cache->state = (room_cache->host_session_id == session_id) ? PlayerInfo::State::HostReady : PlayerInfo::State::Waiting;
#else
        acc_cache->state = (room_cache->host_session_id == session_id) ? PlayerInfo::State::PlayerReady : PlayerInfo::State::Waiting;
#endif
        acc_cache->earnt_battery = 0;
        auto in_party = acc_cache->in_party;
        acc_cache.unlock();
        auto players = main_server->GetRoomSortedPlayerSessionIds(room_cache);

        auto left_while_vote_kicked = room_cache->vote_kick_target_session_id == session_id;

        DEBUGLOG(dark_cyan, "player leave match so will apply penality of 1 lose and 1 clan lose if clan room");
        acc_cache.lock();
        acc_cache->acc_info.Loses += 1;
        if (room_cache->is_clan_room)
        {
            acc_cache->acc_info.ClanLoses += 1;
        }
        acc_cache.unlock();

        if (left_while_vote_kicked)
        {
            room_cache->voters.clear();
            room_cache->vote_kick_target_session_id = 0;
            room_cache->is_kick_vote_running = false;
            if(!room_cache->kicked.contains(acc_index))
				room_cache->kicked.insert(acc_index);
        }

        for (const auto& id : players)
        {
            if (auto player_session = server->GetSessionById(id))
                player_session->SendMsg(callback.message->GetOrder(), 0, 0, 0, reinterpret_cast<uint8_t*>(&my_unique_id), sizeof(my_unique_id)); // notify player leave room
        }
        const uint32_t& total_players_playing = static_cast<uint32_t>(std::count_if(players.begin(), players.end(),
            [main_server](const auto& id)
            {
                auto player_acc_cache = CAccount.get<shared_t>(id);
                auto is_playing = player_acc_cache->playing;
                player_acc_cache.unlock();
                return is_playing;
            }));
        uint32_t total_players_room = static_cast<uint32_t>(players.size());
        if (total_players_room == 1)
        {
            room_cache->is_playing = false;
            main_server->SendCastRoomMatchStateSync(room_cache->room_id, room_cache->host_session_id, false);
        }
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
                acc_cache.lock();
                DEBUGLOG(dark_cyan, "player wasnt special case state: ({}), now room is playing: ({})", static_cast<uint32_t>(acc_cache->state), room_cache->is_playing);
                acc_cache->playing = false;
                acc_cache.unlock();
            }
            if (in_party)
            {
                acc_cache.lock();
                DEBUGLOG(dark_cyan, "leave match while in party battle!");
                if (acc_cache->in_room)
                {
                    DEBUGLOG(dark_cyan, "leave party battle didnt assure leaving room");
                    auto my_team_id = acc_cache->team_id;
                    acc_cache.unlock();
                    main_server->NewRemoveRoomPlayer(room_cache, session_id, my_team_id, NetEngine::Room::Leave::Ack::Result::Leave, true);
                    acc_cache.lock();
                }
                session->SendMsg(120, 0, 45, 0);
                auto party_id = acc_cache->party_id;
                auto party_cache = CParty.get<unique_t>(party_id);

                if (party_cache->party_host_session_id == session_id) {
                    DEBUGLOG(dark_cyan, "party will need change host");
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


                std::erase(party_cache->members, session_id);

                for (const auto& party_member_session_id : party_cache->members)
                {
                    if (auto player_session = server->GetSessionById(party_member_session_id))
                        player_session->SendMsg(419, 0, 0, 0, reinterpret_cast<uint8_t*>(&my_unique_id), sizeof(my_unique_id));
                }

                acc_cache->party_id = 0;
                acc_cache->in_party = false;

                //auto leavePartyReq = reinterpret_cast<MainLeavePartyReq*>(callback.message->GetData());
                session->SendMsg(111, 0, 1, 0);

                DEBUGLOG(dark_cyan, "player ({}) left party id: ({})", acc_cache->acc_info.Nickname.c_str(), party_id);
                DEBUGLOG(dark_cyan, "now party have member count: ({})", party_cache->members.size());

                if (party_cache->members.size() == 0) 
                {
                    DEBUGLOG(dark_cyan, "party is empty so will be deleted id: ({})", party_id);
                    party_cache.unlock();
					CParty->erase(party_id);
					CPartyId->erase_value(party_id);
                    main_server->SetQueuePartyIdAvailable(party_id);
                }
                else party_cache.unlock();
                acc_cache.unlock();
            }
        }
        DEBUGLOG(dark_cyan, "player ({}) left room match -> id: ({})", nick_copy, room_cache->room_id);
    }
}