#pragma once
namespace Game
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Main;

    namespace Handlers
    {
        inline void ServerConnect(std::shared_ptr<CSession> session, CMainServer* main_server)
        {
            //const auto random_number = Utility::Random::CustomGen(100000000, 999999999);
            std::unique_lock lock(session->GetMutex());
            MainEngineServerConnectionAck mainEngineServerConnectionAck = MainEngineServerConnectionAck(static_cast<std::int32_t>(rand() + 1), session->GetSessionId(), 1);

            session->SetEncryptionKey(mainEngineServerConnectionAck.cryptoKey);

            CMessage mainEngineServerConnectionAckMessage = CMessage(session->GetEncryptionKey());
            mainEngineServerConnectionAckMessage.SetSession(session->GetSessionId());
            mainEngineServerConnectionAckMessage.SetCommand(401, 0, 0, 1);
            mainEngineServerConnectionAckMessage.SetData(reinterpret_cast<uint8_t*>(&mainEngineServerConnectionAck), sizeof(MainEngineServerConnectionAck));
            mainEngineServerConnectionAckMessage.SetEncryptMethod(SendOption::EncryptionMethod::Default);
            session->Send(mainEngineServerConnectionAckMessage);

            BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "session id: ({}) connection acknowledged", session->GetSessionId());
        }
    }
    
}