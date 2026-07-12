#pragma once
namespace Game::Handlers
{
    using namespace BaseLib;
    using namespace NetEngine;
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
    inline void IpcMainAuthorize(const std::vector<uint8_t>& payload, CCastServer* server, int retries = 0)
    {
        auto auth_data = Utility::FromVector<AuthoriseData>(payload);
        auto new_player = Player{ static_cast<uint16_t>(auth_data.uid.session), 0, 0, 0,
            static_cast<uint16_t>(auth_data.uid.server),
            PlayerInfo::State::Connected, false, false,
            auth_data.key, auth_data.nickname };
        new_player.account_id = auth_data.aid;
        new_player.hwid = auth_data.hwid;
        new_player.max_health = auth_data.max_health ? auth_data.max_health : 1000;
        new_player.current_health = auth_data.current_health ? std::min(auth_data.current_health, new_player.max_health) : new_player.max_health;
        new_player.health = new_player.current_health;
        new_player.combat_health = new_player.current_health;
        new_player.combat_health_known = true;
        new_player.is_dead = new_player.current_health == 0;
        new_player.current_kill_streak = 0;
        new_player.highest_kill_streak = 0;

        if (auth_data.room_id && CRoom.contains(auth_data.room_id))
        {
            new_player.room_id = auth_data.room_id;
            new_player.in_room = true;
            new_player.state_id = auth_data.is_playing ? PlayerInfo::State::Normal : PlayerInfo::State::WaitingRoom;
        }

        if (server->AdoptSid(auth_data.old_uid.session, auth_data.uid.session))
        {
            CAccount.erase(new_player.session_id);
            CAccount.insert(new_player.session_id, new_player);
            CAuthKey.erase(new_player.auth_key);
            CAuthKey.insert(new_player.auth_key, new_player.session_id);
            if (auto pss = server->GetSessionById(auth_data.uid.session))
            {
                pss->SendMsg(501, 0, 32, 1);
                DEBUGLOG(dark_cyan, "authorized player ({}) with auth key: ({}), sid=({}), server: ({})",
                    new_player.nickname.c_str(), new_player.auth_key, static_cast<uint16_t>(new_player.session_id), static_cast<uint16_t>(new_player.server_id));
            }
            return;
        }

        if (retries < 10)
        {
            if (auto stale = server->GetSessionByIdNoLock(auth_data.uid.session))
            {
                DEBUGLOG(yellow, "disconnecting stale cast session sid=({}) before authorize retry=({})",
                    static_cast<uint32_t>(auth_data.uid.session), retries + 1);
                stale->Disconnect();
            }

            auto payload_copy = payload;
            auto timer = std::make_shared<asio::steady_timer>(server->GetIoContext(), std::chrono::milliseconds(200));
            timer->async_wait([timer, payload_copy = std::move(payload_copy), server, retries](const asio::error_code& ec)
            {
                if (!ec)
                    IpcMainAuthorize(payload_copy, server, retries + 1);
            });
            return;
        }

        DEBUGLOG(red, "failed to adopt session id for player ({}) with auth key: ({}), sid=({}), server: ({}) after ({}) retries",
            new_player.nickname.c_str(), new_player.auth_key, static_cast<uint16_t>(new_player.session_id), static_cast<uint16_t>(new_player.server_id), retries + 1);
        if (auto pss = server->GetSessionByIdNoLock(auth_data.old_uid.session))
            pss->Disconnect();
    }
}
