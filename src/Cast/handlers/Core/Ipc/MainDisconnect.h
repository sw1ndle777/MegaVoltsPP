#pragma once
namespace Game::Handlers
{
    using namespace BaseLib;
    using namespace NetEngine;

    inline void IpcMainDisconnect(const std::vector<uint8_t>& payload, CCastServer* server)
    {
        auto auth_key = Utility::FromVector<uint64_t>(payload);
        auto sid = *CAuthKey.get<shared_t>(auth_key);
        DEBUGLOG(dark_cyan, "ipc disconnect player auth key: ({}), sid=({})", auth_key, sid);
        if (auto pss = server->GetSessionByIdNoLock(sid))
        {
            pss->Disconnect();
            DEBUGLOG(dark_cyan, "sid=({}) disconnected at ipc's request", sid);
        }
        else
            DEBUGLOG(dark_cyan, "sid=({}) disconnected at ipc's request ERROR SESSION ID NOT FOUND", sid);
    }
}