#pragma once
namespace Game
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Cast;

    namespace Handlers
    {
#pragma pack(push, 1)
        union PlayerInfoCastAction
        {
            struct
            {
                uint32_t health : 20;
                uint32_t mode_index : 5;
                uint32_t player_status : 4;
                uint32_t unk : 3;
            };
            uint32_t data;
        };
        struct SinglePlayerJoinInfoResponse
        {
            NetEngine::Packets::Core::UniqueId uid;
            PlayerInfoCastAction player_info;
        };
#pragma pack(pop)
        struct SinglePlayerJoinInfo
        {
            char u0[16]{};
            NetEngine::Packets::Core::UniqueId uid;
            uint32_t unknown{};
        };
        inline void PlayerUnknown2(SCallbackData& callback, CCastServer* cast_server)
        {
            std::shared_lock lock(callback.session->GetMutex());
            CSession* session = callback.session;
            CServer* server = callback.server;
            auto player_session_id = callback.message->GetSession();
            auto host_session_id = session->GetSessionId();
            auto message = callback.message;
            message->SetEncryptMethod(SendOption::EncryptionMethod::None);
            message->SetSession(host_session_id);

            auto cnt = message->GetOption();
            std::vector<SinglePlayerJoinInfoResponse> singleInfoResp(cnt);
            const uint8_t* dataPtr = message->GetData();
            for (int i = 0; i < cnt; i++)
            {
                SinglePlayerJoinInfo sp;
                std::memcpy(&sp, dataPtr + sizeof(SinglePlayerJoinInfo) * i, sizeof(SinglePlayerJoinInfo));
                bool is_dead = (bool)sp.u0[9];
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "player join info: u0: ({}) uid: ({}) unk: ({}) is_dead: ({})", sp.u0, (uint32_t)sp.uid.session, sp.unknown, is_dead);
                singleInfoResp[i].uid = sp.uid;
                singleInfoResp[i].player_info.player_status = 11;
                if (is_dead) singleInfoResp[i].player_info.player_status = 12;
                singleInfoResp[i].player_info.unk = 0;
                auto current_player_cache = cast_server->GetPlayerCacheShared((uint32_t)sp.uid.session);
                singleInfoResp[i].player_info.health = current_player_cache->health;
                current_player_cache.unlock();
            }

            if (auto forwarded_session = server->GetSessionById(player_session_id))
            {
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "host session id: ({}) forward packet to session id: ({})", host_session_id, player_session_id);
                //host forward to player
                forwarded_session->Send(*message);

                message->SetMission(2);
                message->SetData(reinterpret_cast<uint8_t*>(singleInfoResp.data()), singleInfoResp.size() * sizeof(SinglePlayerJoinInfoResponse));
                forwarded_session->Send(*message);
            }
            else
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "host session id: ({}) couldn't forward packet to session id: ({})", host_session_id, player_session_id);
        }
    }
}