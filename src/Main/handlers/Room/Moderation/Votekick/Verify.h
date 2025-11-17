#pragma once
namespace Game::Handlers
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Main;
    inline void VotekickVerify(SCallbackData& callback, CMainServer* main_server)
    {
        auto session = callback.session;
        auto message = callback.message;
        if (!session || !message) return;
        //std::shared_lock lock(session->GetMutex());

        CServer* server = callback.server;

        auto session_id = session->GetSessionId();
        auto acc_cache = CAccount.get<shared_t>(session_id);
        auto acc_index = acc_cache->acc_info.Index;

        if (acc_index == -1)
        {
            DEBUGLOG(dark_cyan,
                "acc_index == -1");
            return;
        }
        if (!acc_cache->in_room)
        {
            DEBUGLOG(dark_cyan,
                "player: ({}) is not in roomId=({})",
                acc_cache->acc_info.Nickname.c_str(), acc_cache->room_id);
            return;
        }

        if (!CRoom.contains(acc_cache->room_id))
        {
            DEBUGLOG(dark_cyan,
                "roomId=({}) doesn't exist",
                acc_cache->room_id);
            return;
        }

        auto room_id = acc_cache->room_id;
        auto room_cache = CRoom.get<unique_t>(room_id);

        if (!room_cache->is_playing)
        {
            DEBUGLOG(dark_cyan,
                "player: ({}) tried to count votes from vote kick while match is not playing",
                acc_cache->acc_info.Nickname.c_str());
            return;
        }

        if (!room_cache->is_kick_vote_running)
        {
            DEBUGLOG(dark_cyan,
                "roomId=({})'s vote kick is no longer running",
                room_id);
            return;
        }
        DEBUGLOG(dark_cyan,
            "player: ({}) is vote kick initiator and started counting votes for vote kick",
            acc_cache->acc_info.Nickname.c_str());

        acc_cache.unlock();

        auto target_session_id = room_cache->vote_kick_target_session_id;

        auto target_acc_cache = CAccount.get<shared_t>(target_session_id);
        auto target_acc_index = target_acc_cache->acc_info.Index;
        target_acc_cache.unlock();

        auto room_playing_players = main_server->GetRoomSortedPlayerPlayingWithoutObserverSessionIds(room_cache);
        auto total_y_voters = room_cache->voters.size();
        auto total_n_voters = room_playing_players.size() - total_y_voters + 1;
        auto target_left = !std::ranges::contains(room_playing_players, target_session_id);
        auto getting_kicked = total_y_voters > total_n_voters || target_left;

        if (getting_kicked)
            room_cache->kicked.insert(target_acc_index);

        if (!getting_kicked)
        {
            room_cache->voters.clear();
            room_cache->vote_kick_target_session_id = 0;
            room_cache->is_kick_vote_running = false;
            return;
        }

        for (const auto& room_player_session_id : room_playing_players)
        {
            if (auto player_session = main_server->GetSessionById(room_player_session_id))
                player_session->SendMsg(125, 0, 42, static_cast<uint8_t>(total_y_voters));
        }

        room_cache->voters.clear();
        room_cache->vote_kick_target_session_id = 0;
        room_cache->is_kick_vote_running = false;

        DEBUGLOG(dark_cyan,
            "vote kicked player sid=({}) gonna get acc cache", target_session_id);
        auto player = CAccount.get<unique_t>(target_session_id);
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

        DEBUGLOG(dark_cyan,
            "player: ({}) got vote kicked out with Y: ({}) : N: ({}) from roomId=({})",
            player_nickname.c_str(), total_y_voters, total_n_voters, player_room_id);

        player.unlock();
        main_server->NewRemoveRoomPlayer(room_cache, player_session_id, player_team_id, NetEngine::Room::Leave::Ack::Result::KickedByKickVote, true);
    }
}