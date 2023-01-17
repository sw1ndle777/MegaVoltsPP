#include "CFrontServer.h"
#include "Utility.h"
namespace Game
{
    struct FrontAuthorize
    {
        enum Type : std::uint8_t
        {
            Wrong = 0x00,
            Success = 0x01,
            DataError = 0x04,
            Busy = 0x05,
            DontExist = 0x0D,
            TimeExpire2 = 0x18,
            Blocked = 0x2A,
            Shutdown = 0x50
        };
    };
    struct ChannelInfo
    {
        enum Status : std::uint8_t
        {
            Normal = 0x00,
            Busy = 0x01,
            VeryBusy = 0x02,
            Offline = 0x03
        };
    };
    
    CFrontServer::CFrontServer()
    {
        this->OnNewSession(handle_FrontEngineServerConnection);
        this->OnSessionDisconnected(handle_FrontEngineServerDisconnection);
        this->On(0x16, handle_FrontLoginAuthorizeReq);
        this->On(0x19, handle_FrontLoginReconnectReq);
        this->On(0x17, handle_FrontServerInfoReq);
    }

    CFrontServer::~CFrontServer()
    {
    }

    void CFrontServer::handle_FrontEngineServerConnection(std::shared_ptr<CSession> session)
    {
        auto random_number = 0;//Utility::Random::Random().Gen();
        auto utc_now = Utility::GetUtcTimeNow();
        auto time_zone = "GMT+2";
        auto readable_time = Utility::GetReadableTime(utc_now, time_zone);
        FrontEngineServerConnectionAck frontEngineServerConnectionAck = FrontEngineServerConnectionAck(random_number, utc_now);

        session->SetEncryptionKey(frontEngineServerConnectionAck.cryptoKey);

        CMessage frontEngineServerConnectionAckMessage = CMessage(session->GetEncryptionKey());
        frontEngineServerConnectionAckMessage.SetSession(session->GetSessionId());
        frontEngineServerConnectionAckMessage.SetCommand(0x191, 0x00, 0x22, 0x00);
        frontEngineServerConnectionAckMessage.SetData(reinterpret_cast<uint8_t*>(&frontEngineServerConnectionAck), sizeof(FrontEngineServerConnectionAck));

        session->Send(frontEngineServerConnectionAckMessage);

        EventLog->Info("CFrontServer() - Sent engine server connection acknowledge");
        EventLog->Info("CFrontServer() - [Crypto key: %d] [Server time: %s] [Time zone: %s]", random_number, readable_time.c_str(), time_zone);
        std::printf("CFrontServer() - Sent engine server connection acknowledge\n");
        std::printf("CFrontServer() - [Crypto key: %d] [Server time: %s] [Time zone: %s]\n", random_number, readable_time.c_str(), time_zone);
    }
    void CFrontServer::handle_FrontEngineServerDisconnection(std::shared_ptr<CSession> session)
    {
        EventLog->Info("CFrontServer() - Session id: %d disconnected", session->GetSessionId());
        std::printf("CFrontServer() - Session id: %d disconnected\n", session->GetSessionId());
    }

    void CFrontServer::handle_FrontLoginAuthorizeReq(SCallbackData& callback)
    {
        CSession* session = callback.session;
        FrontLoginAuthorizeReq* loginAuthorizeReq = (FrontLoginAuthorizeReq*)callback.message->GetData();

        EventLog->Info("CFrontServer() - Received login authorize request from client (id: %s) (pw: %s)", loginAuthorizeReq->username, loginAuthorizeReq->password);
        std::printf("CFrontServer() - Received login authorize request from client (id: %s) (pw: %s)\n", loginAuthorizeReq->username, loginAuthorizeReq->password);

        FrontUserAccountInfo  accountInfo;
        accountInfo.level = 100;
        accountInfo.experience = 300;
        accountInfo.kills = 1337;
        accountInfo.deaths = 69;
        accountInfo.assists = 777;
        accountInfo.wins = 9999;
        accountInfo.losses = 1;
        accountInfo.draws = 444;
        accountInfo.clanLogoFront = 301;
        accountInfo.clanLogoBack = 302;
        accountInfo.unknown = 3;
        
       
        strcpy(accountInfo.nickname, "sw1ndle");
        strcpy(accountInfo.clanName, "");

        FrontLoginAuthorizeAck frontLoginAuthorizeAck = FrontLoginAuthorizeAck(100000000000000, accountInfo);

        CMessage frontLoginAuthorizeAckMessage = CMessage(session->GetEncryptionKey());
        frontLoginAuthorizeAckMessage.SetSession(session->GetSessionId());
        frontLoginAuthorizeAckMessage.SetCommand(0x16, 0x00, FrontAuthorize::Type::Success, 0x09);//player grade
        frontLoginAuthorizeAckMessage.SetData(reinterpret_cast<uint8_t*>(&frontLoginAuthorizeAck), sizeof(FrontLoginAuthorizeAck));


        //auto header_array = Utility::GetBytesArray(reinterpret_cast<uint8_t*>(frontLoginAuthorizeAckMessage.GetHeader().data), sizeof(frontLoginAuthorizeAckMessage.GetHeader().data));
        //auto command_array = Utility::GetBytesArray(reinterpret_cast<uint8_t*>(frontLoginAuthorizeAckMessage.GetCommand().data), sizeof(frontLoginAuthorizeAckMessage.GetCommand().data));
        

        session->Send(frontLoginAuthorizeAckMessage);
        auto data_array = Utility::GetBytesArray(frontLoginAuthorizeAckMessage.GetData(), frontLoginAuthorizeAckMessage.GetDataSize());
        std::printf("CFrontServer(Size:%d) - %s\n", frontLoginAuthorizeAckMessage.GetDataSize(), data_array.c_str());

        EventLog->Info("CFrontServer() - Sent login authorize info for (%s)", accountInfo.nickname);
        std::printf("CFrontServer() - Sent login authorize info for (%s)\n", accountInfo.nickname);
    }

    void CFrontServer::handle_FrontLoginReconnectReq(SCallbackData& callback)
    {
    }

    void CFrontServer::handle_FrontServerInfoReq(SCallbackData& callback)
    {
        CSession* session = callback.session;

        EventLog->Info("CFrontServer() - Received server info request from client");
        std::printf("CFrontServer() - Received server info request from client\n");

        std::vector<FrontServerInfo> server_infos;
        for (std::uint32_t i = 0; i < 6; i++)
        {
            FrontServerInfo server_info;
            server_info.serverId = i;
            server_info.channel1 = ChannelInfo::Status::Normal;
            server_info.channel2 = ChannelInfo::Status::Busy;
            server_info.channel3 = ChannelInfo::Status::VeryBusy;
            server_infos.push_back(server_info);
        }
        FrontServerInfoAck frontServerInfoAck = FrontServerInfoAck(server_infos);

        CMessage frontServerInfoAckMessage = CMessage(session->GetEncryptionKey());
        frontServerInfoAckMessage.SetSession(session->GetSessionId());
        frontServerInfoAckMessage.SetCommand(0x17, 0x00, 0, server_infos.size());
        frontServerInfoAckMessage.SetData(reinterpret_cast<uint8_t*>(&frontServerInfoAck), sizeof(FrontServerInfoAck));

        session->Send(frontServerInfoAckMessage);
        EventLog->Info("CFrontServer() - Sent (%d) server%s channel info", server_infos.size(), (server_infos.size() == 1) ? "" : "s");
        std::printf("CFrontServer() - Sent (%d) server%s channel info\n", server_infos.size(), (server_infos.size() == 1) ? "" : "s");
        
    }
}