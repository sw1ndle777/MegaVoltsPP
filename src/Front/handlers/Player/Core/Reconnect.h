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
        // Copy the request struct out by value: GetData() points into the packet's
        // CMessage buffer, a stack local in onPacket that is freed before the async
        // DB task runs. Capturing the raw pointer would be a use-after-free.
        auto req = *reinterpret_cast<FrontLoginReconnectReq*>(message->GetData());
        [[maybe_unused]] auto ignored_result = BaseLib::DbPool->submit_task([front_server, session = std::move(callback.session), req = std::move(req)]() mutable
            {
                auto sid = session->GetSessionId();
                auto key = req.authKey;
                auto code = req.code2fa;
                DEBUGLOG(blue, "Reconnect attempt with auth key=({}), sid=({})", key, sid);
                if (CAuthKeys.contains(key))
                {
                    auto stale_key_removed = false;
                    {
                        auto accounts = CAccount.get_all(unique);
                        for (auto it = accounts->begin(); it != accounts->end(); ++it)
                        {
#if defined(RELEASE_1_0_3)
                            auto cached_key = it->second.plazaAuth.AuthKey;
#else
                            auto cached_key = it->second.frontAccount.AuthKey;
#endif
                            if (cached_key != key) continue;
                            auto cached_sid = it->second.sid;
                            if (front_server->GetSessionById(cached_sid))
                            {
                                DEBUGLOG(red, "Reconnect attempt with already used auth key=({}), sid=({})", key, sid);
                                return;
                            }
                            accounts->erase(it);
                            stale_key_removed = true;
                            DEBUGLOG(yellow, "evicted stale front auth key=({}) from sid=({})", key, cached_sid);
                            break;
                        }
                    }
                    if (stale_key_removed)
                        CAuthKeys.erase(key);
                    else
                    {
                        DEBUGLOG(red, "Reconnect attempt with already used auth key=({}), sid=({})", key, sid);
                        return;
                    }
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
                if (found && aid > 0)
                {
                    if (auto active_ban = BaseLib::Database->GetActiveBan(aid); active_ban.has_value())
                    {
                        if (active_ban->has_value())
                        {
                            session->SendMsg(AUTH_AUTHORIZE, 0, FrontAuthorize::Type::Blocked, 0);
                            return;
                        }
                    }
                    else
                    {
                        DEBUGLOG(red, "failed to load active ban for aid=({}): {}", aid, active_ban.error().message);
                        session->SendMsg(AUTH_AUTHORIZE, 0, FrontAuthorize::Type::Busy, 0);
                        return;
                    }
                }
                if (!found)
                    session->SendMsg(AUTH_AUTHORIZE, 0, FrontAuthorize::Type::TimeExpire2, 0);
                else
                {
                    if (CAccount.contains(aid))
                    {
                        auto existing = CAccount.get<unique_t>(aid);
                        auto existing_sid = existing->sid;
                        uint64_t existing_key = 0;
#if defined(RELEASE_1_0_3)
                        existing_key = existing->plazaAuth.AuthKey;
#else
                        existing_key = existing->frontAccount.AuthKey;
#endif
                        if (front_server->GetSessionById(existing_sid))
                        {
                            DEBUGLOG(red, "Reconnect attempt aid=({}) already pending on front sid=({})", aid, existing_sid);
                            return;
                        }
                        existing.unlock();
                        if (existing_key)
                            CAuthKeys.erase(existing_key);
                        CAccount.erase(aid);
                        DEBUGLOG(yellow, "evicted stale front cache entry aid=({}) sid=({}) key=({}) before reconnect", aid, existing_sid, existing_key);
                    }
                    auto player = Player();
                    player.sid = sid;

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
                    if (!CAccount.insert(aid, player))
                    {
#if defined(RELEASE_1_0_3)
                        CAuthKeys.erase(player.plazaAuth.AuthKey);
#else
                        CAuthKeys.erase(player.frontAccount.AuthKey);
#endif
                        DEBUGLOG(red, "failed to cache reconnect state aid=({}) sid=({})", aid, sid);
                        return;
                    }
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
