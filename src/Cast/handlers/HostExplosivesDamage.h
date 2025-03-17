#pragma once
namespace Game
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Cast;

    namespace Handlers
    {
        inline void HostExplosivesDamage(SCallbackData& callback, CCastServer* cast_server)
        {
            std::shared_lock lock(callback.session->GetMutex());
            CSession* session = callback.session;
            CServer* server = callback.server;
            auto self_session_id = session->GetSessionId();
            auto self_player = cast_server->GetPlayerCacheShared(self_session_id);
            auto room = cast_server->GetRoomCacheShared(self_player->room_id);
            self_player.unlock();

            auto projectileReq = reinterpret_cast<AddProjectileReq*>(callback.message->GetData());
            auto cnt = callback.message->GetOption();
            for (int i = 0; i < cnt; i++)
            {
                auto current_dmg = projectileReq->player_victims_data[i];
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::red, "player ({}) attacked and now have hp: ({})", (std::uint32_t)current_dmg.victim_unique_id.session, (std::uint32_t)current_dmg.player_info.health);
                auto target_player_cache = cast_server->GetPlayerCacheUnique((std::uint32_t)current_dmg.victim_unique_id.session);
                target_player_cache->health = (std::uint32_t)current_dmg.player_info.health;
                target_player_cache.unlock();
            }

            auto broadcast = [&](auto player_session_id, auto& msg)
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
                broadcast(id, callback.message);
        }
    }  
}