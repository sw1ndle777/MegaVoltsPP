#pragma once
namespace Game::Handlers
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Cast;
    using enum EOrder;
	using enum fmt::color;
    inline void ServerConnect(std::shared_ptr<CSession> session, CCastServer* cast_server)
    {
        if (!session) return;
        auto random = Utility::Random::CustomGen(100000000, 999999999);
        auto connAck = CastEngineServerConnectionAck(random);
        session->SendMsg(INFO_SERVER_CONN, 00, 54, 00, reinterpret_cast<uint8_t*>(&connAck), sizeof(connAck));
        DEBUGLOG(dark_cyan, "sid=({}) connection acknowledged", session->GetSessionId());
    }
}