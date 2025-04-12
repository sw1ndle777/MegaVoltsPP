#pragma once
namespace Game
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Main;

    namespace Handlers
    {
        inline void PlayerDeleteMailbox(SCallbackData& callback, CMainServer* main_server)
        {
            BaseLib::DbPool->submit_task([=]() mutable
            {
                auto send_msg = [&](CSession* session, uint16_t order, uint8_t mission, uint8_t extra, uint8_t option, uint8_t* data = nullptr, uint16_t data_size = 0)
                {
                    CMessage message(session->GetEncryptionKey());
                    message.SetSession(session->GetSessionId());
                    message.SetCommand(order, mission, extra, option);
                    if (data_size > 0 && data != nullptr) message.SetData(data, data_size);
                    session->Send(message);
                };

                std::shared_lock lock(callback.session->GetMutex());
                CSession* session = callback.session;
                auto session_id = session->GetSessionId();
                auto acc_cache = main_server->GetAccCacheSharedBySessionId(session_id);

                auto acc_index = acc_cache->acc_info.Index;
                if (acc_index == -1) return;

                const auto& mailboxReq = reinterpret_cast<MailBoxUpdateReq*>(callback.message->GetData());
                std::vector<uint32_t> mail_ids_sender;
                std::vector<uint32_t> mail_ids_received;
                for (uint32_t i = 0; i < mailboxReq->mail_count; i++)
                {
                    auto mail_id = mailboxReq->mail_info[i].mail_id;
                    auto mailbox_data = main_server->GetMailboxDataCacheShared(mail_id);
                    if (mailbox_data->receiver_account_id == acc_index)
                    {
                        main_server->RemoveMailboxRecvIdCache(acc_index, mail_id);
                        mail_ids_received.push_back(mail_id);
                    }
                    else if (mailbox_data->sender_account_id == acc_index)
                    {
                        main_server->RemoveMailboxSentIdCache(acc_index, mail_id);
                        mail_ids_sender.push_back(mail_id);
                    }
                    main_server->RemoveMailboxDataCache(mail_id);
                }
                if (mail_ids_sender.size() > 0)
                    BaseLib::Database->UpdateOrDeleteMailboxForSender(mail_ids_sender);
                if (mail_ids_received.size() > 0)
                    BaseLib::Database->UpdateOrDeleteMailboxForReceiver(mail_ids_received);

                uint32_t unopened_gifts = 0, unopened_mails = 0;
                auto mail_recv_ids = main_server->GetMailboxRecvCacheShared(acc_index);
                for (uint32_t i = 0; i < mail_recv_ids->size(); i++)
                {
                    auto mail_id = mail_recv_ids->at(i);
                    auto mailbox_data = main_server->GetMailboxDataCacheShared(mail_id);
                    if (mailbox_data->is_new && mailbox_data->gift_itemid == 0) unopened_mails++;
                    else if (mailbox_data->gift_itemid != 0) unopened_gifts++;
                }


                send_msg(session, 105, 0, 37, unopened_mails); // remainder of unopened mails
                send_msg(session, 66, 0, 37, unopened_gifts); // remainder of unopened mails
            });
        }
        inline void PlayerSendMailbox(SCallbackData& callback, CMainServer* main_server)
        {
            BaseLib::DbPool->submit_task([=]() mutable
            {
                auto send_msg = [&](CSession* session, uint16_t order, uint8_t mission, uint8_t extra, uint8_t option, uint8_t* data = nullptr, uint16_t data_size = 0)
                {
                    CMessage message(session->GetEncryptionKey());
                    message.SetSession(session->GetSessionId());
                    message.SetCommand(order, mission, extra, option);
                    if (data_size > 0 && data != nullptr) message.SetData(data, data_size);
                    session->Send(message);
                };

                std::shared_lock lock(callback.session->GetMutex());
                CSession* session = callback.session;
                auto session_id = session->GetSessionId();
                auto acc_cache = main_server->GetAccCacheSharedBySessionId(session_id);
            
                auto acc_index = acc_cache->acc_info.Index;
                if (acc_index == -1) return;

                const auto& mailboxReq = reinterpret_cast<MainMailboxSendReq*>(callback.message->GetData());
                const auto& mailbox_target_name = Utility::ReadMicrovoltsString(mailboxReq->nickname, 16);
                auto msg_size = callback.message->GetDataSize() - 16;
                uint32_t target_index = 0;
                if (!BaseLib::Database->NicknameExists(mailbox_target_name.c_str(), target_index))
                {
                    send_msg(session, 104, 0, Mailbox::SendResult::UserNotFound, 0);
                    return;
                }
                auto my_blockeds = main_server->GetBlockedsList(session_id);
                if (main_server->IsBlockedAlready(my_blockeds, target_index))
                {
                    send_msg(session, 104, 0, Mailbox::SendResult::Blacklist, 0);
                    return;
                }
                auto target_acc_cache = main_server->GetAccCacheSharedByAccountId(target_index);
                auto target_mailbox_received_count = 0;
                if (target_acc_cache->acc_info.Index == -1) //user offline
                {
                    std::vector<BaseLib::BlockedInfo> target_blockeds;
                    BaseLib::Database->GetPlayerBlockeds(static_cast<int32_t>(target_index), target_blockeds);
                    if (main_server->IsBlockedAlready(target_blockeds, acc_index))
                    {
                        send_msg(session, 104, 0, Mailbox::SendResult::Blacklist, 0);
                        return;
                    }
                    target_mailbox_received_count = BaseLib::Database->GetPlayerReceiverMailboxCount(static_cast<int32_t>(target_index));
                }
                else
                {
                    auto target_blockeds = main_server->GetBlockedsList(target_acc_cache->session_id);
                    if (main_server->IsBlockedAlready(target_blockeds, acc_index))
                    {
                        send_msg(session, 104, 0, Mailbox::SendResult::Blacklist, 0);
                        return;
                    }
                    target_mailbox_received_count = static_cast<uint32_t>(main_server->GetMailboxRecvCount(target_index));
                }
                if (target_mailbox_received_count >= 100)
                {
                    send_msg(session, 104, 0, Mailbox::SendResult::FullReceiver, 0);
                    return;
                }
                auto my_sent_count = main_server->GetMailboxSentCount(acc_index);
                if (my_sent_count >= 100)
                {
                    send_msg(session, 104, 0, Mailbox::SendResult::FullSender, 0);
                    return;
                }
                uint32_t new_mailbox_id = 0;
                MailboxInfo mailbox_info = { 0, static_cast<uint32_t>(acc_index), acc_cache->acc_info.Nickname.c_str(), target_index, mailbox_target_name.c_str(), Utility::GetUtcTimeNow(), 0, Utility::ReadMicrovoltsString(mailboxReq->msg, msg_size), true, false, false };
                if (BaseLib::Database->InsertPlayerMailbox(mailbox_info, new_mailbox_id))
                {
                    mailbox_info.mail_id = new_mailbox_id;
                    main_server->AddMailboxDataCache(new_mailbox_id, MailboxData(mailbox_info));
                    main_server->AddMailboxSentIdCache(acc_index, new_mailbox_id);
                    main_server->AddMailboxRecvIdCache(target_index, new_mailbox_id);
                    if (auto player_session = main_server->GetSessionById(target_acc_cache->session_id))
                        send_msg(player_session.get(), 104, 0, Mailbox::SendResult::NewMail, 0);

                    BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "player ({}) sent mailbox to player ({})", acc_cache->acc_info.Nickname.c_str(), mailbox_target_name.c_str());
                }
            });
        }
        inline void PlayerUpdateMailbox(SCallbackData& callback, CMainServer* main_server)
        {
            auto send_msg = [&](CSession* session, uint16_t order, uint8_t mission, uint8_t extra, uint8_t option, uint8_t* data = nullptr, uint16_t data_size = 0)
            {
                CMessage message(session->GetEncryptionKey());
                message.SetSession(session->GetSessionId());
                message.SetCommand(order, mission, extra, option);
                if (data_size > 0 && data != nullptr) message.SetData(data, data_size);
                session->Send(message);
            };

            std::shared_lock lock(callback.session->GetMutex());
            CSession* session = callback.session;
            auto session_id = session->GetSessionId();
            auto acc_cache = main_server->GetAccCacheSharedBySessionId(session_id);

            auto acc_index = acc_cache->acc_info.Index;
            if (acc_index == -1) return;

            const auto& mailboxReq = reinterpret_cast<MailBoxUpdateReq*>(callback.message->GetData());
            std::vector<uint32_t> mail_ids;
            for (uint32_t i = 0; i < mailboxReq->mail_count; i++)
            {
                auto mail_id = mailboxReq->mail_info[i].mail_id;
                auto mailbox_data = main_server->GetMailboxDataCacheUnique(mail_id);
                mailbox_data->is_new = false;
                mail_ids.push_back(mail_id);
            }
            BaseLib::Database->UpdateMailboxIsNew(mail_ids, false);
        }
        inline void PlayerOpenMailbox(SCallbackData& callback, CMainServer* main_server)
        {
            BaseLib::DbPool->submit_task([=]() mutable
            {
                auto send_msg = [&](CSession* session, uint16_t order, uint8_t mission, uint8_t extra, uint8_t option, uint8_t* data = nullptr, uint16_t data_size = 0)
                {
                    CMessage message(session->GetEncryptionKey());
                    message.SetSession(session->GetSessionId());
                    message.SetCommand(order, mission, extra, option);
                    if (data_size > 0 && data != nullptr) message.SetData(data, data_size);
                    session->Send(message);
                };

                std::shared_lock lock(callback.session->GetMutex());
                CSession* session = callback.session;
                auto session_id = session->GetSessionId();
                auto acc_cache = main_server->GetAccCacheSharedBySessionId(session_id);

                auto acc_index = acc_cache->acc_info.Index;
                if (acc_index == -1) return;
                auto mailbox_tab = callback.message->GetMission();

                if (mailbox_tab == 0) // receiver
                {
                    auto received_mail_ids = main_server->GetMailboxRecvCacheShared(acc_index);
                    if (received_mail_ids->empty())
                    {
                        BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "mailbox empty");
                        send_msg(session, 106, mailbox_tab, Mailbox::OpenResult::Empty, 0);
                        return;
                    }
                    std::vector<MailboxMsgInfo> mailbox_msgs;
                    for (const auto& mail_id : *received_mail_ids)
                    {
                        auto mailbox_data = main_server->GetMailboxDataCacheShared(mail_id);
                        BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "check mail id ({})", mailbox_data->mail_id);
                        if (mailbox_data->gift_itemid != 0) continue;
                        if (!mailbox_data->sender_nickname.empty())
                        {
                            BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "add mail id ({})", mailbox_data->mail_id);
                            MailboxMsgInfo new_mailbox_msg;
                            new_mailbox_msg.mail_id = mailbox_data->mail_id;
                            new_mailbox_msg.date = mailbox_data->time;
                            new_mailbox_msg.is_opened = !mailbox_data->is_new;
                            std::strcpy(new_mailbox_msg.nickname, mailbox_data->sender_nickname.c_str());
                            std::strcpy(new_mailbox_msg.msg, mailbox_data->message.c_str());
                            mailbox_msgs.push_back(new_mailbox_msg);
                        }
                    }

                    uint32_t total_mails_fragments = (mailbox_msgs.size() + 1) <= 5 ? 1 : ((mailbox_msgs.size() + 1) / 5) + 1;
               
                    for (uint32_t i = 0; i < total_mails_fragments; i++)
                    {
                        std::vector<MailboxMsgInfo> mails_batch;
                        uint8_t mail_list_result = (i == 0) ? Mailbox::OpenResult::SendMails : Mailbox::OpenResult::SendMails2;
                        if (total_mails_fragments == 1)
                            mail_list_result = Mailbox::OpenResult::SendMails2;
                        uint32_t start_index = i * 5;
                        uint32_t end_index = std::min(start_index + 5, static_cast<uint32_t>(mailbox_msgs.size()));
                        for (auto j = start_index; j < end_index; j++)
                            mails_batch.push_back(mailbox_msgs[j]);

                        auto mailboxAckMsg = MainMailboxAck(mails_batch).Serialize();

                        send_msg(session, 106, mailbox_tab, mail_list_result, static_cast<uint8_t>(mails_batch.size()), reinterpret_cast<uint8_t*>(mailboxAckMsg.data()), mailboxAckMsg.size());
                    }
                    send_msg(session, 106, mailbox_tab, Mailbox::OpenResult::Confirm, 0);
                }
                else if (mailbox_tab == 1)// sender
                {
                    auto sent_mail_ids = main_server->GetMailboxSentCacheShared(acc_index);
                    if (sent_mail_ids->empty())
                    {
                        send_msg(session, 106, mailbox_tab, Mailbox::OpenResult::Empty, 0);
                        return;
                    }
                    std::vector<MailboxMsgInfo> mailbox_msgs;
                    for (const auto& mail_id : *sent_mail_ids)
                    {
                        auto mailbox_data = main_server->GetMailboxDataCacheShared(mail_id);
                        if (mailbox_data->gift_itemid != 0) continue;
                        if (!mailbox_data->receiver_nickname.empty())
                        {
                            MailboxMsgInfo new_mailbox_msg;
                            new_mailbox_msg.mail_id = mailbox_data->mail_id;
                            new_mailbox_msg.date = mailbox_data->time;
                            new_mailbox_msg.is_opened = !mailbox_data->is_new;
                            std::strcpy(new_mailbox_msg.nickname, mailbox_data->receiver_nickname.c_str());
                            std::strcpy(new_mailbox_msg.msg, mailbox_data->message.c_str());
                            mailbox_msgs.push_back(new_mailbox_msg);
                        }
                    }

                    uint32_t total_mails_fragments = (mailbox_msgs.size() + 1) <= 5 ? 1 : ((mailbox_msgs.size() + 1) / 5) + 1;

                    for (uint32_t i = 0; i < total_mails_fragments; i++)
                    {
                        std::vector<MailboxMsgInfo> mails_batch;
                        uint8_t mail_list_result = (i == 0) ? Mailbox::OpenResult::SendMails : Mailbox::OpenResult::SendMails2;
                        if (total_mails_fragments == 1)
                            mail_list_result = Mailbox::OpenResult::SendMails2;
                        uint32_t start_index = i * 5;
                        uint32_t end_index = std::min(start_index + 5, static_cast<uint32_t>(mailbox_msgs.size()));
                        for (auto j = start_index; j < end_index; j++)
                            mails_batch.push_back(mailbox_msgs[j]);

                        auto mailboxAckMsg = MainMailboxAck(mails_batch).Serialize();

                        send_msg(session, 106, mailbox_tab, mail_list_result, static_cast<uint8_t>(mails_batch.size()), reinterpret_cast<uint8_t*>(mailboxAckMsg.data()), mailboxAckMsg.size());
                    }
                    send_msg(session, 106, mailbox_tab, Mailbox::OpenResult::Confirm, 0);
                }
            });
        }
        inline void PlayerOpenGiftbox(SCallbackData& callback, CMainServer* main_server)
        {
            auto send_msg = [&](CSession* session, uint16_t order, uint8_t mission, uint8_t extra, uint8_t option, uint8_t* data = nullptr, uint16_t data_size = 0)
            {
                CMessage message(session->GetEncryptionKey());
                message.SetSession(session->GetSessionId());
                message.SetCommand(order, mission, extra, option);
                if (data_size > 0 && data != nullptr) message.SetData(data, data_size);
                session->Send(message);
            };

            std::shared_lock lock(callback.session->GetMutex());
            CSession* session = callback.session;
            auto session_id = session->GetSessionId();
            auto acc_cache = main_server->GetAccCacheSharedBySessionId(session_id);

            auto acc_index = acc_cache->acc_info.Index;
            if (acc_index == -1) return;
            auto mailbox_tab = callback.message->GetMission();

            BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "User ({}) open his Gift Inbox and read ({})", acc_cache->acc_info.Nickname, (mailbox_tab == 0 ? "RECEIVED" : "SENT"));

            if (mailbox_tab == 0) // receiver
            {
                auto received_mail_ids = main_server->GetGiftboxRecvCacheShared(acc_index);
                if (received_mail_ids->empty())
                {
                    BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "User Gift Inbox is empty");
                    send_msg(session, 67, mailbox_tab, Mailbox::OpenResult::Empty, 0);
                    return;
                }
                std::vector<GiftboxMsgInfo> mailbox_msgs;
                for (const auto& mail_id : *received_mail_ids)
                {
                    auto mailbox_data = main_server->GetMailboxDataCacheShared(mail_id);
                    if(mailbox_data->gift_itemid == 0) continue;
                    if (!mailbox_data->sender_nickname.empty())
                    {
                        GiftboxMsgInfo new_mailbox_msg;
                        new_mailbox_msg.mail_id = mailbox_data->mail_id;
                        new_mailbox_msg.date = mailbox_data->time;
                        new_mailbox_msg.item_id = mailbox_data->gift_itemid;
                        new_mailbox_msg.unknown1 = 1;
                        new_mailbox_msg.unknown2 = 0;
                        std::strcpy(new_mailbox_msg.nickname, mailbox_data->sender_nickname.c_str());
                        std::strcpy(new_mailbox_msg.msg, mailbox_data->message.c_str());
                        mailbox_msgs.push_back(new_mailbox_msg);
                    }
                }
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "User has ({}) gifts.", mailbox_msgs.size());
                uint32_t total_mails_fragments = (mailbox_msgs.size() + 1) <= 4 ? 1 : ((mailbox_msgs.size() + 1) / 4) + 1;

                for (uint32_t i = 0; i < total_mails_fragments; i++)
                {
                    std::vector<GiftboxMsgInfo> mails_batch;
                    uint8_t mail_list_result = (i == 0) ? Mailbox::OpenResult::SendMails : Mailbox::OpenResult::SendMails2;
                    if (total_mails_fragments == 1)
                        mail_list_result = Mailbox::OpenResult::SendMails2;
                    uint32_t start_index = i * 4;
                    uint32_t end_index = std::min(start_index + 4, static_cast<uint32_t>(mailbox_msgs.size()));
                    for (auto j = start_index; j < end_index; j++)
                        mails_batch.push_back(mailbox_msgs[j]);

                    
                    auto mailboxAckMsg = MainGiftboxAck(mails_batch).Serialize();

                    send_msg(session, 67, mailbox_tab, mail_list_result, static_cast<uint8_t>(mails_batch.size()), reinterpret_cast<uint8_t*>(mailboxAckMsg.data()), mailboxAckMsg.size());
                }
                send_msg(session, 67, mailbox_tab, Mailbox::OpenResult::Confirm, 0);
            }
            else
                send_msg(session, 67, mailbox_tab, Mailbox::OpenResult::Empty, 0);
           

        }
        inline void PlayerReceiveGiftbox(SCallbackData& callback, CMainServer* main_server)
        {
            auto send_msg = [&](CSession* session, uint16_t order, uint8_t mission, uint8_t extra, uint8_t option, uint8_t* data = nullptr, uint16_t data_size = 0)
            {
                CMessage message(session->GetEncryptionKey());
                message.SetSession(session->GetSessionId());
                message.SetCommand(order, mission, extra, option);
                if (data_size > 0 && data != nullptr) message.SetData(data, data_size);
                session->Send(message);
            };

            std::shared_lock lock(callback.session->GetMutex());
            CSession* session = callback.session;
            auto session_id = session->GetSessionId();
            auto acc_cache = main_server->GetAccCacheUniqueBySessionId(session_id);

            auto acc_index = acc_cache->acc_info.Index;
            if (acc_index == -1) return;

            const auto& mailboxReq = reinterpret_cast<MailBoxUpdateReq*>(callback.message->GetData());
            std::vector<uint32_t> mail_ids_received;
            std::vector<uint32_t> gift_item_ids;
            for (uint32_t i = 0; i < mailboxReq->mail_count; i++)
            {
                auto mail_id = mailboxReq->mail_info[i].mail_id;
                auto mailbox_data = main_server->GetMailboxDataCacheShared(mail_id);
                if (mailbox_data->gift_itemid == 0) continue;
                gift_item_ids.push_back(mailbox_data->gift_itemid);

                if (mailbox_data->receiver_account_id == acc_index)
                {
                    main_server->RemoveGiftboxRecvIdCache(acc_index, mail_id);
                    mail_ids_received.push_back(mail_id);
                    auto item_info = main_server->GetItemInfoCache(mailbox_data->gift_itemid);
                    if (item_info->Id != -1)
                    {
                        auto serial_index = main_server->FindLowestAvailableItemSerialInfoId(acc_cache->inventory_items);
                        ShopItem new_item = { {item_info->Id , item_info->Stock } , ItemExpire::Type::Unused, ItemSerialInfo(serial_index, 1, 1, Items::Origin::From_GM_Spawn, Utility::GetUtcTimeNow()) };
                        MailboxGift new_gift = { mail_id,mailbox_data->time, new_item };
                        send_msg(session, 66, 0, Mailbox::SendResult::Gift, 0, reinterpret_cast<uint8_t*>(&new_gift), sizeof(MailboxGift));
                    }
                    
                        
                }
                main_server->RemoveMailboxDataCache(mail_id);
            }
            if (mail_ids_received.size() > 0)
                BaseLib::Database->UpdateOrDeleteMailboxForReceiver(mail_ids_received);

            if (gift_item_ids.size() > 0)
                main_server->SendInventoryItem(session, acc_cache, gift_item_ids);

            


            uint32_t unopened_gifts = 0;
            auto mail_recv_ids = main_server->GetGiftboxRecvCacheShared(acc_index);
            for (uint32_t i = 0; i < mail_recv_ids->size(); i++)
            {
                auto mail_id = mail_recv_ids->at(i);
                auto mailbox_data = main_server->GetMailboxDataCacheShared(mail_id);
                if (mailbox_data->gift_itemid != 0) unopened_gifts++;
            }

            send_msg(session, 66, 0, 37, unopened_gifts); // remainder of unopened mails

        }
        inline void PlayerMailbox(SCallbackData& callback, CMainServer* main_server)
        {
            const auto& order = callback.message->GetOrder();
            switch (order)
            {
                case 66:  PlayerReceiveGiftbox(callback, main_server); break;
                case 67:  PlayerOpenGiftbox(callback, main_server); break;
                case 103: PlayerDeleteMailbox(callback, main_server); break;
                case 104: PlayerSendMailbox(callback, main_server); break;
                case 105: PlayerUpdateMailbox(callback, main_server); break;
                case 106: PlayerOpenMailbox(callback, main_server); break;
            }
        }
    } 
}