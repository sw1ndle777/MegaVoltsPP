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
            std::shared_lock lock(callback.session->GetMutex());
            CSession* session = callback.session;
            CServer* server = callback.server;
            auto session_id = session->GetSessionId();
            auto acc_cache = main_server->GetAccCacheUniqueBySessionId(session_id);
            auto acc_index = acc_cache->acc_info.Index;
            auto in_room = acc_cache->in_room;
            auto room_id = acc_cache->room_id;
            acc_cache->sent_ping_once = true;
            if (acc_index == -1 || callback.message->GetMission() != 1)
            {
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "player session ({})'s ping couldn't update due to its account index being -1 or mission header being 1", session_id);
                return;
            }
                

            auto ping_data = reinterpret_cast<PlayerPingUpdateInfo*>(callback.message->GetData());
            acc_cache->ping = ping_data->ping;
            acc_cache->fps_limit = callback.message->GetOption();
            acc_cache.unlock();
            //BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "player ({}) has ping: ({})", acc_cache->acc_info.Nickname.c_str(), acc_cache->ping);
           
            if (!in_room || !main_server->IsRoomAlready(room_id)) return;

            auto update_ping = [&](const auto& id)
            {
                if (id == session_id) return;
                //auto player_cache = main_server->GetAccCacheSharedBySessionId(id);
                //if (player_cache->acc_info.Index == -1) return;

                auto update_data = MainRoomPlayersUpdatePingInfoAck(*ping_data, { session_id, 1 }).Serialize();

                if (auto player_session = server->GetSessionById(id))
                {
                    CMessage updatePingAck(player_session->GetEncryptionKey());
                    updatePingAck.SetSession(id);
                    updatePingAck.SetCommand(callback.message->GetOrder() + 1, callback.message->GetMission() + 1, 0, callback.message->GetOption());
                    updatePingAck.SetData(reinterpret_cast<uint8_t*>(update_data.data()), static_cast<std::uint16_t>(update_data.size()));
                    player_session->Send(updatePingAck);
                }
            };

            auto room = main_server->GetRoomCacheShared(room_id);
            //BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "player ({}) broadcasted ping: ({}) to room id: ({})", acc_cache->acc_info.Nickname.c_str(), acc_cache->ping, room->room_id);
            
            auto player_ids = main_server->GetRoomSortedPlayerSessionIds(room);
            if (callback.message->GetMission() == 1) for (const auto& id : player_ids)
            {
                if (id == session_id) continue;
                update_ping(id);
            }
        }
    }
}