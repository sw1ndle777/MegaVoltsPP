#pragma once
namespace Game::Handlers
{
    using namespace BaseLib;
    using namespace NetEngine;
    struct AuthoriseData
    {
        NetEngine::Packets::Core::UniqueId old_uid;
        NetEngine::Packets::Core::UniqueId uid;
        uint64_t key;
        char nickname[16];
    };
    inline void IpcMainAuthorize(const std::vector<uint8_t>& payload, CCastServer* server)
    {
        auto auth_data = Utility::FromVector<AuthoriseData>(payload);
        auto new_player = Player{ static_cast<uint16_t>(auth_data.uid.session), 0, 0, 0, 
            static_cast<uint16_t>(auth_data.uid.server), 
            PlayerInfo::State::Connected, false, false, 
            auth_data.key, auth_data.nickname };
        CAccount.insert(new_player.session_id, new_player);
        if (server->AdoptSid(auth_data.old_uid.session, auth_data.uid.session))
        {
            if (auto pss = server->GetSessionById(auth_data.uid.session))
            {
                pss->SendMsg(501, 0, 32, 1);
                DEBUGLOG(dark_cyan, "authorized player ({}) with auth key: ({}), sid=({}), server: ({})", 
                    new_player.nickname.c_str(), new_player.auth_key, static_cast<uint16_t>(new_player.session_id), static_cast<uint16_t>(new_player.server_id));
            }
        }
        else
        {
            DEBUGLOG(red, "failed to adopt session id for player ({}) with auth key: ({}), sid=({}), server: ({})", 
                new_player.nickname.c_str(), new_player.auth_key, static_cast<uint16_t>(new_player.session_id), static_cast<uint16_t>(new_player.server_id));
        }
    }
}