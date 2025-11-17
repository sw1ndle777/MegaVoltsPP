#pragma once
namespace Game::Handlers
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Main;
    inline void SwapAndReorderTeamLists(CMainServer* server, RoomCacheResource& room, uint16_t old_sid, uint16_t new_sid)
    {
        auto old_acc = CAccount.get<unique_t>(old_sid);
        auto new_acc = CAccount.get<unique_t>(new_sid);
        auto old_slot = old_acc->slot_id;
        auto new_slot = new_acc->slot_id;
        old_acc->slot_id = new_slot;
        new_acc->slot_id = old_slot;
        auto old_team_id = old_acc->team_id;
        auto new_team_id = new_acc->team_id;
        old_acc.unlock();
        new_acc.unlock();
        server->ReorderTeamList(server->GetTeamList(room, old_team_id), old_sid);
        server->ReorderTeamList(server->GetTeamList(room, new_team_id), new_sid);
        DEBUGLOG(dark_cyan, "Swapped and reordered slots between old host ({}) and new host ({})", old_sid, new_sid);
    }
    inline std::optional<uint16_t> FindTargetSession(CMainServer* main_server, RoomCacheResource& room, uint8_t slot_id)
    {
        auto is_team_based = main_server->IsModeTeamBased(room->ModeIndex);
        const auto room_id = room->room_id;
        using enum PlayerInfo::State;
        auto try_sid = [&](uint16_t sid) -> std::optional<uint16_t>
            {
                auto pc = CAccount.get<shared_t>(sid);
                const bool cond =
                    pc->acc_info.Index != -1 &&
                    pc->in_room &&
                    pc->room_id == room_id &&
                    pc->slot_id == slot_id;

                pc.unlock();
                if (cond) return sid;
                return std::nullopt;
            };
        auto scan = [&](const auto& list) -> std::optional<uint16_t>
            {
                for (auto sid : list) if (auto hit = try_sid(sid)) return hit;
                return std::nullopt;
            };
        if (main_server->IsModeTeamBased(room->ModeIndex))
        {
            if (auto hit = scan(room->blueteam_session_ids)) return hit;
            if (auto hit = scan(room->redteam_session_ids))  return hit;
        }
        else
            if (auto hit = scan(room->neutralteam_session_ids)) return hit;

        return std::nullopt;
    }
    inline void HostChange(SCallbackData& callback, CMainServer* main_server)
    {
        auto session = callback.session;
        auto message = callback.message;
        if (!session || !message) return;
        //std::shared_lock lock(session->GetMutex());

        CServer* server = callback.server;
        auto session_id = session->GetSessionId();
        auto acc = CAccount.get<unique_t>(session_id);
        auto aid = acc->acc_info.Index;
        auto target_slot_id = message->GetOption();
        if (aid == -1) return;
        if (!CRoom.contains(acc->room_id))
        {
            session->SendMsg(128, 0, NetEngine::Room::ChangeHost::Result::Error, 0);
            return;
        }
        auto room = CRoom.get<unique_t>(acc->room_id);
        if (!acc->in_room || room->is_playing)
        {
            session->SendMsg(128, 0, NetEngine::Room::ChangeHost::Result::Error, 0);
            return;
        }
        acc.unlock();
        auto target_session_id = FindTargetSession(main_server, room, target_slot_id);
        if (!target_session_id.has_value() || *target_session_id == session_id)
        {
            session->SendMsg(128, 0, NetEngine::Room::ChangeHost::Result::NotInRoom, 0);
            return;
        }
        acc.lock();
        auto target_acc_cache = CAccount.get<unique_t>(*target_session_id);
        if (target_acc_cache->acc_info.Index == -1 || !target_acc_cache->in_room || target_acc_cache->room_id != room->room_id)
        {
            session->SendMsg(128, 0, NetEngine::Room::ChangeHost::Result::NotInRoom, 0);
            return;
        }
        if (room->host_session_id != session_id)
        {
            session->SendMsg(128, 0, NetEngine::Room::ChangeHost::Result::NotTheHost, 0);
            return;
        }
        room->host_session_id = *target_session_id;
        DEBUGLOG(dark_cyan, "current host have state: ({}) and target host state: ({})", static_cast<uint32_t>(acc->state), static_cast<uint32_t>(target_acc_cache->state));
        using enum PlayerInfo::State;
#if defined(RELEASE_1_0_3)
        if (target_acc_cache->state == Waiting) target_acc_cache->state = HostReady;
        if (acc->state == HostReady)  acc->state = Waiting;
#else
        if (target_acc_cache->state == Waiting) target_acc_cache->state = PlayerReady;
        if (acc->state == PlayerReady) acc->state = Waiting;
#endif
        struct RoomAuthData
        {
            uint16_t room_id;
            uint64_t auth_key;
        };
        RoomAuthData new_host_data{ room->room_id, target_acc_cache->acc_info.AuthKey };
        main_server->SendCastIpc(PacketIds::Ipc::MainToCastHostChange, Utility::ToVector(new_host_data));
        target_acc_cache.unlock();
        acc.unlock();
        SwapAndReorderTeamLists(main_server, room, session_id, *target_session_id);
        auto players_ids = main_server->GetRoomSortedPlayerSessionIds(room);
        for (const auto& id : players_ids)
            if (auto pss = server->GetSessionById(id))
                pss->SendMsg(128, 0, NetEngine::Room::ChangeHost::Result::Success, target_slot_id);
    }
}