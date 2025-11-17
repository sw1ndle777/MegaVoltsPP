#pragma once
namespace Game::Handlers
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Main;

    struct PartyInfo
    {
        uint16_t clanRoomId;
        uint16_t clanRoomNumber;

        union
        {
            struct
            {
                uint32_t numPlayers : 4;//0
                uint32_t maxPlayers : 4;//4
                uint32_t unknown0 : 1;//8
                uint32_t hasMatchStarted : 1;//9
                uint32_t unknown1 : 1;//10
                uint32_t isQueueing : 1;//11
                uint32_t leaderLevel : 7;//12
                uint32_t unknown3 : 12;
            };
            uint32_t data;
        };

        char leaderName[16];

        PartyInfo(uint16_t roomId = 0, uint16_t roomNumber = 0,
            uint32_t players = 1, uint32_t maxP = 8,
            uint32_t matchStarted = 0, uint32_t leaderLvl = 0,
            const char* leader = "")
        {
            std::memset(this, 0, sizeof(PartyInfo));
            this->clanRoomId = roomId;
            this->clanRoomNumber = roomNumber;
            this->numPlayers = players;
            this->maxPlayers = maxP;
            this->hasMatchStarted = (matchStarted == 2 ? 1 : 0);
            this->isQueueing = (matchStarted == 1 ? 1 : 0);
            this->leaderLevel = leaderLvl + 1;
            std::strcpy(this->leaderName, leader);
        }
    };

    std::vector<PartyInfo> CraftPartyInfoList(CMainServer* main_server, uint32_t sid, ClanCacheSharedResource& clan)
    {
        std::vector<PartyInfo> result;
        for (const auto& id : clan->online_members)
        {
            if (id == sid) continue;
            auto acc = CAccount.get<shared_t>(id);
            if (!acc->in_party) continue;
            auto party_id = acc->party_id;
            if (!CParty.contains(party_id))
            {
                DEBUGLOG(dark_cyan, "could not find player's party id ({})", party_id);
                continue;
            }
            auto party = CParty.get<unique_t>(party_id);
            if (!party->is_clan || party->party_host_session_id != id) continue;
            uint32_t queueState = 0;
            if (acc->in_room) queueState = (acc->playing ? 2 : 1);
            PartyInfo current_party =
            {
                static_cast<uint16_t>(party_id),
                static_cast<uint16_t>(clan->clan_id),
                static_cast<uint32_t>(party->members.size()),
                party->max_members,
                queueState,
                acc->acc_info.Level,
                acc->acc_info.Nickname.c_str()
            };
            result.push_back(current_party);
        }
        return result;
    }

    inline void PartyView(SCallbackData& callback, CMainServer* main_server)
    {
        auto session = callback.session;
        auto message = callback.message;
        if (!session || !message) return;
        //std::shared_lock lock(session->GetMutex());
        CServer* server = callback.server;
        auto sid = session->GetSessionId();
        auto acc = CAccount.get<unique_t>(sid);
        auto aid = acc->acc_info.Index;
        if (aid == -1 || acc->acc_info.ClanId == -1) return;
        auto clan_id = acc->acc_info.ClanId;
        auto clan = CClan.get<shared_t>(clan_id);
        acc.unlock();
        auto clanInfoList = CraftPartyInfoList(main_server, sid, clan);
        session->SendMsg(message->GetOrder(), message->GetMission(), 37, clanInfoList.size(), reinterpret_cast<uint8_t*>(clanInfoList.data()), sizeof(PartyInfo) * clanInfoList.size());
    }
}