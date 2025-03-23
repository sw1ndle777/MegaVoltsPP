#pragma once
namespace Game
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Main;

    namespace Handlers
    {
        namespace structs {
#pragma pack(push, 1)
            struct PartyInfo
            {
                std::uint16_t clanRoomId;
                std::uint16_t clanRoomNumber;

                union {
                    struct {
                        std::uint32_t numPlayers : 4;//0
                        std::uint32_t maxPlayers : 4;//4
                        std::uint32_t unknown0 : 1;//8
                        std::uint32_t hasMatchStarted : 1;//9
                        std::uint32_t unknown1 : 1;//10
                        std::uint32_t isQueueing : 1;//11
                        std::uint32_t leaderLevel : 7;//12
                        std::uint32_t unknown3 : 12;
                    };
                    std::uint32_t data;
                };

                char leaderName[16];

                PartyInfo(const std::uint16_t& roomId = 0, const std::uint16_t& roomNumber = 0,
                    const std::uint32_t& players = 1, const std::uint32_t& maxP = 8,
                    const std::uint32_t& matchStarted = 0, const std::uint32_t& leaderLvl = 0,
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

            union PartyPlayerInfo3 {
                struct {
                    std::uint64_t totalClanWins : 23;
                    std::uint64_t totalClanLosses : 23;
                    std::uint64_t totalClanDraws : 14;
                    std::uint64_t padding : 4;
                };
                std::uint64_t data;
            };

            union PartyPlayerInfo2 {
                struct {
                    std::uint32_t ClanPadding : 3;
                    std::uint32_t ClanContribution : 22;
                    std::uint32_t unk1 : 7;
                };
                std::uint32_t data;
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

            struct PartyPlayerInfoJoin {
                char nickname[16];//0
                PartyPlayerInfo2 info2;//16 corect
                PartyPlayerInfo3 info3;//20
                NetEngine::Packets::Core::UniqueId uid;//28
                RoomUserPlayerInfo1 info1;//32

                std::uint32_t EquippedHairItemId;
                std::uint32_t EquippedFaceItemId;
                std::uint32_t EquippedUpperItemId;
                std::uint32_t EquippedUnderItemId;
                std::uint32_t EquippedPantsItemId;
                std::uint32_t EquippedShirtItemId;
                std::uint32_t EquippedBootsItemId;
                std::uint32_t EquippedGlassItemId;
                std::uint32_t EquippedAccessoryWaistItemId;
                std::uint32_t EquippedAccessoryBackItemId;
                std::uint32_t EquippedMeleeItemId;
                std::uint32_t EquippedRifleItemId;
                std::uint32_t EquippedShotgunItemId;
                std::uint32_t EquippedSniperItemId;
                std::uint32_t EquippedGatlingItemId;
                std::uint32_t EquippedGrenadeItemId;
                std::uint32_t EquippedBazookaItemId;

                PartyPlayerInfoJoin(const NetEngine::Packets::Core::UniqueId& playerUid, const RoomUserPlayerInfo1& info1,
                    const PartyPlayerInfo2& info2,
                    const PartyPlayerInfo3& info3,
                    const char* name)
                {
                    std::memset(this, 0, sizeof(PartyPlayerInfoJoin));
                    this->uid = playerUid;
                    this->info1 = info1;
                    this->info2 = info2;
                    this->info3 = info3;
                    std::strcpy(this->nickname, name);
                }
            };

            struct ClanRoomSettings
            {
                std::uint32_t mode : 5;
                std::uint32_t playersPerTeam : 3;
                std::uint32_t u0 : 1;
                std::uint32_t map : 7;
                std::uint32_t u1 : 3;
                std::uint32_t hasPassword : 1;
                std::uint32_t rest : 12;
                char password[8];
                std::uint64_t u2;

                ClanRoomSettings(const std::uint32_t& gameMode = 0, const std::uint32_t& teamSize = 0,
                    const std::uint32_t& gameMap = 18, const std::uint32_t& passFlag = 0,
                    const char* pass = "")
                {
                    std::memset(this, 0, sizeof(ClanRoomSettings));
                    this->mode = gameMode;
                    this->playersPerTeam = teamSize;
                    this->map = gameMap;
                    this->hasPassword = passFlag;
                    std::strcpy(this->password, pass);
                }
            };

            struct JoinPartyInfo2 {
                char nickname[16];

                std::uint32_t padding1;

                RoomUserPlayerInfo1 info;
                RoomUserPlayerInfo2 info2;

                NetEngine::Packets::Core::UniqueId uid;

                std::uint32_t extraField;

                std::uint8_t additionalData[4];

                JoinPartyInfo2(const char* name = "", const NetEngine::Packets::Core::UniqueId& playerUid = 0) {
                    std::memset(this, 0, sizeof(JoinPartyInfo2));
                    this->uid = playerUid;
                    std::strcpy(this->nickname, name);
                }
            };

            struct JoinPartyInfo
            {
                char nickname[16];
                NetEngine::Packets::Core::UniqueId uid;
                std::uint32_t level : 7;
                std::uint32_t unknown : 3;
                std::uint32_t clanContribution : 22;
                std::uint64_t totalClanWins : 23;
                std::uint64_t totalClanLosses : 23;
                std::uint64_t totalClanDraws : 14;
                std::uint64_t padding : 4;

                JoinPartyInfo(const char* name = "", const NetEngine::Packets::Core::UniqueId& playerUid = 0,
                    const std::uint32_t& playerLevel = 0, const std::uint32_t& contribution = 0,
                    const std::uint64_t& wins = 0, const std::uint64_t& losses = 0,
                    const std::uint64_t& draws = 0)
                {
                    std::memset(this, 0, sizeof(JoinPartyInfo));
                    std::strcpy(this->nickname, name);
                    this->uid = playerUid;
                    this->level = playerLevel;
                    this->clanContribution = contribution;
                    this->totalClanWins = wins;
                    this->totalClanLosses = losses;
                    this->totalClanDraws = draws;
                }
            };

            struct RegisteredClanInfo
            {
                std::uint16_t party_id;
                std::uint16_t clan_id;
                union {
                    struct {
                        std::uint32_t unk : 3;//0
                        std::uint32_t has_password : 1;//3
                        std::uint32_t mode_index : 5;//4
                        std::uint32_t unk2 : 4;//9
                        std::uint32_t max_players : 4;//13
                        std::uint32_t map_index : 7;//17
                        std::uint32_t level : 7;//24
                        std::uint32_t clanLogoFrontIdLastBit : 1;//31
                    };
                    std::uint32_t data;
                };
                union {
                    struct {
                        std::uint32_t clanLogoFrontId : 15;
                        std::uint32_t clanLogoBackId : 16;
                        std::uint32_t clanPadding : 1;
                    };
                    std::uint32_t data;
                };
                char clanName[16];
                char leaderName[16];

                RegisteredClanInfo(
                    std::uint16_t party_id,
                    std::uint16_t clan_id,
                    bool has_password,
                    std::uint32_t mode_index,
                    std::uint32_t max_players,
                    std::uint32_t map_index,
                    std::uint32_t level,
                    std::uint32_t clan_front_icon,
                    std::uint32_t clan_back_icon,
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
        }
        inline void SelectVoiceType(SCallbackData& callback, CMainServer* main_server)
        {
            auto send_msg = [&](CSession* session, std::uint16_t order, std::uint8_t mission, std::uint8_t extra, std::uint8_t option, std::uint8_t* data = nullptr, std::uint16_t data_size = 0)
                {
                    CMessage message(session->GetEncryptionKey());
                    message.SetSession(session->GetSessionId());
                    message.SetCommand(order, mission, extra, option);
                    if (data_size > 0 && data != nullptr) message.SetData(data, data_size);
                    session->Send(message);
                };
            std::shared_lock lock(callback.session->GetMutex());
            CSession* session = callback.session;
            CServer* server = callback.server;
            auto session_id = session->GetSessionId();
            auto acc_cache = main_server->GetAccCacheUniqueBySessionId(session_id);
            auto acc_index = acc_cache->acc_info.Index;
            auto my_unique_id = NetEngine::Packets::Core::UniqueId(session_id, 1).data;
            if (acc_index == -1) return;

            acc_cache->voice_id = callback.message->GetOption();
            BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::red, "player select voice id: ({})", acc_cache->voice_id);
            send_msg(session, 160, 0, 0, callback.message->GetOption());
        }
        inline void PlayerAutomatchLobby(SCallbackData& callback, CMainServer* main_server)
        {
            auto send_msg = [&](CSession* session, std::uint16_t order, std::uint8_t mission, std::uint8_t extra, std::uint8_t option, std::uint8_t* data = nullptr, std::uint16_t data_size = 0)
                {
                    CMessage message(session->GetEncryptionKey());
                    message.SetSession(session->GetSessionId());
                    message.SetCommand(order, mission, extra, option);
                    if (data_size > 0 && data != nullptr) message.SetData(data, data_size);
                    session->Send(message);
                };
            std::shared_lock lock(callback.session->GetMutex());
            CSession* session = callback.session;
            CServer* server = callback.server;
            auto session_id = session->GetSessionId();
            auto acc_cache = main_server->GetAccCacheUniqueBySessionId(session_id);
            auto acc_index = acc_cache->acc_info.Index;
            auto my_unique_id = NetEngine::Packets::Core::UniqueId(session_id, 1).data;
            auto my_slot = acc_cache->slot_id;
            auto my_team_id = acc_cache->team_id;
            auto leave_result = static_cast<NetEngine::Room::Leave::Req::Result>(callback.message->GetExtra());
            if (acc_index == -1) return;

            struct info {
                std::uint16_t mod_combo;
            };
            auto req_info = reinterpret_cast<info*>(callback.message->GetData());

            const std::unordered_map<std::uint16_t, std::string> mod_map = {
                {0x0001, "TDM"}, {0x0002, "FFA"}, {0x0004, "ITM"}, {0x0008, "CTB"},
                {0x0010, "CLOSEC"}, {0x0020, "SAB"}, {0x0040, "SIM"}, {0x0080, "ZSM"},
                {0x0100, "GUNRACE"}, {0x0200, "SCRIMMAGE"}, {0x0400, "BOMB"}, {0x0800, "PVE"}
            };

            bool match_mod[12] = { false };
            std::uint16_t c_mod_id = 0;
            for (const auto& [flag, name] : mod_map) {
                if (req_info->mod_combo & flag) {
                    BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::red, "player want to match mod: ({}) ({})", name, c_mod_id);
                    match_mod[c_mod_id] = true;
                }
                c_mod_id++;
            }

            std::shared_lock room_ids_lock(main_server->GetRoomIdsMutex());
            auto room_len = room_ids.size();
            for (int i = 0; i < room_len; i++) {
                auto room = main_server->GetRoomCacheShared(room_ids[i]);
                if (room->title.empty()) continue;
                if (!room->has_password) {
                    auto room_mod_id = (std::uint16_t)room->ModeIndex;
                    if (room_mod_id < 12) {
                        if (match_mod[room_mod_id]) {
                            BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::red, "found match mod: ({})", room_mod_id);
                            struct automatchResponse
                            {
                                std::uint16_t data1;
                                std::uint16_t data2;
                                automatchResponse(const std::uint16_t& data1 = 0, const std::uint16_t& data2 = 0)
                                {
                                    std::memset(this, 0, sizeof(automatchResponse));
                                    this->data1 = data1;
                                    this->data2 = data2;
                                }
                            };
                            auto res = automatchResponse(room->room_id, room->channel_id);
                            room.unlock();
                            room_ids_lock.unlock();
                            send_msg(session, 169, 0, 0, 1, reinterpret_cast<uint8_t*>(&res), sizeof(res));
                            return;
                        }
                    }
                }
            }
            room_ids_lock.unlock();
            send_msg(session, 169, 0, 2, 1);
        }
        inline void CreateParty(SCallbackData& callback, CMainServer* main_server)
        {
            auto send_msg = [&](CSession* session, std::uint16_t order, std::uint8_t mission, std::uint8_t extra, std::uint8_t option, std::uint8_t* data = nullptr, std::uint16_t data_size = 0)
                {
                    CMessage message(session->GetEncryptionKey());
                    message.SetSession(session->GetSessionId());
                    message.SetCommand(order, mission, extra, option);
                    if (data_size > 0 && data != nullptr) message.SetData(data, data_size);
                    session->Send(message);
                };
            std::shared_lock lock(callback.session->GetMutex());
            CSession* session = callback.session;
            CServer* server = callback.server;
            auto session_id = session->GetSessionId();
            auto acc_cache = main_server->GetAccCacheUniqueBySessionId(session_id);
            auto acc_index = acc_cache->acc_info.Index;
            auto my_unique_id = NetEngine::Packets::Core::UniqueId(session_id, 1).data;
            auto my_slot = acc_cache->slot_id;
            auto my_team_id = acc_cache->team_id;
            auto leave_result = static_cast<NetEngine::Room::Leave::Req::Result>(callback.message->GetExtra());
            if (acc_index == -1) return;
            auto createPartyReq = reinterpret_cast<MainCreatePartyReq*>(callback.message->GetData());

            std::uint16_t party_id = 0;
            if (server->GetNextAvailableQueuePartyId(party_id)) {
                Party newParty;
                newParty.party_id = (std::uint32_t)party_id;
                newParty.is_playing = false;
                newParty.is_queueing = false;
                newParty.is_clan = false;
                newParty.is_registered = false;
                newParty.has_password = false;
                newParty.clan_id = acc_cache->acc_info.ClanId;
                newParty.max_members = 4;
                newParty.party_host_session_id = session_id;
                newParty.mod_id = 15;
                newParty.map_id = 14;
                newParty.members.push_back(session_id);
                main_server->AddPartyCache(party_id, newParty);

                party_ids.push_back(party_id);

                acc_cache->in_party = true;
                acc_cache->party_id = party_id;
                
                /*leave plaza start*/
                if (acc_cache->in_plaza) {
                    auto plaza_id = acc_cache->plaza_id;
                    if (main_server->IsPlazaAlready(plaza_id))
                    {
                        BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "player will leave plaza: ({})", plaza_id);
                        auto current_plaza = main_server->GetPlazaCacheUnique(plaza_id);
                        auto& session_ids = current_plaza->session_ids;
                        if (main_server->IsSessionIdAlready(session_id, session_ids))
                        {
                            if (main_server->IsPlazaBroadcastable(current_plaza))
                            {
                                auto my_unique_id = NetEngine::Packets::Core::UniqueId(session_id, 1).data;
                                for (const auto& plaza_player_session_id : session_ids)
                                {
                                    if (plaza_player_session_id == session_id) continue;
                                    if (auto player_session = server->GetSessionById(plaza_player_session_id))
                                        send_msg(player_session.get(), 425, 0, 0, 1, reinterpret_cast<uint8_t*>(&my_unique_id), sizeof(my_unique_id));
                                }
                            }
                            BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "session id: ({}) left plaza id: ({})", session_id, plaza_id);
                            auto remove_myself = std::remove(current_plaza->session_ids.begin(), current_plaza->session_ids.end(), session_id);
                            current_plaza->session_ids.erase(remove_myself, current_plaza->session_ids.end());
                            acc_cache->plaza_id = 0;
                            acc_cache->in_plaza = false;
                        }
                    }
                }
                /*leave plaza end*/

                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "player ({}) created a new party entity id: ({})", acc_cache->acc_info.Nickname.c_str(), party_id);
            }
            else {
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::red, "party pool is full");
            }

            send_msg(session, 109, 0, 1, 0, reinterpret_cast<uint8_t*>(createPartyReq), sizeof(MainCreatePartyReq));

            BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "player ({}) create party -> unknown: ({})", acc_cache->acc_info.Nickname.c_str(), createPartyReq->unknown);

            if (acc_cache->acc_info.GuideMission == 10) // Done guide mission "Create a party"
            {
                auto current_coll = main_server->GetCollectionInfoCache(56);
                acc_cache->acc_info.GuideMission = 11;
                if (current_coll->rewardExp > 0)
                {
                    acc_cache->acc_info.Experience += current_coll->rewardExp;
                }
                if (current_coll->rewardPoint > 0)
                {
                    acc_cache->acc_info.MicroPoints += current_coll->rewardPoint;
                }
                MainCompleteMissionReq mission_data;
                mission_data.collection_id = 56;
                send_msg(session, 168, 0, 2, 0, reinterpret_cast<uint8_t*>(&mission_data.collection_id), sizeof(mission_data.collection_id));
                std::vector<std::uint16_t> empty_vec;
                ProcessLevelUp(main_server, callback.server, acc_cache, session_id, empty_vec);
            }
        }
        inline void PartyList(SCallbackData& callback, CMainServer* main_server)
        {
            auto send_msg = [&](CSession* session, std::uint16_t order, std::uint8_t mission, std::uint8_t extra, std::uint8_t option, std::uint8_t* data = nullptr, std::uint16_t data_size = 0)
                {
                    CMessage message(session->GetEncryptionKey());
                    message.SetSession(session->GetSessionId());
                    message.SetCommand(order, mission, extra, option);
                    if (data_size > 0 && data != nullptr) message.SetData(data, data_size);
                    session->Send(message);
                };
            std::shared_lock lock(callback.session->GetMutex());
            CSession* session = callback.session;
            CServer* server = callback.server;
            auto session_id = session->GetSessionId();
            auto acc_cache = main_server->GetAccCacheUniqueBySessionId(session_id);
            auto acc_index = acc_cache->acc_info.Index;
            auto my_unique_id = NetEngine::Packets::Core::UniqueId(session_id, 1).data;
            auto my_slot = acc_cache->slot_id;
            auto my_team_id = acc_cache->team_id;
            if (acc_index == -1 || acc_cache->acc_info.ClanId == -1) return;
            auto clan_id = acc_cache->acc_info.ClanId;
            auto clan = main_server->GetClanCacheUnique(clan_id);
            acc_cache.unlock();
            std::vector<structs::PartyInfo> selfClanInfoList;
            for (const auto& member_session_id : clan->online_members)
            {
                if (member_session_id == session_id) continue;
                auto member_acc_cache = main_server->GetAccCacheSharedBySessionId(member_session_id);
                //auto member_unique_id = NetEngine::Packets::Core::UniqueId(member_session_id, 1).data;
                if (member_acc_cache->in_party) {
                    auto party_id = member_acc_cache->party_id;
                    if (!main_server->IsPartyAlready(party_id)) {
                        BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "could not find player's party id ({})", party_id);
                        continue;
                    }
                    auto party = main_server->GetPartyCacheUnique(party_id);
                    if (party->is_clan && party->party_host_session_id == member_session_id) {
                        bool queueState = 0;
                        if (member_acc_cache->in_room) {
                            queueState = (member_acc_cache->playing ? 2 : 1);
                        }
                        structs::PartyInfo current_party((std::uint16_t)party_id, (std::uint16_t)clan_id, (std::uint32_t)party->members.size(), party->max_members, queueState, member_acc_cache->acc_info.Level, member_acc_cache->acc_info.Nickname.c_str());
                        selfClanInfoList.push_back(current_party);
                    }
                }
            }
            send_msg(session, callback.message->GetOrder(), callback.message->GetMission(), 37, selfClanInfoList.size(), reinterpret_cast<uint8_t*>(selfClanInfoList.data()), sizeof(structs::PartyInfo) * selfClanInfoList.size());
        }
        inline void PartyJoin(SCallbackData& callback, CMainServer* main_server)
        {
            auto send_msg = [&](CSession* session, std::uint16_t order, std::uint8_t mission, std::uint8_t extra, std::uint8_t option, std::uint8_t* data = nullptr, std::uint16_t data_size = 0)
                {
                    CMessage message(session->GetEncryptionKey());
                    message.SetSession(session->GetSessionId());
                    message.SetCommand(order, mission, extra, option);
                    if (data_size > 0 && data != nullptr) message.SetData(data, data_size);
                    session->Send(message);
                };
            std::shared_lock lock(callback.session->GetMutex());
            CSession* session = callback.session;
            CServer* server = callback.server;
            auto session_id = session->GetSessionId();
            auto acc_cache = main_server->GetAccCacheUniqueBySessionId(session_id);
            auto acc_index = acc_cache->acc_info.Index;
            auto my_unique_id = NetEngine::Packets::Core::UniqueId(session_id, 1).data;
            auto my_slot = acc_cache->slot_id;
            auto my_team_id = acc_cache->team_id;
            if (acc_index == -1) return;

            struct info {
                std::uint16_t roomId;
            };
            auto req_info = reinterpret_cast<info*>(callback.message->GetData());
            BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "player want to join party id: ({})", req_info->roomId);
            if (!main_server->IsPartyAlready(req_info->roomId)) {
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "could not find party id ({})", req_info->roomId);
                return;
            }

            auto party = main_server->GetPartyCacheUnique(req_info->roomId);
            if (main_server->IsSessionIdAlready(session_id, party->kicked_members)) {
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "player try to join but was kick");
                send_msg(session, callback.message->GetOrder(), callback.message->GetMission(), 42, callback.message->GetOption());
                return;
            }

            if (party->is_clan && acc_cache->acc_info.ClanId != party->clan_id) {
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "player try to join a clan party he dont belong with");
                send_msg(session, callback.message->GetOrder(), callback.message->GetMission(), 43, callback.message->GetOption());
                return;
            }
            if (party->members.size() >= party->max_members) {
                send_msg(session, callback.message->GetOrder(), callback.message->GetMission(), 7, callback.message->GetOption());
                return;
            }
            std::uint32_t party_room_id = 0;
            auto leader_acc_cache = main_server->GetAccCacheSharedBySessionId(party->party_host_session_id);
            party_room_id = leader_acc_cache->in_room ? leader_acc_cache->room_id : 0;
            leader_acc_cache.unlock();
            if (party_room_id != 0 || party->is_registered) {
                send_msg(session, callback.message->GetOrder(), callback.message->GetMission(), 5, callback.message->GetOption());
                return;
            }

            std::vector<structs::PartyPlayerInfo> PartyInfoList;

            //structs::JoinPartyInfo2 joinInfo(acc_cache->acc_info.Nickname.c_str(), my_unique_id);
            RoomUserPlayerInfo1 joinInfo1{ acc_cache->acc_info.Grade,
                    0,
                    acc_cache->acc_info.SelectedCharacter,
                    0,
                    acc_cache->acc_info.Level + 1,
                    acc_cache->ping };
            structs::PartyPlayerInfo2 joinInfo2{ 0,
                acc_cache->acc_info.ClanContribution,
                0 };
            structs::PartyPlayerInfo3 joinInfo3{
                    acc_cache->acc_info.ClanWins,
                    acc_cache->acc_info.ClanLoses,
                    acc_cache->acc_info.ClanDraws,
                    0
            };
            auto joinInfo_new = structs::PartyPlayerInfoJoin(
                my_unique_id,
                joinInfo1,
                joinInfo2,
                joinInfo3,
                acc_cache->acc_info.Nickname.c_str()
            );

            std::vector<BaseLib::Item> equipped_items;
            for (auto& item : acc_cache->inventory_items)
                if (item.is_equipped == 1 && item.character_id == static_cast<std::uint8_t>(acc_cache->acc_info.SelectedCharacter))
                    equipped_items.push_back(item);

            auto set_item = main_server->GetItemByType(equipped_items, 25).item_info.item_number.item_id;
            auto setitem_info = main_server->GetSetItemInfoCache(set_item);
            auto assign_item = [&](int type, auto setitem_field)
                {
                    auto item = main_server->GetItemByType(equipped_items, type).item_info.item_number.item_id;
                    return item ? item : (setitem_field != UINT32_MAX ? setitem_info->Id : 0);
                };
            joinInfo_new.EquippedHairItemId = assign_item(0, setitem_info->Hair);
            joinInfo_new.EquippedFaceItemId = assign_item(1, setitem_info->Face);
            joinInfo_new.EquippedUpperItemId = assign_item(2, setitem_info->Upper);
            joinInfo_new.EquippedUnderItemId = assign_item(3, setitem_info->Under);
            joinInfo_new.EquippedPantsItemId = assign_item(4, setitem_info->Pants);
            joinInfo_new.EquippedShirtItemId = assign_item(5, setitem_info->Arms);
            joinInfo_new.EquippedBootsItemId = assign_item(6, setitem_info->Boots);
            joinInfo_new.EquippedGlassItemId = assign_item(7, setitem_info->AccessoryA);
            joinInfo_new.EquippedAccessoryWaistItemId = assign_item(8, setitem_info->AccessoryB);
            joinInfo_new.EquippedAccessoryBackItemId = assign_item(9, setitem_info->AccessoryC);
            joinInfo_new.EquippedMeleeItemId = main_server->GetItemByType(equipped_items, 10).item_info.item_number.item_id;
            joinInfo_new.EquippedRifleItemId = main_server->GetItemByType(equipped_items, 11).item_info.item_number.item_id;
            joinInfo_new.EquippedShotgunItemId = main_server->GetItemByType(equipped_items, 12).item_info.item_number.item_id;
            joinInfo_new.EquippedSniperItemId = main_server->GetItemByType(equipped_items, 13).item_info.item_number.item_id;
            joinInfo_new.EquippedGatlingItemId = main_server->GetItemByType(equipped_items, 14).item_info.item_number.item_id;
            joinInfo_new.EquippedGrenadeItemId = main_server->GetItemByType(equipped_items, 15).item_info.item_number.item_id;
            joinInfo_new.EquippedBazookaItemId = main_server->GetItemByType(equipped_items, 16).item_info.item_number.item_id;

            PlayerRoomClanListInfo my_clan_info;
            if (acc_cache->acc_info.ClanId)
            {
                if (main_server->IsClanAlready(acc_cache->acc_info.ClanId))
                {
                    auto clan_info = main_server->GetClanCacheShared(acc_cache->acc_info.ClanId);
                    my_clan_info = PlayerRoomClanListInfo(party->members.size(), clan_info->clan_name.c_str(), clan_info->logo_front, clan_info->logo_back, acc_cache->acc_info.ClanId, 0);
                    clan_info.unlock();
                }
                else
                    my_clan_info = PlayerRoomClanListInfo(party->members.size(), "", 0, 0, 0, 0);
            }
            else
                my_clan_info = PlayerRoomClanListInfo(1, "", 0, 0, 0, 0);

            std::vector<PlayerRoomClanListInfo> players_clan_info;

            std::uint32_t idx = 0;

            std::vector<MainRoomPlayersEquipInfoAck> others_equipinfo;

            for (const auto& member_session_id : party->members)
            {
                auto member_acc_cache = main_server->GetAccCacheSharedBySessionId(member_session_id);
                auto member_unique_id = NetEngine::Packets::Core::UniqueId(member_session_id, 1).data;
                RoomUserPlayerInfo1 info1{ member_acc_cache->acc_info.Grade,
                    0,
                    member_acc_cache->acc_info.SelectedCharacter,
                    0,
                    member_acc_cache->acc_info.Level + 1,
                    member_acc_cache->ping };
                structs::PartyPlayerInfo2 info2{ 0,
                    member_acc_cache->acc_info.ClanContribution,
                    0 };
                structs::PartyPlayerInfo3 info3{
                        member_acc_cache->acc_info.ClanWins,
                        member_acc_cache->acc_info.ClanLoses,
                        member_acc_cache->acc_info.ClanDraws,
                        0
                };
                auto party_member = structs::PartyPlayerInfo(
                    member_unique_id,
                    info1,
                    info2,
                    info3,
                    member_acc_cache->acc_info.Nickname.c_str()
                );
                PartyInfoList.push_back(party_member);

                std::vector<BaseLib::Item> equipped_items;
                for (const auto& item : member_acc_cache->inventory_items)
                    if (item.is_equipped == 1 && item.character_id == static_cast<std::uint8_t>(member_acc_cache->acc_info.SelectedCharacter))
                        equipped_items.push_back(item);

                const auto& set_item = main_server->GetItemByType(equipped_items, 25).item_info.item_number.item_id;
                auto setitem_info = main_server->GetSetItemInfoCache(set_item);
                const auto& hair_id = main_server->GetItemByType(equipped_items, 0).item_info.item_number.item_id;
                const auto& face_id = main_server->GetItemByType(equipped_items, 1).item_info.item_number.item_id;
                const auto& upper_id = main_server->GetItemByType(equipped_items, 2).item_info.item_number.item_id;
                const auto& under_id = main_server->GetItemByType(equipped_items, 3).item_info.item_number.item_id;
                const auto& pants_id = main_server->GetItemByType(equipped_items, 4).item_info.item_number.item_id;
                const auto& shirt_id = main_server->GetItemByType(equipped_items, 5).item_info.item_number.item_id;
                const auto& boots_id = main_server->GetItemByType(equipped_items, 6).item_info.item_number.item_id;
                const auto& acc_head_id = main_server->GetItemByType(equipped_items, 7).item_info.item_number.item_id;
                const auto& acc_waist_id = main_server->GetItemByType(equipped_items, 8).item_info.item_number.item_id;
                const auto& acc_back_id = main_server->GetItemByType(equipped_items, 9).item_info.item_number.item_id;
                const auto& EquippedHairItemId = hair_id ? hair_id : (setitem_info->Hair != UINT32_MAX ? setitem_info->Id : 0);
                const auto& EquippedFaceItemId = face_id ? face_id : (setitem_info->Face != UINT32_MAX ? setitem_info->Id : 0);
                const auto& EquippedUpperItemId = upper_id ? upper_id : (setitem_info->Upper != UINT32_MAX ? setitem_info->Id : 0);
                const auto& EquippedUnderItemId = under_id ? under_id : (setitem_info->Under != UINT32_MAX ? setitem_info->Id : 0);
                const auto& EquippedPantsItemId = pants_id ? pants_id : (setitem_info->Pants != UINT32_MAX ? setitem_info->Id : 0);
                const auto& EquippedShirtItemId = shirt_id ? shirt_id : (setitem_info->Arms != UINT32_MAX ? setitem_info->Id : 0);
                const auto& EquippedBootsItemId = boots_id ? boots_id : (setitem_info->Boots != UINT32_MAX ? setitem_info->Id : 0);
                const auto& EquippedGlassItemId = acc_head_id ? acc_head_id : (setitem_info->AccessoryA != UINT32_MAX ? setitem_info->Id : 0);
                const auto& EquippedAccessoryWaistItemId = acc_waist_id ? acc_waist_id : (setitem_info->AccessoryB != UINT32_MAX ? setitem_info->Id : 0);
                const auto& EquippedAccessoryBackItemId = acc_back_id ? acc_back_id : (setitem_info->AccessoryC != UINT32_MAX ? setitem_info->Id : 0);
                const auto& EquippedMeleeItemId = main_server->GetItemByType(equipped_items, 10).item_info.item_number.item_id;
                const auto& EquippedRifleItemId = main_server->GetItemByType(equipped_items, 11).item_info.item_number.item_id;
                const auto& EquippedShotgunItemId = main_server->GetItemByType(equipped_items, 12).item_info.item_number.item_id;
                const auto& EquippedSniperItemId = main_server->GetItemByType(equipped_items, 13).item_info.item_number.item_id;
                const auto& EquippedGatlingItemId = main_server->GetItemByType(equipped_items, 14).item_info.item_number.item_id;
                const auto& EquippedGrenadeItemId = main_server->GetItemByType(equipped_items, 15).item_info.item_number.item_id;
                const auto& EquippedBazookaItemId = main_server->GetItemByType(equipped_items, 16).item_info.item_number.item_id;
                auto equip_data = MainRoomPlayersEquipInfoAck(member_unique_id,
                    EquippedHairItemId, EquippedFaceItemId, EquippedUpperItemId,
                    EquippedUnderItemId, EquippedPantsItemId, EquippedShirtItemId,
                    EquippedBootsItemId, EquippedGlassItemId, EquippedAccessoryWaistItemId,
                    EquippedAccessoryBackItemId, EquippedMeleeItemId, EquippedRifleItemId,
                    EquippedShotgunItemId, EquippedSniperItemId, EquippedGatlingItemId,
                    EquippedGrenadeItemId, EquippedBazookaItemId);

                others_equipinfo.push_back(equip_data);

                if (auto player_session = server->GetSessionById(member_session_id)) {
                    send_msg(player_session.get(), 418, 0, 1, 0, reinterpret_cast<uint8_t*>(&joinInfo_new), sizeof(joinInfo_new));

                    send_msg(player_session.get(), 409, NetEngine::Room::Clan::IconUpdateMission::PartyMembers, 37, 1, reinterpret_cast<uint8_t*>(&my_clan_info), sizeof(PlayerRoomClanListInfo));
                }

                if (member_acc_cache->acc_info.ClanId)
                {
                    if (main_server->IsClanAlready(member_acc_cache->acc_info.ClanId))
                    {
                        auto clan_info = main_server->GetClanCacheShared(member_acc_cache->acc_info.ClanId);
                        auto info = PlayerRoomClanListInfo(idx, clan_info->clan_name.c_str(), clan_info->logo_front, clan_info->logo_back, acc_cache->acc_info.ClanId, 0);
                        clan_info.unlock();
                        players_clan_info.push_back(info);
                    }
                }
                else
                    players_clan_info.push_back(PlayerRoomClanListInfo(idx, "", 0, 0, 0, 0));

                idx++;
            }
            send_msg(session, 417, 0, 37, PartyInfoList.size(), reinterpret_cast<uint8_t*>(PartyInfoList.data()), sizeof(structs::PartyPlayerInfo) * PartyInfoList.size());
            send_msg(session, 303, 0, 37, others_equipinfo.size(), reinterpret_cast<uint8_t*>(others_equipinfo.data()), others_equipinfo.size() * sizeof(MainRoomPlayersEquipInfoAck));

            send_msg(session, callback.message->GetOrder(), 0, 1, 0);

            send_msg(session, 409, NetEngine::Room::Clan::IconUpdateMission::PartyMembers, 37, players_clan_info.size(), reinterpret_cast<uint8_t*>(players_clan_info.data()), sizeof(PlayerRoomClanListInfo) * players_clan_info.size());

            acc_cache->in_party = true;
            acc_cache->party_id = req_info->roomId;
            party->members.push_back(session_id);

            /*leave plaza start*/
            if (acc_cache->in_plaza) {
                auto plaza_id = acc_cache->plaza_id;
                if (main_server->IsPlazaAlready(plaza_id))
                {
                    BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "player will leave plaza: ({})", plaza_id);
                    auto current_plaza = main_server->GetPlazaCacheUnique(plaza_id);
                    auto& session_ids = current_plaza->session_ids;
                    if (main_server->IsSessionIdAlready(session_id, session_ids))
                    {
                        if (main_server->IsPlazaBroadcastable(current_plaza))
                        {
                            auto my_unique_id = NetEngine::Packets::Core::UniqueId(session_id, 1).data;
                            for (const auto& plaza_player_session_id : session_ids)
                            {
                                if (plaza_player_session_id == session_id) continue;
                                if (auto player_session = server->GetSessionById(plaza_player_session_id))
                                    send_msg(player_session.get(), 425, 0, 0, 1, reinterpret_cast<uint8_t*>(&my_unique_id), sizeof(my_unique_id));
                            }
                        }
                        BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "session id: ({}) left plaza id: ({})", session_id, plaza_id);
                        auto remove_myself = std::remove(current_plaza->session_ids.begin(), current_plaza->session_ids.end(), session_id);
                        current_plaza->session_ids.erase(remove_myself, current_plaza->session_ids.end());
                        acc_cache->plaza_id = 0;
                        acc_cache->in_plaza = false;
                    }
                }
            }
            /*leave plaza end*/

            struct info2 {
                std::uint8_t partyUi;//1 = CLAN PARTY - 2 = PARTY NORMAL
            };
            info2 party_info;
            party_info.partyUi = (party->is_clan ? 1 : 2);
            send_msg(session, 116, 0, party->mod_id, party->map_id, reinterpret_cast<uint8_t*>(&party_info.partyUi), sizeof(party_info.partyUi));
        }
        inline void GetActiveClanList(SCallbackData& callback, CMainServer* main_server)
        {
            auto send_msg = [&](CSession* session, std::uint16_t order, std::uint8_t mission, std::uint8_t extra, std::uint8_t option, std::uint8_t* data = nullptr, std::uint16_t data_size = 0)
                {
                    CMessage message(session->GetEncryptionKey());
                    message.SetSession(session->GetSessionId());
                    message.SetCommand(order, mission, extra, option);
                    if (data_size > 0 && data != nullptr) message.SetData(data, data_size);
                    session->Send(message);
                };
            std::shared_lock lock(callback.session->GetMutex());
            CSession* session = callback.session;
            CServer* server = callback.server;
            auto order = callback.message->GetOrder();
            auto session_id = session->GetSessionId();
            auto acc_cache = main_server->GetAccCacheUniqueBySessionId(session_id);
            auto acc_index = acc_cache->acc_info.Index;
            auto my_unique_id = NetEngine::Packets::Core::UniqueId(session_id, 1).data;
            auto my_slot = acc_cache->slot_id;
            auto my_team_id = acc_cache->team_id;
            if (acc_index == -1) return;

            if (!acc_cache->in_party) return;

            if (order == 112) {//idk ?

            }
            else if (order == 113) {//Standby list of clan
                std::shared_lock party_ids_lock(main_server->GetPartyIdsMutex());
                auto my_clan_id = acc_cache->acc_info.ClanId;
                acc_cache.unlock();
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "will build party clan list! now exist ({}) party", party_ids.size());
                if (party_ids.size() <= 0)
                {
                    send_msg(session, 113, 0, 6, 0);
                    return;
                }
                std::uint32_t max_batch_size = 31;
                std::uint32_t party_blocks_count = (party_ids.size() + max_batch_size - 1) / max_batch_size;
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "batch block count ({})", party_blocks_count);
                for (std::uint32_t batch_id = 0; batch_id < party_blocks_count; batch_id++)
                {
                    std::vector<structs::RegisteredClanInfo> StandbyClanList;
                    auto extra = (batch_id == 0) ? NetEngine::Room::List::SendRoom : NetEngine::Room::List::SendRoom2;
                    std::uint32_t start_index = batch_id * max_batch_size;
                    std::uint32_t end_index = std::min(start_index + max_batch_size, static_cast<std::uint32_t>(party_ids.size()));
                    BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "run from ({}) to ({})", start_index, end_index);
                    for (auto i = start_index; i < end_index; i++)
                    {
                        if (!main_server->IsPartyAlready(party_ids[i])) {
                            BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "could not find party id ({})", party_ids[i]);
                            continue;
                        }
                        auto c_party = main_server->GetPartyCacheUnique(party_ids[i]);
                        BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "check party id ({})", party_ids[i]);
                        if (!c_party->is_clan) continue;
                        if (!c_party->is_registered) continue;
                        if (!c_party->clan_id == my_clan_id) continue;
                        BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "check passed");
                        auto c_clan = main_server->GetClanCacheUnique(c_party->clan_id);
                        auto c_leader = main_server->GetAccCacheUniqueBySessionId(c_party->party_host_session_id);
                        auto new_clan_party = structs::RegisteredClanInfo(party_ids[i], c_party->clan_id, c_party->has_password, c_party->mod_id, c_party->members.size(), c_party->map_id, c_leader->acc_info.Level + 1, c_clan->logo_front, c_clan->logo_back, c_clan->clan_name, c_leader->acc_info.Nickname);
                        //REPLACE WITH MAX_MEMBERS NOT SIZE OF MEMBERS !!
                        StandbyClanList.push_back(new_clan_party);
                    }
                    send_msg(session, 113, 0, extra, StandbyClanList.size(), reinterpret_cast<uint8_t*>(StandbyClanList.data()), sizeof(structs::RegisteredClanInfo) * StandbyClanList.size());
                }
            }
        }
        inline void PartyChangeLeader(SCallbackData& callback, CMainServer* main_server)
        {
            auto send_msg = [&](CSession* session, std::uint16_t order, std::uint8_t mission, std::uint8_t extra, std::uint8_t option, std::uint8_t* data = nullptr, std::uint16_t data_size = 0)
                {
                    CMessage message(session->GetEncryptionKey());
                    message.SetSession(session->GetSessionId());
                    message.SetCommand(order, mission, extra, option);
                    if (data_size > 0 && data != nullptr) message.SetData(data, data_size);
                    session->Send(message);
                };
            std::shared_lock lock(callback.session->GetMutex());
            CSession* session = callback.session;
            CServer* server = callback.server;
            auto session_id = session->GetSessionId();
            auto acc_cache = main_server->GetAccCacheUniqueBySessionId(session_id);
            auto acc_index = acc_cache->acc_info.Index;
            auto my_unique_id = NetEngine::Packets::Core::UniqueId(session_id, 1).data;
            auto my_slot = acc_cache->slot_id;
            auto my_team_id = acc_cache->team_id;
            if (acc_index == -1) return;

            if (!main_server->IsPartyAlready(acc_cache->party_id)) {
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "could not find party id ({})", acc_cache->party_id);
                return;
            }
            auto party_cache = main_server->GetPartyCacheUnique(acc_cache->party_id);
            BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "party want to change leader");
            if (party_cache->party_host_session_id != session_id) {
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "party change leader request is not the leader!");
                //send_msg(player_session.get(), 114, 0, 1, static_cast<std::uint8_t>(new_leader_index));
                send_msg(session, 114, 0, 16, 0);
                return;
            }
            auto new_leader_index = callback.message->GetOption();
            for (const auto& party_member_session_id : party_cache->members)
            {
                if (auto player_session = server->GetSessionById(party_member_session_id))
                    send_msg(player_session.get(), 114, 0, 1, static_cast<std::uint8_t>(new_leader_index));
            }
            auto new_leader = party_cache->members[new_leader_index];
            party_cache->party_host_session_id = new_leader;
            //send_msg(session, 114, 0, 1, 0);
        }
        inline void PartyKickMember(SCallbackData& callback, CMainServer* main_server)
        {
            auto send_msg = [&](CSession* session, std::uint16_t order, std::uint8_t mission, std::uint8_t extra, std::uint8_t option, std::uint8_t* data = nullptr, std::uint16_t data_size = 0)
                {
                    CMessage message(session->GetEncryptionKey());
                    message.SetSession(session->GetSessionId());
                    message.SetCommand(order, mission, extra, option);
                    if (data_size > 0 && data != nullptr) message.SetData(data, data_size);
                    session->Send(message);
                };
            std::shared_lock lock(callback.session->GetMutex());
            CSession* session = callback.session;
            CServer* server = callback.server;
            auto session_id = session->GetSessionId();
            auto acc_cache = main_server->GetAccCacheUniqueBySessionId(session_id);
            auto acc_index = acc_cache->acc_info.Index;
            auto my_unique_id = NetEngine::Packets::Core::UniqueId(session_id, 1).data;
            auto my_slot = acc_cache->slot_id;
            auto my_team_id = acc_cache->team_id;
            if (acc_index == -1) return;

            struct info {
                std::uint32_t victimUniqueId;
            };
            auto req_info = reinterpret_cast<info*>(callback.message->GetData());

            BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "party will kick player: ({})", req_info->victimUniqueId);

            auto user_uniqueid = NetEngine::Packets::Core::UniqueId(req_info->victimUniqueId);
            auto target_acc_cache = main_server->GetAccCacheSharedBySessionId(static_cast<std::uint16_t>(user_uniqueid.session));

            if (!acc_cache->in_party || !target_acc_cache->in_party || acc_cache->party_id != target_acc_cache->party_id) {

                return;
            }

            if (!main_server->IsPartyAlready(acc_cache->party_id)) {
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "could not find party id ({})", acc_cache->party_id);
                return;
            }

            auto party_cache = main_server->GetPartyCacheUnique(acc_cache->party_id);
            if (party_cache->party_host_session_id != session_id) {

                return;
            }

            BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "party checks passed and will kick");

            auto remove_victim = std::remove(party_cache->members.begin(), party_cache->members.end(), target_acc_cache->session_id);
            party_cache->members.erase(remove_victim, party_cache->members.end());

            for (const auto& party_member_session_id : party_cache->members)
            {
                if (auto player_session = server->GetSessionById(party_member_session_id))
                    send_msg(player_session.get(), 419, 0, 0, 0, reinterpret_cast<uint8_t*>(&req_info->victimUniqueId), sizeof(req_info->victimUniqueId));
            }

            if (auto victim_session = server->GetSessionById(target_acc_cache->session_id))
                send_msg(victim_session.get(), 111, 0, NetEngine::Room::Leave::Ack::Result::KickedByHost, 0);

            target_acc_cache->party_id = 0;
            target_acc_cache->in_party = false;

            party_cache->kicked_members.push_back(target_acc_cache->session_id);

        }

        inline void ClanRegister(SCallbackData& callback, CMainServer* main_server)
        {
            auto send_msg = [&](CSession* session, std::uint16_t order, std::uint8_t mission, std::uint8_t extra, std::uint8_t option, std::uint8_t* data = nullptr, std::uint16_t data_size = 0)
                {
                    CMessage message(session->GetEncryptionKey());
                    message.SetSession(session->GetSessionId());
                    message.SetCommand(order, mission, extra, option);
                    if (data_size > 0 && data != nullptr) message.SetData(data, data_size);
                    session->Send(message);
                };
            std::shared_lock lock(callback.session->GetMutex());
            CSession* session = callback.session;
            CServer* server = callback.server;
            auto session_id = session->GetSessionId();
            auto acc_cache = main_server->GetAccCacheUniqueBySessionId(session_id);
            auto acc_index = acc_cache->acc_info.Index;
            auto my_unique_id = NetEngine::Packets::Core::UniqueId(session_id, 1).data;
            auto my_slot = acc_cache->slot_id;
            auto my_team_id = acc_cache->team_id;
            if (acc_index == -1) return;


            BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "party is clan and will register");

            if (!acc_cache->in_party) {

                return;
            }

            if (!main_server->IsPartyAlready(acc_cache->party_id)) {
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "could not find party id ({})", acc_cache->party_id);
                return;
            }

            auto party_cache = main_server->GetPartyCacheUnique(acc_cache->party_id);
            if (party_cache->party_host_session_id != session_id) {

                return;
            }

            auto c_order = callback.message->GetOrder();
            auto c_extra = callback.message->GetExtra();
            if (c_order == 115) {
                party_cache->is_queueing = true;
                party_cache->has_password = false;
                party_cache->password = "";
                if (callback.message->GetDataSize() > 8) {
                    struct info {
                        char password[16];
                    };
                    auto req_info = reinterpret_cast<info*>(callback.message->GetData());
                    auto new_password = Utility::ReadMicrovoltsString(req_info->password, sizeof(req_info->password));
                    if (new_password.size()) {
                        party_cache->has_password = true;
                        party_cache->password = new_password;
                    }
                }

                char nickname[16];
                std::strcpy(nickname, acc_cache->acc_info.Nickname.c_str());
                for (const auto& party_member_session_id : party_cache->members)
                {
                    if (auto player_session = server->GetSessionById(party_member_session_id))
                        send_msg(player_session.get(), 115, 0, 44, 0);
                }
                //send_msg(session, 115, 0, 44, 0, reinterpret_cast<uint8_t*>(nickname), 16);
            }
            else if (c_order == 120) {
                if (c_extra == 44) {
                    party_cache->is_registered = true;
                }
                else if (c_extra == 45) {
                    party_cache->is_registered = false;
                }
                for (const auto& party_member_session_id : party_cache->members)
                {
                    if (auto player_session = server->GetSessionById(party_member_session_id))
                        send_msg(player_session.get(), 120, callback.message->GetMission(), c_extra, callback.message->GetOption());
                }
                //send_msg(session, 120, callback.message->GetMission(), c_extra, callback.message->GetOption());
            }
        }
        inline void PartySettings(SCallbackData& callback, CMainServer* main_server)
        {
            auto send_msg = [&](CSession* session, std::uint16_t order, std::uint8_t mission, std::uint8_t extra, std::uint8_t option, std::uint8_t* data = nullptr, std::uint16_t data_size = 0)
                {
                    CMessage message(session->GetEncryptionKey());
                    message.SetSession(session->GetSessionId());
                    message.SetCommand(order, mission, extra, option);
                    if (data_size > 0 && data != nullptr) message.SetData(data, data_size);
                    session->Send(message);
                };
            std::shared_lock lock(callback.session->GetMutex());
            CSession* session = callback.session;
            CServer* server = callback.server;
            auto session_id = session->GetSessionId();
            auto acc_cache = main_server->GetAccCacheUniqueBySessionId(session_id);
            auto acc_index = acc_cache->acc_info.Index;
            auto my_unique_id = NetEngine::Packets::Core::UniqueId(session_id, 1).data;
            auto my_slot = acc_cache->slot_id;
            auto my_team_id = acc_cache->team_id;
            if (acc_index == -1) return;

            struct info {
                std::uint8_t partyUi;//1 = CLAN PARTY - 2 = PARTY NORMAL
            };
            auto req_info = reinterpret_cast<info*>(callback.message->GetData());

            if (!acc_cache->in_party) return;

            if (!main_server->IsPartyAlready(acc_cache->party_id)) {
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "could not find party id ({})", acc_cache->party_id);
                return;
            }

            auto party_cache = main_server->GetPartyCacheUnique(acc_cache->party_id);
            if (party_cache->party_host_session_id != session_id) {

                return;
            }

            auto order = callback.message->GetOrder();
            if (order == 116) {//general settings
                party_cache->is_clan = (req_info->partyUi == 1);

                party_cache->map_id = callback.message->GetOption();
                party_cache->mod_id = callback.message->GetExtra();
                for (const auto& party_member_session_id : party_cache->members)
                {
                    if (auto player_session = server->GetSessionById(party_member_session_id))
                        send_msg(player_session.get(), order, callback.message->GetMission(), party_cache->mod_id, party_cache->map_id, reinterpret_cast<uint8_t*>(&req_info->partyUi), sizeof(req_info->partyUi));
                }
            }
            else if (order == 117) {//change size
                party_cache->max_members = callback.message->GetOption();
                for (const auto& party_member_session_id : party_cache->members)
                {
                    if (auto player_session = server->GetSessionById(party_member_session_id))
                        send_msg(player_session.get(), order, callback.message->GetMission(), 1, callback.message->GetOption());
                }
            }
        }
        inline void ClanOtherJoin(SCallbackData& callback, CMainServer* main_server)
        {
            auto send_msg = [&](CSession* session, std::uint16_t order, std::uint8_t mission, std::uint8_t extra, std::uint8_t option, std::uint8_t* data = nullptr, std::uint16_t data_size = 0)
                {
                    CMessage message(session->GetEncryptionKey());
                    message.SetSession(session->GetSessionId());
                    message.SetCommand(order, mission, extra, option);
                    if (data_size > 0 && data != nullptr) message.SetData(data, data_size);
                    session->Send(message);
                };
            std::shared_lock lock(callback.session->GetMutex());
            CSession* session = callback.session;
            CServer* server = callback.server;
            auto session_id = session->GetSessionId();
            auto acc_cache = main_server->GetAccCacheUniqueBySessionId(session_id);
            auto acc_index = acc_cache->acc_info.Index;
            auto my_unique_id = NetEngine::Packets::Core::UniqueId(session_id, 1).data;
            auto my_slot = acc_cache->slot_id;
            auto my_team_id = acc_cache->team_id;
            if (acc_index == -1) return;

            struct info {
                std::uint16_t partyId;
                std::uint16_t channelId;
                char password[16];
            };
            auto req_info = reinterpret_cast<info*>(callback.message->GetData());

            if (!main_server->IsPartyAlready(acc_cache->party_id)) {
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "could not find player's party id ({})", acc_cache->party_id);
                return;
            }
            auto new_password = Utility::ReadMicrovoltsString(req_info->password, sizeof(req_info->password));

            BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "player want to create a battle with party id ({})", req_info->partyId);

            auto self_party_cache = main_server->GetPartyCacheUnique(acc_cache->party_id);
            auto self_host_id = self_party_cache->party_host_session_id;
            auto self_party_id = acc_cache->party_id;
            auto self_player_max = self_party_cache->max_members;
            std::vector self_players = self_party_cache->members;
            auto self_mod = self_party_cache->mod_id;
            auto self_map = self_party_cache->map_id;
            auto self_password = (self_party_cache->has_password ? self_party_cache->password : (std::string)"");
            auto self_clan_id = self_party_cache->clan_id;
            auto self_is_clan = self_party_cache->is_clan;
            auto self_is_register = self_party_cache->is_registered;
            self_party_cache.unlock();
            if (!main_server->IsPartyAlready(req_info->partyId)) {
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "could not find desired target party id ({})", req_info->partyId);
                return;
            }
            auto target_party_cache = main_server->GetPartyCacheUnique(req_info->partyId);
            auto target_host_id = target_party_cache->party_host_session_id;
            auto target_party_id = acc_cache->party_id;
            auto target_player_max = target_party_cache->max_members;
            std::vector target_players = target_party_cache->members;
            auto target_mod = target_party_cache->mod_id;
            auto target_map = target_party_cache->map_id;
            auto target_has_password = target_party_cache->has_password;
            auto target_password = target_party_cache->password;
            auto target_clan_id = target_party_cache->clan_id;
            auto target_is_clan = target_party_cache->is_clan;
            auto target_is_register = target_party_cache->is_registered;
            target_party_cache.unlock();

            BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "all data gather done and will check conditions");

            if (target_password.size() && new_password != target_password) {
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "fail join clan battle, wrong password: ({}), target password: ({})", new_password.c_str(), target_password.c_str());
                send_msg(callback.session, 121, 0, 1, 0);
                return;
            }

            if (self_party_id && target_party_id && (self_players.size() == target_players.size()) && target_is_register) {
                self_party_cache.lock();
                self_party_cache->is_registered = false;
                self_party_cache->is_queueing = false;
                self_party_cache.unlock();
                target_party_cache.lock();
                target_party_cache->is_registered = false;
                target_party_cache->is_queueing = false;
                target_party_cache.unlock();
                //now will create a clan room where the host is the target !
                std::uint32_t score_limit = 5;
                RoomSettings clan_room_setting;
                clan_room_setting.allow_intruders = false;
                clan_room_setting.allow_items = false;
                clan_room_setting.allow_observers = false;
                clan_room_setting.has_password = false;
                clan_room_setting.map_index = target_map;
                clan_room_setting.max_players = (target_players.size() * 2);
                clan_room_setting.mode_index = target_mod;
                clan_room_setting.team_balance = false;
                clan_room_setting.restriction = 7;
                clan_room_setting.unknown1 = true;
                clan_room_setting.unknown2 = true;
                switch (target_mod) {
                case 13: {//clan ctb
                    clan_room_setting.time = 15;
                    clan_room_setting.allow_items = true;
                    break;
                }
                case 14: {//clan sab
                    clan_room_setting.time = 2;
                    break;
                }
                case 15: {//clan tdm
                    clan_room_setting.time = 15;
                    clan_room_setting.allow_items = true;
                    break;
                }
                default: {
                    BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "unknown clan mod id: ({})", target_mod);
                }
                }
                MainCreateRoomReq clan_room_req;
                char empty_title[32] = "Partymatch";
                char empty_password[16] = "";
                std::strcpy(clan_room_req.title, empty_title);
                std::strcpy(clan_room_req.password, empty_password);
                clan_room_req.settings_data = clan_room_setting.data;

                SCallbackData callback;
                callback.server = main_server;
                callback.session = (main_server->GetSessionById(target_host_id)).get();
                CMessage msg = CMessage();
                msg.SetSession(callback.session->GetSessionId());
                //msg.SetCommand(callback.message->GetOrder(), callback.message->GetMission(), callback.message->GetExtra(), callback.message->GetOption());
                msg.SetData(reinterpret_cast<uint8_t*>(&clan_room_req), sizeof(clan_room_req));
                msg.SetExtra(score_limit);
                callback.message = &msg;
                lock.unlock();
                CreateRoom(callback, main_server);

                auto target_host_acc_cache = main_server->GetAccCacheUniqueBySessionId(target_host_id);
                auto new_clan_room_id = target_host_acc_cache->room_id;
                target_host_acc_cache.unlock();

                if (self_is_clan)
                {
                    auto target_room_cache = main_server->GetRoomCacheUnique(new_clan_room_id);
                    target_room_cache->is_clan_room = true;
                    target_room_cache->clan_id_1 = self_clan_id;
                    target_room_cache->clan_id_2 = target_clan_id;
                    target_room_cache.unlock();
                }

                acc_cache.unlock();

                auto join_clan_room = [&](std::vector<uint16_t> players)
                    {
                        MainJoinRoomReq clan_room_join_req;
                        clan_room_join_req.channel_id = 1;
                        clan_room_join_req.room_id = new_clan_room_id;
                        for (const auto& party_member_session_id : players)
                        {
                            if (party_member_session_id == target_host_id) continue;
                            if (auto player_session = server->GetSessionById(party_member_session_id)) {
                                SCallbackData callback;
                                callback.server = main_server;
                                callback.session = player_session.get();
                                CMessage msg = CMessage();
                                msg.SetSession(callback.session->GetSessionId());
                                //msg.SetCommand(callback.message->GetOrder(), callback.message->GetMission(), callback.message->GetExtra(), callback.message->GetOption());
                                msg.SetData(reinterpret_cast<uint8_t*>(&clan_room_join_req), sizeof(clan_room_join_req));
                                callback.message = &msg;
                                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "now call join room");
                                JoinRoom(callback, main_server);
                                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "now done join room");
                            }
                        }
                    };

                join_clan_room(self_players);
                join_clan_room(target_players);

                auto host_cache = main_server->GetAccCacheUniqueBySessionId(target_host_id);
                auto room_cache = main_server->GetRoomCacheUnique(host_cache->room_id);

                RoomSettingsInfo2 settings_info{};
                settings_info.map_index = room_cache->MapIndex;
                settings_info.mode_index = room_cache->ModeIndex;
                settings_info.max_players = room_cache->max_players;
                settings_info.restriction = room_cache->Restriction;
                settings_info.allow_intruders = room_cache->allow_intruders;
                settings_info.allow_observers = room_cache->allow_observers;
                settings_info.team_balance = NetEngine::Room::Balance::State::Disabled;//room_cache->TeamBalance;
                if (room_cache->ModeIndex == NetEngine::Room::Mode::Index::BombBattle)
                    settings_info.team_balance = NetEngine::Room::Balance::State::Disabled;
                settings_info.has_password = room_cache->has_password;
                settings_info.hide_password = false;
                settings_info.is_clan_room = (host_cache->in_party ? (self_is_clan ? 2 : 1) : 0);
                auto settings_data = MainRoomSettingsInfoAck(room_cache->password.c_str(), settings_info).Serialize();
                std::uint8_t high_room_id_part = (room_cache->room_id >> 8) & 0xFF; // Extract the high 8 bits
                std::uint8_t low_room_id_part = room_cache->room_id & 0xFF;
                send_msg(callback.session, 139, room_cache->has_password, low_room_id_part, high_room_id_part, reinterpret_cast<uint8_t*>(settings_data.data()), settings_data.size());

                room_cache.unlock();
                host_cache.unlock();
            }
            else {
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "conditions failed");
            }
        }
        inline void ClanRoomLeave(SCallbackData& callback, CMainServer* main_server)
        {
            auto send_msg = [&](CSession* session, std::uint16_t order, std::uint8_t mission, std::uint8_t extra, std::uint8_t option, std::uint8_t* data = nullptr, std::uint16_t data_size = 0)
                {
                    CMessage message(session->GetEncryptionKey());
                    message.SetSession(session->GetSessionId());
                    message.SetCommand(order, mission, extra, option);
                    if (data_size > 0 && data != nullptr) message.SetData(data, data_size);
                    session->Send(message);
                };
            std::shared_lock lock(callback.session->GetMutex());
            CSession* session = callback.session;
            CServer* server = callback.server;
            auto session_id = session->GetSessionId();
            auto acc_cache = main_server->GetAccCacheUniqueBySessionId(session_id);
            auto acc_index = acc_cache->acc_info.Index;
            auto my_unique_id = NetEngine::Packets::Core::UniqueId(session_id, 1).data;
            auto my_slot = acc_cache->slot_id;
            auto my_team_id = acc_cache->team_id;
            if (acc_index == -1) return;

            BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "player want to leave clan room");
            if (!acc_cache->in_party || !acc_cache->in_room) {
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "player isnt in any clan room");
            }

            auto self_party_id = acc_cache->party_id;
            auto self_party_cache = main_server->GetPartyCacheUnique(self_party_id);
            auto self_party_leader = self_party_cache->party_host_session_id;
            auto target_room_id = acc_cache->room_id;
            bool is_leader_leave = (self_party_leader == session_id);
            bool other_party_assure_leave = false;
            if (is_leader_leave) {
                self_party_cache->is_registered = false;
                self_party_cache->is_queueing = false;
            }
            self_party_cache.unlock();

            acc_cache.unlock();

            if (is_leader_leave) {//destroy the room
                auto room_cache = main_server->GetRoomCacheUnique(target_room_id);
                auto players = main_server->GetRoomSortedPlayerSessionIds(room_cache);
                for (const auto& player_id : players)
                {
                    auto player_cache = main_server->GetAccCacheSharedBySessionId(player_id);
                    if (player_cache->acc_info.Index == -1 || !player_cache->in_room || player_cache->room_id != room_cache->room_id)
                    {
                        player_cache.unlock();
                        continue;
                    }
                    else
                    {
                        if (!other_party_assure_leave && player_cache->party_id != self_party_id) {
                            auto target_party_cache = main_server->GetPartyCacheUnique(player_cache->party_id);
                            BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "target party assured leave");
                            target_party_cache->is_registered = false;
                            target_party_cache->is_queueing = false;
                            target_party_cache.unlock();
                            other_party_assure_leave = true;
                        }
                        player_cache->in_room = false;
                        player_cache->slot_id = 0xFF;
                        player_cache->playing = false;
                        player_cache->state = PlayerInfo::State::Waiting;
                        player_cache.unlock();

                        if (auto player_session = main_server->GetSessionById(player_id))
                        {
                            send_msg(player_session.get(), 141, 0, NetEngine::Room::Leave::Ack::Result::ClosedByGm, 0);
                        }

                    }
                }
                main_server->RemoveRoomCache(target_room_id);
                main_server->SetRoomIdAvailable(target_room_id);

                return;
            }
            //just throw member out of party

            LeaveRoom(callback, main_server);
            LeaveParty(callback, main_server);

        }
        inline void PartyRegister(SCallbackData& callback, CMainServer* main_server)
        {
            auto send_msg = [&](CSession* session, std::uint16_t order, std::uint8_t mission, std::uint8_t extra, std::uint8_t option, std::uint8_t* data = nullptr, std::uint16_t data_size = 0)
                {
                    CMessage message(session->GetEncryptionKey());
                    message.SetSession(session->GetSessionId());
                    message.SetCommand(order, mission, extra, option);
                    if (data_size > 0 && data != nullptr) message.SetData(data, data_size);
                    session->Send(message);
                };
            std::shared_lock lock(callback.session->GetMutex());
            CSession* session = callback.session;
            CServer* server = callback.server;
            auto session_id = session->GetSessionId();
            auto acc_cache = main_server->GetAccCacheUniqueBySessionId(session_id);
            auto acc_index = acc_cache->acc_info.Index;
            auto my_unique_id = NetEngine::Packets::Core::UniqueId(session_id, 1).data;
            auto my_slot = acc_cache->slot_id;
            auto my_team_id = acc_cache->team_id;
            if (acc_index == -1) return;

            if (!acc_cache->in_party) {

                return;
            }
            if (!main_server->IsPartyAlready(acc_cache->party_id)) {
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "could not find player's party id ({})", acc_cache->party_id);
                return;
            }

            auto party_cache = main_server->GetPartyCacheUnique(acc_cache->party_id);
            if (party_cache->party_host_session_id != session_id) {
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "is not leader of party");
                return;
            }

            auto my_party_count = party_cache->members.size();
            auto my_party_is_clan = party_cache->is_clan;

            party_cache.unlock();

            auto c_extra = callback.message->GetExtra();
            if (c_extra == 44) {
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "want to find a match for ({}) players", my_party_count);
                std::uint16_t match_party_id = 0;
                std::shared_lock party_ids_lock(main_server->GetPartyIdsMutex());
                std::uint32_t party_ids_count = party_ids.size();
                for (std::uint32_t i = 0; i < party_ids_count; i++) {
                    auto c_party_id = party_ids[i];
                    auto c_party = main_server->GetPartyCacheShared(c_party_id);
                    BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "will check party id: ({}) by ({})", c_party_id, c_party->party_host_session_id);
                    bool is_match = (c_party->is_registered && !c_party->is_clan && c_party->members.size() == my_party_count);
                    c_party.unlock();
                    if (is_match) {
                        BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "party register found a match: ({})", c_party_id);
                        match_party_id = c_party_id;
                        break;
                    }
                }
                party_ids_lock.unlock();

                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "party register and found a match id: ({})", match_party_id);
                if (match_party_id) {
                    struct info_req {
                        std::uint16_t targetPartyId;
                    };
                    info_req new_req;
                    new_req.targetPartyId = match_party_id;
                    CMessage msg = CMessage();
                    msg.SetSession(callback.session->GetSessionId());
                    //msg.SetCommand(callback.message->GetOrder(), callback.message->GetMission(), callback.message->GetExtra(), callback.message->GetOption());
                    msg.SetData(reinterpret_cast<uint8_t*>(&new_req), sizeof(new_req));
                    callback.message = &msg;
                    acc_cache.unlock();
                    lock.unlock();
                    BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "will join battle with party id: ({})", match_party_id);
                    ClanOtherJoin(callback, main_server);
                    return;
                }
                party_cache->is_registered = true;
            }
            else if (c_extra == 45) {
                party_cache->is_registered = false;
            }
            for (const auto& party_member_session_id : party_cache->members)
            {
                if (auto player_session = server->GetSessionById(party_member_session_id))
                    send_msg(player_session.get(), 119, callback.message->GetMission(), c_extra, callback.message->GetOption());
            }
        }
    }
}