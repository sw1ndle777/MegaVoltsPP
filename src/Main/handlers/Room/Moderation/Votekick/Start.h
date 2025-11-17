#pragma once
namespace Game::Handlers
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Main;
    inline void VotekickStart(SCallbackData& callback, CMainServer* main_server)
    {
        auto session = callback.session;
        auto message = callback.message;
        if (!session || !message) return;
        //std::shared_lock lock(session->GetMutex());

        CServer* server = callback.server;

        auto sid = session->GetSessionId();
        auto acc_cache = CAccount.get<unique_t>(sid);
        auto acc_index = acc_cache->acc_info.Index;
        auto order = message->GetOrder();
        auto option = message->GetOption();
        auto extra = message->GetExtra();
        auto mission = message->GetMission();
        auto mode_id = static_cast<NetEngine::Room::Mode::Index>(option);
        auto data_size = message->GetDataSize();
        auto room_id = acc_cache->room_id;

        if (acc_index == -1 || !acc_cache->in_room || !CRoom.contains(room_id)) return;
        auto acc_cache_nickname = acc_cache->acc_info.Nickname;
        auto room_cache = CRoom.get<unique_t>(room_id);
        if (!room_cache->is_playing)
        {
            DEBUGLOG(dark_cyan,
                "player: ({}) tried to vote kick while match is not playing",
                acc_cache_nickname.c_str());
            return;
        }
        if (room_cache->voteKickers.contains(sid))
        {
            DEBUGLOG(dark_cyan,
                "player: ({}) tried to vote kick more than once",
                acc_cache_nickname.c_str());
            return;
        }
        const auto& req = reinterpret_cast<MainVoteKickReq*>(message->GetData());
        auto target_uid = NetEngine::Packets::Core::UniqueId(req->target_unique_id);
        if (target_uid.session == sid)
        {
            DEBUGLOG(dark_cyan,
                "player: ({}) tried to vote kick himself",
                acc_cache_nickname.c_str());
            return;
        }
        if (acc_cache->acc_info.MicroPoints < 100) // 100mp cost for kick check
        {
            DEBUGLOG(dark_cyan,
                "player: ({}) tried to vote kick without enough mp",
                acc_cache_nickname.c_str());
            session->SendMsg(125, 0, 14, 0); // KICK_VOTE_ERROR_5
            return;
        }
        acc_cache.unlock();
        auto room_playing_players = main_server->GetRoomSortedPlayerPlayingWithoutObserverSessionIds(room_cache);
        if (!std::ranges::contains(room_playing_players, target_uid.session))
        {
            DEBUGLOG(dark_cyan,
                "player: ({}) tried to vote kick non playing player session: ({})",
                acc_cache_nickname.c_str(), static_cast<uint16_t>(target_uid.session));
            session->SendMsg(125, 0, 13, 0); // KICK_VOTE_ERROR_3
            return;
        }
        acc_cache.lock();
        DatabaseUpdateCtx dctx{ .sid = sid,.aid = acc_cache->acc_info.Index };
        using enum CurrencyType;
        dctx.ops.emplace_back(AccountCurrencyDelta{ .type = MP, .value = 100, .is_reward = false });

        auto validated = main_server->ValidateDatabaseUpdates(acc_cache, dctx);
        if (!validated.has_value())
        {
            DEBUGLOG(red, "ValidateDatabaseUpdates failed for [{}] [{}]: {}", acc_cache->acc_info.Index, acc_cache->acc_info.Nickname.c_str(), static_cast<int>(validated.error()));
            return;
        }
        acc_cache.unlock();
        room_cache.unlock();
        [[maybe_unused]] auto ignored = BaseLib::DbPool->submit_task([main_server,
            session = std::move(callback.session),
            s_id = sid,
            target_uid = target_uid,
            reason_id = req->reason_id,
            room_id = room_id,
            ids = std::move(room_playing_players),
            v = std::move(validated.value())
        ]() mutable
            {
                if (!session) return;
                ResultDbUpdateInfo dbres;
                if (!BaseLib::Database->UpdateAccount(v, dbres).has_value()) return;
                auto new_acc = CAccount.get<unique_t>(s_id);
                auto applied = main_server->ApplyDatabaseUpdates(new_acc, v);
                if (!applied.has_value())
                {
                    DEBUGLOG(red, "ApplyDatabaseUpdates failed for [{}] [{}]: {}", new_acc->acc_info.Index, new_acc->acc_info.Nickname.c_str(), static_cast<int>(applied.error()));
                    return;
                }
                auto target_acc = CAccount.get<shared_t>(target_uid.session);
                auto server_time = Utility::GetUtcTimeNowInMilliseconds() - main_server->GetStartTime();
                auto vote_kick_tick = (server_time + 30000) / 10;
                auto my_uid = NetEngine::Packets::Core::UniqueId(s_id, 1).data;
                auto voteKickAck = MainVoteKickAck(target_uid.data, my_uid, reason_id, vote_kick_tick);
                DEBUGLOG(dark_cyan,
                    "player: ({}) vote kicked other player: ({}) for reason id: ({})",
                    new_acc->acc_info.Nickname.c_str(), target_acc->acc_info.Nickname.c_str(), reason_id);

                auto room = CRoom.get<unique_t>(room_id);
                room->is_kick_vote_running = true;
				room->voters.insert(s_id);
				room->voteKickers.insert(s_id);
                room->vote_kick_target_session_id = target_uid.session;
                for (const auto& id : ids)
                    if (auto pss = main_server->GetSessionById(id))
                        pss->SendMsg(125, 0, 28, 1, reinterpret_cast<uint8_t*>(&voteKickAck), sizeof(MainVoteKickAck));
            });
    }
}