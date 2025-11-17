#pragma once
namespace Game::Handlers
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Main;
    inline void RoomLeave(SCallbackData& callback, CMainServer* main_server)
    {
        auto session = callback.session;
        auto message = callback.message;
        if (!session || !message) return;

        //std::shared_lock lock(session->GetMutex());
        CServer* server = callback.server;
        auto extra = callback.message->GetExtra();
        auto session_id = session->GetSessionId();
        auto leaveRoomReq = reinterpret_cast<MainLeaveRoomReq*>(message->GetData());
        auto target_unique_id = NetEngine::Packets::Core::UniqueId(leaveRoomReq->uniqueId);
        if (extra == 28) // kick player
        {
            auto acc_cache = CAccount.get<unique_t>(session_id);
            auto acc_index = acc_cache->acc_info.Index;

            if (acc_index == -1) return;
            if (!acc_cache->in_room || !CRoom.contains(acc_cache->room_id)) return;
            auto room = CRoom.get<unique_t>(acc_cache->room_id);
            if (room->host_session_id != session_id) return;
            if (target_unique_id.session == session_id) return;
            acc_cache.unlock();

            auto target_acc_cache = CAccount.get<unique_t>(target_unique_id.session);
            auto target_acc_index = target_acc_cache->acc_info.Index;
            if (target_acc_index == -1) return;
            if (!target_acc_cache->in_room || target_acc_cache->room_id != room->room_id) return;
			if (room->kicked.contains(acc_index)) return;
            auto target_team_id = target_acc_cache->team_id;
            auto target_slot = target_acc_cache->slot_id;
            DEBUGLOG(dark_cyan, "player ({}) force kicked by host from room -> id: ({})", target_acc_cache->acc_info.Nickname.c_str(), room->room_id);
            target_acc_cache.unlock();

            main_server->NewRemoveRoomPlayer(room, target_unique_id.session, target_team_id, NetEngine::Room::Leave::Ack::Result::KickedByHost, true);
        }
        else
        {
            auto acc_cache = CAccount.get<unique_t>(session_id);
            auto acc_index = acc_cache->acc_info.Index;
            auto my_unique_id = NetEngine::Packets::Core::UniqueId(session_id, 1).data;
            auto my_slot = acc_cache->slot_id;
            auto my_team_id = acc_cache->team_id;
            auto leave_result = static_cast<NetEngine::Room::Leave::Req::Result>(callback.message->GetExtra());
            if (acc_index == -1) return;
            auto leaveRoomReq = reinterpret_cast<MainLeaveRoomReq*>(callback.message->GetData());
            if (leave_result != NetEngine::Room::Leave::Req::Result::Leave || !acc_cache->in_room || !CRoom.contains(acc_cache->room_id)) return;
            auto room = CRoom.get<unique_t>(acc_cache->room_id);
            auto room_id = room->room_id;
            acc_cache.unlock();
            main_server->NewRemoveRoomPlayer(room, session_id, my_team_id, NetEngine::Room::Leave::Ack::Result::Leave, true);
        }
    }
}