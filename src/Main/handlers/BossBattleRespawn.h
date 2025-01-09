#pragma onceauto session_id = session->GetSessionId();
namespace Game
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Main;

    namespace Handlers
    {
        inline void BossBattleRespawn(SCallbackData& callback, CMainServer* main_server)
        {
            auto send_msg = [&](CSession* session, std::uint16_t order, std::uint8_t mission, std::uint8_t extra, std::uint8_t option, std::uint8_t* data = nullptr, std::size_t data_size = 0)
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
            auto session_id = session->GetSessionId();
            auto acc_cache = main_server->GetAccCacheSharedBySessionId(session_id);
            auto acc_index = acc_cache->acc_info.Index;

            if (acc_index == -1 || !acc_cache->in_room || !main_server->IsRoomAlready(acc_cache->room_id)) return;
            //auto unknown_info = reinterpret_cast<std::uint32_t>(callback.message->GetData());
            send_msg(session, callback.message->GetOrder(), 0, 1, 0, callback.message->GetData(), callback.message->GetDataSize()); // 1 = success, 2 NoEnergy
        }
    }
    
}