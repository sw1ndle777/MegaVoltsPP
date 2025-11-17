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
            //std::unique_lock lock(session->GetMutex());
			auto sid = session->GetSessionId();
            const auto random_number = Utility::Random::CustomGen(100000000, 999999999);
            auto utc_now = Utility::GetUtcTimeNow();
            auto time_zone = "GMT+2";
            auto readable_time = Utility::GetReadableTime(utc_now, time_zone);
            auto ack = FrontEngineServerConnectionAck(random_number, utc_now);
            session->SetEncryptionKey(ack.cryptoKey);
            session->SendMsg(INFO_SERVER_CONN, 0, 34, 0, reinterpret_cast<uint8_t*>(&ack), sizeof(ack), SendOption::EncryptionMethod::Default);
			DEBUGLOG(dark_cyan, "connected sid=({}) crypto key=({}) utc now=({}) readable time=({})", sid, ack.cryptoKey, utc_now, readable_time.c_str());
        }
    } 
}