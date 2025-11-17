#pragma once
namespace Game::Handlers
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Main;
    inline void PartyLeave(SCallbackData& callback, CMainServer* main_server)
    {
        auto session = callback.session;
        auto message = callback.message;
        if (!session || !message) return;

        std::shared_lock lock(session->GetMutex());
        CServer* server = callback.server;
        auto session_id = session->GetSessionId();
        auto acc_cache = CAccount.get<unique_t>(session_id);
        auto acc_index = acc_cache->acc_info.Index;
        auto my_unique_id = NetEngine::Packets::Core::UniqueId(session_id, 1).data;
        auto my_slot = acc_cache->slot_id;
        auto my_team_id = acc_cache->team_id;
        //auto leave_result = static_cast<NetEngine::Room::Leave::Req::Result>(callback.message->GetExtra());
        if (acc_index == -1) return;

        auto party_id = acc_cache->party_id;
        auto party_cache = CParty.get<unique_t>(party_id);

        if (party_cache->party_host_session_id == session_id) {
            DEBUGLOG(dark_cyan, "party will need change host");
            if (!acc_cache->in_room)
            {
                party_cache->is_registered = false;
                party_cache->is_queueing = false;
                for (const auto& party_member_session_id : party_cache->members)
                {
                    if (auto player_session = server->GetSessionById(party_member_session_id))
                        player_session->SendMsg(120, 0, 45, 0);
                }
            }
            uint16_t new_leader_index = 0;
            uint16_t new_leader = 0;
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
                    player_session->SendMsg(114, 0, 1, static_cast<uint8_t>(new_leader_index));
            }
            party_cache->party_host_session_id = new_leader;
        }


        auto remove_myself = std::remove(party_cache->members.begin(), party_cache->members.end(), session_id);
        party_cache->members.erase(remove_myself, party_cache->members.end());

        for (const auto& party_member_session_id : party_cache->members)
        {
            if (auto player_session = server->GetSessionById(party_member_session_id))
                player_session->SendMsg(419, 0, 0, 0, reinterpret_cast<uint8_t*>(&my_unique_id), sizeof(my_unique_id));
        }

        acc_cache->party_id = 0;
        acc_cache->in_party = false;

        //auto leavePartyReq = reinterpret_cast<MainLeavePartyReq*>(callback.message->GetData());
        session->SendMsg(111, 0, 1, 0);

        DEBUGLOG(dark_cyan, "player ({}) left party id: ({})", acc_cache->acc_info.Nickname.c_str(), party_id);
        DEBUGLOG(dark_cyan, "now party have member count: ({})", party_cache->members.size());

        if (party_cache->members.size() == 0) 
        {
            party_cache.unlock();
            DEBUGLOG(dark_cyan, "party is empty so will be deleted id: ({})", party_id);
			CParty.erase(party_id);
			CPartyId.erase_value(party_id);
            main_server->SetQueuePartyIdAvailable(party_id);
        }
    }
}