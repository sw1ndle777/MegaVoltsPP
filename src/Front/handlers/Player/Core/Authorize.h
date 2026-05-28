#pragma once
#include "BaseLib/CLogging.h"
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
                    session->SendMsg(AUTH_AUTHORIZE, 0, FrontAuthorize::Type::DontExist, 0);
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
                            DEBUGLOG(red, "authorize attempt aid=({}) already pending on front sid=({})", aid, existing_sid);
                            return;
                        }
                        existing.unlock();
                        if (existing_key)
                            CAuthKeys.erase(existing_key);
                        CAccount.erase(aid);
                        DEBUGLOG(yellow, "evicted stale front cache entry aid=({}) sid=({}) key=({}) before authorize", aid, existing_sid, existing_key);
                    }

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
                    if (!CAccount.insert(aid, player))
                    {
#if defined(RELEASE_1_0_3)
                        CAuthKeys.erase(player.plazaAuth.AuthKey);
#else
                        CAuthKeys.erase(player.frontAccount.AuthKey);
#endif
                        DEBUGLOG(red, "failed to cache authorize state aid=({}) sid=({})", aid, sid);
                        return;
                    }
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

namespace NetEngine
{
    template <>
    struct PacketRateLimitPolicy<&Game::Handlers::Authorize>
    {
        inline static const std::optional<RateLimit::Rule> value = RateLimit::Rule{
            .enabled = true,
            .bucket_scope = RateLimit::IdentityScope::Ip,
            .max_packets = 5,
            .window = std::chrono::seconds{ 10 },
            .on_limit = [](RateLimit::ActionContext& ctx)
            {
                ctx.CooldownIp(std::chrono::seconds{ 30 });

                BaseLib::AcDetectionLogEntry entry{};
                entry.aid = ctx.identity.aid > 0 ? ctx.identity.aid : 0;
                entry.ip = ctx.identity.ip;
                entry.hwid = ctx.identity.hwid;
                entry.detection_flag = BaseLib::AcDetection::Flag::LoginSpam;
                entry.extra = ctx.packet_count;
                entry.server_id = 0;

                [[maybe_unused]] auto ignored = BaseLib::DbPool->submit_task([
                    entry = std::move(entry)
                ]() mutable
                {
                    BaseLib::Database->PersistAcDetectionLogs({ entry });
                });
            },
        };
    };
}
