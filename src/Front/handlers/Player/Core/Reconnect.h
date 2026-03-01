#pragma once
#include "BaseLib/otp/cotp.hpp"
namespace Game::Handlers
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Front;
    using enum fmt::color;
    inline void Reconnect(SCallbackData& callback, CFrontServer* front_server)
    {
        auto session = callback.session;
        auto message = callback.message;
        if (!session || !message) return;

		//auto auth_key = message->GetData<uint64_t>();
        auto req = reinterpret_cast<FrontLoginReconnectReq*>(message->GetData());
        [[maybe_unused]] auto ignored_result = BaseLib::DbPool->submit_task([front_server, session = std::move(callback.session), req = std::move(req)]() mutable
            {
                auto sid = session->GetSessionId();
				auto key = req->authKey;
				auto code = req->code2fa;
				DEBUGLOG(blue, "Reconnect attempt with auth key=({}), sid=({})", key, sid);
                if (CAuthKeys.contains(key))
                {
					DEBUGLOG(red, "Reconnect attempt with already used auth key=({}), sid=({})", key, sid);
                    return;
                }
                auto found = false;
                int32_t aid = 0;
#if defined(RELEASE_1_0_3)
                BaseLib::PlazaAuth plazaAuth;
                found = BaseLib::Database->GetPlazaAuthKey(session->GetIpAddress(), key, &plazaAuth);
                aid = plazaAuth.Index;
#else
                BaseLib::FrontAccount frontAccount;
                BaseLib::ClanInfo clanInfo;
                found = BaseLib::Database->GetFrontAccount(key, &frontAccount, &clanInfo);
                aid = frontAccount.Index;
#endif
                if (!found)
                    session->SendMsg(AUTH_AUTHORIZE, 0, FrontAuthorize::Type::TimeExpire2, 0);
                else
                {
                    auto player = Player();
                    player.sid = sid;
                    if (CAccount.contains(aid)) return;

#if defined(RELEASE_1_0_3)

                    cotp::TOTP totp(
                        plazaAuth.secret2fa,
                        "SHA1",   // Google Auth uses SHA1
                        6,        // 6-digit code
                        30        // 30-second interval
                    );


					plazaAuth.isVerified2fa = totp.verify(code, 1);
                    if (!plazaAuth.isVerified2fa)
                    {
                        cotp::HOTP hotp(
                            plazaAuth.secret2fa,
                            "SHA1",   // Google Auth uses SHA1
                            6        // 6-digit code
						);
                        plazaAuth.isVerified2fa = hotp.verify(code);
                    }

                    if (!plazaAuth.isVerified2fa)
                    {
                        session->SendMsg(AUTH_AUTHORIZE, 0, FrontAuthorize::Type::Fail2fa, 0);
                        return;
                    }

					DEBUGLOG(blue, "2FA verification result for aid=({}), sid=({}): {} code=({}), secret=({})", aid, sid, plazaAuth.isVerified2fa ? "Success" : "Failure", code, plazaAuth.secret2fa.c_str());
                    player.plazaAuth = plazaAuth;
                    CAuthKeys.insert(player.plazaAuth.AuthKey);
#else
                    player.frontAccount = frontAccount;
                    player.clanInfo = clanInfo;
                    CAuthKeys.insert(player.frontAccount.AuthKey);
#endif           
                    CAccount.insert(aid, player);
                    struct ReqDisconnectAid
                    {
                        uint32_t sid;
                        int32_t aid;
                    } req{ sid, aid };
                    front_server->SendMainIpc(PacketIds::Ipc::FrontToMainDisconnectPlayer, Utility::ToVector(req));
                    DEBUGLOG(dark_cyan, "authorize reconnect ipc for key=({})", key);

                }
            }, BS::pr::highest);
    }
}