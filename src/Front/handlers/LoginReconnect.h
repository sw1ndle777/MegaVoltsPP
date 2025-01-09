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
            std::shared_lock lock(callback.session->GetMutex());
            CSession* session = callback.session;
            CServer* server = callback.server;
            auto order = callback.message->GetOrder();
            auto mission = callback.message->GetMission();
            auto extra = callback.message->GetExtra();
            auto option = callback.message->GetOption();

            auto loginReconnectReq = reinterpret_cast<FrontLoginReconnectReq*>(callback.message->GetData());
            auto auth_key = loginReconnectReq->authKey;
            
            BaseLib::FrontAccount frontAccount;
            auto authorize = [&](const FrontAuthorize::Type& authorize_type, const std::uint8_t& grade, FrontLoginAuthorizeAck* optionalData = nullptr)
            {
                CMessage authorizeAck = CMessage(session->GetEncryptionKey());
                authorizeAck.SetSession(session->GetSessionId());
                authorizeAck.SetCommand(22, 0, authorize_type, grade);

                if (optionalData != nullptr)
                    authorizeAck.SetData(reinterpret_cast<uint8_t*>(optionalData), sizeof(FrontLoginAuthorizeAck));

                session->Send(authorizeAck);
            };

            if (!BaseLib::Database->GetFrontAccount(auth_key, &frontAccount))
            {
                if (!BaseLib::Database->GetFrontAccount(auth_key, &frontAccount))
                {
                    authorize(FrontAuthorize::Type::DontExist, 0x00);
                    EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "couldn't find auth key: ({})", auth_key);
                    return;
                }
            }

            FrontUserAccountInfo  accountInfo = {
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

            auto player_cache = front_server->GetPlayerCacheUnique(frontAccount.AuthKey);
            if (player_cache->auth_key)
            {
                player_cache->forcefully_logged_out = true;
                front_server->SendMainIpc(PacketIds::Ipc::FrontToMainDisconnectPlayer, Utility::ToVector(auth_key));
                FrontLoginAuthorizeAck authorizeData = FrontLoginAuthorizeAck(frontAccount.AuthKey, accountInfo);
                authorize(FrontAuthorize::Type::Success, frontAccount.Grade, &authorizeData);
            }
            player_cache.unlock();
            /*
            asio::post([callback, front_server, auth_key]()
            {
                
            });   
            */
        }
    } 
}