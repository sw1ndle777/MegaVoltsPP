#pragma once
#include "secure_channel.hpp"

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
        auto sid = session->GetSessionId();

        Game::Anticheat::ConnectAckData connectData;
        connectData.cryptoKey = random_number;
        connectData.uniqueId = NetEngine::Packets::Core::UniqueId(sid, 1);

        Game::Anticheat::ServerSecureChannel channel;
        Game::Anticheat::ServerKeyPayload payload{};
        channel.buildConnectPayload(connectData, payload);

        Game::Anticheat::g_secureChannels.store(sid, std::move(channel));

        session->SetEncryptionKey(connectData.cryptoKey);
        session->SendMsg(401, 0, 0, 1, reinterpret_cast<uint8_t*>(&payload), sizeof(Game::Anticheat::ServerKeyPayload), SendOption::EncryptionMethod::Default);

        DEBUGLOG(dark_cyan, "connection acknowledged sid=({})", session->GetSessionId());
    }
}