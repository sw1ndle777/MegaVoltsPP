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

        if (acc_index == -1 || !acc_cache->in_room || !CRoom.contains(acc_cache->room_id)) return;
        auto room_cache = CRoom.get<unique_t>(acc_cache->room_id);

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
    }
}