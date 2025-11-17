#pragma once
namespace Game::Handlers
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Main;
    inline void MailView(SCallbackData& callback, CMainServer* main_server)
    {
        auto session = callback.session;
        auto message = callback.message;
        if (!session) return;
        //std::shared_lock lock(session->GetMutex());
        auto session_id = session->GetSessionId();
        auto acc_cache = CAccount.get<shared_t>(session_id);
        auto acc_index = acc_cache->acc_info.Index;
        if (acc_index == -1) return;
        auto mailbox_tab = message->GetMission();
        auto is_receiver_tab = (mailbox_tab == 0);
        auto ids_ptr = is_receiver_tab ? CMailRecv.get<shared_t>(acc_index) : CMailSent.get<shared_t>(acc_index);
        DEBUGLOG(dark_cyan, "User ({}) opened mailbox tab: ({}) and has ({}) mails.", acc_cache->acc_info.Nickname.c_str(), mailbox_tab, ids_ptr->size());
        auto make_msg = [is_receiver_tab](auto& md)
            {
                MailboxMsgInfo out{ .mail_id = md->mail_id, .date = md->time, .is_opened = !md->is_new };
                const std::string& nick = is_receiver_tab ? md->sender_nickname : md->receiver_nickname;
                std::strcpy(out.nickname, nick.c_str());
                std::strcpy(out.msg, md->message.c_str());
                return out;
            };
        std::vector<MailboxMsgInfo> mailbox_msgs;
        mailbox_msgs.reserve(ids_ptr->size());
        for (const auto& mail_id : *ids_ptr)
        {
            auto data = CMailboxData.get<shared_t>(mail_id);
            if (data->gift_itemid != 0) continue;
            if (is_receiver_tab && data->sender_nickname.empty()) continue;
            if (!is_receiver_tab && data->receiver_nickname.empty()) continue;
            mailbox_msgs.push_back(make_msg(data));
        }
        if (mailbox_msgs.empty())
        {
            session->SendMsg(106, mailbox_tab, Mailbox::OpenResult::Empty, 0);
            return;
        }
        constexpr size_t kBatch = 5;
        const size_t fragments = (mailbox_msgs.size() + kBatch - 1) / kBatch;
        for (size_t i = 0; i < fragments; i++)
        {
            const size_t start = i * kBatch;
            const size_t end = std::min<size_t>(start + kBatch, mailbox_msgs.size());
            std::vector<MailboxMsgInfo> batch;
            batch.reserve(end - start);
            for (uint32_t j = start; j < end; j++) batch.push_back(mailbox_msgs[j]);
            uint8_t result = (i == 0) ? Mailbox::OpenResult::SendMails : Mailbox::OpenResult::SendMails2;
            if (fragments == 1) result = Mailbox::OpenResult::SendMails2;
            auto payload = MainMailboxAck(batch).Serialize();
            session->SendMsg(106, mailbox_tab, result, static_cast<uint8_t>(batch.size()), reinterpret_cast<uint8_t*>(payload.data()), payload.size());
        }
        session->SendMsg(106, mailbox_tab, Mailbox::OpenResult::Confirm, 0);
    }
}