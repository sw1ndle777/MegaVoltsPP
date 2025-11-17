#pragma once
namespace Game::Handlers
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Front;
    inline void IpcCastSid(const std::vector<uint8_t>& payload, CMainServer* main_server)
    {
        auto sid = Utility::FromVector<uint32_t>(payload);
        if (auto pss = main_server->GetSessionById(sid))
        {
            auto msg = fmt::format("[CAST] sid=({})", static_cast<uint32_t>(sid));
            main_server->SendServerMessage(pss.get(), msg.c_str());
        }
        else
            DEBUGLOG(red, "ipc cast sid request sid=({}) is null", sid);
    }
}