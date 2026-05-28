#pragma once
namespace Game::Handlers
{
    using namespace BaseLib;
    using namespace NetEngine;

    inline void UpdateCastPlayerRoomState(const uint16_t sid,
        const uint16_t room_id,
        const bool in_room,
        const bool is_playing,
        const bool reset_streaks)
    {
        auto player = CAccount.get<unique_t>(sid);
        if (!player)
            return;

        player->room_id = room_id;
        player->in_room = in_room;
        player->state_id = in_room
            ? (is_playing ? PlayerInfo::State::Normal : PlayerInfo::State::WaitingRoom)
            : PlayerInfo::State::Lobby;

        if (reset_streaks)
        {
            player->current_kill_streak = 0;
            player->highest_kill_streak = 0;
        }
    }

    inline void EnsureCastRoomExists(const uint16_t room_id, const uint16_t host_sid, const bool is_playing)
    {
        if (CRoom.contains(room_id))
        {
            auto room = CRoom.get<unique_t>(room_id);
            if (!room)
                return;

            room->host_session_id = host_sid;
            room->is_playing = is_playing;
            return;
        }

        Game::Room room{ room_id, host_sid };
        room.is_playing = is_playing;
        CRoom.insert(room_id, std::move(room));
        CRoomId.emplace_back(room_id);
    }

    inline void IpcMainRoomLifecycleSync(const std::vector<uint8_t>& payload, CCastServer* server)
    {
        if (!server || payload.size() < sizeof(NetEngine::Packets::Ipc::MainToCastRoomLifecycleSync))
        {
            DEBUGLOG(yellow, "IpcMainRoomLifecycleSync invalid payload size=({})", payload.size());
            return;
        }

        const auto sync = Utility::FromVector<NetEngine::Packets::Ipc::MainToCastRoomLifecycleSync>(payload);
        const auto is_playing = sync.is_playing != 0;

        using enum NetEngine::Packets::Ipc::MainToCastRoomLifecycleAction;
        switch (sync.action)
        {
        case Create:
        case Join:
        {
            EnsureCastRoomExists(sync.room_id, sync.host_sid, is_playing);
            auto room = CRoom.get<unique_t>(sync.room_id);
            if (!room)
                return;

            room->host_session_id = sync.host_sid;
            room->is_playing = is_playing;
            if (sync.sid && !std::ranges::contains(room->players_session_id, sync.sid))
                room->players_session_id.push_back(sync.sid);

            room.unlock();
            if (sync.sid)
                UpdateCastPlayerRoomState(sync.sid, sync.room_id, true, is_playing, false);

            DEBUGLOG(dark_cyan,
                "IpcMainRoomLifecycleSync action=({}) roomId=({}) sid=({}) hostSid=({}) playing=({})",
                sync.action == Create ? "create" : "join",
                sync.room_id,
                sync.sid,
                sync.host_sid,
                is_playing ? "true" : "false");
            break;
        }
        case Leave:
        {
            if (CRoom.contains(sync.room_id))
            {
                auto room = CRoom.get<unique_t>(sync.room_id);
                if (room)
                {
                    if (sync.host_sid)
                        room->host_session_id = sync.host_sid;
                    room->is_playing = is_playing;
                    std::erase(room->players_session_id, sync.sid);
                }
            }

            if (sync.sid)
                UpdateCastPlayerRoomState(sync.sid, 0, false, false, true);

            DEBUGLOG(dark_cyan,
                "IpcMainRoomLifecycleSync action=(leave) roomId=({}) sid=({}) hostSid=({}) playing=({})",
                sync.room_id,
                sync.sid,
                sync.host_sid,
                is_playing ? "true" : "false");
            break;
        }
        case Remove:
        {
            std::vector<uint16_t> room_players;
            if (CRoom.contains(sync.room_id))
            {
                auto room = CRoom.get<unique_t>(sync.room_id);
                if (room)
                    room_players = room->players_session_id;
                room.unlock();
                CRoom.erase(sync.room_id);
                CRoomId.erase_value(sync.room_id);
            }

            for (const auto sid : room_players)
                UpdateCastPlayerRoomState(sid, 0, false, false, true);

            DEBUGLOG(dark_cyan, "IpcMainRoomLifecycleSync action=(remove) roomId=({}) players=({})",
                sync.room_id,
                room_players.size());
            break;
        }
        case MatchState:
        {
            EnsureCastRoomExists(sync.room_id, sync.host_sid, is_playing);
            auto room = CRoom.get<unique_t>(sync.room_id);
            if (!room)
                return;

            room->host_session_id = sync.host_sid;
            room->is_playing = is_playing;
            const auto room_players = room->players_session_id;
            room.unlock();

            for (const auto sid : room_players)
                UpdateCastPlayerRoomState(sid, sync.room_id, true, is_playing, true);

            DEBUGLOG(dark_cyan,
                "IpcMainRoomLifecycleSync action=(match-state) roomId=({}) hostSid=({}) playing=({}) players=({})",
                sync.room_id,
                sync.host_sid,
                is_playing ? "true" : "false",
                room_players.size());
            break;
        }
        default:
            DEBUGLOG(yellow, "IpcMainRoomLifecycleSync unknown action=({}) roomId=({})", static_cast<uint32_t>(sync.action), sync.room_id);
            break;
        }
    }
}
