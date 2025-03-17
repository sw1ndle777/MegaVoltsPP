#pragma once
namespace Game
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Cast;

    namespace Handlers
    {
        float ConvertHalfToFloat(std::uint16_t half)
        {
            std::int32_t exponent = (half >> 10) & 0x1F;
            std::int32_t sign = half >> 15;
            std::int32_t mantissa = half & 0x3FF;
            std::uint32_t result;

            if (!exponent)
            {
                if ((half & 0x3FF) == 0)
                    result = sign << 31;
                else
                {
                    do {
                        mantissa *= 2;
                        --exponent;
                    } while ((mantissa & 0x400) == 0);
                    ++exponent;
                    mantissa &= ~0x400u;
                    result = ((exponent + 112) << 23) | ((mantissa | (sign << 18)) << 13);
                }
            }
            else if (exponent != 31)
            {
                result = ((exponent + 112) << 23) | ((mantissa | (sign << 18)) << 13);
            }
            else if ((half & 0x3FF) != 0)
            {
                result = (mantissa | (sign << 18) | 0x3FC00) << 13; // NaN case
            }
            else
            {
                result = (sign << 31) | 0x7F800000;
            }

            float finalResult;
            std::memcpy(&finalResult, &result, sizeof(finalResult));
            return finalResult;
        }
        inline void HostExplosives(SCallbackData& callback, CCastServer* cast_server)
        {
            std::shared_lock lock(callback.session->GetMutex());
            CSession* session = callback.session;
            CServer* server = callback.server;
            auto self_session_id = session->GetSessionId();
            auto self_player = cast_server->GetPlayerCacheShared(self_session_id);
            auto room = cast_server->GetRoomCacheShared(self_player->room_id);

            auto req_info = reinterpret_cast<ImpactProjectileReq*>(callback.message->GetData());
            //BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::red, "ImpactProjectileReq idk: ({})", (std::uint32_t)req_info->idk);
            BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::red, "ImpactProjectileReq: idk: ({}), coord1_x: ({}), coord1_y: ({}), coord1_z: ({}), coord2_x: ({}), coord2_y: ({}), coord2_z: ({}), attacker_unique_id: ({}), projectile_id: ({})", static_cast<std::uint32_t>(req_info->idk), ConvertHalfToFloat(req_info->coord1_x), ConvertHalfToFloat(req_info->coord1_y), ConvertHalfToFloat(req_info->coord1_z), ConvertHalfToFloat(req_info->coord2_x), ConvertHalfToFloat(req_info->coord2_y), ConvertHalfToFloat(req_info->coord2_z), static_cast<std::uint32_t>(req_info->attacker_unique_id.session), static_cast<std::uint32_t>(req_info->projectile_id));


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