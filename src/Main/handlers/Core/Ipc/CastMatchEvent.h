#pragma once
namespace Game::Handlers
{
    using namespace BaseLib;
    using namespace NetEngine;

    [[nodiscard]] inline std::string_view ToMatchEventLabel(const uint8_t event_type)
    {
        switch (event_type)
        {
        case 1: return "respawn";
        case 2: return "bomb";
        case 3: return "pickup";
        case 4: return "round-end";
        default: return "unknown";
        }
    }

    // Receives a non-combat match-timeline event (respawn / bomb plant-defuse /
    // item pickup) relayed from a Cast host handler, resolves the actor's account
    // id, and buffers it on the room for per-match persistence at match end.
    inline void IpcCastMatchEvent(const std::vector<uint8_t>& payload, CMainServer* main_server)
    {
        if (payload.size() < sizeof(NetEngine::Packets::Ipc::CastMatchTimelineEvent))
        {
            DEBUGLOG(red, "IpcCastMatchEvent payload size too small: {}", payload.size());
            return;
        }

        const auto ev = Utility::FromVector<NetEngine::Packets::Ipc::CastMatchTimelineEvent>(payload);
        if (!ev.room_id || !ev.actor_sid || !ev.event_type)
        {
            DEBUGLOG(yellow, "IpcCastMatchEvent skip invalid room=({}) actorSid=({}) type=({})",
                ev.room_id, ev.actor_sid, static_cast<uint32_t>(ev.event_type));
            return;
        }
        if (!CRoom.contains(ev.room_id))
        {
            DEBUGLOG(yellow, "IpcCastMatchEvent skip missing room room=({})", ev.room_id);
            return;
        }

        auto room = CRoom.get<unique_t>(ev.room_id);
        if (!room)
            return;

        auto actor = CAccount.get<shared_t>(ev.actor_sid);
        if (!actor)
        {
            DEBUGLOG(yellow, "IpcCastMatchEvent skip missing actor actorSid=({})", ev.actor_sid);
            return;
        }
        const int32_t aid = actor->acc_info.Index;
        const auto actor_nick = actor->acc_info.Nickname;
        actor.unlock();
        if (!aid)
            return;

        room->match_events.push_back(PendingMatchEvent{
            .aid = aid,
            .event_type = ev.event_type,
            .sub_a = ev.sub_a,
            .sub_b = ev.sub_b,
            .value = ev.value,
            .event_ms = Utility::GetUtcTimeNowInMilliseconds() }); // epoch ms (matches sessions/match times)

        DEBUGLOG(green, "[Match Event] Room {}, {} (sid {}) [{}] a=({}) b=({}) value=({})",
            ev.room_id, actor_nick, ev.actor_sid, ToMatchEventLabel(ev.event_type),
            static_cast<uint32_t>(ev.sub_a), static_cast<uint32_t>(ev.sub_b), ev.value);
    }
}
