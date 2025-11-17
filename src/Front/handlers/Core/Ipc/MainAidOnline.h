#pragma once
namespace Game
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Front;

    namespace Handlers
    {
        struct AcknowledgeAidOnline
        {
            uint32_t sid;
            int32_t aid;
            bool isOnline;
        };
        inline void IpcMainAidOnline(const std::vector<uint8_t>& payload, CFrontServer* front_server)
        {
            auto data = Utility::FromVector<AcknowledgeAidOnline>(payload);
            auto player = CAccount.get<unique_t>(data.aid);

            DEBUGLOG(dark_cyan, "ipc acknowledge aid: ({}), isOnline: ({})", data.aid, data.isOnline);

#if defined(RELEASE_1_0_3)
            if (auto ss = front_server->GetSessionById(data.sid))
                ss->SendMsg(22, 0, data.isOnline ? FrontAuthorize::Type::Busy : FrontAuthorize::Type::Success, player->plazaAuth.Grade, reinterpret_cast<uint8_t*>(&player->plazaAuth.AuthKey), sizeof(player->plazaAuth.AuthKey));

            CAuthKeys.erase(player->plazaAuth.AuthKey);
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
            FrontLoginAuthorizeAck authorizeData = FrontLoginAuthorizeAck(player->frontAccount.AuthKey, accountInfo);
            if (auto ss = front_server->GetSessionById(data.sid))
            {
                ss->SendMsg(AUTH_AUTHORIZE, 0, data.isOnline ? FrontAuthorize::Type::Busy : FrontAuthorize::Type::Success, player->frontAccount.Grade, reinterpret_cast<uint8_t*>(&authorizeData), sizeof(FrontLoginAuthorizeAck));
				PACKETLOG(ACK, AUTH_AUTHORIZE, "sid=({}) user: ({})", data.sid, player->frontAccount.Username.c_str());
            }
                

            CAuthKeys.erase(player->frontAccount.AuthKey);
#endif
			
            player.unlock();
			CAccount.erase(data.aid);
            //front_server->RemovePlayerCache(data.aid);
        }
    }
}