#pragma once
namespace Game::Handlers
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Main;
    inline void PartyPassword(SCallbackData& callback, CMainServer* main_server)
    {
        auto session = callback.session;
        auto message = callback.message;
        if (!session || !message) return;

        std::shared_lock lock(session->GetMutex());
        auto sid = session->GetSessionId();
        auto acc = CAccount.get<unique_t>(sid);
        auto aid = acc->acc_info.Index;
        if (aid == -1) return;
        DEBUGLOG(dark_cyan, "party is clan and will register");
        if (!acc->in_party) return;
        if (!CParty.contains(acc->party_id))
        {
            DEBUGLOG(dark_cyan, "could not find party id ({})", acc->party_id);
            return;
        }
        auto party = CParty.get<unique_t>(acc->party_id);
        if (party->party_host_session_id != sid) return;

        auto order = message->GetOrder();
        auto extra = message->GetExtra();
        party->is_queueing = true;
        party->has_password = false;
        party->password = "";
        if (message->GetDataSize() > 8)
        {
            struct info { char password[14]; };
            auto req_info = reinterpret_cast<info*>(message->GetData());
            auto new_password = Utility::ReadMicrovoltsString(req_info->password, sizeof(req_info->password));
            if (new_password.size())
            {
                party->has_password = true;
                party->password = new_password;
            }
        }
        for (const auto& id : party->members)
        {
            if (auto pss = main_server->GetSessionById(id))
            {
                if (party->has_password)
                    pss->SendMsg(115, 0, 45, 0, message->GetData(), message->GetDataSize());
                else
					pss->SendMsg(115, 0, 44, 0);
            }
        }         
    }
}