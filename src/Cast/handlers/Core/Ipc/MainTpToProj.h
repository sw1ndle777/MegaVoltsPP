#pragma once
namespace Game::Handlers
{
    using namespace BaseLib;
    using namespace NetEngine;

    inline void IpcMainTpToProj(const std::vector<uint8_t>& payload, CCastServer* /*server*/)
    {
        if (payload.size() < sizeof(NetEngine::Packets::Ipc::MainToCastTpToProjToggle))
            return;

        const auto data = Utility::FromVector<NetEngine::Packets::Ipc::MainToCastTpToProjToggle>(payload);
        if (!data.sid)
            return;

        auto enabled_sids = Game::g_tp_to_proj_sids.get_all(unique);
        if (data.enabled)
            enabled_sids->insert(data.sid);
        else
            enabled_sids->erase(data.sid);
        enabled_sids.unlock();

        DEBUGLOG(dark_cyan, "MainToCastTpToProjToggle sid=({}) enabled=({})", data.sid, data.enabled ? "true" : "false");
    }
}
