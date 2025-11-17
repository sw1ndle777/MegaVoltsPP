#pragma once
namespace Game::Handlers
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Cast;
    inline void PlazaLeave(SCallbackData& callback, CCastServer* server)
    {
        auto session = callback.session;
        auto message = callback.message;
        if (!session || !message) return;

        auto sid = session->GetSessionId();
        auto acc = CAccount.get<shared_t>(sid);
        if (acc->in_plaza && CPlaza.contains(acc->plaza_id))
        {
            auto plaza = CPlaza.get<unique_t>(acc->plaza_id);
            std::erase_if(plaza->players_session_id, [&](const uint16_t& id) { return id == sid; });
            DEBUGLOG(dark_cyan, "sid=({}) left plaza id: ({})", sid, acc->plaza_id);

            if (plaza->players_session_id.empty())
            {
                plaza.unlock();
                CPlaza.erase(acc->plaza_id);
                DEBUGLOG(dark_cyan, "sid=({}) removed plaza id: ({})", sid, acc->plaza_id);
            }

            acc->state_id = PlayerInfo::State::Lobby;
            acc->in_plaza = false;
            acc.unlock();
        }
        session->SendMsg(176, message->GetMission(), message->GetExtra(), message->GetOption());
    }
}