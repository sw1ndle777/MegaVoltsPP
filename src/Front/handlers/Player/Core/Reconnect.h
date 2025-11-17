#pragma once
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

		auto auth_key = message->GetData<uint64_t>();
        [[maybe_unused]] auto ignored_result = BaseLib::DbPool->submit_task([front_server, session = std::move(callback.session), key = std::move(auth_key)]() mutable
            {
                auto sid = session->GetSessionId();
                if (CAuthKeys.contains(key)) return;
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