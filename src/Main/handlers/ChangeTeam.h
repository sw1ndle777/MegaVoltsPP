#pragma once
namespace Game
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Main;

    namespace Handlers
    {
        inline void ChangeTeam(SCallbackData& callback, CMainServer* main_server)
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
            auto team_option = static_cast<NetEngine::Team::IdType>(callback.message->GetOption());
            bool broadcast = false;
            //BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "player ({}) attempt equip but can't find item in item info cache or it has no namme item id: ({})", acc_cache->acc_info.Nickname.c_str(), item.item_info.item_number.item_id);
            if (acc_index == -1 || !acc_cache->in_room || !main_server->IsRoomAlready(acc_cache->room_id)) return;
            auto room_cache = main_server->GetRoomCacheUnique(acc_cache->room_id);
            auto isPlaying = acc_cache->playing;
            auto isHostObserver = (acc_cache->session_id == room_cache->host_session_id) && (team_option == NetEngine::Team::IdType::Observer);
            auto isNonHostNotWaiting = (acc_cache->session_id != room_cache->host_session_id) && (acc_cache->state != PlayerInfo::State::Waiting);
            auto is_mode_teambased = main_server->IsModeTeamBased(room_cache->ModeIndex);
            auto is_ZombieMode = room_cache->ModeIndex == NetEngine::Room::Mode::Index::ZombieMode;
            BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "player ({}) wants to change team to team id ({})", acc_cache->acc_info.Nickname.c_str(), callback.message->GetOption());
            BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "is playing: ({})", isPlaying);
            auto self_remove = [&](auto& team_session_ids)
            {
                auto remove_myself = std::remove(team_session_ids.begin(), team_session_ids.end(), session_id);
                team_session_ids.erase(remove_myself, team_session_ids.end());
            };
            if (isPlaying && !is_ZombieMode)
            {
                send_msg(session, 313, 0, NetEngine::Team::Change::Result::MatchRunning, acc_cache->team_id);
                return;
            }
            if (isHostObserver)
            {
                send_msg(session, 313, 0, NetEngine::Team::Change::Result::CantChange, acc_cache->team_id);
                return;
            }
            if (isNonHostNotWaiting && !is_ZombieMode)
            {
                send_msg(session, 313, 0, NetEngine::Team::Change::Result::NoBehavior, acc_cache->team_id);
                return;
            }
            acc_cache->zombie_team = 0;
            if (is_ZombieMode)
            {
                acc_cache->zombie_team = team_option;
                //acc_cache->team_id = team_option;
                //send_msg(session, 313, 0, NetEngine::Team::Change::Result::Success, team_option);
                broadcast = true;
            }
            else if (team_option == NetEngine::Team::IdType::Observer)
            {
                if (!room_cache->allow_observers || room_cache->observers_session_ids.size() >= 10)
                {
                    send_msg(session, 313, 0, NetEngine::Team::Change::Result::TeamFull, acc_cache->team_id);
                    return;
                }
                if (main_server->IsSessionIdAlready(session_id, room_cache->observers_session_ids))
                {
                    send_msg(session, 313, 0, NetEngine::Team::Change::Result::NoBehavior, acc_cache->team_id);
                    return;
                }
                if (acc_cache->team_id == Team::IdType::Neutral)
                    self_remove(room_cache->neutralteam_session_ids);
                else if (acc_cache->team_id == Team::IdType::Blue)
                    self_remove(room_cache->blueteam_session_ids);
                else if (acc_cache->team_id == Team::IdType::Red)
                    self_remove(room_cache->redteam_session_ids);
                room_cache->observers_session_ids.push_back(session_id);
                acc_cache->team_id = Team::IdType::Observer;
                send_msg(session, 313, 0, NetEngine::Team::Change::Result::Success, team_option);
                broadcast = true;
            }
            else
            {
                if (!is_mode_teambased)
                {
                    if (room_cache->neutralteam_session_ids.size() >= room_cache->max_players)
                    {
                        send_msg(session, 313, 0, NetEngine::Team::Change::Result::TeamFull, acc_cache->team_id);
                        return;
                    }
                    if (main_server->IsSessionIdAlready(session_id, room_cache->neutralteam_session_ids))
                    {
                        send_msg(session, 313, 0, NetEngine::Team::Change::Result::NoBehavior, acc_cache->team_id);
                        return;
                    }
                    if (acc_cache->team_id == Team::IdType::Observer)
                        self_remove(room_cache->observers_session_ids);
                    else if (acc_cache->team_id == Team::IdType::Blue)
                        self_remove(room_cache->blueteam_session_ids);
                    else if (acc_cache->team_id == Team::IdType::Red)
                        self_remove(room_cache->redteam_session_ids);
                    room_cache->neutralteam_session_ids.push_back(session_id);
                    acc_cache->team_id = Team::IdType::Neutral;
                    send_msg(session, 313, 0, NetEngine::Team::Change::Result::Success, team_option);
                    broadcast = true;
                }
                else
                {
                    if (acc_cache->team_id != Team::IdType::Observer)
                    {
                        if (acc_cache->team_id == Team::IdType::Blue)
                        {
                            if (room_cache->redteam_session_ids.size() >= room_cache->max_players / 2)
                            {
                                send_msg(session, 313, 0, NetEngine::Team::Change::Result::TeamFull, acc_cache->team_id);
                                return;
                            }
                            self_remove(room_cache->blueteam_session_ids);
                            room_cache->redteam_session_ids.push_back(session_id);
                            acc_cache->team_id = Team::IdType::Red;
                            send_msg(session, 313, 0, NetEngine::Team::Change::Result::Success, team_option);
                            broadcast = true;
                        }
                        else if (acc_cache->team_id == Team::IdType::Red)
                        {
                            if (room_cache->blueteam_session_ids.size() >= room_cache->max_players / 2)
                            {
                                send_msg(session, 313, 0, NetEngine::Team::Change::Result::TeamFull, acc_cache->team_id);
                                return;
                            }
                            self_remove(room_cache->redteam_session_ids);
                            room_cache->blueteam_session_ids.push_back(session_id);
                            acc_cache->team_id = Team::IdType::Blue;
                            send_msg(session, 313, 0, NetEngine::Team::Change::Result::Success, team_option);
                            broadcast = true;
                        }
                    }
                    else
                    {
                        if (room_cache->blueteam_session_ids.size() <= room_cache->redteam_session_ids.size())
                        {
                            if (room_cache->blueteam_session_ids.size() >= room_cache->max_players / 2)
                            {
                                send_msg(session, 313, 0, NetEngine::Team::Change::Result::TeamFull, acc_cache->team_id);
                                return;
                            }
                            self_remove(room_cache->observers_session_ids);
                            room_cache->blueteam_session_ids.push_back(session_id);
                            acc_cache->team_id = Team::IdType::Blue;
                            send_msg(session, 313, 0, NetEngine::Team::Change::Result::Success, team_option);
                            broadcast = true;
                        }
                        else
                        {
                            if (room_cache->redteam_session_ids.size() >= room_cache->max_players / 2)
                            {
                                send_msg(session, 313, 0, NetEngine::Team::Change::Result::TeamFull, acc_cache->team_id);
                                return;
                            }
                            self_remove(room_cache->observers_session_ids);
                            room_cache->redteam_session_ids.push_back(session_id);
                            acc_cache->team_id = Team::IdType::Red;
                            send_msg(session, 313, 0, NetEngine::Team::Change::Result::Success, team_option);
                            broadcast = true;
                        }
                    }
                }
            }
            if (broadcast)
            {
                acc_cache.unlock();
                const auto& players_ids = main_server->GetRoomSortedPlayerSessionIds(room_cache);

                std::vector<PlayerRoomClanListInfo> players_clan_info;
                for (const auto& room_player_session_id : players_ids)
                {
                    auto player_cache = main_server->GetAccCacheSharedBySessionId(room_player_session_id);
                    if (player_cache->acc_info.ClanId)
                    {
                        if (main_server->IsClanAlready(player_cache->acc_info.ClanId))
                        {
                            auto clan_info = main_server->GetClanCacheShared(player_cache->acc_info.ClanId);
                            auto info = PlayerRoomClanListInfo(player_cache->slot_id, clan_info->clan_name.c_str(), clan_info->logo_front, clan_info->logo_back, acc_cache->acc_info.ClanId, 0);
                            clan_info.unlock();
                            players_clan_info.push_back(info);
                        }
                    }
                    else
                        players_clan_info.push_back(PlayerRoomClanListInfo(player_cache->slot_id, "", 0, 0, 0, 0));
                }

                auto my_unique_id = NetEngine::Packets::Core::UniqueId(session_id, 1).data;
                for (const auto& room_player_session_id : players_ids)
                {
                    //if (room_player_session_id == session_id) continue;
                    if (auto player_session = server->GetSessionById(room_player_session_id))
                    {
                        send_msg(player_session.get(), 313, 0, NetEngine::Team::Change::Result::Success, team_option, reinterpret_cast<uint8_t*>(&my_unique_id), sizeof(my_unique_id));

                        send_msg(player_session.get(), 409, 0, 37, players_clan_info.size(), reinterpret_cast<uint8_t*>(players_clan_info.data()), sizeof(PlayerRoomClanListInfo)* players_clan_info.size());
                    }
                }    
            }
        }
    }
    
}