#pragma once
namespace Game::Handlers
{
    using namespace BaseLib;
    using namespace NetEngine;

    inline void IpcMainPingAssure(const std::vector<uint8_t>& payload, CCastServer* server)
    {
        DEBUGLOG(yellow, "send ping assure from cast");
        auto sid = Utility::FromVector<uint32_t>(payload);
        if (auto player_session = server->GetSessionByIdNoLock(sid))
            player_session->SendMsg(0, 0, 0, 0); // send keep alive ack
    }
}