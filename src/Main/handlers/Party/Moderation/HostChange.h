#pragma once
namespace Game::Handlers
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Main;
    inline void PartyHostChange(SCallbackData& callback, CMainServer* main_server)
    {
        auto session = callback.session;
        auto message = callback.message;
        if (!session || !message) return;

        std::shared_lock lock(session->GetMutex());
        CServer* server = callback.server;
        auto sid = session->GetSessionId();
        auto acc = CAccount.get<unique_t>(sid);
        auto aid = acc->acc_info.Index;
        auto my_unique_id = NetEngine::Packets::Core::UniqueId(sid, 1).data;
        if (aid == -1) return;

        if (!CParty.contains(acc->party_id))
        {
            DEBUGLOG(dark_cyan, "could not find party id ({})", acc->party_id);
            return;
        }
        auto party = CParty.get<unique_t>(acc->party_id);
        DEBUGLOG(dark_cyan, "party want to change leader");
        if (party->party_host_session_id != sid)
        {
            DEBUGLOG(dark_cyan, "party change leader request is not the leader!");
            session->SendMsg(114, 0, 16, 0);
            return;
        }
        auto new_leader_slot = message->GetOption();
        if (new_leader_slot >= party->members.size())
        {
            DEBUGLOG(dark_cyan, "party change leader request invalid slot!");
            session->SendMsg(114, 0, 16, 0);
            return;
        }

        for (const auto& id : party->members)
            if (auto pss = server->GetSessionById(id))
                pss->SendMsg(114, 0, 1, static_cast<uint8_t>(new_leader_slot));
        auto new_leader = party->members[new_leader_slot];
        party->party_host_session_id = new_leader;
    }
}