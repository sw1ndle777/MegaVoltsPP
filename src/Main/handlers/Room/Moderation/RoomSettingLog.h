#pragma once
namespace Game::Handlers
{
    using namespace BaseLib;

    // Emit a room-setting-change room log (host-only, pre-match paths). No-op when the value
    // didn't actually change, so re-applying the same setting doesn't spam the log. old/new go
    // into the generic OldValue/NewValue columns. Async via the DB pool, fire-and-forget.
    inline void LogRoomSettingChange(int32_t aid, int32_t host_aid, uint32_t server_id, uint32_t room_id,
        RoomLog::EventType type, int32_t old_val, int32_t new_val)
    {
        if (old_val == new_val) return;
        RoomLogEntry log;
        log.aid = aid;
        log.host_aid = host_aid;
        log.server_id = server_id;
        log.room_id = room_id;
        log.event_type = type;
        log.old_value = old_val;
        log.new_value = new_val;
        [[maybe_unused]] auto ig = BaseLib::DbPool->submit_task([log]() mutable { BaseLib::Database->PersistRoomLogs({ log }); });
    }
}
