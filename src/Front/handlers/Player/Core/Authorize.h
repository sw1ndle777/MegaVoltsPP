#pragma once
namespace Game::Handlers
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Front;
    using enum EOrder;
    inline void Authorize(SCallbackData& callback, CFrontServer* front_server)
    {
        auto session = callback.session;
        auto message = callback.message;
        if (!session || !message) return;

        auto req = reinterpret_cast<FrontLoginAuthorizeReq*>(message->GetData());
        auto acc_user = Utility::ReadMVString({ req->username, sizeof(req->username) });
        auto acc_pass = Utility::ReadMVString({ req->password, sizeof(req->password) });

        [[maybe_unused]] auto ignored_result = BaseLib::DbPool->submit_task([front_server, session = std::move(callback.session), user = std::move(acc_user), pass = std::move(acc_pass)]() mutable
            {
                if (!session) return;
                auto sid = session->GetSessionId();
				PACKETLOG(REQ, AUTH_AUTHORIZE, "sid=({}) user=({})", sid, user.c_str());
                auto found = false;
                int32_t aid = 0;
#if defined(RELEASE_1_0_3)
                BaseLib::PlazaAuth plazaAuth;
                found = BaseLib::Database->GetPlazaAuthKey(session->GetIpAddress(), user, pass, &plazaAuth);
                aid = plazaAuth.Index;
#else
                BaseLib::FrontAccount frontAccount;
                BaseLib::ClanInfo clanInfo;
                found = BaseLib::Database->GetFrontAccount(session->GetIpAddress(), user, pass, &frontAccount, &clanInfo);
                aid = frontAccount.Index;
#endif
                if (!found)
                    session->SendMsg(AUTH_AUTHORIZE, 0, FrontAuthorize::Type::DontExist, 0);
                else
                {


                    auto player = Player();
                    player.sid = sid;
#if defined(RELEASE_1_0_3)    
                    if (!plazaAuth.emailVerified)
                    {
                        session->SendMsg(AUTH_AUTHORIZE, 0, FrontAuthorize::Type::EmailNotVerified, 0);
                        return;
                    }
                    player.plazaAuth = std::move(plazaAuth);
                    CAuthKeys.insert(player.plazaAuth.AuthKey);
#else
                    player.frontAccount = std::move(frontAccount);
                    player.clanInfo = std::move(clanInfo);
                    CAuthKeys.insert(player.frontAccount.AuthKey);
#endif
                    CAccount.insert(aid, player);
                    struct ReqAidOnline
                    {
                        uint32_t sid;
                        int32_t aid;
                    } req{ sid, aid };
                    front_server->SendMainIpc(PacketIds::Ipc::FrontToMainTryLoginPlayer, Utility::ToVector(req));
                }
            }, BS::pr::highest);
    }
}
