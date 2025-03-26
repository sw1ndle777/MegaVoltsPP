#pragma once
namespace Game
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Main;

    namespace Handlers
    {
        inline void LeaveParty(SCallbackData& callback, CMainServer* main_server)
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
            auto session_id = session->GetSessionId();
            auto acc_cache = main_server->GetAccCacheUniqueBySessionId(session_id);
            auto acc_index = acc_cache->acc_info.Index;
            
            auto my_slot = acc_cache->slot_id;
            auto my_team_id = acc_cache->team_id;
            //auto leave_result = static_cast<NetEngine::Room::Leave::Req::Result>(callback.message->GetExtra());
            if (acc_index == -1) return;
            auto my_unique_id = NetEngine::Packets::Core::UniqueId(session_id, acc_cache->server_id).data;
            auto party_id = acc_cache->party_id;
            auto party_cache = main_server->GetPartyCacheUnique(party_id);
            
            if (party_cache->party_host_session_id == session_id) {
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "party will need change host");
                if (!acc_cache->in_room)
                {
                    party_cache->is_registered = false;
                    party_cache->is_queueing = false;
                    for (const auto& party_member_session_id : party_cache->members)
                    {
                        if (auto player_session = server->GetSessionById(party_member_session_id))
                            send_msg(player_session.get(), 120, 0, 45, 0);
                    }
                }
                std::uint16_t new_leader_index = 0;
                std::uint16_t new_leader = 0;
                for (const auto& member : party_cache->members)
                {
                    if (member != party_cache->party_host_session_id) {
                        new_leader = member;
                        break;
                    }
                    new_leader_index++;
                }
                for (const auto& party_member_session_id : party_cache->members)
                {
                    if (party_member_session_id == party_cache->party_host_session_id) continue;
                    if (auto player_session = server->GetSessionById(party_member_session_id))
                        send_msg(player_session.get(), 114, 0, 1, static_cast<std::uint8_t>(new_leader_index));
                }
                party_cache->party_host_session_id = new_leader;
            }

            
            auto remove_myself = std::remove(party_cache->members.begin(), party_cache->members.end(), session_id);
            party_cache->members.erase(remove_myself, party_cache->members.end());

            for (const auto& party_member_session_id : party_cache->members)
            {
                if (auto player_session = server->GetSessionById(party_member_session_id))
                    send_msg(player_session.get(), 419, 0, 0, 0, reinterpret_cast<uint8_t*>(&my_unique_id), sizeof(my_unique_id));
            }

            acc_cache->party_id = 0;
            acc_cache->in_party = false;

            //auto leavePartyReq = reinterpret_cast<MainLeavePartyReq*>(callback.message->GetData());
            send_msg(session, 111, 0, 1, 0);

            BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "player ({}) left party id: ({})", acc_cache->acc_info.Nickname.c_str(), party_id);
            BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "now party have member count: ({})", party_cache->members.size());

            if (party_cache->members.size() == 0) {
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "party is empty so will be deleted id: ({})", party_id);
                main_server->RemovePartyCache(party_id);
                main_server->SetQueuePartyIdAvailable(party_id);
            }
        }
    }
}