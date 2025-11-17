#pragma once
namespace Game::Handlers
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Main;
    inline void ServerConnect(std::shared_ptr<CSession> session, CMainServer* main_server)
    {
        if (!session) return;
       // std::unique_lock lock(session->GetMutex());
        const auto random_number = Utility::Random::CustomGen(100000000, 999999999);
        MainEngineServerConnectionAck mainEngineServerConnectionAck = MainEngineServerConnectionAck(random_number, session->GetSessionId(), 1);

        session->SetEncryptionKey(mainEngineServerConnectionAck.cryptoKey);
        session->SendMsg(401, 0, 0, 1, reinterpret_cast<uint8_t*>(&mainEngineServerConnectionAck), sizeof(MainEngineServerConnectionAck), SendOption::EncryptionMethod::Default);

        DEBUGLOG(dark_cyan, "connection acknowledged sid=({})", session->GetSessionId());
    }
}