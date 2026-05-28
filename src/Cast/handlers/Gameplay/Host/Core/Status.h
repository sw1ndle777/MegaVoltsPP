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

        std::vector<ExPlayerUpdateInfoAck> update;
        update.resize(cnt);
        std::string playerName{};
        for (uint8_t i = 0; i < cnt; i++)
        {
            auto data = reinterpret_cast<ExPlayerUpdateInfoReq*>(message->GetData() + sizeof(ExPlayerUpdateInfoReq) * i);
			PACKETLOG(REQ, order, "roomId=({}) user=({}) sid=({}) from host=({}) hostSid=({}) m_uiSpawnTick=({}) uid=({}) m_bIdk1=({}) m_bIdk2=({}) m_bIdk3=({}) m_bIdk4=({}) m_bIdk5=({}) m_bIdk6=({}) m_bIdk7=({}) m_bIdk8=({}) m_bIdk9=({}) m_bIsDead=({}) m_bIdk11=({}) m_bIdk12=({}) m_bIdk13=({}) m_bIdk14=({}) m_bIdk15=({}) m_bIdk16=({})",
                roomId, userName, userSid, hostName, hostSid, data->m_uiSpawnTick, data->uid.data, data->m_bIdk1, data->m_bIdk2, data->m_bIdk3, data->m_bIdk4, data->m_bIdk5, data->m_bIdk6, data->m_bIdk7, data->m_bIdk8, data->m_bIdk9, data->m_bIsDead, data->m_bIdk11, data->m_bIdk12, data->m_bIdk13, data->m_bIdk14, data->m_bIdk15, data->m_bIdk16);

            update[i].uid = data->uid;
            update[i].info.player_status = data->m_bIsDead ? 12 : 11;
			auto updateSid = static_cast<uint16_t>(data->uid.session);
            auto player = CAccount.get<unique_t>(updateSid);
            if (!player) continue;
            const bool was_dead = player->is_dead;
            player->is_dead = data->m_bIsDead != 0;
            if (player->is_dead)
            {
                player->current_kill_streak = 0;
                player->current_health = 0;
                player->health = 0;
                player->combat_health = 0;
                player->combat_health_known = true;
            }
            else if (was_dead || player->health == 0)
            {
                const auto full_health = player->max_health ? player->max_health : kCastDefaultHealthRaw;
                player->current_health = full_health;
                player->health = full_health;
                player->combat_health = full_health;
                player->combat_health_known = true;
            }
            update[i].info.health = player->current_health;
            PACKETLOG(ACK, order, "roomId=({}) player=({}) playerSid=({}) from host=({}) hostSid=({}) health=({}) m_bIsDead=({})",
				roomId, player->nickname, updateSid, hostName, hostSid, player->current_health, data->m_bIsDead ? "true" : "false");
        }
        server->Forward(userSid, hostSid, *message);
        CMessage statusMsg;
        statusMsg.SetCommand(order, 2, message->GetExtra(), message->GetOption());
        statusMsg.SetData(reinterpret_cast<uint8_t*>(update.data()), static_cast<uint16_t>(update.size() * sizeof(ExPlayerUpdateInfoAck)));
        server->Forward(userSid, hostSid, statusMsg);
    }
}
