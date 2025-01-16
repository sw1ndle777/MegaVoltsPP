#pragma once
namespace Game
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Main;

    namespace Handlers
    {
        inline void PlayerNameChange(SCallbackData& callback, CMainServer* main_server)
        {
            /*
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
            auto acc_cache = main_server->GetAccCache(session_id);
            const auto& auth_key = acc_cache->acc_info.AuthKey;
            auto acc_index = acc_cache->acc_info.Index;
            if (acc_index == -1) return;

            const auto& nicknameCreationReq = reinterpret_cast<MainNicknameCreationReq*>(callback.message->GetData());
            const std::string nickname = Utility::ToLowercase(Utility::ReadMVString({ nicknameCreationReq->Nickname, sizeof(nicknameCreationReq->Nickname) }));

            if (!acc_cache->acc_info.Nickname.empty())
            {
                send_msg(session, 69, 0, NicknameCreationInfo::Result::NoPermission, 0);
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "session id: ({}) with auth key: ({}) attempted namechange exploit", session->GetSessionId(), auth_key);
                return;
            }
            if (nickname.empty())
            {
                send_msg(session, 69, 0, NicknameCreationInfo::Result::CreationFailed, 0);
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "session id: ({}) with auth key: ({}) attempted namechange, but nickname input is empty", session->GetSessionId(), auth_key);
                return;
            }
            if (nickname.length() < 2)
            {
                send_msg(session, 69, 0, NicknameCreationInfo::Result::ShortName, 0);
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "session id: ({}) with auth key: ({}) attempted namechange, but nickname is too short", session->GetSessionId(), auth_key);
                return;
            }
            else if (nickname.length() > 16)
            {
                send_msg(session, 69, 0, NicknameCreationInfo::Result::CreationFailed, 0);
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "session id: ({}) with auth key: ({}) attempted namechange, but nickname is too long", session->GetSessionId(), auth_key);
                return;
            }
            if (!Utility::IsValidNickname(nickname.c_str()))
            {
                send_msg(session, 69, 0, NicknameCreationInfo::Result::CreationFailed, 0);
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "session id: ({}) with auth key: ({}) attempted to use blacklisted nickname", session->GetSessionId(), auth_key);
                return;
            }
            BaseLib::ThreadPool->post([nickname, session, server, auth_key, main_server, send_msg]()
            {
                if (BaseLib::Database->NicknameExists(nickname.c_str()))
                {
                    send_msg(session, 69, 0, NicknameCreationInfo::Result::AlreadyInUse, 0);
                    BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "session id: ({}) with auth key: ({}) attempted to use a nickname that is already used.", session->GetSessionId(), auth_key);
                    return;
                }

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

                auto itemsFound = BaseLib::Database->GetInventoryItems(frontAccount.Index, acc_items);
                std::unordered_map<std::uint8_t, std::vector<InventoryItemInfo>> player_equipped_items;
                std::vector<Item> player_inventory_items;
                const auto& server_time = Utility::GetUtcTimeNowInMilliseconds() - server->GetStartTime();
                const auto& newPlayer = Player({ session->GetSessionId(), Utility::GetUtcTimeNowInMilliseconds() - server->GetStartTime(), frontAccount, acc_items });
                main_server->TransformItems(acc_items, player_inventory_items);
                main_server->TransformEquippedItems(acc_items, player_equipped_items);
                main_server->AddAccCache(session->GetSessionId(), newPlayer);
                MainAccountInfoAck accInfoMsg = MainAccountInfoAck();
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
                accInfoMsg.PlayTime = frontAccount.PlayTime;
                accInfoMsg.ClanId = frontAccount.ClanId;
                accInfoMsg.ClanPadding = 0;
                accInfoMsg.ZombieKillPoints = frontAccount.ZombieKills * 3;
                accInfoMsg.Infections = frontAccount.Infections;
                accInfoMsg.Unknown3 = 0;
                accInfoMsg.ServerTime = server_time;
                accInfoMsg.UniqueId = NetEngine::Packets::Core::UniqueId(session->GetSessionId(), 1).data;
                accInfoMsg.Grade = frontAccount.Grade;
                accInfoMsg.SelectedCharacter = frontAccount.SelectedCharacter;
                accInfoMsg.OwnedCharacters = 511;//all chars
                accInfoMsg.Level = frontAccount.Level + 1;
            #if defined(RELEASE_1_0_3)
                accInfoMsg.Energy = 50;//frontAccount.Energy;
                accInfoMsg.Energy2 = frontAccount.Energy;
                accInfoMsg.GoldenMode = 3;
            #else
                accInfoMsg.Coins = frontAccount.Coins;
                accInfoMsg.Energy = frontAccount.Energy;
            #endif
                //accInfoMsg.Coins = frontAccount.Coins;
                //accInfoMsg.Energy = frontAccount.Energy;
                accInfoMsg.LuckyPoints = frontAccount.LuckyPoints;
                accInfoMsg.Experience = frontAccount.Experience;
                accInfoMsg.MicroPoints = frontAccount.MicroPoints;
                accInfoMsg.RockTokens = frontAccount.RockTokens;
                accInfoMsg.Tutorial = 1;//frontAccount.Tutorial;
                accInfoMsg.MaximumItems = frontAccount.MaximumItems;
                accInfoMsg.MaximumEnergy = frontAccount.MaximumEnergy;
                accInfoMsg.DailyAttempts = frontAccount.SingleWaveDailyAttempts;
                accInfoMsg.HighestWave = frontAccount.SingleWaveHighestWave;
                accInfoMsg.SinglewaveHighscore = frontAccount.SingleWaveHighScore;
                accInfoMsg.Unknown4 = 9999;
                accInfoMsg.Story = frontAccount.Story;
            #if defined(RELEASE_1_1_1)
                accInfoMsg.VIPLevel = frontAccount.VIPExperience;
            #endif
                accInfoMsg.AccountAuthkey = auth_key;
                accInfoMsg.AccountId = frontAccount.Index;
                accInfoMsg.ClanLogoFront = 0;
                accInfoMsg.ClanLogoBack = 0;
                accInfoMsg.ClanContribution = 0;
                accInfoMsg.ClanWins = 0;
                accInfoMsg.ClanLoses = 0;
                accInfoMsg.ClanDraws = 0;
                accInfoMsg.ClanKills = 0;
                accInfoMsg.ClanDeaths = 0;
                accInfoMsg.ClanAssists = 0;
                std::strcpy(accInfoMsg.Unused, "");
                std::strcpy(accInfoMsg.Nickname, nickname.c_str());
                std::strcpy(accInfoMsg.ClanName, "");
                send_msg(session, 413, 0, 1, 0, reinterpret_cast<uint8_t*>(&accInfoMsg), sizeof(MainAccountInfoAck));
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "session id: ({}) auth key: ({}) received account informations", session->GetSessionId(), auth_key);

                if (player_inventory_items.empty() && newPlayer.acc_info.Coupons == 0)
                    send_msg(session, 77, 0, 6, 0); // empty inventory
                if (newPlayer.acc_info.Coupons > 0)
                {
                    const std::uint32_t& coupons = (newPlayer.acc_info.Coupons << 23) + 0xF4240;
                    Item coupons_item;
                    coupons_item.is_equipped = 0;
                    coupons_item.stock = 0;
                    coupons_item.character_id = -1;
                    coupons_item.item_info = InventoryItemInfo();
                    coupons_item.item_info.item_number.item_id = coupons;
                    coupons_item.item_info.expire_date = Utility::GetUnixEpoch();
                    coupons_item.item_info.serial_info = ItemSerialInfo(0, 0, 0, 0, Utility::GetUnixEpoch());
                    player_inventory_items.insert(player_inventory_items.begin(), coupons_item);
                }
                const std::uint32_t& total_inventory_fragments = (player_inventory_items.size() + 1) <= 35 ? 1 : ((player_inventory_items.size() + 1) / 35) + 1;
                for (std::uint32_t i = 0; i < total_inventory_fragments; i++)
                {
                    auto items_batch = main_server->GetTransformStockItems(player_inventory_items, i, 35);
                    send_msg(session, 77, 0, (i == 0) ? 37 : 0, items_batch.size(), reinterpret_cast<uint8_t*>(items_batch.data()), items_batch.size() * sizeof(InventoryItemInfo));
                }
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "session id: ({}) received ({}) inventory items", session->GetSessionId(), player_inventory_items.size() - 1);

                for (std::uint8_t i = 0; i < 5; i++)
                {
                    std::vector<EquipItemInfo> equipped_items;
                    const auto& current_char_items = main_server->GetTransformEquippedItems(player_equipped_items[i]);
                    for (const auto& item : current_char_items)
                    {
                        if (main_server->IsItemWeapon(item.item_number.item_id) || main_server->IsItemCostume(item.item_number.item_id) || main_server->IsItemDiorama(item.item_number.item_id))
                            equipped_items.push_back(EquipItemInfo(item));
                        else if (main_server->IsItemSet(item.item_number.item_id))
                        {
                            const auto& set_pieces_count = main_server->GetSetPiecesCount(item.item_number.item_id);
                            equipped_items.insert(equipped_items.end(), (set_pieces_count == 0) ? 1 : set_pieces_count, EquipItemInfo(item));
                        }
                    }
                    send_msg(session, 75, 0, i, equipped_items.size(), reinterpret_cast<uint8_t*>(equipped_items.data()), equipped_items.size() * sizeof(EquipItemInfo));
                    BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "session id: ({}) received ({}) equip items on ({})", session->GetSessionId(), equipped_items.size(), main_server->GetCharacterStr(i).c_str());
                }

                send_msg(session, 75, 0, 16, 0); // final equip info
                send_msg(session, 413, 0, 59, 0); // final account info

                std::unordered_map<std::uint32_t, std::uint32_t> accountToSessionMap;
                std::shared_lock lock(main_server->GetAccountsCacheMutex());
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
                    friends_cache[session->GetSessionId()] = acc_friends;
                }
                if (BaseLib::Database->GetPlayerBlockeds(frontAccount.Index, acc_blockeds))
                {
                    for (auto& blockedInfo : acc_blockeds)
                    {
                        auto it = accountToSessionMap.find(blockedInfo.blocked_account_id);
                        if (it != accountToSessionMap.end())
                            blockedInfo.blocked_session_id = it->second;
                    }
                    blockeds_cache[session->GetSessionId()] = acc_blockeds;
                }
                send_msg(session, 61, 0, Userlist::Friends::AddResult::SendPending, friends_pending.size(), reinterpret_cast<uint8_t*>(friends_pending.data()), friends_pending.size() * sizeof(PlayerFriendInfo));
                std::uint32_t my_acc_index = frontAccount.Index;
                for (const auto& friend_info : friends_accepted)
                {
                    if (!friend_info.friend_session_id) continue;
                    send_msg(server->GetSessionById(friend_info.friend_session_id).get(), 85, 0, Userlist::Friends::DetailsType::FriendState, Userlist::FriendsState::Login, reinterpret_cast<uint8_t*>(&my_acc_index), sizeof(my_acc_index));
                }
                BaseLib::Database->DeletePlayerFriends(friends_pending_db);

                BaseLib::Database->UpdateNickname(nickname.c_str(), auth_key);
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "session id: ({}) with auth key: ({}) updated nickname from ({}) to ({})", session->GetSessionId(), auth_key, acc_cache.acc_info.Nickname.c_str(), nickname.c_str());
            });

            */
        }
    }
    
}