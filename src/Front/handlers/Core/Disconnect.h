#pragma once
namespace Game
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Front;

    namespace Handlers
    {
        inline void ServerDisconnect(std::shared_ptr<CSession> session, CFrontServer* front_server)
        {
            if (!session) return;
            auto sid = session->GetSessionId();

            DEBUGLOG(dark_cyan, "disconnected sid=({})", sid);
            uint64_t auth_key = 0;
            int32_t aid = 0;
            {
                auto accounts = CAccount.get_all(unique);
                for (auto it = accounts->begin(); it != accounts->end(); ++it)
                {
                    if (it->second.sid != sid) continue;

                    aid = it->first;
#if defined(RELEASE_1_0_3)
                    auth_key = it->second.plazaAuth.AuthKey;
#else
                    auth_key = it->second.frontAccount.AuthKey;
#endif
                    accounts->erase(it);
                    break;
                }
            }

            if (auth_key)
                CAuthKeys.erase(auth_key);
            if (aid)
                DEBUGLOG(dark_cyan, "removed front cache aid=({}) sid=({}) key=({})", aid, sid, auth_key);

            front_server->RemoveSession(sid);
        }
    }
}
