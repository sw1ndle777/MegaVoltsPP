#pragma once
namespace Game
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Front;

    namespace Handlers
    {
        struct ReqDisconnectAid
        {
            uint32_t sid;
            int32_t aid;
        };
        inline void IpcMainDisconnect(const std::vector<uint8_t>& payload, CFrontServer* front_server)
        {
            auto data = Utility::FromVector<ReqDisconnectAid>(payload);
            if (!CAccount.contains(data.aid))
            {
                DEBUGLOG(red, "ipc disconnect aid=({}), but not found in cache", data.aid);
                return;
            }
            auto player = CAccount.get<shared_t>(data.aid);
            uint64_t key = 0;
#if defined(RELEASE_1_0_3)
            key = player->plazaAuth.AuthKey;
#else
            key = player->frontAccount.AuthKey;
#endif
            DEBUGLOG(dark_cyan, "main server disconnect aid=({}) key=({}), you can reconnect now", data.aid, key);
#if defined(RELEASE_1_0_3)
            if (auto ss = front_server->GetSessionById(data.sid))
            {
                if (player->plazaAuth.has2fa && !player->plazaAuth.isVerified2fa)
                    ss->SendMsg(AUTH_AUTHORIZE, 0, FrontAuthorize::Type::EmailNotVerified, 0);
                else
                    ss->SendMsg(AUTH_AUTHORIZE, 0, FrontAuthorize::Type::Success, player->plazaAuth.Grade, reinterpret_cast<uint8_t*>(&key), sizeof(key));
                    
            }
                
#else
            FrontUserAccountInfo  accountInfo =
            {
                player->frontAccount.Level + 1,
                player->frontAccount.Experience,
                player->frontAccount.Kills,
                player->frontAccount.Deaths,
                player->frontAccount.Assists,
                player->frontAccount.Wins,
                player->frontAccount.Loses,
                player->frontAccount.Draws,
                player->frontAccount.Nickname.c_str(),
                static_cast<std::uint16_t>(player->frontAccount.ClanId ? player->clanInfo.logo_front : 0),
                static_cast<std::uint16_t>(player->frontAccount.ClanId ? player->clanInfo.logo_back : 0),
                player->frontAccount.ClanId ? player->clanInfo.name.c_str() : "",
                0
            };
            FrontLoginAuthorizeAck authorizeData = FrontLoginAuthorizeAck(key, accountInfo);
            if (auto ss = front_server->GetSessionById(data.sid))
                ss->SendMsg(22, 0, FrontAuthorize::Type::Success, player->frontAccount.Grade, reinterpret_cast<uint8_t*>(&authorizeData), sizeof(FrontLoginAuthorizeAck));
#endif
            player.unlock();
			CAuthKeys.erase(key);
			CAccount.erase(data.aid);
        }
    }
}