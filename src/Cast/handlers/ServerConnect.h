#pragma once
namespace Game
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Cast;

    namespace Handlers
    {
        inline void ServerConnect(std::shared_ptr<CSession> session, CCastServer* cast_server)
        {
            if (!session) return;
            std::unique_lock lock(session->GetMutex());
            auto random_number = Utility::Random::CustomGen(100000000, 999999999);
            CastEngineServerConnectionAck castEngineServerConnectionAck = CastEngineServerConnectionAck(random_number);

            session->SendMsg(401, 00, 54, 00, reinterpret_cast<uint8_t*>(&castEngineServerConnectionAck), sizeof(CastEngineServerConnectionAck));

            BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "session id: ({}) connection acknowledged", session->GetSessionId());
        }
    }
    
}