#pragma once
namespace Game::Handlers
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Main;

#pragma pack(push, 1)

    union PartyPlayerInfo3 {
        struct {
            uint64_t totalClanWins : 23;
            uint64_t totalClanLosses : 23;
            uint64_t totalClanDraws : 14;
            uint64_t padding : 4;
        };
        uint64_t data;
    };

    union PartyPlayerInfo2 {
        struct {
            uint32_t ClanPadding : 3;
            uint32_t ClanContribution : 22;
            uint32_t unk1 : 7;
        };
        uint32_t data;
    };

    struct PartyPlayerInfo
    {
        NetEngine::Packets::Core::UniqueId uid;//0

        RoomUserPlayerInfo1 info1;//4

        PartyPlayerInfo2 info2;//8

        PartyPlayerInfo3 info3;//12

        char nickname[16];//20

        PartyPlayerInfo(const NetEngine::Packets::Core::UniqueId& playerUid, const RoomUserPlayerInfo1& info1,
            const PartyPlayerInfo2& info2,
            const PartyPlayerInfo3& info3,
            const char* name)
        {
            std::memset(this, 0, sizeof(PartyPlayerInfo));
            this->uid = playerUid;
            this->info1 = info1;
            this->info2 = info2;
            this->info3 = info3;
            std::strcpy(this->nickname, name);
        }
    };

    struct PartyPlayerInfoJoin
    {
        char nickname[16];//0
        PartyPlayerInfo2 info2;//16 corect
        PartyPlayerInfo3 info3;//20
        NetEngine::Packets::Core::UniqueId uid;//28
        RoomUserPlayerInfo1 info1;//32
        EquipItemNumber equipped[17];
        PartyPlayerInfoJoin(const NetEngine::Packets::Core::UniqueId& playerUid,
            const RoomUserPlayerInfo1& info1,
            const PartyPlayerInfo2& info2,
            const PartyPlayerInfo3& info3,
            const std::vector<EquipItemNumber>& equippedItems,
            const char* name)
        {
            std::memset(this, 0, sizeof(PartyPlayerInfoJoin));
            this->uid = playerUid;
            this->info1 = info1;
            this->info2 = info2;
            this->info3 = info3;
            std::strcpy(this->nickname, name);
            for (auto i = 0; i < 17 && i < equippedItems.size(); i++)
                this->equipped[i] = equippedItems[i];
        }
    };
