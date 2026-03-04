#pragma once
namespace Game::Handlers
{
    using namespace BaseLib;
    using namespace NetEngine;

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

    inline uint8_t MakeFlags(bool in_room, bool in_plaza)
    {
        return (in_room ? 0x1 : 0) | (in_plaza ? 0x2 : 0);
    }

    inline void IpcMainServerInfo(const std::vector<uint8_t>& payload, CCastServer* server)
    {
        auto auth_key = Utility::FromVector<uint64_t>(payload);
#ifdef _WIN32
        HANDLE m_process_handle = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, false, GetCurrentProcessId());
        auto cpu_usage = Utility::GetCpuUsage(m_process_handle);
        auto mem_usage = static_cast<uint32_t>(Utility::GetMemoryUsage(m_process_handle));
        CloseHandle(m_process_handle);
#else
        auto cpu_usage = Utility::GetCpuUsage(nullptr);
        auto mem_usage = static_cast<uint32_t>(Utility::GetMemoryUsage(nullptr));
#endif
        auto sessions_list = server->GetSessions();


        ServerInfoHeader hdr
        {
            .auth_key = auth_key,
            .count = static_cast<uint16_t>(sessions_list->size()),
            .mem = mem_usage,
            .cpu = cpu_usage
        };

        std::vector<uint8_t> out;
        out.resize(sizeof(hdr) + hdr.count * sizeof(PlayerInfoWire));
        std::memcpy(out.data(), &hdr, sizeof(hdr));
        std::vector<PlayerInfoWire> players;
        for (auto& sid : *sessions_list)
        {
            auto player = CAccount.get<shared_t>(sid.first);
            auto& room_id = player->room_id;
            auto& plaza_id = player->plaza_id;
            auto& in_room = player->in_room;
            auto& in_plaza = player->in_plaza;

            players.emplace_back(PlayerInfoWire{
                .session_id = sid.first,
                .room_id = room_id,
                .plaza_id = plaza_id,
                .flags = MakeFlags(in_room, in_plaza)
                });
        }

        auto* p = reinterpret_cast<PlayerInfoWire*>(out.data() + sizeof(hdr));
        for (size_t i = 0; i < players.size(); i++)
        {
            const auto& src = players[i];
            p[i] = src;
        }

        server->SendMainIpc(PacketIds::Ipc::CastToMainAckServerInfo, std::move(out));
    }
}
