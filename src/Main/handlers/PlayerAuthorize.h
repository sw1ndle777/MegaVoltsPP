#pragma once
namespace Game
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Main;

    namespace Handlers
    {
        struct guide_daily_mission
        {
            uint32_t mission_id = 46;
            uint32_t unk = 0;
            uint32_t goal = 0;
            uint32_t type = 1;//1 guide mission, 4 daily mission

            guide_daily_mission(uint32_t id, uint32_t goal, uint32_t type)
            {
                this->mission_id = id;
                this->goal = goal;
                this->type = type;
            };
        };

        struct MainToCastSendPingAssureInfo
        {
            uint32_t session_id;
        } cast_ping_info;

        inline void PlayerAuthorize(SCallbackData& callback, CMainServer* main_server)
        {
            const auto& versionCheckReq = reinterpret_cast<MainVersionCheckReq*>(callback.message->GetData());
            BaseLib::DbPool->submit_task([=]() mutable
            {
                auto auth_key = versionCheckReq->authKey;

                std::shared_lock lock(callback.session->GetMutex());
                CSession* session = callback.session;
                auto session_id = session->GetSessionId();
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "session id: ({}) connected with auth key ({})", session_id, auth_key);
                CServer* server = callback.server;

                auto send_msg = [&](CSession* session, uint16_t order, uint8_t mission, uint8_t extra, uint8_t option, uint8_t* data = nullptr, uint16_t data_size = 0)
                {
                    CMessage message(session->GetEncryptionKey());
                    message.SetSession(session->GetSessionId());
                    message.SetCommand(order, mission, extra, option);
                    if (data_size > 0 && data != nullptr) message.SetData(data, data_size);
                    session->Send(message);
                };


                BaseLib::FrontAccount frontAccount;
                BaseLib::ClanInfo clanInfo{};
                PlayerDailyMission playerDailyMissionData{};
                std::vector<Item> acc_items;
                std::vector<BlockedInfo> acc_blockeds;
                std::vector<FriendInfo> acc_friends;
				std::vector<BaseLib::MailboxInfo> mailbox_list;
               
                
                auto daily_mission_ids_random = main_server->GetRandomDailyMissionIds(3,0,0,0);
                if (!BaseLib::Database->GetMainFrontAccount(auth_key, &frontAccount, &clanInfo, &playerDailyMissionData, acc_items, acc_blockeds, acc_friends, mailbox_list, daily_mission_ids_random))
                {
                    BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan,
                                             "session id: ({}) with auth key: ({}) doesn't exist in database",
                                             session->GetSessionId(), auth_key);
                    return;
                }
                if (frontAccount.IsOnline)
                    main_server->DisconnectPlayerMultipleLogin(auth_key, main_server);


				frontAccount.IsOnline = true;


                boost::unordered_flat_map<uint8_t, std::vector<InventoryItemInfo>> player_equipped_items;
                std::vector<Item> player_inventory_items;
                auto server_time = Utility::GetUtcTimeNowInMilliseconds() - server->GetStartTime();
                auto newPlayer = Player({ session->GetSessionId(), server_time, frontAccount, acc_items });
                main_server->TransformItems(acc_items, player_inventory_items);//check here
                main_server->TransformEquippedItems(acc_items, player_equipped_items);
                main_server->AddAccCache(session->GetSessionId(), newPlayer);
                MainAccountInfoAck accInfoMsg = MainAccountInfoAck();
                if (frontAccount.ClanId)
                {
                    accInfoMsg.ClanLogoFront = clanInfo.logo_front;
                    accInfoMsg.ClanLogoBack = clanInfo.logo_back;
                    std::strcpy(accInfoMsg.ClanName, clanInfo.name.c_str());
                    if (main_server->IsClanAlready(frontAccount.ClanId))
                    {
                        auto clan = main_server->GetClanCacheUnique(frontAccount.ClanId);
                        clan->online_members.push_back(session_id);
                        clan.unlock();
                    }
                    else
                    {
                        Clan newClan;
                        newClan.clan_id = frontAccount.ClanId;
                        newClan.logo_front = clanInfo.logo_front;
                        newClan.logo_back = clanInfo.logo_back;
                        newClan.clan_name = clanInfo.name;
                        newClan.online_members.push_back(session_id);
                        main_server->AddClanCache(frontAccount.ClanId, newClan);
                    }
                    accInfoMsg.ClanContribution = frontAccount.ClanContribution;
                    accInfoMsg.ClanWins = frontAccount.ClanWins;
                    accInfoMsg.ClanLoses = frontAccount.ClanLoses;
                    accInfoMsg.ClanDraws = frontAccount.ClanDraws;
                    accInfoMsg.ClanKills = frontAccount.ClanKills;
                    accInfoMsg.ClanDeaths = frontAccount.ClanDeaths;
                    accInfoMsg.ClanAssists = frontAccount.ClanAssists;
                }
                else
                {
                    accInfoMsg.ClanLogoFront = 0;
                    accInfoMsg.ClanLogoBack = 0;
                    std::strcpy(accInfoMsg.ClanName, "");
                    accInfoMsg.ClanLogoFront = 0;
                    accInfoMsg.ClanLogoBack = 0;
                    accInfoMsg.ClanContribution = 0;
                    accInfoMsg.ClanWins = 0;
                    accInfoMsg.ClanLoses = 0;
                    accInfoMsg.ClanDraws = 0;
                    accInfoMsg.ClanKills = 0;
                    accInfoMsg.ClanDeaths = 0;
                    accInfoMsg.ClanAssists = 0;
                }

                accInfoMsg.Diorama = 0;
                accInfoMsg.Kills = frontAccount.Kills;
                accInfoMsg.Deaths = frontAccount.Deaths;
                accInfoMsg.Assists = frontAccount.Assists;
                accInfoMsg.Wins = frontAccount.Wins;
                accInfoMsg.Loses = frontAccount.Loses;
                accInfoMsg.Draws = frontAccount.Draws;
                accInfoMsg.Melee = frontAccount.MeleeKills;
                accInfoMsg.Rifle = frontAccount.RifleKills;
                accInfoMsg.Shotgun = frontAccount.ShotgunKills;
                accInfoMsg.Sniper = frontAccount.SniperKills;
                accInfoMsg.Gatling = frontAccount.GatlingKills;
                accInfoMsg.Bazooka = frontAccount.BazookaKills;
                accInfoMsg.Grenade = frontAccount.GrenadeKills;
                accInfoMsg.Headshots = frontAccount.Headshots;
                accInfoMsg.HighestKillStreak = frontAccount.HighestKillStreak;
                accInfoMsg.Unknown2 = 0;
                accInfoMsg.PlayTime = static_cast<uint32_t>(frontAccount.PlayTime);
                accInfoMsg.ClanId = frontAccount.ClanId;
                accInfoMsg.ClanPadding = 0;
                accInfoMsg.ZombieKillPoints = frontAccount.ZombieKills * 3;
                accInfoMsg.Infections = frontAccount.Infections;
                accInfoMsg.Unknown3 = 210;
                accInfoMsg.ServerTime = server_time;
                accInfoMsg.UniqueId = NetEngine::Packets::Core::UniqueId(session->GetSessionId(), 1).data;
                accInfoMsg.Grade = frontAccount.Grade;
                accInfoMsg.SelectedCharacter = frontAccount.SelectedCharacter;
                accInfoMsg.OwnedCharacters = 511;//all chars
                accInfoMsg.Level = frontAccount.Level + 1;
            #if defined(RELEASE_1_0_3)
                accInfoMsg.Energy = 50;//frontAccount.Energy;
                accInfoMsg.Energy2 = frontAccount.Energy;
                accInfoMsg.GoldenMode = frontAccount.PCRoom;//PCROOM PC BANG PC ROOM
                accInfoMsg.unused = 38;

            #else
                accInfoMsg.Coins = frontAccount.Coins;
                accInfoMsg.Energy = frontAccount.Energy;
            #endif


                accInfoMsg.LuckyPoints = frontAccount.LuckyPoints;
                accInfoMsg.Experience = frontAccount.Experience;
                accInfoMsg.MicroPoints = frontAccount.MicroPoints;
                accInfoMsg.RockTokens = frontAccount.RockTokens;
                accInfoMsg.Tutorial = frontAccount.Tutorial;
                accInfoMsg.MaximumItems = frontAccount.MaximumItems;
                accInfoMsg.MaximumEnergy = frontAccount.MaximumEnergy;
                accInfoMsg.DailyAttempts = frontAccount.SingleWaveDailyAttempts;
                accInfoMsg.HighestWave = frontAccount.SingleWaveHighestWave;
                accInfoMsg.SinglewaveHighscore = frontAccount.SingleWaveHighScore;
                accInfoMsg.Unknown4 = 24;
                accInfoMsg.Story = frontAccount.Story;
                accInfoMsg.Achievements[0] = frontAccount.Achievement;
            #if defined(RELEASE_1_1_1)
                accInfoMsg.VIPLevel = frontAccount.VIPExperience;
            #endif
                accInfoMsg.AccountAuthkey = auth_key;
                //accInfoMsg.AccountId = frontAccount.Index;

                std::strcpy(accInfoMsg.Unused, "");
                std::strcpy(accInfoMsg.Nickname, frontAccount.Nickname.c_str());

                //option is chat channel id
                send_msg(session, 413, 0, 1, 1, reinterpret_cast<uint8_t*>(&accInfoMsg), sizeof(MainAccountInfoAck));
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "session id: ({}) auth key: ({}) received account informations", session->GetSessionId(), auth_key);

                if (player_inventory_items.empty() && newPlayer.acc_info.Coupons == 0)
                    send_msg(session, 77, 0, 6, 0); // empty inventory
                if (newPlayer.acc_info.Coupons > 0)
                {
                    uint32_t coupons = (newPlayer.acc_info.Coupons << 23) + 0xF4240;
                    Item coupons_item;
                    coupons_item.is_equipped = 0;
                    coupons_item.stock = newPlayer.acc_info.Coupons;
                    coupons_item.character_id = -1;
                    coupons_item.item_info = InventoryItemInfo();
                    coupons_item.item_info.item_number.data = coupons;//InventoryItemNumber(1000000, newPlayer.acc_info.Coupons);
                    coupons_item.item_info.expire_date = Utility::GetUnixEpoch();
                    coupons_item.item_info.serial_info = ItemSerialInfo(0, 0, 0, 0, Utility::GetUnixEpoch());
                    player_inventory_items.insert(player_inventory_items.begin(), coupons_item);
                }
                constexpr std::uint32_t max_packet_size = 1440;
                constexpr std::uint32_t full_header_size = 8;
                constexpr std::uint32_t split_size = (max_packet_size - full_header_size) / sizeof(InventoryItemInfo);
                uint32_t total_inventory_fragments = (player_inventory_items.size() + 1) <= split_size ? 1 : ((player_inventory_items.size() + 1) / split_size) + 1;
                for (uint32_t i = 0; i < total_inventory_fragments; i++)
                {
                    if (!player_inventory_items.empty())
                    {
                        auto items_batch = main_server->GetTransformStockItems(player_inventory_items, i, split_size);
                        if (!items_batch.empty())
                            send_msg(session, 77, 0, (i == 0) ? 37 : 0, items_batch.size(), reinterpret_cast<uint8_t*>(items_batch.data()), items_batch.size() * sizeof(InventoryItemInfo));
                    }

                }
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "session id: ({}) received ({}) inventory items", session->GetSessionId(), player_inventory_items.size() - 1);

                for (uint8_t i = 0; i < 5; i++)
                {
                    std::vector<EquipItemInfo> equipped_items;
                    auto current_char_items = main_server->GetTransformEquippedItems(player_equipped_items[i]);
                    for (auto& item : current_char_items)
                    {
                        if (main_server->IsItemSet(item.item_number.item_id))
                        {
                            const auto& set_item_types = main_server->GetSetItemTypes(item.item_number.item_id);
                            for (const auto& item_type : set_item_types)
                            {
                                item.item_number.item_type = 17;
                                equipped_items.push_back(item);
                            }
                        }
                        else
                        {
                            if (item.item_number.item_type == 22)
                                item.item_number.item_type = 19;
                            if (item.item_number.item_type == 23)
                                item.item_number.item_type = 20;
                            equipped_items.push_back(item);
                        }
                    }
                    BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "session id: ({}) received ({}) equip items on ({})", session->GetSessionId(), equipped_items.size(), main_server->GetCharacterStr(i).c_str());
                    send_msg(session, 75, 0, i, equipped_items.size(), reinterpret_cast<uint8_t*>(equipped_items.data()), equipped_items.size() * sizeof(EquipItemInfo));
                }
            #if defined(RELEASE_1_0_3)
                send_msg(session, 75, 0, 5, 0); // final equip info
            #else
                send_msg(session, 75, 0, 16, 0); // final equip info
            #endif

                auto timer = std::make_shared<asio::steady_timer>(main_server->GetIoContext(), std::chrono::milliseconds(100));
                timer->async_wait([timer, main_server, session_id, send_msg, session](const asio::error_code& ec)
                {
                    CMessage keepAliveMsg = CMessage();
                    keepAliveMsg.SetSession(session->GetSessionId());
                    keepAliveMsg.SetCommand(0, 0, 0, 0);
                    session->Send(keepAliveMsg);
                    cast_ping_info.session_id = session_id;
                    main_server->SendCastIpc(PacketIds::Ipc::MainToCastSendPingAssure, Utility::ToVector(cast_ping_info));
                });


                main_server->SendServerMessage(session, fmt::format("[MegaVolts Online] Welcome, {}", accInfoMsg.Nickname).c_str());
                main_server->SendServerMessage(session, fmt::format("[MegaVolts Online] Server's uptime {}", Utility::FormatMilliseconds(server_time).c_str()).c_str());


                boost::unordered_flat_map<uint32_t, uint32_t> accountToSessionMap;
                std::shared_lock acc_lock(main_server->GetAccountsCacheMutex());
                for (const auto& session : accounts_cache)  accountToSessionMap[session.second.acc_info.Index] = session.first;
                std::vector<PlayerFriendInfo> friends_pending;
                std::vector<FriendInfo> friends_pending_db;
                std::vector<FriendInfo> friends_accepted;

                //friends
                for (auto& friendInfo : acc_friends)
                {
                    auto it = accountToSessionMap.find(friendInfo.friend_account_id);
                    if (it != accountToSessionMap.end())
                        friendInfo.friend_session_id = it->second;

                    if (friendInfo.state == Userlist::Friends::State::Pending)
                    {
                        friends_pending.push_back({ (friendInfo.friend_session_id != 0) ? NetEngine::Packets::Core::UniqueId(friendInfo.friend_session_id, 1).data : NetEngine::Packets::Core::UniqueId(0).data ,
                                                  friendInfo.friend_account_id, friendInfo.friend_nickname.c_str() });
                        friends_pending_db.push_back(friendInfo);
                    }
                    else if (friendInfo.state == Userlist::Friends::State::Accepted)
                        friends_accepted.push_back(friendInfo);
                }

                main_server->AddPlayerFriends(session->GetSessionId(), acc_friends);

                //blockeds
                for (auto& blockedInfo : acc_blockeds)
                {
                    auto it = accountToSessionMap.find(blockedInfo.blocked_account_id);
                    if (it != accountToSessionMap.end())
                        blockedInfo.blocked_session_id = it->second;
                }

                main_server->AddPlayerBlockeds(session->GetSessionId(), acc_blockeds);


                //friends pendings
                send_msg(session, 61, 0, Userlist::Friends::AddResult::SendPending, friends_pending.size(), reinterpret_cast<uint8_t*>(friends_pending.data()), friends_pending.size() * sizeof(PlayerFriendInfo));
                for (const auto& friend_info : friends_accepted)
                {
                    if (!friend_info.friend_session_id) continue;
                    BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "for session id {} ({}) should notify friend ({})", session->GetSessionId(), accInfoMsg.Nickname, friend_info.friend_nickname);
                    send_msg(server->GetSessionById(friend_info.friend_session_id).get(), 85, 0, Userlist::Friends::DetailsType::FriendState, Userlist::FriendsState::Login, reinterpret_cast<uint8_t*>(&frontAccount.Index), sizeof(frontAccount.Index));
                    main_server->RemovePlayerFriends(friend_info.friend_session_id, frontAccount.Index);
                    main_server->AddPlayerFriends(friend_info.friend_session_id, { friend_info.friend_account_id, frontAccount.Index, Userlist::Friends::State::Accepted, session_id, frontAccount.Nickname });

                }
                BaseLib::Database->DeletePlayerFriends(friends_pending_db);

                //mailbox
                for (const auto& mail : mailbox_list)
                {
                    main_server->AddMailboxDataCache(mail.mail_id, MailboxData(mail));
                    if (mail.gift_itemid == 0)
                    {
                        main_server->AddMailboxSentIdCache(mail.mail_id, mail.sender_account_id);
                        main_server->AddMailboxRecvIdCache(mail.mail_id, mail.receiver_account_id);
                    }
                    else
                        main_server->AddGiftboxRecvIdCache(mail.mail_id, mail.receiver_account_id);

                }
                uint32_t unopened_gifts = 0, unopened_mails = 0;
                auto mail_recv_ids = main_server->GetMailboxRecvCacheShared(frontAccount.Index);
                for (uint32_t i = 0; i < mail_recv_ids->size(); i++)
                {
                    auto mail_id = mail_recv_ids->at(i);
                    auto mailbox_data = main_server->GetMailboxDataCacheShared(mail_id);
                    if (mailbox_data->is_new && mailbox_data->gift_itemid == 0) unopened_mails++;
                }
                auto gift_recv_ids = main_server->GetGiftboxRecvCacheShared(frontAccount.Index);
                for (uint32_t i = 0; i < gift_recv_ids->size(); i++)
                {
                    auto mail_id = gift_recv_ids->at(i);
                    auto mailbox_data = main_server->GetMailboxDataCacheShared(mail_id);
                    if (mailbox_data->gift_itemid != 0) unopened_gifts++;
                }


                send_msg(session, 105, 0, 37, unopened_mails); // remainder of unopened mails
                send_msg(session, 66, 0, 37, unopened_gifts); // remainder of unopened mails



                auto guideMissionData = guide_daily_mission(46, 0, 1);
                guideMissionData.mission_id = frontAccount.GuideMission + 46;

                send_msg(session, 167, 0, 1, 1, reinterpret_cast<uint8_t*>(&guideMissionData), sizeof(guideMissionData));

                std::vector<guide_daily_mission> dailyMissions;
                dailyMissions.push_back(guide_daily_mission(daily_mission_ids_random[0], 0, 4));
                dailyMissions.push_back(guide_daily_mission(daily_mission_ids_random[1], 0, 4));
                dailyMissions.push_back(guide_daily_mission(daily_mission_ids_random[2], 0, 4));

                send_msg(session, 167, 0, 1, dailyMissions.size(), reinterpret_cast<uint8_t*>(dailyMissions.data()), sizeof(guide_daily_mission)* dailyMissions.size());

                auto acc_cache = main_server->GetAccCacheUniqueBySessionId(session_id);
                acc_cache->daily_mission_info = playerDailyMissionData;
                acc_cache.unlock();

                send_msg(session, 413, 0, 59, 0, reinterpret_cast<uint8_t*>(&frontAccount.VoiceType), sizeof(frontAccount.VoiceType)); // final account info

            });
        }
        /*
        inline void PlayerAuthorize(SCallbackData& callback, CMainServer* main_server)
        {
            const auto& versionCheckReq = reinterpret_cast<MainVersionCheckReq*>(callback.message->GetData());
            const auto& auth_key = versionCheckReq->authKey;
           
            //session->SetAuthKey(versionCheckReq->authKey);
            std::shared_lock lock(callback.session->GetMutex());
            CSession* session = callback.session;
            auto session_id = session->GetSessionId();
            BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "session id: ({}) connected with auth key ({})", session_id, auth_key);
            CServer* server = callback.server;

            auto send_msg = [&](CSession* session, uint16_t order, uint8_t mission, uint8_t extra, uint8_t option, uint8_t* data = nullptr, uint16_t data_size = 0)
            {
                CMessage message(session->GetEncryptionKey());
                message.SetSession(session->GetSessionId());
                message.SetCommand(order, mission, extra, option);
                if (data_size > 0 && data != nullptr) message.SetData(data, data_size);
                session->Send(message);
            };



            BaseLib::FrontAccount frontAccount;
            std::vector<Item> acc_items;
            std::vector<FriendInfo> acc_friends;
            std::vector<BlockedInfo> acc_blockeds;
            if (!BaseLib::Database->GetFrontAccount(auth_key, &frontAccount))
            {
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan,
                    "session id: ({}) with auth key: ({}) doesn't exist in database",
                    session->GetSessionId(), auth_key);
                return;
            }
            if (frontAccount.Nickname.empty())
            {
                send_msg(session, 68, 0, 0, VersionCheckInfo::Result::NicknameDialog);

                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan,
                    "session id: ({}) player doesn't have a nickname, redirecting to nickname dialog",
                    session->GetSessionId());
                return;
            }
            auto itemsFound = BaseLib::Database->GetInventoryItems(frontAccount.Index, acc_items);
            //std::unordered_map<uint8_t, std::vector<InventoryItemInfo>> player_equipped_items;
            boost::unordered_flat_map<uint8_t, std::vector<InventoryItemInfo>> player_equipped_items;
            std::vector<Item> player_inventory_items;
            auto server_time = Utility::GetUtcTimeNowInMilliseconds() - server->GetStartTime();
            auto newPlayer = Player({ session->GetSessionId(), server_time, frontAccount, acc_items });
            main_server->TransformItems(acc_items, player_inventory_items);//check here
            main_server->TransformEquippedItems(acc_items, player_equipped_items);
            main_server->AddAccCache(session->GetSessionId(), newPlayer);
            MainAccountInfoAck accInfoMsg = MainAccountInfoAck();
            if (frontAccount.ClanId)
            {
                BaseLib::ClanInfo clanInfo;
                if (!BaseLib::Database->GetClanInfo(frontAccount.ClanId, &clanInfo))
                {
                    BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan,
                        "session id: ({}) player clan id: ({}) doesn't exist in database",
                        session->GetSessionId(), frontAccount.ClanId);

                    accInfoMsg.ClanLogoFront = 0;
                    accInfoMsg.ClanLogoBack = 0;
                    std::strcpy(accInfoMsg.ClanName, "");
                }
                else
                {
                    accInfoMsg.ClanLogoFront = clanInfo.logo_front;
                    accInfoMsg.ClanLogoBack = clanInfo.logo_back;
                    std::strcpy(accInfoMsg.ClanName, clanInfo.name.c_str());
                    if (main_server->IsClanAlready(frontAccount.ClanId))
                    {
                        auto clan = main_server->GetClanCacheUnique(frontAccount.ClanId);
                        clan->online_members.push_back(session_id);
                        clan.unlock();
                    }
                    else
                    {
                        Clan newClan;
                        newClan.clan_id = frontAccount.ClanId;
                        newClan.logo_front = clanInfo.logo_front;
                        newClan.logo_back = clanInfo.logo_back;
                        newClan.clan_name = clanInfo.name;
                        newClan.online_members.push_back(session_id);
                        main_server->AddClanCache(frontAccount.ClanId, newClan);
                    }
                }
                accInfoMsg.ClanContribution = frontAccount.ClanContribution;
                accInfoMsg.ClanWins = frontAccount.ClanWins;
                accInfoMsg.ClanLoses = frontAccount.ClanLoses;
                accInfoMsg.ClanDraws = frontAccount.ClanDraws;
                accInfoMsg.ClanKills = frontAccount.ClanKills;
                accInfoMsg.ClanDeaths = frontAccount.ClanDeaths;
                accInfoMsg.ClanAssists = frontAccount.ClanAssists;
            }
            else
            {
                accInfoMsg.ClanLogoFront = 0;
                accInfoMsg.ClanLogoBack = 0;
                std::strcpy(accInfoMsg.ClanName, "");
                accInfoMsg.ClanLogoFront = 0;
                accInfoMsg.ClanLogoBack = 0;
                accInfoMsg.ClanContribution = 0;
                accInfoMsg.ClanWins = 0;
                accInfoMsg.ClanLoses = 0;
                accInfoMsg.ClanDraws = 0;
                accInfoMsg.ClanKills = 0;
                accInfoMsg.ClanDeaths = 0;
                accInfoMsg.ClanAssists = 0;
            }

            accInfoMsg.Diorama = 0;
            accInfoMsg.Kills = frontAccount.Kills;
            accInfoMsg.Deaths = frontAccount.Deaths;
            accInfoMsg.Assists = frontAccount.Assists;
            accInfoMsg.Wins = frontAccount.Wins;
            accInfoMsg.Loses = frontAccount.Loses;
            accInfoMsg.Draws = frontAccount.Draws;
            accInfoMsg.Melee = frontAccount.MeleeKills;
            accInfoMsg.Rifle = frontAccount.RifleKills;
            accInfoMsg.Shotgun = frontAccount.ShotgunKills;
            accInfoMsg.Sniper = frontAccount.SniperKills;
            accInfoMsg.Gatling = frontAccount.GatlingKills;
            accInfoMsg.Bazooka = frontAccount.BazookaKills;
            accInfoMsg.Grenade = frontAccount.GrenadeKills;
            accInfoMsg.Headshots = frontAccount.Headshots;
            accInfoMsg.HighestKillStreak = frontAccount.HighestKillStreak;
            accInfoMsg.Unknown2 = 0;
            accInfoMsg.PlayTime = static_cast<uint32_t>(frontAccount.PlayTime);
            accInfoMsg.ClanId = frontAccount.ClanId;
            accInfoMsg.ClanPadding = 0;
            accInfoMsg.ZombieKillPoints = frontAccount.ZombieKills * 3;
            accInfoMsg.Infections = frontAccount.Infections;
            accInfoMsg.Unknown3 = 210;
            accInfoMsg.ServerTime = server_time;
            accInfoMsg.UniqueId = NetEngine::Packets::Core::UniqueId(session->GetSessionId(), 1).data;
            accInfoMsg.Grade = frontAccount.Grade;
            accInfoMsg.SelectedCharacter = frontAccount.SelectedCharacter;
            accInfoMsg.OwnedCharacters = 511;//all chars
            accInfoMsg.Level = frontAccount.Level + 1;
        #if defined(RELEASE_1_0_3)
            accInfoMsg.Energy = 50;//frontAccount.Energy;
            accInfoMsg.Energy2 = frontAccount.Energy;
            accInfoMsg.GoldenMode = frontAccount.PCRoom;//PCROOM PC BANG PC ROOM
            accInfoMsg.unused = 38;

        #else
            accInfoMsg.Coins = frontAccount.Coins;
            accInfoMsg.Energy = frontAccount.Energy;
        #endif


            accInfoMsg.LuckyPoints = frontAccount.LuckyPoints;
            accInfoMsg.Experience = frontAccount.Experience;
            accInfoMsg.MicroPoints = frontAccount.MicroPoints;
            accInfoMsg.RockTokens = frontAccount.RockTokens;
            accInfoMsg.Tutorial = frontAccount.Tutorial;
            accInfoMsg.MaximumItems = frontAccount.MaximumItems;
            accInfoMsg.MaximumEnergy = frontAccount.MaximumEnergy;
            accInfoMsg.DailyAttempts = frontAccount.SingleWaveDailyAttempts;
            accInfoMsg.HighestWave = frontAccount.SingleWaveHighestWave;
            accInfoMsg.SinglewaveHighscore = frontAccount.SingleWaveHighScore;
            accInfoMsg.Unknown4 = 24;
            accInfoMsg.Story = frontAccount.Story;
            accInfoMsg.Achievements[0] = frontAccount.Achievement;
        #if defined(RELEASE_1_1_1)
            accInfoMsg.VIPLevel = frontAccount.VIPExperience;
        #endif
            accInfoMsg.AccountAuthkey = auth_key;
            //accInfoMsg.AccountId = frontAccount.Index;

            std::strcpy(accInfoMsg.Unused, "");
            std::strcpy(accInfoMsg.Nickname, frontAccount.Nickname.c_str());

            //option is chat channel id
            send_msg(session, 413, 0, 1, 1, reinterpret_cast<uint8_t*>(&accInfoMsg), sizeof(MainAccountInfoAck));
            BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "session id: ({}) auth key: ({}) received account informations", session->GetSessionId(), auth_key);

            if (player_inventory_items.empty() && newPlayer.acc_info.Coupons == 0)
                send_msg(session, 77, 0, 6, 0); // empty inventory
            if (newPlayer.acc_info.Coupons > 0)
            {
                uint32_t coupons = (newPlayer.acc_info.Coupons << 23) + 0xF4240;
                Item coupons_item;
                coupons_item.is_equipped = 0;
                coupons_item.stock = newPlayer.acc_info.Coupons;
                coupons_item.character_id = -1;
                coupons_item.item_info = InventoryItemInfo();
                coupons_item.item_info.item_number.data = coupons;//InventoryItemNumber(1000000, newPlayer.acc_info.Coupons);
                coupons_item.item_info.expire_date = Utility::GetUnixEpoch();
                coupons_item.item_info.serial_info = ItemSerialInfo(0, 0, 0, 0, Utility::GetUnixEpoch());
                player_inventory_items.insert(player_inventory_items.begin(), coupons_item);
            }
            constexpr std::uint32_t max_packet_size = 1440;
            constexpr std::uint32_t full_header_size = 8;
			constexpr std::uint32_t split_size = (max_packet_size - full_header_size) / sizeof(InventoryItemInfo);
            uint32_t total_inventory_fragments = (player_inventory_items.size() + 1) <= split_size ? 1 : ((player_inventory_items.size() + 1) / split_size) + 1;
            for (uint32_t i = 0; i < total_inventory_fragments; i++)
            {
                if (!player_inventory_items.empty())
                {
                    auto items_batch = main_server->GetTransformStockItems(player_inventory_items, i, split_size);
                    if (!items_batch.empty())
                        send_msg(session, 77, 0, (i == 0) ? 37 : 0, items_batch.size(), reinterpret_cast<uint8_t*>(items_batch.data()), items_batch.size() * sizeof(InventoryItemInfo));
                }

            }
            BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "session id: ({}) received ({}) inventory items", session->GetSessionId(), player_inventory_items.size() - 1);

            for (uint8_t i = 0; i < 5; i++)
            {
                std::vector<EquipItemInfo> equipped_items;
                const auto& current_char_items = main_server->GetTransformEquippedItems(player_equipped_items[i]);
                for (const auto& item : current_char_items)
                {
                    //if (main_server->IsItemWeapon(item.item_number.item_id) || main_server->IsItemCostume(item.item_number.item_id) || main_server->IsItemDiorama(item.item_number.item_id))
                    //    equipped_items.push_back(EquipItemInfo(item));
                    if (main_server->IsItemSet(item.item_number.item_id))
                    {

                        const auto& set_item_types = main_server->GetSetItemTypes(item.item_number.item_id);
                        for (const auto& item_type : set_item_types)
                        {
                            auto set_item = EquipItemInfo(item);
                            set_item.item_number.item_type = 17;
                            equipped_items.push_back(set_item);
                        }
                    }
                    else
                    {
                        auto eq_item = EquipItemInfo(item);
                        if (eq_item.item_number.item_type == 22)
                        {
                            eq_item.item_number.item_type = 19;
                        }
                        if (eq_item.item_number.item_type == 23)
                        {
                            eq_item.item_number.item_type = 20;
                        }
                        equipped_items.push_back(eq_item);
                    }
                }
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "session id: ({}) received ({}) equip items on ({})", session->GetSessionId(), equipped_items.size(), main_server->GetCharacterStr(i).c_str());
                send_msg(session, 75, 0, i, equipped_items.size(), reinterpret_cast<uint8_t*>(equipped_items.data()), equipped_items.size() * sizeof(EquipItemInfo));
            }
        #if defined(RELEASE_1_0_3)
            send_msg(session, 75, 0, 5, 0); // final equip info
        #else
            send_msg(session, 75, 0, 16, 0); // final equip info
        #endif

            //uint64_t unlocked_voice_types = 0xFFFFFFFFFFFFFFFF;
            

            auto timer = std::make_shared<asio::steady_timer>(main_server->GetIoContext(), std::chrono::milliseconds(100));
            timer->async_wait([timer, main_server, session_id, send_msg, session](const asio::error_code& ec)
                {
                    CMessage keepAliveMsg = CMessage();
                    keepAliveMsg.SetSession(session->GetSessionId());
                    keepAliveMsg.SetCommand(0, 0, 0, 0);
                    session->Send(keepAliveMsg);

                    struct MainToCastSendPingAssureInfo
                    {
                        uint32_t session_id;
                    } info;
                    info.session_id = session_id;
                    main_server->SendCastIpc(PacketIds::Ipc::MainToCastSendPingAssure, Utility::ToVector(info));
                });

            
            main_server->SendServerMessage(session, fmt::format("[MegaVolts Online] Welcome, {}", accInfoMsg.Nickname).c_str());
            main_server->SendServerMessage(session, fmt::format("[MegaVolts Online] Server's uptime {}", Utility::FormatMilliseconds(server_time).c_str()).c_str());
           // main_server->SendServerMessage(session, fmt::format("[MAIN] session id: ({}), server time: ({})", session->GetSessionId(), server_time).c_str());

            //std::unordered_map<uint32_t, uint32_t> accountToSessionMap;
            boost::unordered_flat_map<uint32_t, uint32_t> accountToSessionMap;
            std::shared_lock acc_lock(main_server->GetAccountsCacheMutex());
            for (const auto& session : accounts_cache)  accountToSessionMap[session.second.acc_info.Index] = session.first;
            std::vector<PlayerFriendInfo> friends_pending;
            std::vector<FriendInfo> friends_pending_db;
            std::vector<FriendInfo> friends_accepted;

            if (BaseLib::Database->GetPlayerFriends(frontAccount.Index, acc_friends))
            {
                for (auto& friendInfo : acc_friends)
                {
                    auto it = accountToSessionMap.find(friendInfo.friend_account_id);
                    if (it != accountToSessionMap.end())
                        friendInfo.friend_session_id = it->second;

                    if (friendInfo.state == Userlist::Friends::State::Pending)
                    {
                        friends_pending.push_back({ (friendInfo.friend_session_id != 0) ? NetEngine::Packets::Core::UniqueId(friendInfo.friend_session_id, 1).data : NetEngine::Packets::Core::UniqueId(0).data ,
                           friendInfo.friend_account_id, friendInfo.friend_nickname.c_str() });
                        friends_pending_db.push_back(friendInfo);
                    }
                    else if (friendInfo.state == Userlist::Friends::State::Accepted)
                        friends_accepted.push_back(friendInfo);
                }

                main_server->AddPlayerFriends(session->GetSessionId(), acc_friends);
                //friends_cache[session->GetSessionId()] = acc_friends;
            }
            if (BaseLib::Database->GetPlayerBlockeds(frontAccount.Index, acc_blockeds))
            {
                for (auto& blockedInfo : acc_blockeds)
                {
                    auto it = accountToSessionMap.find(blockedInfo.blocked_account_id);
                    if (it != accountToSessionMap.end())
                        blockedInfo.blocked_session_id = it->second;
                }
                //blockeds_cache[session->GetSessionId()] = acc_blockeds;
                main_server->AddPlayerBlockeds(session->GetSessionId(), acc_blockeds);
            }
            send_msg(session, 61, 0, Userlist::Friends::AddResult::SendPending, friends_pending.size(), reinterpret_cast<uint8_t*>(friends_pending.data()), friends_pending.size() * sizeof(PlayerFriendInfo));
            for (const auto& friend_info : friends_accepted)
            {
                if (!friend_info.friend_session_id) continue;
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "for session id {} ({}) should notify friend ({})", session->GetSessionId(), accInfoMsg.Nickname, friend_info.friend_nickname);
                send_msg(server->GetSessionById(friend_info.friend_session_id).get(), 85, 0, Userlist::Friends::DetailsType::FriendState, Userlist::FriendsState::Login, reinterpret_cast<uint8_t*>(&frontAccount.Index), sizeof(frontAccount.Index));
                main_server->RemovePlayerFriends(friend_info.friend_session_id, frontAccount.Index);
                main_server->AddPlayerFriends(friend_info.friend_session_id, { friend_info.friend_account_id, frontAccount.Index, Userlist::Friends::State::Accepted, session_id, frontAccount.Nickname });

            }
            BaseLib::Database->DeletePlayerFriends(friends_pending_db);
            auto mails = BaseLib::Database->GetPlayerMailbox(frontAccount.Index);
            
            for (const auto& mail : mails)
            {
                main_server->AddMailboxDataCache(mail.mail_id, MailboxData(mail));
                if (mail.gift_itemid == 0)
                {
                    main_server->AddMailboxSentIdCache(mail.mail_id, mail.sender_account_id);
                    main_server->AddMailboxRecvIdCache(mail.mail_id, mail.receiver_account_id);
                }
                else
                    main_server->AddGiftboxRecvIdCache(mail.mail_id, mail.receiver_account_id);
                
            }
            uint32_t unopened_gifts = 0, unopened_mails = 0;
            auto mail_recv_ids = main_server->GetMailboxRecvCacheShared(frontAccount.Index);
            for (uint32_t i = 0; i < mail_recv_ids->size(); i++)
            {
                auto mail_id = mail_recv_ids->at(i);
                auto mailbox_data = main_server->GetMailboxDataCacheShared(mail_id);
                if (mailbox_data->is_new && mailbox_data->gift_itemid == 0) unopened_mails++;
            }
            auto gift_recv_ids = main_server->GetGiftboxRecvCacheShared(frontAccount.Index);
            for (uint32_t i = 0; i < gift_recv_ids->size(); i++)
            {
                auto mail_id = gift_recv_ids->at(i);
                auto mailbox_data = main_server->GetMailboxDataCacheShared(mail_id);
                if (mailbox_data->gift_itemid != 0) unopened_gifts++;
            }


            send_msg(session, 105, 0, 37, unopened_mails); // remainder of unopened mails
            send_msg(session, 66, 0, 37, unopened_gifts); // remainder of unopened mails


            auto is_inventory_full = player_inventory_items.size() >= frontAccount.MaximumItems;
            auto is_gift_box_full = unopened_gifts >= 100;
            BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "player inventory is full: ({}), giftbox is full: ({})", is_inventory_full, is_gift_box_full);
            SystemMonthlyRewards monthlyRewardsData;
            auto current_month = Utility::GetCurrentMonth();
            if (BaseLib::Database->GetSystemMonthlyRewards(current_month, &monthlyRewardsData))
            {
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "exist a monthly reward for current month!");
                PlayerMonthlyReward playerMonthlyRewardData;
                if (BaseLib::Database->GetPlayerMonthlyDayCount(frontAccount.Index, &playerMonthlyRewardData))
                {
                    BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "exist player info for monthly reward tracking");
                    auto last_sign_in = playerMonthlyRewardData.last_time_update;
                    auto last_sign_in_date = Utility::ConvertUtcTimestampToDate(last_sign_in);
                    auto current_date = Utility::ConvertUtcTimestampToDate(Utility::GetUtcTimeNow64());
                    BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "player LastSignIn: ({}), LastSignInDate: ({}-{}-{}), CurrentDate: ({}-{}-{})", last_sign_in, last_sign_in_date.tm_year, last_sign_in_date.tm_mon, last_sign_in_date.tm_mday, current_date.tm_year, current_date.tm_mon, current_date.tm_mday);
                    if (last_sign_in_date.tm_year != current_date.tm_year ||
                        last_sign_in_date.tm_mon != current_date.tm_mon ||
                        last_sign_in_date.tm_mday != current_date.tm_mday)
                    {
                        if (playerMonthlyRewardData.day_count < 31)
                        {
                            if (is_inventory_full && is_gift_box_full)
                            {
                                send_msg(callback.session, 66, 0, 7, 0);
                            }
                            else
                            {
                                playerMonthlyRewardData.day_count++;
                                if (BaseLib::Database->UpdatePlayerMonthlyDayCount(frontAccount.Index, playerMonthlyRewardData.day_count, Utility::GetUtcTimeNow64()))
                                {
                                    if (playerMonthlyRewardData.day_count >= 1)
                                    {
                                        auto current_signin = static_cast<uint32_t>(playerMonthlyRewardData.day_count - 1);// - 1 cuz rewards starts at 0
                                        auto current_day_reward = monthlyRewardsData.rewards[current_signin];
                                        auto acc_cache = main_server->GetAccCacheUniqueBySessionId(session_id);
                                        if (!is_inventory_full)
                                            main_server->SendInventoryItem(session, acc_cache, { current_day_reward });
                                        else
                                            main_server->SendGiftItem(session, acc_cache, current_day_reward, "You've received your monthly reward here due to inventory being full.");

                                    }
                                }
                            }
                           
                        }
                        auto monthly_reward_ack = MainMonthlyRewardAck(current_month, playerMonthlyRewardData.day_count, monthlyRewardsData.rewards).Serialize();
                        send_msg(callback.session, 172, 0, 28, 1, reinterpret_cast<uint8_t*>(monthly_reward_ack.data()), monthly_reward_ack.size());
                    }
                    else
                    {
                        auto monthly_reward_ack = MainMonthlyRewardAck(current_month, playerMonthlyRewardData.day_count, monthlyRewardsData.rewards).Serialize();
                        send_msg(callback.session, 172, 0, 28, 0, reinterpret_cast<uint8_t*>(monthly_reward_ack.data()), monthly_reward_ack.size());
                    }  
                }
                else
                {
                    auto current_day = 0;
                    if (!is_gift_box_full || !is_inventory_full)
                    {
                        current_day = 1;
                        BaseLib::Database->InsertPlayerMonthlyDayCount(frontAccount.Index, current_day, Utility::GetUtcTimeNow64());
                        auto current_day_reward = monthlyRewardsData.rewards[0];
                        auto acc_cache = main_server->GetAccCacheUniqueBySessionId(session_id);
                        if (!is_inventory_full)
                            main_server->SendInventoryItem(session, acc_cache, { current_day_reward });
                        else
                            main_server->SendGiftItem(session, acc_cache, current_day_reward, "You've received your monthly reward here due to inventory being full.");
                    }

                    auto monthly_reward_ack = MainMonthlyRewardAck(current_month, current_day, monthlyRewardsData.rewards).Serialize();
                    send_msg(callback.session, 172, 0, 28, current_day, reinterpret_cast<uint8_t*>(monthly_reward_ack.data()), monthly_reward_ack.size());
                }
            }
            else
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "session id: ({}) failed to get monthly rewards info for month ({})", session->GetSessionId(), current_month);

            BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "session id: ({}) has guide missions ({})", session->GetSessionId(), frontAccount.GuideMission);

            struct guide_daily_mission
            {
                uint32_t mission_id = 46;
                uint32_t unk = 0;
                uint32_t goal = 0;
                uint32_t type = 1;//1 guide mission, 4 daily mission

                guide_daily_mission(uint32_t id, uint32_t goal, uint32_t type)
                {
                    this->mission_id = id;
                    this->goal = goal;
                    this->type = type;
                };
            };
            auto guideMissionData = guide_daily_mission(46, 0, 1);
            guideMissionData.mission_id = frontAccount.GuideMission + 46;

            send_msg(session, 167, 0, 1, 1, reinterpret_cast<uint8_t*>(&guideMissionData), sizeof(guideMissionData));


            BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "now check daily mission data of player");
            PlayerDailyMission playerDailyMissionData;
            playerDailyMissionData.update_time = 0;
            bool daily_mission_existed = BaseLib::Database->GetPlayerDailyMission(frontAccount.Index, &playerDailyMissionData);
            uint64_t now_time = Utility::GetUtcTimeNow64();
            uint64_t last_6am = Utility::GetLast6AMUtc();
            BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "player last update daily mission: ({}) and last 6am was: ({}) and now is time: ({})", playerDailyMissionData.update_time, last_6am, now_time);
            if (playerDailyMissionData.update_time < last_6am)
            {
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "player daily mission was outdated and now will update new");
                auto new_ids = main_server->GetRandomDailyMissionIds(3,0,0,0);
                playerDailyMissionData.mission1 = new_ids[0];
                playerDailyMissionData.goal_mission1 = 0;
                playerDailyMissionData.mission2 = new_ids[1];
                playerDailyMissionData.goal_mission2 = 0;
                playerDailyMissionData.mission3 = new_ids[2];
                playerDailyMissionData.goal_mission3 = 0;
                playerDailyMissionData.update_time = now_time;
                if (daily_mission_existed)
                    BaseLib::Database->UpdatePlayerDailyMission(frontAccount.Index, playerDailyMissionData);
                else
                    BaseLib::Database->InsertPlayerDailyMission(frontAccount.Index, playerDailyMissionData);
            }
            std::vector<guide_daily_mission> dailyMissions;
            dailyMissions.push_back(guide_daily_mission(playerDailyMissionData.mission1, playerDailyMissionData.goal_mission1, 4));
            dailyMissions.push_back(guide_daily_mission(playerDailyMissionData.mission2, playerDailyMissionData.goal_mission2, 4));
            dailyMissions.push_back(guide_daily_mission(playerDailyMissionData.mission3, playerDailyMissionData.goal_mission3, 4));

            send_msg(session, 167, 0, 1, dailyMissions.size(), reinterpret_cast<uint8_t*>(dailyMissions.data()), sizeof(guide_daily_mission)* dailyMissions.size());

            auto acc_cache = main_server->GetAccCacheUniqueBySessionId(session_id);
            acc_cache->daily_mission_info = playerDailyMissionData;
            acc_cache.unlock();

            send_msg(session, 413, 0, 59, 0, reinterpret_cast<uint8_t*>(&frontAccount.VoiceType), sizeof(frontAccount.VoiceType)); // final account info

            /*
            struct daily_mission
            {
                uint32_t id;
                uint32_t goal;
                uint32_t reward;

                daily_mission(uint32_t id, uint32_t goal, uint32_t reward)
                {
                    this->id = id;
                    this->goal = goal;
                    this->reward = reward;
                };
            };
            std::vector<daily_mission> dailyMissions;
            dailyMissions.push_back(daily_mission(30000, 5, 4060071));
            dailyMissions.push_back(daily_mission(30001, 1, 4060071));
            dailyMissions.push_back(daily_mission(30002, 30, 4060071));

            send_msg(session, 93, 0, 0, 3, reinterpret_cast<uint8_t*>(dailyMissions.data()), sizeof(daily_mission) * 3);

        }
        */
    }
}