#pragma once
namespace Game
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Cast;

    namespace Handlers
    {
        inline void NpcsProjectiles(SCallbackData& callback, CCastServer* cast_server)
        {
            std::shared_lock lock(callback.session->GetMutex());
            CSession* session = callback.session;
            CServer* server = callback.server;
            auto self_session_id = session->GetSessionId();
            auto self_player = cast_server->GetPlayerCacheShared(self_session_id);
            auto room = cast_server->GetRoomCacheShared(self_player->room_id);

            //auto req_info = reinterpret_cast<PveProjectileImpactReq*>(callback.message->GetData());
            //req_info->unique_id = NetEngine::Packets::Core::UniqueId(self_session_id, 1);

            /*
            auto og_data = callback.message->GetData();
            BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::red, "change at 18 original: ({})", og_data[18]);
            og_data[18] = 0x1;
            callback.message->SetData(og_data, callback.message->GetDataSize());
            */
            auto broadcast = [&](auto player_session_id, auto& msg)
            {
                msg->SetEncryptMethod(SendOption::EncryptionMethod::None);
                msg->SetSession(player_session_id);
                //msg->SetData(reinterpret_cast<uint8_t*>(req_info), sizeof(PveProjectileImpactReq));
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