#pragma once
#include "../Weapon/CombatIpc.h"
namespace Game::Handlers
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Cast;
#pragma pack(push, 1)
    struct ExPlayerUpdateInfoAck
    {
        NetEngine::Packets::Core::UniqueId uid;
        union
        {
            struct
            {
                uint32_t health : 20;
                uint32_t mode_index : 5;
                uint32_t player_status : 4;
                uint32_t unk : 3;
            };
            uint32_t data{};
        } info{};
    };
#pragma pack(pop)
    struct ExPlayerUpdateInfoReq
    {
        uint8_t m_bIdk1{};//0x0000
        uint8_t m_bIdk2{};//0x0001
        uint8_t m_bIdk3{};//0x0002
        uint8_t m_bIdk4{};//0x0003

        uint8_t m_bIdk5{};//0x0004
        uint8_t m_bIdk6{};//0x0005
        uint8_t m_bIdk7{};//0x0006
        uint8_t m_bIdk8{};//0x0007

        uint8_t m_bIdk9{};//0x0008
        uint8_t m_bIsDead{};//0x0009
        uint8_t m_bIdk11{};//0x000A
        uint8_t m_bIdk12{};//0x000B
        uint8_t m_bIdk13{};//0x000C
        uint8_t m_bIdk14{};//0x000D
        uint8_t m_bIdk15{};//0x000E
        uint8_t m_bIdk16{};//0x000F
        NetEngine::Packets::Core::UniqueId uid;
        uint32_t m_uiSpawnTick{};
    };

    // HOST_OTHER_STATUS (order 306) is the JOIN SNAPSHOT the host sends to a player who is
    // loading into an in-progress match (it fires only on join, never on combat deaths — those
    // flow through the attack handlers / CombatIpc). The host's per-player m_bIsDead in this
    // snapshot is momentary and unreliable (it has reported already-respawned players as dead),
    // which made late joiners render live players as corpses ("everyone invisible until I die").
    //
    // So, like ToyBattles' roomInfoJoinHandler (which fills playerState from the server's own
    // targetSession->isDead, NOT the packet), we answer the joiner from the SERVER's spawn flag:
    // is_dead is a spawn-state flag here — set true when a player syncs in with 0 health
    // (a mid-match joiner awaiting their first respawn) and false on respawn; the combat path
    // (CombatIpc) deliberately does NOT touch it. So a player who has spawned reads
    // STATE_NORMAL=11 (even if momentarily combat-dead, which self-corrects on their next
    // respawn) and only a player still awaiting (re)spawn reads STATE_DYING=12. We do NOT mutate
    // is_dead/health or credit kills here — that is owned by the combat path; we report + forward.
    inline void HostStatus(SCallbackData& callback, CCastServer* server)
    {
        auto session = callback.session;
        auto message = callback.message;
        if (!session || !message) return;

        auto order = magic_enum::enum_cast<EOrder>(u16_cast(message->GetOrder())).value_or(EOrder::NONE);

        auto userSid = message->GetSession();
        auto hostSid = session->GetSessionId();
        auto cnt = message->GetOption();

        auto host = CAccount.get<shared_t>(hostSid);
        if (!host) return;
        std::string hostName, userName;
        hostName = host->nickname;
        auto roomId = host->room_id;
        host.unlock();

        if (userSid != hostSid)
        {
            auto user = CAccount.get<shared_t>(userSid);
            if (!user) return;
            userName = user->nickname;
            user.unlock();
        }
        else
            userName = hostName;

        auto room = CRoom.get<shared_t>(roomId);
        if (!room) return;
        if (hostSid != room->host_session_id)
        {
            auto orderName = magic_enum::enum_name(order);
            DEBUGLOG(yellow, "({}): host=({}) hostSid=({}) is not host of roomId=({})", orderName, hostName, hostSid, roomId);
            return;
        }
        room.unlock();

        std::vector<ExPlayerUpdateInfoAck> update;
        update.resize(cnt);
        for (uint8_t i = 0; i < cnt; i++)
        {
            auto data = reinterpret_cast<ExPlayerUpdateInfoReq*>(message->GetData() + sizeof(ExPlayerUpdateInfoReq) * i);

            update[i].uid = data->uid;
            const auto updateSid = static_cast<uint16_t>(data->uid.session);

            // Authoritative alive/dead from the server's spawn flag (is_dead), NOT the host
            // packet's momentary m_bIsDead. Spawned players read alive so the joiner instantiates
            // them; only players still awaiting their (re)spawn read dying.
            uint32_t health = 0;
            bool dead = false;
            if (auto player = CAccount.get<shared_t>(updateSid))
            {
                health = player->current_health;
                dead = player->is_dead;
                player.unlock();
            }
            update[i].info.player_status = dead ? 12 : 11;
            update[i].info.health = health;

            PACKETLOG(ACK, order, "join-snapshot roomId=({}) recipient=({}) recipientSid=({}) from host=({}) hostSid=({}) playerSid=({}) health=({}) status=({}) hostReportedDead=({})",
                roomId, userName, userSid, hostName, hostSid, updateSid, health, dead ? "dead" : "alive", data->m_bIsDead ? "true" : "false");
        }
        server->Forward(userSid, hostSid, *message);
        CMessage statusMsg;
        statusMsg.SetCommand(order, 2, message->GetExtra(), message->GetOption());
        statusMsg.SetData(reinterpret_cast<uint8_t*>(update.data()), static_cast<uint16_t>(update.size() * sizeof(ExPlayerUpdateInfoAck)));
        server->Forward(userSid, hostSid, statusMsg);
    }
}
