#pragma once
namespace Game
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Front;
	using namespace NetEngine::PacketId::Front;
    namespace Handlers
    {
        inline void ServerConnect(std::shared_ptr<CSession> session, CFrontServer* front_server)
        {
            if (!session) return;
            std::unique_lock lock(session->GetMutex());
            const auto random_number = Utility::Random::CustomGen(100000000, 999999999);
            auto utc_now = Utility::GetUtcTimeNow();
            auto time_zone = "GMT+2";
            auto readable_time = Utility::GetReadableTime(utc_now, time_zone);
            FrontEngineServerConnectionAck frontEngineServerConnectionAck = FrontEngineServerConnectionAck(random_number, utc_now);
            session->SetEncryptionKey(frontEngineServerConnectionAck.cryptoKey);
            session->SendMsg(GsToCl::EngineConnectionInit, 0, 34, 0, reinterpret_cast<uint8_t*>(&frontEngineServerConnectionAck), sizeof(FrontEngineServerConnectionAck), SendOption::EncryptionMethod::Default);
            EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "crypto Key: ({}), utc now: ({})", frontEngineServerConnectionAck.cryptoKey, utc_now);
        }
    } 
}