#pragma once
namespace Game
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Main;

    namespace Handlers
    {
        inline void CreateParty(SCallbackData& callback, CMainServer* main_server)
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
            auto acc_cache = main_server->GetAccCacheUniqueBySessionId(session_id);
            auto acc_index = acc_cache->acc_info.Index;
            auto my_unique_id = NetEngine::Packets::Core::UniqueId(session_id, 1).data;
            auto my_slot = acc_cache->slot_id;
            auto my_team_id = acc_cache->team_id;
            auto leave_result = static_cast<NetEngine::Room::Leave::Req::Result>(callback.message->GetExtra());
            if (acc_index == -1) return;
            auto createPartyReq = reinterpret_cast<MainCreatePartyReq*>(callback.message->GetData());
            send_msg(session, 109, 0, 1, 0, reinterpret_cast<uint8_t*>(createPartyReq), sizeof(MainCreatePartyReq));
            
            BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "player ({}) create party -> unknown: ({})", acc_cache->acc_info.Nickname.c_str(), createPartyReq->unknown);
        }
    }
}