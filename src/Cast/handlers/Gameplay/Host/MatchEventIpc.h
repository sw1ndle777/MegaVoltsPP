#pragma once
namespace Game::Handlers
{
    using namespace BaseLib;
    using namespace NetEngine;

    using MatchEventType = NetEngine::Packets::Ipc::CastMatchEventType;

    // Relay a non-combat match-timeline event (respawn, bomb plant/defuse, item
    // pickup) from the host-authoritative Cast handler to Main, which resolves the
    // actor's account id and buffers it for per-match persistence (match timeline).
    inline void SendMatchTimelineEventIpc(CCastServer* server,
        const uint16_t room_id,
        const uint16_t actor_sid,
        const MatchEventType event_type,
        const uint8_t sub_a = 0,
        const uint8_t sub_b = 0,
        const uint32_t value = 0)
    {
        if (!server || !room_id || !actor_sid)
            return;

        NetEngine::Packets::Ipc::CastMatchTimelineEvent event{};
        event.room_id = room_id;
        event.actor_sid = actor_sid;
        event.event_type = static_cast<uint8_t>(event_type);
        event.sub_a = sub_a;
        event.sub_b = sub_b;
        event.value = value;

        server->SendMainIpc(PacketIds::Ipc::CastToMainMatchEvent, Utility::ToVector(event));
    }
}
