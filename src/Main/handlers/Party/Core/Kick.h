#pragma once
namespace Game::Handlers
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Main;
    inline void PartyKick(SCallbackData& callback, CMainServer* main_server)
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

        auto target_uid = *reinterpret_cast<NetEngine::Packets::Core::UniqueId*>(message->GetData());

        DEBUGLOG(dark_cyan, "party will kick player: ({})", target_uid.data);

        auto target_acc_cache = CAccount.get<shared_t>(static_cast<uint16_t>(target_uid.session));

        if (!acc->in_party || !target_acc_cache->in_party || acc->party_id != target_acc_cache->party_id) return;

        if (!CParty.contains(acc->party_id))
        {
            DEBUGLOG(dark_cyan, "could not find party id ({})", acc->party_id);
            return;
        }

        auto party = CParty.get<unique_t>(acc->party_id);
        if (party->party_host_session_id != sid) return;

        DEBUGLOG(dark_cyan, "party checks passed and will kick");

		std::erase_if(party->kicked_members, [&](const uint16_t& id) { return id == target_acc_cache->session_id; });

        for (const auto& id : party->members)
            if (auto pss = server->GetSessionById(id))
                pss->SendMsg(419, 0, 0, 0, reinterpret_cast<uint8_t*>(&target_uid), sizeof(target_uid));

        if (auto victim_session = server->GetSessionById(target_acc_cache->session_id))
            victim_session->SendMsg(111, 0, NetEngine::Room::Leave::Ack::Result::KickedByHost, 0);

        target_acc_cache->party_id = 0;
        target_acc_cache->in_party = false;

        party->kicked_members.push_back(target_acc_cache->session_id);

    }
}