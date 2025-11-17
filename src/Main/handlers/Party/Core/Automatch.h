#pragma once
namespace Game::Handlers
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Main;
    inline void PartyAutomatch(SCallbackData& callback, CMainServer* main_server)
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
        if (acc_index == -1) return;

        if (!acc_cache->in_party) {

            return;
        }
        if (!CParty.contains(acc_cache->party_id)) {
            DEBUGLOG(dark_cyan, "could not find player's party id ({})", acc_cache->party_id);
            return;
        }

        auto party_cache = CParty.get<unique_t>(acc_cache->party_id);
        if (party_cache->party_host_session_id != session_id) {
            DEBUGLOG(dark_cyan, "is not leader of party");
            return;
        }

        auto my_party_count = party_cache->members.size();
        auto my_party_is_clan = party_cache->is_clan;

        party_cache.unlock();

        auto c_extra = message->GetExtra();
        if (c_extra == 44) 
        {
            DEBUGLOG(dark_cyan, "want to find a match for ({}) players", my_party_count);
            uint16_t match_party_id = 0;
            auto party_ids = CPartyId.get_all(shared);
            uint32_t party_ids_count = party_ids->size();
            for (uint32_t i = 0; i < party_ids_count; i++) {
                auto c_party_id = party_ids->at(i);
                auto c_party = CParty.get<shared_t>(c_party_id);
                DEBUGLOG(dark_cyan, "will check party id: ({}) by ({})", c_party_id, c_party->party_host_session_id);
                bool is_match = (c_party->is_registered && !c_party->is_clan && c_party->members.size() == my_party_count);
                c_party.unlock();
                if (is_match) {
                    DEBUGLOG(dark_cyan, "party register found a match: ({})", c_party_id);
                    match_party_id = c_party_id;
                    break;
                }
            }
            party_ids.unlock();
            DEBUGLOG(dark_cyan, "party register and found a match id: ({})", match_party_id);
            if (match_party_id) {
                struct info_req {
                    uint16_t targetPartyId;
                };
                info_req new_req;
                new_req.targetPartyId = match_party_id;
                auto msg = CMessage();
                msg.SetSession(callback.session->GetSessionId());
                //msg.SetCommand(callback.message->GetOrder(), callback.message->GetMission(), callback.message->GetExtra(), callback.message->GetOption());
                msg.SetData(reinterpret_cast<uint8_t*>(&new_req), sizeof(new_req));
                callback.message = &msg;
                acc_cache.unlock();
                lock.unlock();
                DEBUGLOG(dark_cyan, "will join battle with party id: ({})", match_party_id);
                PartyClanOtherJoin(callback, main_server);
                return;
            }
            party_cache->is_registered = true;
        }
        else if (c_extra == 45) {
            party_cache->is_registered = false;
        }
        for (const auto& party_member_session_id : party_cache->members)
        {
            if (auto player_session = server->GetSessionById(party_member_session_id))
                player_session->SendMsg(119, callback.message->GetMission(), c_extra, callback.message->GetOption());
        }
    }
}