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
        int32_t aid;
        uint64_t key;
        uint32_t max_health;
        uint32_t current_health;
        char nickname[16];
        char hwid[65];
        uint16_t room_id;
        uint8_t is_playing;
        uint8_t _pad{};
    };
    inline void IpcCastAuthorize(const std::vector<uint8_t>& payload, CMainServer* main_server, int retries = 0)
    {
        auto req = Utility::FromVector<NetEngine::Packets::Cast::CastConnectionReq>(payload);
        auto sid = *CAuthKey.get<shared_t>(req.Authkey);
        DEBUGLOG(dark_cyan, "ipc acknowledge request sid=({}) key=({}) attempt=({})", sid, req.Authkey, retries + 1);
        if (sid)
        {
            auto player = CAccount.get<shared_t>(sid);
            AuthoriseData auth_data{};
            auth_data.old_uid = req.UniqueId;
            auth_data.uid = player->uid;
            auth_data.aid = player->acc_info.Index;
            auth_data.key = player->acc_info.AuthKey;
            auth_data.max_health = player->max_health;
            auth_data.current_health = player->current_health;
            std::strcpy(auth_data.nickname, player->acc_info.Nickname.c_str());
            std::strncpy(auth_data.hwid, player->hwid.c_str(), sizeof(auth_data.hwid) - 1);
            auth_data.room_id = player->in_room ? player->room_id : 0;
            auth_data.is_playing = 0;
            if (player->in_room && CRoom.contains(player->room_id))
            {
                auto room = CRoom.get<shared_t>(player->room_id);
                if (room)
                    auth_data.is_playing = room->is_playing ? 1 : 0;
            }
            main_server->SendCastIpc(PacketIds::Ipc::MainToCastAuthorizePlayer, Utility::ToVector(auth_data));
            player.unlock();
            DEBUGLOG(dark_cyan, "ipc acknowledge auth sid=({}) old uid=({}) new uid=({}) key=({}) name=({})", sid, req.UniqueId.data, auth_data.uid.data, auth_data.key, auth_data.nickname);
        }
        else if (retries < 10)
        {
            auto payloadCopy = payload;
            auto timer = std::make_shared<asio::steady_timer>(main_server->GetIoContext(), std::chrono::milliseconds(200));
            timer->async_wait([timer, payloadCopy = std::move(payloadCopy), main_server, retries](const asio::error_code& ec)
            {
                if (!ec)
                    IpcCastAuthorize(payloadCopy, main_server, retries + 1);
            });
        }
        else
            DEBUGLOG(red, "ipc acknowledge auth sid=({}) failed after ({}) retries", sid, retries);
    }
}
