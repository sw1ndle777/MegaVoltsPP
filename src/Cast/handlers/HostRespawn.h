#pragma once
namespace Game
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Cast;

    namespace Handlers
    {
        inline void HostRespawn(SCallbackData& callback, CCastServer* cast_server)
        {
            auto session = callback.session;
            auto message = callback.message;
            if (!session || !message) return;

            std::shared_lock lock(session->GetMutex());

            CServer* server = callback.server;
            auto self_session_id = session->GetSessionId();
            auto self_player = cast_server->GetPlayerCacheShared(self_session_id);
            auto room = cast_server->GetRoomCacheUnique(self_player->room_id);
            self_player.unlock();

            auto respawnReq = reinterpret_cast<RespawnRequest*>(message->GetData());
            auto target_session_id = (uint32_t)respawnReq->target_unique_id.session;
            auto target_player_cache = cast_server->GetPlayerCacheUnique(target_session_id);
            BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::red, "player id: ({}) reset max hp", target_session_id);
            target_player_cache->health = 0xF4240;
            target_player_cache.unlock();

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