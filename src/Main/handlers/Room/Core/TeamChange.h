#pragma once
namespace Game::Handlers
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Main;
    inline void TeamChange(SCallbackData& callback, CMainServer* main_server)
    {
        auto session = callback.session;
        auto message = callback.message;
        if (!session || !message) return;

        //std::shared_lock lock(session->GetMutex());
        CServer* server = callback.server;
        auto session_id = session->GetSessionId();
        auto acc_cache = CAccount.get<unique_t>(session_id);
        auto acc_index = acc_cache->acc_info.Index;
        auto team_option = static_cast<NetEngine::Team::IdType>(callback.message->GetOption());
        bool broadcast = false;
        //DEBUGLOG(dark_cyan, "player ({}) attempt equip but can't find item in item info cache or it has no namme item id: ({})", acc_cache->acc_info.Nickname.c_str(), item.item_info.item_number.item_id);
        if (acc_index == -1 || !acc_cache->in_room || !CRoom.contains(acc_cache->room_id)) return;
		auto server_id = acc_cache->server_id;
        auto room_cache = CRoom.get<unique_t>(acc_cache->room_id);
		auto roomId = room_cache->room_id;
        auto hostAid = 0;
        if (session_id != room_cache->host_session_id)
        {
            auto host_cache = CAccount.get<shared_t>(room_cache->host_session_id);
            hostAid = host_cache->acc_info.Index;
            host_cache.unlock();

        }
        else
            hostAid = acc_index;
        auto isPlaying = acc_cache->playing;
        auto isHostObserver = (acc_cache->session_id == room_cache->host_session_id) && (team_option == NetEngine::Team::IdType::Observer);
        auto isNonHostNotWaiting = (acc_cache->session_id != room_cache->host_session_id) && (acc_cache->state != PlayerInfo::State::Waiting);
        auto is_mode_teambased = main_server->IsModeTeamBased(room_cache->ModeIndex);
        auto is_ZombieMode = room_cache->ModeIndex == NetEngine::Room::Mode::Index::ZombieMode;
        
        DEBUGLOG(dark_cyan, "player ({}) wants to change team to team id ({})", acc_cache->acc_info.Nickname.c_str(), message->GetOption());
        DEBUGLOG(dark_cyan, "is playing: ({})", isPlaying);
        auto self_remove = [&](auto& team_session_ids)
            {
                auto remove_myself = std::remove(team_session_ids.begin(), team_session_ids.end(), session_id);
                team_session_ids.erase(remove_myself, team_session_ids.end());
            };
        if (isPlaying && !is_ZombieMode)
        {
            session->SendMsg(313, 0, NetEngine::Team::Change::Result::MatchRunning, acc_cache->team_id);
            return;
        }
        if (isHostObserver)
        {
            session->SendMsg(313, 0, NetEngine::Team::Change::Result::CantChange, acc_cache->team_id);
            return;
        }
        if (isNonHostNotWaiting && !is_ZombieMode)
        {
            session->SendMsg(313, 0, NetEngine::Team::Change::Result::NoBehavior, acc_cache->team_id);
            return;
        }
        acc_cache->zombie_team = 0;
        if (is_ZombieMode)
        {
            acc_cache->zombie_team = team_option;
            broadcast = true;
        }
        else if (team_option == NetEngine::Team::IdType::Observer)
        {
            if (!room_cache->allow_observers || room_cache->observers_session_ids.size() >= 10)
            {
                session->SendMsg(313, 0, NetEngine::Team::Change::Result::TeamFull, acc_cache->team_id);
                return;
            }
            if(std::ranges::contains(room_cache->observers_session_ids, session_id))
            {
                session->SendMsg(313, 0, NetEngine::Team::Change::Result::NoBehavior, acc_cache->team_id);
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
            session->SendMsg(313, 0, NetEngine::Team::Change::Result::Success, team_option);
            broadcast = true;
        }
        else
        {
            if (!is_mode_teambased)
            {
                if (room_cache->neutralteam_session_ids.size() >= room_cache->max_players)
                {
                    session->SendMsg(313, 0, NetEngine::Team::Change::Result::TeamFull, acc_cache->team_id);
                    return;
                }
                if (std::ranges::contains(room_cache->neutralteam_session_ids, session_id))
                {
                    session->SendMsg(313, 0, NetEngine::Team::Change::Result::NoBehavior, acc_cache->team_id);
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
                session->SendMsg(313, 0, NetEngine::Team::Change::Result::Success, team_option);
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
                            session->SendMsg(313, 0, NetEngine::Team::Change::Result::TeamFull, acc_cache->team_id);
                            return;
                        }
                        self_remove(room_cache->blueteam_session_ids);
                        room_cache->redteam_session_ids.push_back(session_id);
                        acc_cache->team_id = Team::IdType::Red;
                        session->SendMsg(313, 0, NetEngine::Team::Change::Result::Success, team_option);
                        broadcast = true;
                    }
                    else if (acc_cache->team_id == Team::IdType::Red)
                    {
                        if (room_cache->blueteam_session_ids.size() >= room_cache->max_players / 2)
                        {
                            session->SendMsg(313, 0, NetEngine::Team::Change::Result::NoBehavior, acc_cache->team_id);
                            return;
                        }
                        self_remove(room_cache->redteam_session_ids);
                        room_cache->blueteam_session_ids.push_back(session_id);
                        acc_cache->team_id = Team::IdType::Blue;
                        session->SendMsg(313, 0, NetEngine::Team::Change::Result::Success, team_option);
                        broadcast = true;
                    }
                }
                else
                {
                    if (room_cache->blueteam_session_ids.size() <= room_cache->redteam_session_ids.size())
                    {
                        if (room_cache->blueteam_session_ids.size() >= room_cache->max_players / 2)
                        {
                            session->SendMsg(313, 0, NetEngine::Team::Change::Result::TeamFull, acc_cache->team_id);
                            return;
                        }
                        self_remove(room_cache->observers_session_ids);
                        room_cache->blueteam_session_ids.push_back(session_id);
                        acc_cache->team_id = Team::IdType::Blue;
                        session->SendMsg(313, 0, NetEngine::Team::Change::Result::Success, team_option);
                        broadcast = true;
                    }
                    else
                    {
                        if (room_cache->redteam_session_ids.size() >= room_cache->max_players / 2)
                        {
                            session->SendMsg(313, 0, NetEngine::Team::Change::Result::TeamFull, acc_cache->team_id);
                            return;
                        }
                        self_remove(room_cache->observers_session_ids);
                        room_cache->redteam_session_ids.push_back(session_id);
                        acc_cache->team_id = Team::IdType::Red;
                        session->SendMsg(313, 0, NetEngine::Team::Change::Result::Success, team_option);
                        broadcast = true;
                    }
                }
            }
        }

       

        if (broadcast)
        {

            RoomLogEntry room_log;
            room_log.aid = acc_index;
            room_log.event_type = RoomLog::EventType::TeamChanged;
            room_log.server_id = server_id;
            room_log.room_id = roomId;
            room_log.host_aid = hostAid;
            room_log.team_id = static_cast<uint8_t>(acc_cache->team_id);

            acc_cache.unlock();
            const auto& ids = main_server->GetRoomSortedPlayerSessionIds(room_cache);
            std::vector<PlayerRoomClanListInfo> players_clan_info;
            for (const auto& id : ids)
            {
                auto acc = CAccount.get<shared_t>(id);
                if (acc->acc_info.ClanId)
                {
                    if (CClan.contains(acc->acc_info.ClanId))
                    {
                        auto clan_info = CClan.get<shared_t>(acc->acc_info.ClanId);
                        auto info = PlayerRoomClanListInfo(acc->slot_id, clan_info->clan_name.c_str(), clan_info->logo_front, clan_info->logo_back, acc_cache->acc_info.ClanId, 0);
                        clan_info.unlock();
                        players_clan_info.push_back(info);
                    }
                }
                else
                    players_clan_info.push_back(PlayerRoomClanListInfo(acc->slot_id, "", 0, 0, 0, 0));
            }
            auto my_unique_id = NetEngine::Packets::Core::UniqueId(session_id, 1).data;
            for (const auto& id : ids)
            {
                if (auto player_session = server->GetSessionById(id))
                {
                    player_session->SendMsg(313, 0, NetEngine::Team::Change::Result::Success, team_option, reinterpret_cast<uint8_t*>(&my_unique_id), sizeof(my_unique_id));
                    player_session->SendMsg(409, 0, 37, players_clan_info.size(), reinterpret_cast<uint8_t*>(players_clan_info.data()), sizeof(PlayerRoomClanListInfo) * players_clan_info.size());
                }
            } 

            [[maybe_unused]] auto ignored = BaseLib::DbPool->submit_task([room_log]() mutable
                {
                    BaseLib::Database->PersistRoomLogs({ room_log });
                });

        }
    }
}