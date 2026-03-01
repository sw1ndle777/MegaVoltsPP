#pragma once
#include "secure_channel.hpp"

namespace Game::Handlers
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Main;

    inline void ServerDisconnect(std::shared_ptr<CSession> session, CMainServer* main_server)
    {

        if (!session) return;

        //std::shared_lock lock(session->GetMutex());
        auto sid = session->GetSessionId();
        Game::Anticheat::g_secureChannels.remove(sid);
        Game::Anticheat::g_heartbeatManager.stopSession(sid);
        auto acc = CAccount.get<unique_t>(sid);
        auto aid = acc->acc_info.Index;
        if (aid == -1)
        {
            main_server->RemoveSession(sid);
            DEBUGLOG(dark_cyan, "disconnected sid=({})", sid);
            acc.unlock();
            return;
        }
        auto auth_key = acc->acc_info.AuthKey;
        auto multiple_accs_logged_in = acc->multiple_accs_logged_in;
        auto front_sid = acc->front_sid;

        DatabaseUpdateCtx dctx{ .sid = sid, .aid = aid };
        if (!multiple_accs_logged_in)
        {
            dctx.ops.emplace_back(AccountInfoPatch{ .server_id = 0 });
            dctx.ops.emplace_back(PlayerSessionsPatch{ .op = PlayerSessionsPatch::Op::Delete, .aid = aid, .key = auth_key });
        }
        auto validated = main_server->ValidateDatabaseUpdates(acc, dctx);
        if (!validated.has_value())
        {
            DEBUGLOG(red, "ValidateDatabaseUpdates failed for aid=({}) name=({}) error=({})", acc->acc_info.Index, acc->acc_info.Nickname.c_str(), static_cast<int>(validated.error()));
            return;
        }
        if (multiple_accs_logged_in)
            acc->multiple_accs_logged_in = false;
        acc.unlock();

        [[maybe_unused]] auto ignored = BaseLib::DbPool->submit_task([main_server,
            sid = sid,
            aid = aid,
            front_sid = front_sid,
            multiple_accs_logged_in = multiple_accs_logged_in,
            auth_key = auth_key,
            v = std::move(validated.value())
        ]() mutable
            {
                ResultDbUpdateInfo dbres;
                if (!multiple_accs_logged_in)
                    if (!BaseLib::Database->UpdateAccount(v, dbres).has_value()) {};

                auto acc = CAccount.get<unique_t>(sid);
                auto clan_id = acc->acc_info.ClanId;

                if (clan_id && CClan.contains(clan_id))
                {
                    auto clan = CClan.get<unique_t>(clan_id);
                    std::erase_if(clan->online_members, [&](auto m) { return m == sid; });
                    if (clan->online_members.empty())
                    {
                        clan.unlock();
                        CClan.erase(clan_id);
                    }
                }
                auto in_room = acc->in_room;
                auto room_id = acc->room_id;
                auto team_id = acc->team_id;

                if (in_room && CRoom.contains(room_id))
                {
                    auto room = CRoom.get<unique_t>(room_id);
                    auto left_while_vote_kicked = room->vote_kick_target_session_id == sid;
                    acc.unlock();
                    main_server->NewRemoveRoomPlayer(room, sid, team_id, (left_while_vote_kicked ? NetEngine::Room::Leave::Ack::Result::KickedByKickVote : NetEngine::Room::Leave::Ack::Result::Leave), false);
                    acc.lock();
                }
                auto in_party = acc->in_party;
                auto party_id = acc->party_id;
                auto uid = acc->uid;
                if (in_party && CParty.contains(party_id))
                {
                    DEBUGLOG(dark_cyan, "now leave party on disconnect");
                    auto party = CParty.get<unique_t>(party_id);

                    if (party->party_host_session_id == sid)
                    {
                        if (!in_room)
                        {
                            party->is_registered = false;
                            party->is_queueing = false;
                            for (const auto& id : party->members)
                                if (auto pss = main_server->GetSessionById(id))
                                    pss->SendMsg(120, 0, 45, 0);
                        }
                        uint16_t new_leader_index = 0;
                        uint16_t new_leader = 0;
                        for (const auto& member : party->members)
                        {
                            if (member != party->party_host_session_id)
                            {
                                new_leader = member;
                                break;
                            }
                            new_leader_index++;
                        }
                        for (const auto& id : party->members)
                        {
                            if (id == party->party_host_session_id) continue;
                            if (auto pss = main_server->GetSessionById(id))
                                pss->SendMsg(114, 0, 1, static_cast<uint8_t>(new_leader_index));
                        }
                        party->party_host_session_id = new_leader;
                    }

                    std::erase_if(party->members, [&](auto m) { return m == sid; });

                    for (const auto& id : party->members)
                        if (auto pss = main_server->GetSessionById(id))
                            pss->SendMsg(419, 0, 0, 0, reinterpret_cast<uint8_t*>(&uid), sizeof(uid));

                    if (party->members.empty())
                    {
                        party.unlock();
                        CParty.erase(party_id);
                        CPartyId.erase_value(party_id);
                        //main_server->RemovePartyCache(party_id);
                        main_server->SetQueuePartyIdAvailable(party_id);
                    }
                }

                auto plaza_id = acc->plaza_id;
                if (CPlaza.contains(plaza_id))
                {
                    auto plaza = CPlaza.get<unique_t>(plaza_id);
                    if (std::ranges::contains(plaza->session_ids, sid))
                    {
                        for (const auto& id : plaza->session_ids)
                        {
                            if (id == sid) continue;
                            if (auto pss = main_server->GetSessionById(id))
                                pss->SendMsg(425, 0, 0, 1, reinterpret_cast<uint8_t*>(&uid), sizeof(uid));
                        }
                        std::erase_if(plaza->session_ids, [&](auto m) { return m == sid; });
                    }
                }

                //auto friends = main_server->GetFriendsList(sid);
                auto socials = CSocial.get<shared_t>(sid);
                for (const auto& player : *socials)
                {
                    if (player.State != Socials::State::Accepted) continue;
                    auto target_sid = *CAidSid.get<shared_t>(player.targetAid);
                    if (auto pss = main_server->GetSessionById(target_sid))
                        pss->SendMsg(85, 0, Userlist::Friends::DetailsType::FriendState, Userlist::FriendsState::Logout, reinterpret_cast<uint8_t*>(&aid), sizeof(aid));
                }
                socials.unlock();
                acc.unlock();
                CAccount.erase(sid);
                CAidSid.erase(aid);
                CAuthKey.erase(auth_key);
                DEBUGLOG(dark_cyan, "removed acc cache for sid=({})", sid);
                CSocial.erase(sid);
                DEBUGLOG(dark_cyan, "removed socials cache for sid=({})", sid);
                main_server->RemoveSession(sid);
                DEBUGLOG(dark_cyan, "sid=({}) disconnected", sid);

                if (multiple_accs_logged_in)
                {
                    struct ReqDisconnectAid
                    {
                        uint32_t sid;
                        int32_t aid;
                    }req{ front_sid, aid };
                    main_server->SendFrontIpc(PacketIds::Ipc::MainToFrontAcknowledgeAidDisconnected, Utility::ToVector(req));
                    DEBUGLOG(dark_cyan, "sent acknowledge aid disconnected to front server for aid: ({})", aid);
                }
                else
                    DEBUGLOG(dark_cyan, "not sending acknowledge aid disconnected to front server for aid: ({}) because multiple_accs_logged_in is true", aid);
            });
    }
}