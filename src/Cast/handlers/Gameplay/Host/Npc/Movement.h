#pragma once
namespace Game::Handlers
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Cast;
#pragma pack(push, 1)
    struct npc_mov_data
    {
        uint32_t npc_id;//0
        uint16_t x, y, z; //4,6,8
        uint16_t x2, y2, z2; //10,12,14
        uint16_t x3, y3, z3; //16,18,20
        uint8_t bIdk1; // 22
        uint8_t State; // 23
        uint16_t rotation; // 24
        uint16_t idk; // 26
        uint32_t data; // 28
    };
#pragma pack(pop)
    inline void NpcMovement(SCallbackData& callback, CCastServer* server)
    {

        auto session = callback.session;
        auto message = callback.message;
        if (!session || !message) return;

        auto order = magic_enum::enum_cast<EOrder>(u16_cast(message->GetOrder())).value_or(EOrder::NONE);

        auto hostSid = session->GetSessionId();
        auto host = CAccount.get<shared_t>(hostSid);
        if (!host) return;
        auto room_id = host->room_id;
        host.unlock();

        auto room = CRoom.get<shared_t>(room_id);
        if (!room)
        {
            DEBUGLOG(red, "NpcMovement: invalid room for hostSid=({})", hostSid);
            return;
        }

        auto player_ids = room->players_session_id;
        room.unlock();
        server->Broadcast(player_ids, *message, hostSid);
    }
}