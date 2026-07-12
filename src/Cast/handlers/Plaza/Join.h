#pragma once
namespace Game::Handlers
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Cast;
    inline void PlazaJoin(SCallbackData& callback, CCastServer* server)
    {
        auto session = callback.session;
        auto message = callback.message;
        if (!session || !message) return;

        auto sid = session->GetSessionId();
        auto req = reinterpret_cast<CastJoinPlazaReq*>(message->GetData());
        auto plazaId = static_cast<uint16_t>(req->plaza_id);

        DEBUGLOG(dark_cyan, "sid=({}) attempt to join plaza id: ({}), mission: ({}),  extra: ({}), option: ({})", sid, plazaId, message->GetMission(), message->GetExtra(), message->GetOption());

        if (CPlaza.contains(plazaId))
        {
            auto plaza = CPlaza.get<unique_t>(plazaId);

            if (std::ranges::contains(plaza->players_session_id, sid))
            {
                plaza.unlock();
                DEBUGLOG(dark_cyan, "sid=({}) already in plaza id: ({})", sid, plazaId);
                return;
            }

            plaza->players_session_id.push_back(sid);
            DEBUGLOG(dark_cyan, "sid=({}) joined plaza id: ({})", sid, plazaId);
            plaza.unlock();
        }
        else
        {
            Game::Plaza newPlaza = { plazaId };
            newPlaza.players_session_id.push_back(sid);
            CPlaza.insert(plazaId, newPlaza);
            DEBUGLOG(dark_cyan, "plaza id: ({}) doesn't exist, auto create", plazaId);
        }
        auto acc = CAccount.get<unique_t>(sid);
        if (!acc) return; // pre-auth session: never deref/unlock a null locker (would throw "operation not permitted")
        auto oldPlazaId = acc->plaza_id;
        acc->plaza_id = plazaId;
        acc->in_plaza = true;
        acc.unlock();

        if (plazaId != oldPlazaId)
        {
            if (CPlaza.contains(oldPlazaId))
            {
                auto oldPlaza = CPlaza.get<unique_t>(oldPlazaId);
                std::erase_if(oldPlaza->players_session_id, [&](const uint16_t& id) { return id == sid; });
                DEBUGLOG(dark_cyan, "sid=({}) left plaza id: ({})", sid, oldPlazaId);
                if (oldPlaza->players_session_id.empty())
                {
                    oldPlaza.unlock();
                    CPlaza.erase(oldPlazaId);
                    DEBUGLOG(dark_cyan, "sid=({}) removed plaza id: ({})", sid, oldPlazaId);
                }
                else
                    oldPlaza.unlock();
            }
        }
        session->SendMsg(176, message->GetMission(), 6, message->GetOption());
    }
}