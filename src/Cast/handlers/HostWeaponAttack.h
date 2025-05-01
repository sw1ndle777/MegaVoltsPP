#pragma once
namespace Game
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Cast;

    namespace Handlers
    {
        inline void HostWeaponAttack(SCallbackData& callback, CCastServer* cast_server)
        {
            auto session = callback.session;
            auto message = callback.message;
            if (!session || !message) return;

            std::shared_lock lock(session->GetMutex());
            CServer* server = callback.server;
            auto self_session_id = session->GetSessionId();
            auto self_player = cast_server->GetPlayerCacheShared(self_session_id);
            auto room = cast_server->GetRoomCacheShared(self_player->room_id);
            self_player.unlock();

            BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::red, "receive attack packet");

            switch (message->GetOrder())
            {
                case 266:
                case 269:
                {//PlayerVictimWeapon2Req
                    auto attackReq = reinterpret_cast<PlayerVictimWeapon2Req*>(message->GetData());
                    auto cnt = message->GetOption();
                    for (int i = 0; i < cnt; i++)
                    {
                        auto current_dmg = attackReq->player_victims_data[i];
                        BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::red, "player ({}) attacked and now have hp: ({})", (uint32_t)current_dmg.victim_unique_id.session, (uint32_t)current_dmg.player_info.health);
                        auto target_player_cache = cast_server->GetPlayerCacheUnique((uint32_t)current_dmg.victim_unique_id.session);
                        target_player_cache->health = (uint32_t)current_dmg.player_info.health;
                        target_player_cache.unlock();
                    }
                    break;
                }
                case 267:
                case 268:
                case 270:
                {//PlayerVictimWeaponReq
                    auto attackReq = reinterpret_cast<PlayerVictimWeaponReq*>(message->GetData());
                    auto victim_session_id = (uint32_t)attackReq->victim_unique_id.session;
                    auto victim_new_health = (uint32_t)attackReq->player_info.health;
                    BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::red, "player ({}) attacked and now have hp: ({})", victim_session_id, victim_new_health);
                    auto victim_cache = cast_server->GetPlayerCacheUnique(victim_session_id);
                    victim_cache->health = victim_new_health;
                    victim_cache.unlock();
                    break;
                }
            }

            static auto broadcast = [&](auto player_session_id, auto& msg)
            {
                msg->SetEncryptMethod(SendOption::EncryptionMethod::None);
                msg->SetSession(player_session_id);
                if (auto player_session = server->GetSessionById(player_session_id))
                    player_session->Send(*msg);
                else
                    BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::red, "couldn't broadcast packet to session id: ({})", player_session_id);
            };
            lock.unlock();
            for (const auto& id : room->players_session_id)
                broadcast(id, message);
        }
    } 
}