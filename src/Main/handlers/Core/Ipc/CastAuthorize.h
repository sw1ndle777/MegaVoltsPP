#pragma once
namespace Game::Handlers
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Front;
    struct AuthoriseData
    {
        NetEngine::Packets::Core::UniqueId old_uid;
        NetEngine::Packets::Core::UniqueId uid;
        uint64_t key;
        char nickname[16];
    };
    inline void IpcCastAuthorize(const std::vector<uint8_t>& payload, CMainServer* main_server)
    {
        auto req = Utility::FromVector<NetEngine::Packets::Cast::CastConnectionReq>(payload);
        auto sid = *CAuthKey.get<shared_t>(req.Authkey);
        DEBUGLOG(dark_cyan, "ipc acknowledge request sid=({}) key=({})", sid, req.Authkey);
        if (sid)
        {
            auto player = CAccount.get<shared_t>(sid);
            AuthoriseData auth_data{};
            auth_data.old_uid = req.UniqueId;
            auth_data.uid = player->uid;
            auth_data.key = player->acc_info.AuthKey;
            std::strcpy(auth_data.nickname, player->acc_info.Nickname.c_str());
            main_server->SendCastIpc(PacketIds::Ipc::MainToCastAuthorizePlayer, Utility::ToVector(auth_data));
            player.unlock();
            DEBUGLOG(dark_cyan, "ipc acknowledge auth sid=({}) old uid=({}) new uid=({}) key=({}) name=({})", sid, req.UniqueId.data, auth_data.uid.data, auth_data.key, auth_data.nickname);
        }
        else
            DEBUGLOG(red, "ipc acknowledge auth sid=({}) is null", sid);
    }
}