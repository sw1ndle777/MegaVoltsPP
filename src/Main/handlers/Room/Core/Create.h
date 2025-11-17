#pragma once
namespace Game::Handlers
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Main;
    struct CreateRoomCtx
    {
        CMainServer* main;
        CSession* session;
        AccCacheResource& acc;
    };
    enum class CreateRoomError
    {
        AlreadyInRoom,
        NoRoomIdAvailable,
        NoRoomOptions,
    };
    inline void TryLeavePlaza(CreateRoomCtx& ctx)
    {
        auto sid = ctx.session->GetSessionId();
        if (!ctx.acc->in_plaza) return;
        auto plaza_id = ctx.acc->plaza_id;
        if (!ctx.main->IsPlazaAlready(plaza_id)) return;
        DEBUGLOG(dark_cyan, "player will leave plaza: ({})", plaza_id);
        auto current_plaza = CPlaza.get<unique_t>(plaza_id);
        auto& ids = current_plaza->session_ids;
		if (!std::ranges::contains(ids, sid)) return;

        DEBUGLOG(dark_cyan, "sid=({}) left plaza id: ({})", sid, plaza_id);
        std::erase_if(ids, [&](const uint16_t& id) { return id == sid; });
        ctx.acc->plaza_id = 0;
        ctx.acc->in_plaza = false;
        auto uid = ctx.acc->uid.data;
        for (const auto& id : ids)
        {
            if (id == sid) continue;
            if (auto player_session = ctx.main->GetSessionById(id))
                player_session->SendMsg(425, 0, 0, 1, reinterpret_cast<uint8_t*>(&uid), sizeof(uid));
        }

        
    }
    inline std::expected<Game::Room, CreateRoomError> ValidateCreateRoom(CreateRoomCtx& ctx, RoomSettingsInfo& info, uint8_t score_limit)
    {
        using enum NetEngine::Room::Create::Result;
        if (ctx.acc->in_room)
        {
            DEBUGLOG(dark_cyan, "player want to create room while he is in room or party: refuse");
            ctx.session->SendMsg(138, 0, Failed, 0);
            return std::unexpected(CreateRoomError::AlreadyInRoom);
        }
        
        uint16_t current_room_id = 0;
        auto room_options = CRoomOptionsInfo.get<shared_t>(info.settings.mode_index);
        if (!ctx.main->GetNextAvailableRoomId(current_room_id))
        {
            ctx.session->SendMsg(138, 0, Failed, 0);
            return std::unexpected(CreateRoomError::NoRoomIdAvailable);
        }
        if (!room_options->size()) return std::unexpected(CreateRoomError::NoRoomOptions);

        using enum NetEngine::Room::Option::Type;
        const auto& gamemode_info = ctx.main->GetRoomOptionInfoByTypeCache(room_options, ModeInfo, info.settings.mode_index);
        const auto& kill_info = ctx.main->GetRoomOptionInfoByTypeCache(room_options, KillInfo, score_limit);
        const auto& time_info = ctx.main->GetRoomOptionInfoByTypeCache(room_options, TimeInfo, info.settings.time);
        const auto& playerlimit_info = ctx.main->GetRoomOptionInfoByTypeCache(room_options, PlayerLimit, info.settings.max_players * 2);
        const auto& weaponrestriction_info = ctx.main->GetRoomOptionInfoByTypeCache(room_options, WeaponLimit, info.settings.restriction);
        auto room_mode = static_cast<NetEngine::Room::Mode::Index>(info.settings.mode_index);
        auto new_map_index = static_cast<NetEngine::Room::Map::Index>(info.settings.map_index);
        Game::Room new_room =
        {
            current_room_id, static_cast<uint16_t>(1),
            info.title, info.password, new_map_index, room_mode,
            static_cast<NetEngine::Room::Restriction::Type>(info.settings.restriction),
            //static_cast<NetEngine::Room::Balance::State>(room_settings.settings.team_balance),
            NetEngine::Room::Balance::State::Disabled,
            static_cast<uint32_t>(info.settings.max_players * 2),
            score_limit, info.settings.time,
            static_cast<bool>(info.settings.allow_intruders),
            static_cast<bool>(info.settings.allow_items),
            static_cast<bool>(info.settings.allow_observers),
            false, false,
            ctx.session->GetSessionId()
        };
        new_room.is_clan_room = false;
        new_room.clan_id_1 = 0;
        new_room.clan_id_2 = 0;
        new_room.has_password = !(new_room.password.empty());
        DEBUGLOG(dark_cyan, "create room with password: ({})", info.password);
        return new_room;
    }
    inline void FinalizeCreateRoom(CreateRoomCtx& ctx, Game::Room& room)
    {
        auto sid = ctx.session->GetSessionId();
        auto is_team_mode = ctx.main->IsModeTeamBased(room.ModeIndex);
        is_team_mode ? room.blueteam_session_ids.push_back(sid) : room.neutralteam_session_ids.push_back(sid);
        ctx.acc->team_id = is_team_mode ? Team::IdType::Blue : Team::IdType::Neutral;
        auto room_id = room.room_id;
        auto title = room.title;
        auto has_pw = room.has_password;
        auto pw = room.password;
        auto map_id = room.MapIndex;
        auto mode_id = room.ModeIndex;
		CRoom.insert(room.room_id, room);
		CRoomId.emplace_back(room.room_id);
        ctx.acc->room_id = room_id;
        ctx.acc->in_room = true;
        ctx.acc->playing = false;
#if defined(RELEASE_1_0_3)
        ctx.acc->state = PlayerInfo::State::HostReady;
#else
        ctx.acc->state = PlayerInfo::State::PlayerReady;
#endif
        ctx.acc->slot_id = 0;
        TryLeavePlaza(ctx);

        if (has_pw)
        {
            DEBUGLOG(dark_cyan, "player ({}) created Room No. ({}), title: ({}), password: ({}) map: ({}), mode: ({})",
                sid, room_id, title.c_str(), pw.c_str(), ctx.main->GetMapName(map_id), ctx.main->GetModeName(mode_id));
        }
        else
        {
            DEBUGLOG(dark_cyan, "player ({}) created Room No. ({}), title: ({}), map: ({}), mode: ({})",
                sid, room_id, title.c_str(), ctx.main->GetMapName(map_id), ctx.main->GetModeName(mode_id));
        }
        auto create_ack = MainRoomCreateAck(room_id, 1);

        ctx.session->SendMsg(138, 0, NetEngine::Room::Create::Result::Success, 0, reinterpret_cast<uint8_t*>(&create_ack), sizeof(MainRoomCreateAck));
    }
    inline void RoomCreate(SCallbackData& callback, CMainServer* main_server)
    {
        DEBUGLOG(dark_cyan, "want to create room");
        auto session = callback.session;
        auto message = callback.message;
        if (!session || !message) return;

        //std::shared_lock lock(session->GetMutex());
        CServer* server = callback.server;
        auto session_id = session->GetSessionId();
        auto acc_cache = CAccount.get<unique_t>(session_id);
        auto acc_index = acc_cache->acc_info.Index;
        if (acc_index == -1) return;
        const auto& req = reinterpret_cast<MainCreateRoomReq*>(message->GetData());
        auto score_limit = message->GetExtra();
        auto room_settings = RoomSettingsInfo(req->settings_data, req->title, message->GetDataSize() == sizeof(MainCreateRoomReq) ? req->password : "");

        CreateRoomCtx ctx
        {
            .main = main_server,
            .session = session,
            .acc = acc_cache
        };

        auto validated_room = ValidateCreateRoom(ctx, room_settings, score_limit);
        if (!validated_room.has_value())
        {
            DEBUGLOG(red, "ValidateCreateRoom failed for player [{}] [{}]: {}", acc_cache->acc_info.Index, acc_cache->acc_info.Nickname.c_str(), static_cast<int>(validated_room.error()));
            return;
        }
        auto& new_room = validated_room.value();
        FinalizeCreateRoom(ctx, new_room);
    }
}