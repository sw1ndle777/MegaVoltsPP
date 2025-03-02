#pragma once
namespace Game
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Main;

    namespace Handlers
    {
        inline void CreateRoom(SCallbackData& callback, CMainServer* main_server)
        {
            BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "want to create room");
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
            auto acc_cache = main_server->GetAccCacheUniqueBySessionId(session_id);
            auto acc_index = acc_cache->acc_info.Index;

            auto score_limit = callback.message->GetExtra();
            if (acc_index == -1) return;
            const auto& createRoomReq = reinterpret_cast<MainCreateRoomReq*>(callback.message->GetData());
            auto room_settings = RoomSettingsInfo(createRoomReq->settings_data, createRoomReq->title, callback.message->GetDataSize() == sizeof(MainCreateRoomReq) ? createRoomReq->password : "");

            std::uint16_t current_room_id = 0;
            auto room_options = main_server->GetRoomOptionInfosGameModeCache(room_settings.settings.mode_index);
            if (!server->GetNextAvailableRoomId(current_room_id))
            {
                //room list full
                send_msg(session, 138, 0, NetEngine::Room::Create::Result::Failed, 0);
                return;
            }
            if (!room_options->size()) return;

            BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "will create room id: ({})", current_room_id);

            const auto& gamemode_info = main_server->GetRoomOptionInfoByTypeCache(room_options, NetEngine::Room::Option::Type::ModeInfo, room_settings.settings.mode_index);
            const auto& kill_info = main_server->GetRoomOptionInfoByTypeCache(room_options, NetEngine::Room::Option::Type::KillInfo, score_limit);
            const auto& time_info = main_server->GetRoomOptionInfoByTypeCache(room_options, NetEngine::Room::Option::Type::TimeInfo, room_settings.settings.time);
            const auto& playerlimit_info = main_server->GetRoomOptionInfoByTypeCache(room_options, NetEngine::Room::Option::Type::PlayerLimit, room_settings.settings.max_players * 2);
            const auto& weaponrestriction_info = main_server->GetRoomOptionInfoByTypeCache(room_options, NetEngine::Room::Option::Type::WeaponLimit, room_settings.settings.restriction);
            auto room_mode = static_cast<NetEngine::Room::Mode::Index>(room_settings.settings.mode_index);
            auto new_map_index = static_cast<NetEngine::Room::Map::Index>(room_settings.settings.map_index);
            if (new_map_index == NetEngine::Room::Map::Index::Random) //remove this later!
            {
                new_map_index = NetEngine::Room::Map::Index::HouseTop;
            }
            Game::Room new_room =
            {
                current_room_id,
                static_cast<std::uint16_t>(1),
                room_settings.title,
                room_settings.password,
                new_map_index,
                room_mode,
                static_cast<NetEngine::Room::Restriction::Type>(room_settings.settings.restriction),
                static_cast<NetEngine::Room::Balance::State>(room_settings.settings.team_balance),
                static_cast<std::uint32_t>(room_settings.settings.max_players * 2),
                score_limit,
                room_settings.settings.time,
                static_cast<bool>(room_settings.settings.allow_intruders),
                static_cast<bool>(room_settings.settings.allow_items),
                static_cast<bool>(room_settings.settings.allow_observers),
                false,
                !std::string(room_settings.password).empty(),
                session_id
            };

            BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "create room with password: ({})", room_settings.password);

            if (main_server->IsModeTeamBased(room_mode))
            {
                new_room.blueteam_session_ids.push_back(session_id);
                acc_cache->team_id = Team::IdType::Blue;
            }
            else
            {
                new_room.neutralteam_session_ids.push_back(session_id);
                acc_cache->team_id = Team::IdType::Neutral;
            }
            main_server->AddRoomCache(current_room_id, new_room);
            acc_cache->room_id = current_room_id;
            acc_cache->in_room = true;
            acc_cache->playing = false;
            acc_cache->state = PlayerInfo::State::HostReady;
            acc_cache->slot_id = 0;
            //server->SetRoomIdAvailable(current_room_id);

            new_room.has_password = !(new_room.password.empty());
            BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "NewRoom password: ({}), HasPassword: ({})", new_room.password, new_room.has_password);

            if (new_room.has_password)
            {
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "player ({}) created Room No. ({}), title: ({}), password: ({}) map: ({}), mode: ({})",
                    session->GetSessionId(), current_room_id, new_room.title.c_str(), new_room.password.c_str(), main_server->GetMapName(room_settings.settings.map_index), main_server->GetModeName(room_settings.settings.mode_index));
            }
            else
            {
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "player ({}) created Room No. ({}), title: ({}), map: ({}), mode: ({})",
                    session->GetSessionId(), current_room_id, new_room.title.c_str(), main_server->GetMapName(room_settings.settings.map_index), main_server->GetModeName(room_settings.settings.mode_index));
            }
           
            auto create_ack = MainRoomCreateAck(current_room_id, 1);
            send_msg(session, 138, 0, NetEngine::Room::Create::Result::Success, 0, reinterpret_cast<uint8_t*>(&create_ack), sizeof(MainRoomCreateAck));
        }
    }
    
}