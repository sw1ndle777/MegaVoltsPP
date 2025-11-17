#pragma once
namespace Game::Handlers
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Front;
#pragma pack(push, 1)

    struct ServerInfoHeader
    {
        uint64_t auth_key;
        uint16_t count;
        uint32_t mem;
        double   cpu;
    };

    struct PlayerInfoWire
    {
        uint16_t session_id;
        uint16_t room_id;
        uint16_t plaza_id;
        uint8_t  flags;
    };

#pragma pack(pop)
    inline void ParseFlags(uint8_t flags, bool& in_room, bool& in_plaza)
    {
        in_room = (flags & 0x1) != 0;
        in_plaza = (flags & 0x2) != 0;
    }
    inline void IpcCastMetrics(const std::vector<uint8_t>& payload, CMainServer* main_server)
    {
        if (payload.size() < sizeof(ServerInfoHeader))
        {
            DEBUGLOG(red, "IpcCastMetrics payload size is too small: {}", payload.size());
            return;
        }

        ServerInfoHeader hdr{};
        std::memcpy(&hdr, payload.data(), sizeof(hdr));

        size_t expected = sizeof(ServerInfoHeader) + hdr.count * sizeof(PlayerInfoWire);
        if (payload.size() < expected)
        {
            DEBUGLOG(red, "IpcCastMetrics payload size is too small for player count {}: {} < {}", hdr.count, payload.size(), expected);
            return;
        }

        const auto* p = reinterpret_cast<const PlayerInfoWire*>(payload.data() + sizeof(hdr));

        std::vector<PlayerInfoWire> players;
        players.reserve(hdr.count);


        auto player_sid = *CAuthKey.get<shared_t>(hdr.auth_key);
        if (auto player_session = main_server->GetSessionById(player_sid))
        {
            auto msg = fmt::format("[MegaVolts Online] cast: sids size={} mem usage={}MB cpu usage={:.2f}%",
                static_cast<uint16_t>(hdr.count),
                static_cast<uint32_t>(hdr.mem),
                static_cast<double>(hdr.cpu));
            main_server->SendServerMessage(player_session.get(), msg.c_str());

            for (uint16_t i = 0; i < hdr.count; i++)
            {
                bool in_room{}, in_plaza{};
                ParseFlags(p[i].flags, in_room, in_plaza);
                auto msg2 = fmt::format("sid={} roomId={} plazaId={} inRoom={}, inPlaza={}",
                    p[i].session_id, p[i].room_id, p[i].plaza_id, in_room ? "true" : "false", in_plaza ? "true" : "false");
                main_server->SendServerMessage(player_session.get(), msg2.c_str());
            }
        }
        else
            DEBUGLOG(red, "IpcCastMetrics: player session not found for key={}", hdr.auth_key);
    }
}