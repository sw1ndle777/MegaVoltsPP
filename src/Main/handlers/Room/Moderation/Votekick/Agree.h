#pragma once
namespace Game::Handlers
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Main;
    inline void VotekickAgree(SCallbackData& callback, CMainServer* main_server)
    {
        auto session = callback.session;
        auto message = callback.message;
        if (!session || !message) return;
        //std::shared_lock lock(session->GetMutex());

        CServer* server = callback.server;

        auto session_id = session->GetSessionId();
        auto acc_cache = CAccount.get<shared_t>(session_id);
        auto acc_index = acc_cache->acc_info.Index;
		auto server_id = acc_cache->server_id;
		auto room_id = acc_cache->room_id;

        if (acc_index == -1 || !acc_cache->in_room || !CRoom.contains(room_id)) return;
        auto room_cache = CRoom.get<unique_t>(room_id);

        if (!room_cache->is_playing)
        {
            DEBUGLOG(dark_cyan,
                "player: ({}) tried to agree to vote kick while match is not playing",
                acc_cache->acc_info.Nickname.c_str());
            return;
        }

        if (room_cache->voters.contains(session_id))
        {
            DEBUGLOG(dark_cyan,
                "player: ({}) tried to agree to vote kick more than once",
                acc_cache->acc_info.Nickname.c_str());
            return;
        }
        DEBUGLOG(dark_cyan,
            "player: ({}) pressed Y in roomId=({})'s vote kick",
            acc_cache->acc_info.Nickname.c_str(), acc_cache->room_id);
		room_cache->voters.insert(session_id);

        auto targetAid = 0;
        if (session_id != room_cache->vote_kick_target_session_id)
        {
            auto targetcache = CAccount.get<shared_t>(room_cache->vote_kick_target_session_id);
            targetAid = targetcache->acc_info.Index;
            targetcache.unlock();

        }
        else
            targetAid = acc_index;

        RoomLogEntry room_log;
        room_log.aid = acc_index; 
        room_log.target_aid = targetAid;
        room_log.event_type = RoomLog::EventType::VoteKickAgreed;
        room_log.server_id = server_id;
        room_log.room_id = room_id;

        [[maybe_unused]] auto log_ignored = BaseLib::DbPool->submit_task([room_log]() mutable
            {
                BaseLib::Database->PersistRoomLogs({ room_log });
            });

    }
}