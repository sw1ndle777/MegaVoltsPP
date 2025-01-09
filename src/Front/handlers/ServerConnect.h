#pragma once
namespace Game
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Front;

    namespace Handlers
    {
        inline void ServerConnect(std::shared_ptr<CSession> session, CFrontServer* front_server)
        {
            std::unique_lock lock(session->GetMutex());
            //const auto random_number = Utility::Random::CustomGen(100000000, 999999999);
            auto utc_now = Utility::GetUtcTimeNow();
            auto time_zone = "GMT+2";
            auto readable_time = Utility::GetReadableTime(utc_now, time_zone);
            FrontEngineServerConnectionAck frontEngineServerConnectionAck = FrontEngineServerConnectionAck(static_cast<std::int32_t>(rand() + 1), utc_now);

            session->SetEncryptionKey(frontEngineServerConnectionAck.cryptoKey);

            CMessage frontEngineServerConnectionAckMessage = CMessage(session->GetEncryptionKey());
            frontEngineServerConnectionAckMessage.SetSession(session->GetSessionId());
            frontEngineServerConnectionAckMessage.SetCommand(0x191, 0x00, 0x22, 0x00);
            frontEngineServerConnectionAckMessage.SetData(reinterpret_cast<uint8_t*>(&frontEngineServerConnectionAck), sizeof(FrontEngineServerConnectionAck));//sizeof(FrontEngineServerConnectionAck));
            frontEngineServerConnectionAckMessage.SetEncryptMethod(SendOption::EncryptionMethod::Default);
            session->Send(frontEngineServerConnectionAckMessage);


            EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "crypto Key: ({}), utc now: ({})", frontEngineServerConnectionAck.cryptoKey, utc_now);
        }
    }
    
}