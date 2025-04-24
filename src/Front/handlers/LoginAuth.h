#pragma once
namespace Game
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Front;

    namespace Handlers
    {


        inline void LoginAuth(SCallbackData& callback, CFrontServer* front_server)
        {    
            auto session = callback.session;
            auto message = callback.message;
            if (!session || !message) return;
            
            
            BaseLib::DbPool->submit_task([=]() mutable
            {
                std::shared_lock lock(session->GetMutex());
                auto loginAuthorizeReq = reinterpret_cast<FrontLoginAuthorizeReq*>(message->GetData());
                auto acc_user = Utility::ReadMVString({ loginAuthorizeReq->username, sizeof(loginAuthorizeReq->username) });
                auto acc_pass = Utility::ReadMVString({ loginAuthorizeReq->password, sizeof(loginAuthorizeReq->password) });
                EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "authorize request id: ({}), password: ({})", acc_user.c_str(), acc_pass.c_str());
                BaseLib::FrontAccount frontAccount;
                BaseLib::ClanInfo clanInfo;
                auto found = BaseLib::Database->GetFrontAccount(acc_user, acc_pass, &frontAccount, &clanInfo);


                if (!found)
                {
                    CMessage authorizeAck = CMessage(session->GetEncryptionKey());
                    authorizeAck.SetSession(session->GetSessionId());
                    authorizeAck.SetCommand(22, 0, FrontAuthorize::Type::DontExist, 0);
                    session->Send(authorizeAck);
                }
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

                    CMessage authorizeAck = CMessage(session->GetEncryptionKey());
                    authorizeAck.SetSession(session->GetSessionId());
                    authorizeAck.SetCommand(22, 0, frontAccount.IsOnline ? FrontAuthorize::Type::Busy : FrontAuthorize::Type::Success, 0);
                    authorizeAck.SetData(reinterpret_cast<uint8_t*>(&authorizeData), sizeof(FrontLoginAuthorizeAck));
                    session->Send(authorizeAck);
                }
            }, BS::pr::highest);
            /*
            BaseLib::FrontAccount frontAccount;
            auto authorize = [&](const FrontAuthorize::Type& authorize_type, const uint8_t& grade, FrontLoginAuthorizeAck* optionalData = nullptr)
            {
                CMessage authorizeAck = CMessage(session->GetEncryptionKey());
                authorizeAck.SetSession(session->GetSessionId());
                authorizeAck.SetCommand(22, 0, authorize_type, grade);

                if (optionalData != nullptr)
                    authorizeAck.SetData(reinterpret_cast<uint8_t*>(optionalData), sizeof(FrontLoginAuthorizeAck));

                session->Send(authorizeAck);
            };

            if (!BaseLib::Database->GetFrontAccount(acc_user, acc_pass, acc_pass, &frontAccount))
            {
                if (!BaseLib::Database->RegisterAccount(acc_user, acc_pass, 4, 10000000, 10000000, 250, 100, 5000, 1000, 5000, acc_user))
                {
                    authorize(FrontAuthorize::Type::DontExist, 0x00);
                    EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "couldn't register id: ({}), password: ({})", acc_user.c_str(), acc_pass.c_str());
                    return;
                }
                else
                {
                    if (!BaseLib::Database->GetFrontAccount(acc_user, acc_pass, acc_pass, &frontAccount))
                    {
                        authorize(FrontAuthorize::Type::DontExist, 0x00);
                        EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "couldn't find id: ({}), password: ({})", acc_user.c_str(), acc_pass.c_str());
                        return;
                    }
                }
            }

            if (!Utility::IsPasswordValid(acc_pass, frontAccount.Password.c_str(), frontAccount.Salt.c_str()))
            {
                authorize(FrontAuthorize::Type::DontExist, 0x00);
                EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "incorrect id: ({}) or password: ({})", acc_user.c_str(), acc_pass.c_str());
                return;
            }

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
                0,
                0,
                "",
                0
            };
            if (frontAccount.ClanId)
            {
                BaseLib::ClanInfo clanInfo;
                if (BaseLib::Database->GetClanInfo(frontAccount.ClanId, &clanInfo))
                {
                    accountInfo.clanLogoFront = clanInfo.logo_front;
                    accountInfo.clanLogoBack = clanInfo.logo_back;
                    std::strcpy(accountInfo.clanName, clanInfo.name.c_str());
                }
            }
            FrontLoginAuthorizeAck authorizeData = FrontLoginAuthorizeAck(frontAccount.AuthKey, accountInfo);
            auto is_player_online = front_server->IsPlayerAlready(frontAccount.AuthKey);
            if (is_player_online)
            {
                authorize(FrontAuthorize::Type::Busy, frontAccount.Grade, &authorizeData);
            }
            else
            {
                authorize(FrontAuthorize::Type::Success, frontAccount.Grade, &authorizeData);
                front_server->AddPlayerCache(frontAccount.AuthKey, Player{ frontAccount.AuthKey });
            }
            */
        }
    }
}
