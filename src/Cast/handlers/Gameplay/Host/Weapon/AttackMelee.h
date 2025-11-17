#pragma once
namespace Game::Handlers
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Cast;
    inline void HostAttackMelee (SCallbackData& callback, CCastServer* server)
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

        auto req = message->GetData<PlayerVictimWeaponReq*>();
        auto victim_acc = CAccount.get<unique_t>(static_cast<uint16_t>(req->victim_unique_id.session));
        victim_acc->health = req->player_info.health;
        victim_acc.unlock();

        server->Broadcast(room->players_session_id, *message);
    }
}