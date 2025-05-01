#pragma once
namespace Game
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Main;

    namespace Handlers
    {
        inline void UpdateRoomList(SCallbackData& callback, CMainServer* main_server)
        {
            auto session = callback.session;
            auto message = callback.message;
            if (!session || !message) return;
            std::shared_lock lock(session->GetMutex());

            CServer* server = callback.server;
            auto session_id = session->GetSessionId();
            auto acc_cache = main_server->GetAccCacheSharedBySessionId(session_id);
            auto acc_index = acc_cache->acc_info.Index;
            acc_cache.unlock();
            auto channel_id = message->GetOption() + 1;
            auto score_limit = message->GetExtra();
            if (acc_index == -1) return;
            
            std::shared_lock room_ids_lock(main_server->GetRoomIdsMutex());
            if (room_ids.size() <= 0)
            {
                session->SendMsg(142, 0x0, NetEngine::Room::List::Result::NoRooms, 0);
                return;
            }
            uint32_t max_batch_size = 31;
            uint32_t room_blocks_count = (room_ids.size() + max_batch_size - 1) / max_batch_size;
            for (uint32_t batch_id = 0; batch_id < room_blocks_count; batch_id++)
            {
                auto extra = (batch_id == 0) ? NetEngine::Room::List::SendRoom : NetEngine::Room::List::SendRoom2;
                std::vector<RoomListInfo> new_rooms;
                uint32_t start_index = batch_id * max_batch_size;
                uint32_t end_index = std::min(start_index + max_batch_size, static_cast<uint32_t>(room_ids.size()));
                for (auto i = start_index; i < end_index; i++)
                {
                    auto room = main_server->GetRoomCacheShared(room_ids[i]);
                    if (room->title.empty()) continue;
                    auto host_cache = main_server->GetAccCacheSharedBySessionId(room->host_session_id);
                    if (host_cache->acc_info.Index == -1) continue;
                    if (host_cache->in_party) continue;
                    auto room_size = main_server->IsModeTeamBased(room->ModeIndex) ? room->redteam_session_ids.size() + room->blueteam_session_ids.size() : room->neutralteam_session_ids.size();
                    auto new_roomListInfo = RoomListInfo(room->title.c_str(), room->room_id, room->channel_id, room->MapIndex, room->ModeIndex, room->max_players, room_size, room->is_playing, room->has_password, room->allow_observers, room->Restriction, 1, host_cache->ping);
                    new_rooms.push_back(new_roomListInfo);
                }
                auto rooms_data = MainRoomListInfoAck(static_cast<uint16_t>(new_rooms.size()), static_cast<uint16_t>(room_ids.size()), new_rooms).Serialize(extra);
                session->SendMsg(142, 0, extra, 0, reinterpret_cast<uint8_t*>(rooms_data.data()), rooms_data.size());
            }
        }
    }
}