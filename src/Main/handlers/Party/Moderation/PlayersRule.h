#pragma once
namespace Game::Handlers
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Main;
    inline void PartyPlayersRule(SCallbackData& callback, CMainServer* main_server)
    {
        auto session = callback.session;
        auto message = callback.message;
        if (!session || !message) return;

        std::shared_lock lock(session->GetMutex());
        CServer* server = callback.server;
        auto sid = session->GetSessionId();
        auto acc = CAccount.get<unique_t>(sid);
        auto aid = acc->acc_info.Index;
        if (aid == -1) return;


        if (!acc->in_party) return;
        if (!CParty.contains(acc->party_id))
        {
            DEBUGLOG(dark_cyan, "could not find party id ({})", acc->party_id);
            return;
        }

        auto party = CParty.get<unique_t>(acc->party_id);
        if (party->party_host_session_id != sid) return;

        auto order = message->GetOrder(); 

        party->max_members = message->GetOption();
        for (const auto& id : party->members)
            if (auto pss = server->GetSessionById(id))
                pss->SendMsg(order, message->GetMission(), 1, message->GetOption());
    }
}