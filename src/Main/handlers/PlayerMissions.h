#pragma once
namespace Game
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Main;

    namespace Handlers
    {
        inline void PlayerCompleteGuideMission(SCallbackData& callback, CMainServer* main_server)
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
            auto session_id = session->GetSessionId();
            auto acc_cache = main_server->GetAccCacheUniqueBySessionId(session_id);
            
            auto acc_index = acc_cache->acc_info.Index;
            if (acc_index == -1) return;
            auto mission_data = reinterpret_cast<MainCompleteMissionReq*>(callback.message->GetData());
            if (mission_data->mission_type == 1 && mission_data->set_index == 9)
            {

            }

        }

        inline void PlayerMissions(SCallbackData& callback, CMainServer* main_server)
        {
            const auto& order = callback.message->GetOrder();
            switch (order)
            {
                case 168: PlayerCompleteGuideMission(callback, main_server); break;
            }
        }
    }
    
}