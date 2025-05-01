#pragma once
namespace Game
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Main;

    namespace Handlers
    {
        inline void PlayerPing(SCallbackData& callback, CMainServer* main_server)
        {
            auto session = callback.session;
            auto message = callback.message;
            if (!session || !message) return;
            std::shared_lock lock(session->GetMutex());

            CServer* server = callback.server;
            auto session_id = session->GetSessionId();
            auto acc_cache = main_server->GetAccCacheUniqueBySessionId(session_id);
            auto acc_index = acc_cache->acc_info.Index;
            auto in_room = acc_cache->in_room;
            auto room_id = acc_cache->room_id;
            acc_cache->sent_ping_once = true;
            if (acc_index == -1 || message->GetMission() != 1)
            {
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "player session ({})'s ping couldn't update due to its account index being -1 or mission header being 1", session_id);
                return;
            }
                

            auto ping_data = reinterpret_cast<PlayerPingUpdateInfo*>(message->GetData());
            acc_cache->ping = ping_data->ping;
            acc_cache->fps_limit = message->GetOption();
            acc_cache.unlock();
            //BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "player ({}) has ping: ({})", acc_cache->acc_info.Nickname.c_str(), acc_cache->ping);
           
            if (!in_room || !main_server->IsRoomAlready(room_id)) return;

            auto room = main_server->GetRoomCacheShared(room_id);

            auto player_ids = main_server->GetRoomSortedPlayerSessionIds(room);
            if (message->GetMission() == 1) for (const auto& id : player_ids)
            {
                if (id == session_id) continue;
                auto update_data = MainRoomPlayersUpdatePingInfoAck(*ping_data, { session_id, 1 }).Serialize();
                if (auto player_session = server->GetSessionById(id))
                    player_session->SendMsg(message->GetOrder() + 1, message->GetMission() + 1, 0, message->GetOption(), reinterpret_cast<uint8_t*>(update_data.data()), static_cast<uint16_t>(update_data.size()));
            }
        }
    }
}