#pragma once
namespace Game::Handlers
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Main;
    inline void GiftView(SCallbackData& callback, CMainServer* main_server)
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
        auto ids_ptr = is_receiver_tab ? CGiftRecv.get<shared_t>(acc_index) : CGiftSent.get<shared_t>(acc_index);
        DEBUGLOG(dark_cyan, "User ({}) opened giftbox tab: ({}) and has ({}) gifts.", acc_cache->acc_info.Nickname.c_str(), mailbox_tab, ids_ptr->size());
        auto make_msg = [is_receiver_tab](auto& md)
            {
                GiftboxMsgInfo out{ .mail_id = md->mail_id, .date = md->time, .item_id = md->gift_itemid, .unknown1 = 1, .unknown2 = 0 };
                const std::string& nick = is_receiver_tab ? md->sender_nickname : md->receiver_nickname;
                std::strcpy(out.nickname, nick.c_str());
                std::strcpy(out.msg, md->message.c_str());
				DEBUGLOG(dark_cyan, "Made giftbox msg info for mail_id: ({}) with item_id: ({}) and nickname: ({}).", out.mail_id, out.item_id, out.nickname);
                return out;
            };
        std::vector<GiftboxMsgInfo> giftbox_msgs;
        giftbox_msgs.reserve(ids_ptr->size());
        for (const auto& mail_id : *ids_ptr)
        {
            auto data = CMailboxData.get<shared_t>(mail_id);
            if (data->gift_itemid == 0) {
                DEBUGLOG(red, "Skipping mail_id: ({}) with no gift item.", mail_id);
				continue;
            }
            if (is_receiver_tab && data->sender_nickname.empty()) {
                DEBUGLOG(red, "Skipping mail_id: ({}) in receiver tab with empty sender nickname.", mail_id);
				continue;
            }
            if (!is_receiver_tab && data->receiver_nickname.empty()) {
				DEBUGLOG(red, "Skipping mail_id: ({}) in sent tab with empty receiver nickname.", mail_id);
                continue;
            }
            giftbox_msgs.push_back(make_msg(data));
        }
        if (giftbox_msgs.empty())
        {
            session->SendMsg(67, mailbox_tab, Mailbox::OpenResult::Empty, 0);
			DEBUGLOG(dark_cyan, "No gifts to show for user ({}), sending empty response.", acc_cache->acc_info.Nickname.c_str());
            return;
        }
        constexpr size_t kBatch = 5;
        const size_t fragments = (giftbox_msgs.size() + kBatch - 1) / kBatch;
        for (size_t i = 0; i < fragments; i++)
        {
            const size_t start = i * kBatch;
            const size_t end = std::min<size_t>(start + kBatch, giftbox_msgs.size());
            std::vector<GiftboxMsgInfo> batch;
            batch.reserve(end - start);
            for (uint32_t j = start; j < end; j++) batch.push_back(giftbox_msgs[j]);
            uint8_t result = (i == 0) ? Mailbox::OpenResult::SendMails : Mailbox::OpenResult::SendMails2;
            if (fragments == 1) result = Mailbox::OpenResult::SendMails2;
            auto payload = MainGiftboxAck(batch).Serialize();
            session->SendMsg(67, mailbox_tab, result, static_cast<uint8_t>(batch.size()), reinterpret_cast<uint8_t*>(payload.data()), payload.size());
			DEBUGLOG(dark_cyan, "Sent batch ({}/{}) of giftbox messages to user ({}), batch size: ({}).", i + 1, fragments, acc_cache->acc_info.Nickname.c_str(), batch.size());
        }
        session->SendMsg(67, mailbox_tab, Mailbox::OpenResult::Confirm, 0);
    }
}