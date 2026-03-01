#pragma once
namespace Game::Handlers
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Main;
    inline void PartyClanOtherJoin(SCallbackData& callback, CMainServer* main_server)
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

        struct info {
            uint16_t partyId;
            uint16_t channelId;
            char password[16];
        };
        auto req_info = reinterpret_cast<info*>(message->GetData());

        if (!CParty.contains(acc_cache->party_id)) {
            DEBUGLOG(dark_cyan, "could not find player's party id ({})", acc_cache->party_id);
            return;
        }
        auto new_password = Utility::ReadMicrovoltsString(req_info->password, sizeof(req_info->password));

        DEBUGLOG(dark_cyan, "player want to create a battle with party id ({})", req_info->partyId);

        auto self_party_cache = CParty.get<unique_t>(acc_cache->party_id);
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
        if (!CParty.contains(req_info->partyId)) {
            DEBUGLOG(dark_cyan, "could not find desired target party id ({})", req_info->partyId);
            return;
        }
        auto target_party_cache = CParty.get<unique_t>(req_info->partyId);
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

        DEBUGLOG(dark_cyan, "all data gather done and will check conditions");

        if (target_password.size() && new_password != target_password) {
            DEBUGLOG(dark_cyan, "fail join clan battle, wrong password: ({}), target password: ({})", new_password.c_str(), target_password.c_str());
            session->SendMsg(121, 0, 1, 0);
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
            uint32_t score_limit = 5;
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
                DEBUGLOG(dark_cyan, "unknown clan mod id: ({})", target_mod);
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
            callback.session = main_server->GetSessionById(target_host_id).get();
            auto msg = CMessage();
            msg.SetSession(callback.session->GetSessionId());
            msg.SetData(reinterpret_cast<uint8_t*>(&clan_room_req), sizeof(clan_room_req));
            msg.SetExtra(score_limit);

            callback.message = &msg;
            lock.unlock();
            RoomCreate(callback, main_server);

            auto target_host_acc_cache = CAccount.get<unique_t>(target_host_id);
            auto new_clan_room_id = target_host_acc_cache->room_id;
            target_host_acc_cache.unlock();

            if (self_is_clan)
            {
                auto target_room_cache = CRoom.get<unique_t>(new_clan_room_id);
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
                        if (auto player_session = server->GetSessionById(party_member_session_id))
                        {
                            SCallbackData callback;
                            callback.server = main_server;
                            callback.session = player_session.get();
                            auto msg = CMessage();
                            msg.SetSession(callback.session->GetSessionId());
                            //msg.SetCommand(callback.message->GetOrder(), callback.message->GetMission(), callback.message->GetExtra(), callback.message->GetOption());
                            msg.SetData(reinterpret_cast<uint8_t*>(&clan_room_join_req), sizeof(clan_room_join_req));
                            callback.message = &msg;
                            DEBUGLOG(dark_cyan, "now call join room");
                            RoomJoin(callback, main_server);
                            DEBUGLOG(dark_cyan, "now done join room");
                        }
                    }
                };

            join_clan_room(self_players);
            join_clan_room(target_players);

            auto host_cache = CAccount.get<unique_t>(target_host_id);
            auto room_cache = CRoom.get<unique_t>(host_cache->room_id);

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
            uint8_t high_room_id_part = (room_cache->room_id >> 8) & 0xFF; // Extract the high 8 bits
            uint8_t low_room_id_part = room_cache->room_id & 0xFF;
            callback.session->SendMsg(139, room_cache->has_password, low_room_id_part, high_room_id_part, reinterpret_cast<uint8_t*>(settings_data.data()), settings_data.size());

            room_cache.unlock();
            host_cache.unlock();
        }
        else {
            DEBUGLOG(dark_cyan, "conditions failed");
        }
    }
}