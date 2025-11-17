#pragma once
namespace Game::Handlers
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Main;
    inline void RoomModinfoFull(SCallbackData& callback, CMainServer* main_server)
    {
        auto session = callback.session;
        auto message = callback.message;
        if (!session || !message) return;
        //std::shared_lock lock(session->GetMutex());
        CServer* server = callback.server;
        auto broadcast = [&](std::vector<uint16_t>& ids, const uint8_t& order, const uint8_t& mission = 0, const uint8_t& extra = 0, const uint8_t& option = 0)
            {
                for (const auto& id : ids)
                    if (auto pss = server->GetSessionById(id))
                        pss->SendMsg(order, mission, extra, option, message->GetData(), message->GetDataSize());
            };

        auto session_id = session->GetSessionId();
        auto acc_cache = CAccount.get<shared_t>(session_id);
        auto acc_index = acc_cache->acc_info.Index;
        auto order = message->GetOrder();
        auto option = message->GetOption();
        auto extra = message->GetExtra();
        auto mission = message->GetMission();
        auto mode_id = static_cast<NetEngine::Room::Mode::Index>(option);
        auto data_size = message->GetDataSize();

        if (acc_index == -1 || !acc_cache->in_room || !CRoom.contains(acc_cache->room_id)) return;

        auto room_cache = CRoom.get<unique_t>(acc_cache->room_id);
        auto previous_mode = room_cache->ModeIndex;
        if (room_cache->is_playing || room_cache->host_session_id != session_id) return;
        acc_cache.unlock();
        auto players_ids = main_server->GetRoomSortedPlayerSessionIds(room_cache);

        const auto& settings_info = reinterpret_cast<RoomSettingsUpdateTitlePassword*>(message->GetData());
        room_cache->max_players = settings_info->update_info.max_players;
        room_cache->time_rule = settings_info->update_info.time;
        room_cache->score_rule = settings_info->update_info.score_limit;
        room_cache->Restriction = static_cast<NetEngine::Room::Restriction::Type>(settings_info->update_info.restriction);
        room_cache->allow_drops = settings_info->update_info.allow_items;
        room_cache->allow_intruders = settings_info->update_info.allow_intruders;
        room_cache->MapIndex = static_cast<NetEngine::Room::Map::Index>(settings_info->update_info.map_index);
        room_cache->TeamBalance = NetEngine::Room::Balance::State::Disabled;//static_cast<NetEngine::Room::Balance::State>(settings_info->update_info.team_balance);
        room_cache->ModeIndex = mode_id;
        room_cache->title = settings_info->title;
        room_cache->password = settings_info->password;
        room_cache->has_password = !room_cache->password.empty();

        message->SetData(reinterpret_cast<uint8_t*>(settings_info), data_size);
        broadcast(players_ids, order, mission, extra, option);
        BalanceTeams(main_server, previous_mode, room_cache, players_ids);
    }
}