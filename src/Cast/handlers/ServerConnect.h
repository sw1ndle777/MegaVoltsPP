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
            std::shared_lock lock(session->GetMutex());
            auto random_number = Utility::Random::CustomGen(100000000, 999999999);
            CastEngineServerConnectionAck castEngineServerConnectionAck = CastEngineServerConnectionAck(random_number);

            CMessage castEngineServerConnectionAckMessage = CMessage();
            castEngineServerConnectionAckMessage.SetSession(session->GetSessionId());
            castEngineServerConnectionAckMessage.SetCommand(401, 0x00, 54, 0x00);
            castEngineServerConnectionAckMessage.SetData(reinterpret_cast<uint8_t*>(&castEngineServerConnectionAck), sizeof(CastEngineServerConnectionAck));
            session->Send(castEngineServerConnectionAckMessage);

            BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "session id: ({}) connection acknowledged", session->GetSessionId());
        }
    }
    
}