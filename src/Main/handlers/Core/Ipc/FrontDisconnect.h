#pragma once
namespace Game::Handlers
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Front;
    struct ReqDisconnectAid
    {
        uint32_t sid;
        int32_t aid;
    };
    inline void IpcFrontDisconnect(const std::vector<uint8_t>& payload, CMainServer* main_server)
    {
        auto req = Utility::FromVector<ReqDisconnectAid>(payload);
        auto is_online = CAidSid.contains(req.aid);
        DEBUGLOG(dark_cyan, "ipc disconnect player request aid=({}) online=({})", req.aid, is_online ? "true" : "false");
        if (is_online)
        {
            auto online_sid = *CAidSid.get<shared_t>(req.aid);

            auto player = CAccount.get<unique_t>(online_sid);
            auto player_session_id = player->session_id;
            if (player_session_id)
            {
                player->multiple_accs_logged_in = true;
                player->front_sid = req.sid;
                player.unlock();
                DEBUGLOG(dark_cyan, "ipc disconnect player request sid=({})", player_session_id);
                main_server->DisconnectPlayer(player_session_id, Disconnect::Block);
            }
            else
            {
                player.unlock();
                DEBUGLOG(red, "ipc disconnect player request sid=({}) is null", player_session_id);
            }
        }
        else
        {
            struct ReqDisconnectAid
            {
                uint32_t sid;
                int32_t aid;
            }data{ req.sid, req.aid };
            main_server->SendFrontIpc(PacketIds::Ipc::MainToFrontAcknowledgeAidDisconnected, Utility::ToVector(data));
        }
    }
}