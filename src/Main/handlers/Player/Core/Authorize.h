#pragma once
namespace Game::Handlers
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Main;

    struct guide_daily_mission
    {
        uint32_t mission_id = 46;
        uint32_t set_index = 0;
        uint32_t goal = 0;
        uint32_t type = 1;//1 guide mission, 4 daily mission

        guide_daily_mission(uint32_t id, uint32_t goal, uint32_t type)
        {
            this->mission_id = id;
            this->goal = goal;
            this->type = type;
        };
    };

    inline void Authorize(SCallbackData& callback, CMainServer* main_server)
    {
        auto session = callback.session;
        auto message = callback.message;
        if (!session || !message) return;

        //std::shared_lock lock(session->GetMutex());
        auto sid = session->GetSessionId();
        const auto& req = reinterpret_cast<MainVersionCheckReq*>(message->GetData());
        auto auth_key = req->authKey;
        auto server_id = req->serverId;
        DEBUGLOG(dark_cyan, "sid=({}) connected on server id ({}) with auth key ({})", sid, server_id, auth_key);
        [[maybe_unused]] auto ignored_result = BaseLib::DbPool->submit_task([
            main_server,
            session = std::move(callback.session),
            auth_key = auth_key,
            server_id = server_id,
            sid = sid
        ]() mutable
            {
                if (!session) return;

                BaseLib::FrontAccount accInfo;
                BaseLib::ClanInfo clanInfo{};
                PlayerDailyMission playerDailyMissionData{};
                std::vector<Item> acc_items;
                std::vector<BlockedInfo> acc_blockeds;
                std::vector<FriendInfo> acc_friends;
                std::vector<SocialInfo> acc_socials;
                std::vector<BaseLib::MailboxInfo> mailbox_list;

                auto daily_mission_ids_random = main_server->GetRandomDailyMissionIds(3, 0, 0, 0);
                if (!BaseLib::Database->GetMainFrontAccount(auth_key, server_id, &accInfo, &clanInfo, &playerDailyMissionData, acc_items, acc_socials, acc_blockeds, acc_friends, mailbox_list, daily_mission_ids_random))
                {
                    DEBUGLOG(dark_cyan,
                        "sid=({}) with auth key: ({}) doesn't exist in database",
                        session->GetSessionId(), auth_key);
                    main_server->DisconnectPlayer(sid, Disconnect::Reason::Busy);
                    return;
                }
                accInfo.ServerId = server_id;

                boost::unordered_flat_map<uint8_t, std::vector<InventoryItemInfo>> player_equipped_items;
                std::vector<Item> player_inventory_items;
                auto server_time = Utility::GetUtcTimeNowInMilliseconds() - main_server->GetStartTime();
                auto newPlayer = Player({ session->GetSessionId(), server_time, accInfo, acc_items });
                newPlayer.server_id = server_id;
                newPlayer.uid = NetEngine::Packets::Core::UniqueId(sid, server_id);
                main_server->TransformItems(acc_items, player_inventory_items);//check here
                main_server->TransformEquippedItems(acc_items, player_equipped_items);
				CAccount.insert(session->GetSessionId(), newPlayer);
				CAidSid.insert(accInfo.Index, session->GetSessionId());
				CSid.emplace_back(session->GetSessionId());
				CAuthKey.insert(auth_key, session->GetSessionId());
                
                auto acc = CAccount.get<unique_t>(sid);
                auto clan_id = acc->acc_info.ClanId;
                if (clan_id && !CClan.contains(clan_id))
                {
                    Clan newClan;
                    newClan.clan_id = clan_id;
                    newClan.logo_front = clanInfo.logo_front;
                    newClan.logo_back = clanInfo.logo_back;
                    newClan.clan_name = clanInfo.name;
                    newClan.online_members.push_back(sid);
					CClan.insert(clan_id, newClan);
                }
                acc->daily_mission_info = playerDailyMissionData;
                auto accInfoMsg = main_server->CraftAccInfoAck(acc, server_id, clanInfo.name.c_str(), clanInfo.logo_front, clanInfo.logo_back);
                acc.unlock();
                //option is chat channel id
                session->SendMsg(413, 0, 1, 1, reinterpret_cast<uint8_t*>(&accInfoMsg), sizeof(MainAccountInfoAck));
                DEBUGLOG(dark_cyan, "sid=({}) auth key: ({}) received account informations", session->GetSessionId(), auth_key);

                if (player_inventory_items.empty() && newPlayer.acc_info.Coupons == 0)
                    session->SendMsg(77, 0, 6, 0); // empty inventory
                if (newPlayer.acc_info.Coupons > 0)
                {

                    InventoryItemInfo coupon_info = { {1000000 , newPlayer.acc_info.Coupons } , Utility::GetUnixEpoch() , ItemSerialInfo(0, 0, 0, 0, Utility::GetUnixEpoch()), 0, 0 };
                    Item coupons_item = { coupon_info ,newPlayer.acc_info.Coupons, 0, 0 };
                    player_inventory_items.insert(player_inventory_items.begin(), coupons_item);
                }
                constexpr std::uint32_t max_packet_size = 1440;
                constexpr std::uint32_t full_header_size = 8;
                constexpr std::uint32_t split_size = (max_packet_size - full_header_size) / sizeof(InventoryItemInfo);
                uint32_t total_inventory_fragments = (player_inventory_items.size() + 1) <= split_size ? 1 : ((player_inventory_items.size() + 1) / split_size) + 1;
                if (!player_inventory_items.empty())
                {
                    for (auto i = 0; i < total_inventory_fragments; i++)
                    {
                        auto items_batch = main_server->GetTransformStockItems(player_inventory_items, i, split_size);
                        if (!items_batch.empty())
                            session->SendMsg(77, 0, (i == 0) ? 37 : 0, items_batch.size(), reinterpret_cast<uint8_t*>(items_batch.data()), items_batch.size() * sizeof(InventoryItemInfo));
                    }
                }
                DEBUGLOG(dark_cyan, "sid=({}) received ({}) inventory items", session->GetSessionId(), player_inventory_items.size());

#if defined(RELEASE_1_0_3)
                constexpr uint8_t max_characters = 5;
#else
                constexpr uint8_t max_characters = 16;
#endif
                for (uint8_t i = 0; i < max_characters; i++)
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
                    DEBUGLOG(dark_cyan, "sid=({}) received ({}) equip items on ({})", session->GetSessionId(), equipped_items.size(), main_server->GetCharacterStr(i).c_str());
                    session->SendMsg(75, 0, i, equipped_items.size(), reinterpret_cast<uint8_t*>(equipped_items.data()), equipped_items.size() * sizeof(EquipItemInfo));
                }
                session->SendMsg(75, 0, max_characters, 0); // final equip info

                //wake up client due to plaza client having player select bug if client is opened before server up time tick
                auto timer = std::make_shared<asio::steady_timer>(main_server->GetIoContext(), std::chrono::milliseconds(100));
                timer->async_wait([timer, main_server, sid, session](const asio::error_code& ec)
                    {
                        session->SendMsg(0, 0, 0, 0);
                        main_server->SendCastIpc(PacketIds::Ipc::MainToCastSendPingAssure, Utility::ToVector(sid));
                    });


                main_server->SendServerMessage(session, std::format("[MegaVolts Online] Welcome, {}", accInfoMsg.Nickname).c_str());
                main_server->SendServerMessage(session, std::format("[MegaVolts Online] Server's uptime {}", Utility::FormatMilliseconds(server_time).c_str()).c_str());


                using enum Socials::State;
                std::vector<PlayerFriendInfo> friends_pending;
                for (auto& socials : acc_socials)
                {
                    auto target_sid = *CAidSid.get<shared_t>(socials.targetAid);//main_server->GetSidByAid(socials.targetAid);
                    if (socials.State == Pending)
                    {
                        auto target_acc = CAccount.get<shared_t>(target_sid);
                        auto target_uid = NetEngine::Packets::Core::UniqueId(0);
                        if (target_acc->acc_info.Index)
                            target_uid = target_acc->uid;

                        target_acc.unlock();

                        friends_pending.push_back({ target_uid.data , socials.targetAid, socials.TargetNickname.c_str() });
                    }
                    if (socials.State != Accepted) continue;
                    if (auto pss = main_server->GetSessionById(target_sid))
                    {
                        DEBUGLOG(dark_cyan, "for session id {} ({}) should notify friend ({})", session->GetSessionId(), accInfoMsg.Nickname, socials.TargetNickname);
                        pss->SendMsg(85, 0, Userlist::Friends::DetailsType::FriendState, Userlist::FriendsState::Login, reinterpret_cast<uint8_t*>(&accInfo.Index), sizeof(accInfo.Index));
                    }
                }
                session->SendMsg(61, 0, Userlist::Friends::AddResult::SendPending, friends_pending.size(), reinterpret_cast<uint8_t*>(friends_pending.data()), friends_pending.size() * sizeof(PlayerFriendInfo));
                //main_server->AddPlayerSocials(session->GetSessionId(), acc_socials);
				CSocial.insert(session->GetSessionId(), acc_socials);
                uint32_t unopened_gifts = 0, unopened_mails = 0;
                for (const auto& mail : mailbox_list)
                {
                    DEBUGLOG(dark_cyan, "mail id ({})", mail.mail_id);
					CMailboxData.insert(mail.mail_id, MailboxData(mail));
                    //main_server->AddMailboxDataCache(mail.mail_id, MailboxData(mail)); // it has internal check if mail id already exists
                    if (mail.gift_itemid == 0)
                    {
                        if (!mail.deleted_from_sender)
							CMailSent.insert(mail.sender_account_id, mail.mail_id);
                        if (!mail.deleted_from_receiver)
                        {
							CMailRecv.insert(mail.receiver_account_id, mail.mail_id);
                            if (mail.receiver_account_id == accInfo.Index && mail.is_new)
                                unopened_mails++;
                        }
                    }
                    else
                    {
                        if (!mail.deleted_from_sender)
							CGiftSent.insert(mail.sender_account_id, mail.mail_id);
                        if (!mail.deleted_from_receiver)
                        {
							CGiftRecv.insert(mail.receiver_account_id, mail.mail_id);
                            if (mail.receiver_account_id == accInfo.Index)
                                unopened_gifts++; // count gifts even if read to remind players
                        }

                    }
                }

                session->SendMsg(105, 0, 37, unopened_mails); // remainder of unopened mails
                session->SendMsg(66, 0, 37, unopened_gifts); // remainder of unopened gifts

                auto guideMissionData = guide_daily_mission(46, 0, 1);
                DEBUGLOG(dark_cyan, "current progress of guide mission is ({})", accInfo.GuideMission);
                guideMissionData.mission_id = accInfo.GuideMission + 46;
                std::vector<guide_daily_mission> dailyMissions;
                dailyMissions.push_back(guideMissionData);
                dailyMissions.push_back(guide_daily_mission(playerDailyMissionData.mission1, playerDailyMissionData.goal_mission1, 4));
                dailyMissions.push_back(guide_daily_mission(playerDailyMissionData.mission2, playerDailyMissionData.goal_mission2, 4));
                dailyMissions.push_back(guide_daily_mission(playerDailyMissionData.mission3, playerDailyMissionData.goal_mission3, 4));

                session->SendMsg(167, 0, 1, dailyMissions.size(), reinterpret_cast<uint8_t*>(dailyMissions.data()), sizeof(guide_daily_mission) * dailyMissions.size());
                session->SendMsg(413, 0, 59, 0, reinterpret_cast<uint8_t*>(&accInfo.VoiceType), sizeof(accInfo.VoiceType)); // final account info
                DEBUGLOG(dark_cyan, "server_time ({})", server_time);
            });
    }
}