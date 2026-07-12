#pragma once
namespace Game::Handlers
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Main;
    inline void InviteJoin(SCallbackData& callback, CMainServer* main_server)
    {
        auto session = callback.session;
        auto message = callback.message;
        if (!session || !message) return;
        //std::shared_lock lock(session->GetMutex());

        CServer* server = callback.server;
        auto session_id = session->GetSessionId();
        auto option = message->GetOption();
        auto extra = message->GetExtra();

        auto acc_cache = CAccount.get<shared_t>(session_id);


        DEBUGLOG(dark_cyan, "[InviteJoin] option: ({}), extra: ({})", option, extra);
        switch (option)
        {
        case 1: {//JOIN
            auto joinReq = reinterpret_cast<MainPlayerBlockedRemoveReq*>(message->GetData());
            DEBUGLOG(dark_cyan, "[InviteJoin] player want to join player id ({})", joinReq->player_id);
			auto target_sid = *CAidSid.get<shared_t>(joinReq->player_id);
            auto target_acc_cache = CAccount.get<unique_t>(target_sid);
            if (target_acc_cache->in_party)
            {
                session->SendMsg(163, 0, 9, 0);//generic fail msg, you can only invite to party not join
                return;
            }
            if (target_acc_cache->in_plaza)
            {
                if (acc_cache->in_plaza && acc_cache->plaza_id == target_acc_cache->plaza_id) //in the same plaza dont do anything
                {
                    session->SendMsg(163, 0, 9, 0);
                    return;
                }
                auto current_plaza = CPlaza.get<unique_t>(target_acc_cache->plaza_id);
                if (current_plaza->session_ids.size() >= current_plaza->max_players) //plaza is full
                {
                    session->SendMsg(163, 0, 14, 0);
                    return;
                }
                auto join_confirm_ack_data = MainUserJoinConfirmAck(1, (uint16_t)target_acc_cache->plaza_id, 1).Serialize();
                session->SendMsg(163, 0, 0, 1, reinterpret_cast<uint8_t*>(join_confirm_ack_data.data()), join_confirm_ack_data.size());
                return;
            }
            if (acc_cache->in_room)
            {
                if (target_acc_cache->in_room && acc_cache->room_id == target_acc_cache->room_id)  //is in same room
                {
                    session->SendMsg(163, 0, 9, 0);//generic fail msg
                    return;
                }
                if (acc_cache->state == PlayerInfo::State::PlayerReady) //is ready or host
                {
                    session->SendMsg(163, 0, 5, 0);
                    return;
                }
            }
            if (target_acc_cache->acc_info.Index != -1)
            {
                DEBUGLOG(dark_cyan, "[InviteJoin] found target join player acc info nickname: ({})", target_acc_cache->acc_info.Nickname);
                if (!target_acc_cache->in_room)
                {
                    session->SendMsg(163, 0, 2, 0);//target is in lobby
                    return;
                }

                if (!CRoom.contains(target_acc_cache->room_id))
                {
                    session->SendMsg(163, 0, 2, 0);//target is in lobby
                    return;
                }
                //now need info for current room
                auto room_cache = CRoom.get<unique_t>(target_acc_cache->room_id);
                if (room_cache->max_players == room_cache->neutralteam_session_ids.size() || room_cache->max_players == (room_cache->blueteam_session_ids.size() + room_cache->redteam_session_ids.size())) //room is full
                {
                    session->SendMsg(163, 0, 14, 0);
                    return;
                }
                DEBUGLOG(dark_cyan, "[InviteJoin] target join player is okay to join and will receive confirmation");
                if (extra == 28) //send him cumfirmation
                {
                    auto join_confirm_ack_data = MainUserJoinConfirmAck(1, room_cache->room_id, room_cache->channel_id).Serialize();
                    session->SendMsg(163, 0, room_cache->has_password ? 44 : 0, 0, reinterpret_cast<uint8_t*>(join_confirm_ack_data.data()), join_confirm_ack_data.size());
                    return;
                }
            }
            else
            {
                DEBUGLOG(dark_cyan, "[InviteJoin] target join player cannot find cache");
                session->SendMsg(163, 0, 13, 0);//target is logged out!
            }
            break;
        }
        case 2: //INVITE
        {
            if (!acc_cache->in_room && !acc_cache->in_party)
            {
                DEBUGLOG(dark_cyan, "[InviteJoin] invite but self isnt in any room or party");
                return;
            }

            const auto& inviteReq = reinterpret_cast<MainPlayerBlockedAddReq*>(message->GetData());
            const auto& target_nickname = Utility::ReadMicrovoltsString(inviteReq->nickname, sizeof(inviteReq->nickname));
            DEBUGLOG(dark_cyan, "[InviteJoin] player want to invite player ({})", target_nickname);
            // lock-free resolve (we hold acc_cache) — avoids ABBA, see ResolveOnlineByNickname
            const auto target = CMainServer::ResolveOnlineByNickname(target_nickname);
            if (target.aid != -1)
            {
                DEBUGLOG(dark_cyan, "[InviteJoin] found target invite player acc info nickname: ({})", target.nickname);
                if (target.in_room)
                {
                    if (target.room_id != acc_cache->room_id)  //invite someone who is in another room
                    {
                        if (target.playing)
                        {
                            session->SendMsg(319, 0, 5, 0);//target is in battle!
                            return;
                        }
                    }
                    else //is already in your room
                    {
                        session->SendMsg(319, 0, 11, 0);//Invite fail
                        return;
                    }
                }
                if (acc_cache->in_party)
                {
                    auto party_cache = CParty.get<unique_t>(acc_cache->party_id);
                    if (target.in_party && target.party_id == acc_cache->party_id)//in same party already
                    {
                        session->SendMsg(319, 0, 11, 0);
                        return;
                    }
                    if (party_cache->members.size() >= party_cache->max_members)//party is full
                    {
                        session->SendMsg(319, 0, 7, 0);
                        return;
                    }
                    DEBUGLOG(dark_cyan, "[InviteJoin] party is okay to propose the player an invite");
                    MainUserInvitePartyAck inviteInfo = { 1, acc_cache->acc_info.Nickname.c_str(), acc_cache->party_id };
                    auto sender_uniqueId = NetEngine::Packets::Core::UniqueId(target.sid, 1);
                    if (auto target_session = server->GetSessionById(sender_uniqueId.session))
                    {
                        target_session->SendMsg(319, 0, 60, 0, reinterpret_cast<uint8_t*>(&inviteInfo), sizeof(inviteInfo));
                    }
                    return;
                }
                if (!CRoom.contains(acc_cache->room_id))
                {
                    session->SendMsg(319, 0, 11, 0);//Invite fail
                    return;
                }
                //now need info for current room
                auto room_cache = CRoom.get<unique_t>(acc_cache->room_id);
                if (room_cache->max_players == room_cache->neutralteam_session_ids.size() || room_cache->max_players == (room_cache->blueteam_session_ids.size() + room_cache->redteam_session_ids.size())) //room is full
                {
                    session->SendMsg(319, 0, 7, 0);
                    return;
                }
                //all good, now send him an invite
                DEBUGLOG(dark_cyan, "[InviteJoin] room is okay to propose the player an invite");
                auto sender_uniqueId = NetEngine::Packets::Core::UniqueId(target.sid, 1);
                if (auto target_session = server->GetSessionById(sender_uniqueId.session))
                {
                    auto invite_ack_data = MainUserInviteAck(1, room_cache->room_id, room_cache->channel_id, acc_cache->acc_info.Nickname.c_str(), room_cache->title.c_str(), room_cache->password.c_str()).Serialize(room_cache->has_password);
                    target_session->SendMsg(319, 0, room_cache->has_password ? 44 : 0, 0, reinterpret_cast<uint8_t*>(invite_ack_data.data()), invite_ack_data.size());
                }
            }
            else
            {
                DEBUGLOG(dark_cyan, "[InviteJoin] target invite player cannot find cache");
                session->SendMsg(319, 0, 6, 0);//target is logged out!
            }
            break;
        }
        }
    }
}