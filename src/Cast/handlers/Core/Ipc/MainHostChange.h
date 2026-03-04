#pragma once
namespace Game::Handlers
{
    using namespace BaseLib;
    using namespace NetEngine;
    struct RoomAuthData
    {
        uint16_t room_id;
        uint64_t auth_key;
    };
    inline void IpcMainHostChange(const std::vector<uint8_t>& payload, CCastServer* server)
    {
        
        auto room_auth_data = Utility::FromVector<RoomAuthData>(payload);
		auto roomId = room_auth_data.room_id;
        if (!CRoom.contains(roomId))
        {
            DEBUGLOG(red, "ipc change host failed, roomId=({}) not found", roomId);
            return;
        }
        auto sid = *CAuthKey.get<shared_t>(room_auth_data.auth_key);
        auto room = CRoom.get<unique_t>(roomId);
        if (!std::ranges::contains(room->players_session_id, sid))
        {
            DEBUGLOG(red, "ipc change host failed, roomId=({}) sid=({}) not in room", roomId, sid);
            return;
        }
        room->host_session_id = sid;
        DEBUGLOG(dark_cyan, "roomId=({}) new host sid=({})", roomId, sid);
    }
}