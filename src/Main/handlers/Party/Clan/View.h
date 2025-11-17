#pragma once
namespace Game::Handlers
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Main;
#pragma pack(push, 1)
    struct RegisteredClanInfo
    {
        uint16_t party_id;
        uint16_t clan_id;
        union
        {
            struct
            {
                uint32_t unk : 3;//0
                uint32_t has_password : 1;//3
                uint32_t mode_index : 5;//4
                uint32_t unk2 : 4;//9
                uint32_t max_players : 4;//13
                uint32_t map_index : 7;//17
                uint32_t level : 7;//24
                uint32_t clanLogoFrontIdLastBit : 1;//31
            };
            uint32_t info_data;
        };
        union
        {
            struct
            {
                uint32_t clanLogoFrontId : 15;
                uint32_t clanLogoBackId : 16;
                uint32_t clanPadding : 1;
            };
            uint32_t logo_data;
        };
        char clanName[16];
        char leaderName[16];

        RegisteredClanInfo(
            uint16_t party_id,
            uint16_t clan_id,
            bool has_password,
            uint32_t mode_index,
            uint32_t max_players,
            uint32_t map_index,
            uint32_t level,
            uint32_t clan_front_icon,
            uint32_t clan_back_icon,
            std::string clan_name,
            std::string party_leader_name
        ) {
            std::memset(this, 0, sizeof(RegisteredClanInfo));

            this->party_id = party_id;
            this->clan_id = clan_id;
            this->has_password = has_password;
            this->mode_index = mode_index;
            this->max_players = max_players;
            this->map_index = map_index;
            this->level = level;
            this->clanLogoFrontId = (clan_front_icon >> 1) & 0x7FFF;
            this->clanLogoFrontIdLastBit = clan_front_icon & 0x1;
            this->clanLogoBackId = clan_back_icon;

            std::strcpy(this->clanName, clan_name.c_str());
            std::strcpy(this->leaderName, party_leader_name.c_str());
        }
    };
#pragma pack(pop)

    inline void PartyClanView(SCallbackData& callback, CMainServer* main_server)
    {
        auto session = callback.session;
        auto message = callback.message;
        if (!session || !message) return;

        std::shared_lock lock(session->GetMutex());
        CServer* server = callback.server;
        auto order = message->GetOrder();
        auto sid = session->GetSessionId();
        auto acc = CAccount.get<unique_t>(sid);
        auto aid = acc->acc_info.Index;
        auto my_team_id = acc->team_id;
        if (aid == -1) return;

        if (!acc->in_party) return;

        auto party_ids = CPartyId.get_all(shared);
        auto my_clan_id = acc->acc_info.ClanId;
        acc.unlock();
        DEBUGLOG(dark_cyan, "will build party clan list! now exist ({}) party order ({})", party_ids->size(), order);
        if (party_ids->empty())
        {
            session->SendMsg(order, 0, 6, 0);
            return;
        }
        uint32_t max_batch_size = 31;
        uint32_t party_blocks_count = (party_ids->size() + max_batch_size - 1) / max_batch_size;
        DEBUGLOG(dark_cyan, "batch block count ({})", party_blocks_count);
        for (uint32_t batch_id = 0; batch_id < party_blocks_count; batch_id++)
        {
            std::vector<RegisteredClanInfo> StandbyClanList;
            auto extra = (batch_id == 0) ? NetEngine::Room::List::SendRoom : NetEngine::Room::List::SendRoom2;
            uint32_t start_index = batch_id * max_batch_size;
            uint32_t end_index = std::min(start_index + max_batch_size, static_cast<uint32_t>(party_ids->size()));
            DEBUGLOG(dark_cyan, "run from ({}) to ({})", start_index, end_index);
            for (auto i = start_index; i < end_index; i++)
            {
                if (!CParty.contains(party_ids->at(i)))
                {
                    DEBUGLOG(dark_cyan, "could not find party id ({})", party_ids->at(i));
                    continue;
                }
                auto party = CParty.get<unique_t>(party_ids->at(i));
                DEBUGLOG(dark_cyan, "check party id ({})", party_ids->at(i));
				DEBUGLOG(dark_cyan, "is clan ({}) is registered ({}) clan id ({}) my clan id ({})", party->is_clan, party->is_registered, party->clan_id, my_clan_id);
                if (!party->is_clan || !party->is_registered || party->clan_id == my_clan_id) continue;
                DEBUGLOG(dark_cyan, "check passed");
                auto clan = CClan.get<unique_t>(party->clan_id);
                auto leader = CAccount.get<unique_t>(party->party_host_session_id);

                //REPLACE WITH MAX_MEMBERS NOT SIZE OF MEMBERS !!
                StandbyClanList.emplace_back(RegisteredClanInfo(
                    party_ids->at(i),
                    party->clan_id,
                    party->has_password,
                    party->mod_id,
                    party->members.size(),
                    party->map_id,
                    leader->acc_info.Level + 1,
                    clan->logo_front,
                    clan->logo_back,
                    clan->clan_name,
                    leader->acc_info.Nickname
                ));
            }
            session->SendMsg(order, 0, extra, StandbyClanList.size(), reinterpret_cast<uint8_t*>(StandbyClanList.data()), sizeof(RegisteredClanInfo) * StandbyClanList.size());
        }
    }
}