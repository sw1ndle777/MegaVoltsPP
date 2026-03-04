#pragma once
namespace Game::Handlers
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Cast;
    inline void HostImpactProjectile(SCallbackData& callback, CCastServer* server)
    {
        auto session = callback.session;
        auto message = callback.message;
        if (!session || !message) return;

        auto order = magic_enum::enum_cast<EOrder>(u16_cast(message->GetOrder())).value_or(EOrder::NONE);

        auto hostSid = session->GetSessionId();
        auto host = CAccount.get<shared_t>(hostSid);
        auto room = CRoom.get<shared_t>(host->room_id);

        if (!host || !room) return;
        if (host->session_id != room->host_session_id)
        {
            auto orderName = magic_enum::enum_name(order);
            DEBUGLOG(yellow, "({}): host=({}) hostSid=({}) is not host of roomId=({})", orderName, host->nickname, hostSid, room->room_id);
            return;
        }

        auto extra = message->GetExtra();
        auto cnt = message->GetOption();
		auto req = message->GetData<AddProjectileReq*>();

        //auto pos_x = DirectX::PackedVector::XMConvertHalfToFloat(req->coord_x);
        //auto pos_y = DirectX::PackedVector::XMConvertHalfToFloat(req->coord_y);
        //auto pos_z = DirectX::PackedVector::XMConvertHalfToFloat(req->coord_z);

        PACKETLOG(ACK, order, "roomId=({}) from host=({}) hostSid=({}) victimsCount=({})", host->room_id, host->nickname, hostSid, cnt);
        host.unlock();
        for (uint8_t i = 0; i < cnt; i++)
        {
            auto data = reinterpret_cast<PlayerVictimDataReq*>(message->GetData() + sizeof(AddProjectileReq) + i * sizeof(PlayerVictimDataReq));  
            auto victim_acc = CAccount.get<unique_t>(static_cast<uint16_t>(data->victim_unique_id.session));
            if (victim_acc)
                victim_acc->health = data->player_info.health;
        }

        server->Broadcast(room->players_session_id, *message);
    }
}