#pragma pack(pop)

    inline void PartyJoin(SCallbackData& callback, CMainServer* main_server)
    {
        auto session = callback.session;
        auto message = callback.message;
        if (!session || !message) return;

        std::shared_lock lock(session->GetMutex());
        CServer* server = callback.server;
        auto sid = session->GetSessionId();
        auto acc = CAccount.get<unique_t>(sid);
        auto aid = acc->acc_info.Index;
        const auto nick = acc->acc_info.Nickname; // local copy for logging
        auto my_uid = NetEngine::Packets::Core::UniqueId(sid, 1).data;
        if (aid == -1) return;

        auto roomId = *reinterpret_cast<uint32_t*>(callback.message->GetData());
        DEBUGLOG(dark_cyan, "player want to join party id: ({})", roomId);
        if (!CParty.contains(roomId))
        {
            DEBUGLOG(dark_cyan, "could not find party id ({})", roomId);
            return;
        }
        auto party = CParty.get<unique_t>(roomId);

        if (std::ranges::contains(party->kicked_members, sid))
        {
            DEBUGLOG(dark_cyan, "player try to join but was kick");
            session->SendMsg(message->GetOrder(), message->GetMission(), 42, message->GetOption());
            return;
        }
        if (party->is_clan && acc->acc_info.ClanId != party->clan_id)
        {
            DEBUGLOG(dark_cyan, "player try to join a clan party he dont belong with");
            session->SendMsg(message->GetOrder(), message->GetMission(), 43, message->GetOption());
            return;
        }
        if (party->members.size() >= party->max_members)
        {
            session->SendMsg(message->GetOrder(), message->GetMission(), 7, message->GetOption());
            return;
        }

        uint32_t party_room_id = 0;
        auto leader_acc_cache = CAccount.get<shared_t>(party->party_host_session_id);
        party_room_id = leader_acc_cache->in_room ? leader_acc_cache->room_id : 0;
        leader_acc_cache.unlock();
        if (party_room_id != 0 || party->is_registered)
        {
            session->SendMsg(message->GetOrder(), message->GetMission(), 5, message->GetOption());
            return;
        }

		auto my_equipped = main_server->GetEquippedItems(acc);
        auto my_info1 = main_server->GetRoomUserPlayerInfo1(acc);
        PartyPlayerInfo2 my_info2{};
		my_info2.ClanContribution = acc->acc_info.ClanContribution;
        PartyPlayerInfo3 my_info3 =
        {
            acc->acc_info.ClanWins,
            acc->acc_info.ClanLoses,
            acc->acc_info.ClanDraws
        };
        auto myJoininfo = PartyPlayerInfoJoin(acc->uid, my_info1, my_info2, my_info3, my_equipped, acc->acc_info.Nickname.c_str());

        std::vector<PartyPlayerInfo> PartyInfoList;


        PlayerRoomClanListInfo my_clan_info;
        if (acc->acc_info.ClanId && CClan.contains(acc->acc_info.ClanId))
        {
            auto clan_info = CClan.get<shared_t>(acc->acc_info.ClanId);
            my_clan_info = PlayerRoomClanListInfo(party->members.size(), clan_info->clan_name.c_str(), clan_info->logo_front, clan_info->logo_back, acc->acc_info.ClanId, 0);
            clan_info.unlock();
        }
        else
            my_clan_info = PlayerRoomClanListInfo(1, "", 0, 0, 0, 0);

        std::vector<PlayerRoomClanListInfo> players_clan_info;

        uint32_t idx = 0;

        std::vector<PartyEquipInfoAck> others_equipinfo;

        for (const auto& id : party->members)
        {
            auto other_acc = CAccount.get<shared_t>(id);
            auto other_uid = NetEngine::Packets::Core::UniqueId(id, 1).data;

            auto other_info1 = main_server->GetRoomUserPlayerInfo1(other_acc);
            PartyPlayerInfo2 other_info2{};
            other_info2.ClanContribution = other_acc->acc_info.ClanContribution;
            PartyPlayerInfo3 other_info3 =
            {
                other_acc->acc_info.ClanWins,
                other_acc->acc_info.ClanLoses,
                other_acc->acc_info.ClanDraws
            };
       
            auto party_member = PartyPlayerInfo(other_uid, other_info1, other_info2, other_info3, other_acc->acc_info.Nickname.c_str());
            PartyInfoList.push_back(party_member);
			auto other_items = main_server->GetEquippedItems(other_acc);
            others_equipinfo.push_back(PartyEquipInfoAck(other_uid, other_items));

            if (auto pss = server->GetSessionById(id))
            {
                pss->SendMsg(418, 0, 1, 0, reinterpret_cast<uint8_t*>(&myJoininfo), sizeof(myJoininfo));
                pss->SendMsg(409, NetEngine::Room::Clan::IconUpdateMission::PartyMembers, 37, 1, reinterpret_cast<uint8_t*>(&my_clan_info), sizeof(PlayerRoomClanListInfo));
            }

            if (other_acc->acc_info.ClanId)
            {
                if (CClan.contains(other_acc->acc_info.ClanId))
                {
                    auto clan_info = CClan.get<shared_t>(other_acc->acc_info.ClanId);
                    auto info = PlayerRoomClanListInfo(idx, clan_info->clan_name.c_str(), clan_info->logo_front, clan_info->logo_back, acc->acc_info.ClanId, 0);
                    clan_info.unlock();
                    players_clan_info.push_back(info);
                }
            }
            else
                players_clan_info.push_back(PlayerRoomClanListInfo(idx, "", 0, 0, 0, 0));

            idx++;
        }
        session->SendMsg(417, 0, 37, PartyInfoList.size(), reinterpret_cast<uint8_t*>(PartyInfoList.data()), sizeof(PartyPlayerInfo) * PartyInfoList.size());
        session->SendMsg(303, 0, 37, others_equipinfo.size(), reinterpret_cast<uint8_t*>(others_equipinfo.data()), others_equipinfo.size() * sizeof(PartyEquipInfoAck));

        session->SendMsg(message->GetOrder(), 0, 1, 0);

        session->SendMsg(409, NetEngine::Room::Clan::IconUpdateMission::PartyMembers, 37, players_clan_info.size(), reinterpret_cast<uint8_t*>(players_clan_info.data()), sizeof(PlayerRoomClanListInfo) * players_clan_info.size());

        acc->in_party = true;
        acc->party_id = roomId;
        party->members.push_back(sid);

        /*leave plaza start*/
        if (acc->in_plaza)
        {
            auto plaza_id = acc->plaza_id;
            if (main_server->IsPlazaAlready(plaza_id))
            {
                DEBUGLOG(dark_cyan, "player will leave plaza: ({})", plaza_id);
                auto plaza = CPlaza.get<unique_t>(plaza_id);
                auto& sids = plaza->session_ids;

                if (std::ranges::contains(sids, sid))
                {
                    auto my_uid = acc->uid;
                    for (const auto& id : sids)
                    {
                        if (id == sid) continue;
                        if (auto pss = server->GetSessionById(id))
                            pss->SendMsg(425, 0, 0, 1, reinterpret_cast<uint8_t*>(&my_uid), sizeof(my_uid));
                    }
                    DEBUGLOG(dark_cyan, "user=({}) sid=({}) left plaza id: ({})", nick.c_str(), sid, plaza_id);
                    std::erase(plaza->session_ids, sid);
                    acc->plaza_id = 0;
                    acc->in_plaza = false;
                }
            }
        }
        /*leave plaza end*/

        uint32_t room_type = (party->is_clan ? 1 : 2); // 0 = normal room, 1 = clan party, 2 = normal party
        session->SendMsg(116, 0, party->mod_id, party->map_id, reinterpret_cast<uint8_t*>(&room_type), sizeof(room_type));
    }
}