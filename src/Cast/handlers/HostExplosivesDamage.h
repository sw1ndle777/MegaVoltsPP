#pragma once
namespace Game
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Cast;

    namespace Handlers
    {
        class Test264Data
        {
        public:
            std::uint32_t projectile_id;
            std::uint16_t coord_x;
            std::uint16_t coord_y;
            std::uint16_t coord_z;
            std::uint16_t idk;
            std::vector<PlayerVictimDataReq> player_victims_data;
            Test264Data(std::uint16_t projectile_id, std::uint16_t coord_x, std::uint16_t coord_y, std::uint16_t coord_z, std::uint16_t idk, std::vector<PlayerVictimDataReq>& data)
            {
                std::memset(this, 0, sizeof(PlayerVictimDataReq));
                this->projectile_id = projectile_id;
                this->coord_x = coord_x;
                this->coord_y = coord_y;
                this->coord_z = coord_z;
                this->idk = idk;
                this->player_victims_data = data;

            }

            std::vector<std::uint8_t> Serialize() const
            {
                std::vector<std::uint8_t> data;
                const auto* projectile_id_bytes = reinterpret_cast<const uint8_t*>(&projectile_id);
                data.insert(data.end(), projectile_id_bytes, projectile_id_bytes + sizeof(projectile_id));
                const auto* coord_x_bytes = reinterpret_cast<const uint8_t*>(&coord_x);
                data.insert(data.end(), coord_x_bytes, coord_x_bytes + sizeof(coord_x));

                const auto* coord_y_bytes = reinterpret_cast<const uint8_t*>(&coord_y);
                data.insert(data.end(), coord_y_bytes, coord_y_bytes + sizeof(coord_y));

                const auto* coord_z_bytes = reinterpret_cast<const uint8_t*>(&coord_z);
                data.insert(data.end(), coord_z_bytes, coord_z_bytes + sizeof(coord_z));

                const auto* idk_bytes = reinterpret_cast<const uint8_t*>(&idk);
                data.insert(data.end(), idk_bytes, idk_bytes + sizeof(idk));

                for (const auto& mail_info : player_victims_data)
                {
                    const auto* mail_data_bytes = reinterpret_cast<const uint8_t*>(&mail_info);
                    data.insert(data.end(), mail_data_bytes, mail_data_bytes + sizeof(mail_info));
                }

                return data;
            }
        };
        inline void HostExplosivesDamage(SCallbackData& callback, CCastServer* cast_server)
        {
            auto send_msg = [&](CSession* session, std::uint16_t order, std::uint8_t mission, std::uint8_t extra, std::uint8_t option, std::uint8_t* data = nullptr, std::uint16_t data_size = 0)
                {
                    CMessage message(session->GetEncryptionKey());
                    message.SetSession(session->GetSessionId());
                    message.SetCommand(order, mission, extra, option);
                    if (data_size > 0 && data != nullptr) message.SetData(data, data_size);
                    session->Send(message);
                };

            std::shared_lock lock(callback.session->GetMutex());
            CSession* session = callback.session;
            CServer* server = callback.server;
            auto self_session_id = session->GetSessionId();
            auto self_player = cast_server->GetPlayerCacheShared(self_session_id);
            auto room = cast_server->GetRoomCacheShared(self_player->room_id);
            self_player.unlock();

            auto extra = callback.message->GetExtra();
            auto cnt = callback.message->GetOption();

            auto projectileReq = reinterpret_cast<AddProjectileReq*>(callback.message->GetData());

            auto pos_x = DirectX::PackedVector::XMConvertHalfToFloat(projectileReq->coord_x);
            auto pos_y = DirectX::PackedVector::XMConvertHalfToFloat(projectileReq->coord_y);
            auto pos_z = DirectX::PackedVector::XMConvertHalfToFloat(projectileReq->coord_z);

            BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::red, "projId: ({}) posX: ({}) posY: ({}) posZ: ({}) unk: ({})", projectileReq->projectile_id, pos_x, pos_y, pos_z, projectileReq->idk);

            std::vector<PlayerVictimDataReq> player_victims_data;
            for (int i = 0; i < cnt; i++)
            {
                auto data = reinterpret_cast<PlayerVictimDataReq*>(callback.message->GetData() + sizeof(AddProjectileReq) + i * sizeof(PlayerVictimDataReq));
                player_victims_data.push_back(*data);
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::red, "idk2: ({})", data->idk2);
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::red, "player uid: ({}) ({}) attacked and now have hp: ({})", (std::uint32_t)data->victim_unique_id.data, (std::uint16_t)data->victim_unique_id.session, (std::uint32_t)data->player_info.health);
                auto target_player_cache = cast_server->GetPlayerCacheUnique((std::uint16_t)data->victim_unique_id.session);
                target_player_cache->health = (std::uint32_t)data->player_info.health;
                target_player_cache.unlock();
            }

            //auto projectileReqNew = Test264Data(projectileReq->projectile_id, projectileReq->coord_x, projectileReq->coord_y, projectileReq->coord_z, projectileReq->idk, player_victims_data).Serialize();

            /*
            for (const auto& id : room->players_session_id)
            {
                if (auto player_session = cast_server->GetSessionById(id))
                {
                    send_msg(player_session.get(), 264, callback.message->GetMission(), extra, cnt, reinterpret_cast<uint8_t*>(projectileReqNew.data()), projectileReqNew.size());
                }
            }
            */

            //[14]
            //auto og_data = callback.message->GetData();
            //BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::red, "change at 14 original: ({})", og_data[14]);
            //og_data[12] = 0x1;
            //og_data[14] = 0x1;
            //callback.message->SetData(og_data, callback.message->GetDataSize());

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