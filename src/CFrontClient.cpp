#include "CFrontClient.h"

namespace Game
{
    CFrontClient::CFrontClient()
    {
        //Initialize front client callbacks
        this->On(0x191, handle_FrontEngineServerConnectionAck);
        this->On(0x16, handle_FrontLoginAuthorizeAck);
        this->On(0x19, handle_FrontLoginReconnectAck);
        this->On(0x17, handle_FrontServerInfoAck);
    }

    CFrontClient::~CFrontClient()
    {
    }

    void CFrontClient::handle_FrontEngineServerConnectionAck(SCallbackData& callback)
    {
        CSession* session = callback.session;
        FrontEngineServerConnectionAck* connectionAck = (FrontEngineServerConnectionAck*)callback.message->GetData();

        EventLog->Info("CFrontClient() - Received handshake request from server");
        std::printf("CFrontClient() - Received handshake request from server\n");

        session->SetSessionId(callback.message->GetSession());
        session->SetEncryptionKey(connectionAck->cryptoKey);

        EventLog->Info("CFrontClient() - Updated session id and encryption key");
        std::printf("CFrontClient() - Updated session id and encryption key\n");

        FrontLoginAuthorizeReq loginAuthorizeReq = FrontLoginAuthorizeReq();
        loginAuthorizeReq.cryptoKey = connectionAck->cryptoKey;
        loginAuthorizeReq.serverTime = connectionAck->serverTime;
        strcpy(loginAuthorizeReq.password, "test");
        strcpy(loginAuthorizeReq.username, "test");

        CMessage loginAuthorizeReqMessage = CMessage(session->GetEncryptionKey());
        loginAuthorizeReqMessage.SetSession(session->GetSessionId());
        loginAuthorizeReqMessage.SetCommand(0x16, 0x00, 0x34, 0x00);
        loginAuthorizeReqMessage.SetData(reinterpret_cast<uint8_t*>(&loginAuthorizeReq), sizeof(FrontLoginAuthorizeReq));

        session->Send(loginAuthorizeReqMessage);

        EventLog->Info("CFrontClient() - Sent login authorize acknowledge");
        std::printf("CFrontClient() - Sent login authorize acknowledge\n");
    }

    void CFrontClient::handle_FrontLoginAuthorizeAck(SCallbackData& callback)
    {
        CSession* session = callback.session;
        FrontLoginAuthorizeAck* authorizeAck = (FrontLoginAuthorizeAck*)callback.message->GetData();

        EventLog->Info("CFrontClient() - Received front authorization response from server");
        std::printf("CFrontClient() - Received front authorization response from server\n");
    }

    void CFrontClient::handle_FrontLoginReconnectAck(SCallbackData& callback)
    {
    }

    void CFrontClient::handle_FrontServerInfoAck(SCallbackData& callback)
    {
    }
}