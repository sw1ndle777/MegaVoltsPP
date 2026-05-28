#pragma once
namespace Game::Handlers
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Main;
    inline void PartyClanLeave(SCallbackData& callback, CMainServer* main_server)
    {
        auto session = callback.session;
        auto message = callback.message;
        if (!session || !message) return;

        std::shared_lock lock(session->GetMutex());
        CServer* server = callback.server;
        auto session_id = session->GetSessionId();
        auto acc_cache = CAccount.get<unique_t>(session_id);
        auto acc_index = acc_cache->acc_info.Index;
        auto my_unique_id = NetEngine::Packets::Core::UniqueId(session_id, 1).data;
        auto my_slot = acc_cache->slot_id;
        auto my_team_id = acc_cache->team_id;
        if (acc_index == -1) return;

        DEBUGLOG(dark_cyan, "player want to leave clan room");
        if (!acc_cache->in_party || !acc_cache->in_room) {
            DEBUGLOG(dark_cyan, "player isnt in any clan room");
        }

        auto self_party_id = acc_cache->party_id;
        auto self_party_cache = CParty.get<unique_t>(self_party_id);
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
            auto room_cache = CRoom.get<unique_t>(target_room_id);
            auto players = main_server->GetRoomSortedPlayerSessionIds(room_cache);
            for (const auto& player_id : players)
            {
                auto player_cache = CAccount.get<shared_t>(player_id);
                if (player_cache->acc_info.Index == -1 || !player_cache->in_room || player_cache->room_id != room_cache->room_id)
                {
                    player_cache.unlock();
                    continue;
                }
                else
                {
                    if (!other_party_assure_leave && player_cache->party_id != self_party_id) {
                        auto target_party_cache = CParty.get<unique_t>(player_cache->party_id);
                        DEBUGLOG(dark_cyan, "target party assured leave");
                        target_party_cache->is_registered = false;
                        target_party_cache->is_queueing = false;
                        target_party_cache.unlock();
                        other_party_assure_leave = true;
                    }
                    player_cache->in_room = false;
                    player_cache->slot_id = 0;
                    player_cache->playing = false;
                    player_cache->state = PlayerInfo::State::Waiting;
                    player_cache.unlock();

                    if (auto player_session = main_server->GetSessionById(player_id))
                    {
                        player_session->SendMsg(141, 0, NetEngine::Room::Leave::Ack::Result::ClosedByGm, 0);
                    }

                }
            }
            room_cache.unlock();
			CRoom.erase(target_room_id);
			CRoomId.erase_value(target_room_id);
            main_server->SendCastRoomRemoveSync(target_room_id);
            main_server->SetRoomIdAvailable(target_room_id);

            return;
        }
        //just throw member out of party

        RoomLeave(callback, main_server);
        PartyLeave(callback, main_server);

    }
}