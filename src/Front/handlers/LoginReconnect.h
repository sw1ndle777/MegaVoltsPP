#pragma once
namespace Game
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Front;

    namespace Handlers
    {
        inline void LoginReconnect(SCallbackData& callback, CFrontServer* front_server)
        {
            auto session = callback.session;
            if (!session) return;
            [[maybe_unused]] auto ignored_result = BaseLib::DbPool->submit_task([session = std::move(callback.session), message = std::move(*callback.message)]() mutable
            {
                std::shared_lock lock(session->GetMutex());
                auto loginReconnectReq = reinterpret_cast<FrontLoginReconnectReq*>(message.GetData());
                auto auth_key = loginReconnectReq->authKey;
                EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "authorize reconnect request auth key: ({})", auth_key);
                BaseLib::FrontAccount frontAccount;
                BaseLib::ClanInfo clanInfo;
                auto found = BaseLib::Database->GetFrontAccount(auth_key, &frontAccount, &clanInfo);

                if (!found)
                    session->SendMsg(22, 0, FrontAuthorize::Type::DontExist, 0);
                else
                {
                    FrontUserAccountInfo  accountInfo =
                    {
                        frontAccount.Level + 1,
                        frontAccount.Experience,
                        frontAccount.Kills,
                        frontAccount.Deaths,
                        frontAccount.Assists,
                        frontAccount.Wins,
                        frontAccount.Loses,
                        frontAccount.Draws,
                        frontAccount.Nickname.c_str(),
                        static_cast<std::uint16_t>(frontAccount.ClanId ? clanInfo.logo_front : 0),
                        static_cast<std::uint16_t>(frontAccount.ClanId ? clanInfo.logo_back : 0),
                        frontAccount.ClanId ? clanInfo.name.c_str() : "",
                        0
                    };
                    FrontLoginAuthorizeAck authorizeData = FrontLoginAuthorizeAck(frontAccount.AuthKey, accountInfo);

                    session->SendMsg(22, 0, FrontAuthorize::Type::Success, 0, reinterpret_cast<uint8_t*>(&authorizeData), sizeof(FrontLoginAuthorizeAck));
                }
            }, BS::pr::highest);
        }
    } 
}