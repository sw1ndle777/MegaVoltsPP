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
            if (CAccount.contains(sid))
            {
				auto acc = CAccount.get<shared_t>(sid);
				uint64_t key = 0;
#if defined(RELEASE_1_0_3)
				key = acc->plazaAuth.AuthKey;
#else
                key = acc->frontAccount.AuthKey;
#endif
                acc.unlock();
				CAuthKeys.erase(key);
                CAccount.erase(sid);
            } 
            front_server->RemoveSession(sid);
        }
    }
}