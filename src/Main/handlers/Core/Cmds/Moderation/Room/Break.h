#pragma once
namespace Game::Commands
{
    struct BreakCommand
    {
        static constexpr std::string_view name = "break";
        static constexpr uint8_t required_grade = Userlist::User::Grade::GameMaster;

        static void Run(std::span<const std::string_view> args, CommandContext& ctx)
        {
			const std::string nickname = ctx.acc_cache->acc_info.Nickname; 
            ctx.acc_cache.unlock();

            auto myself_in_room = ctx.acc_cache->in_room;
            auto myself_room_id = ctx.acc_cache->room_id;

            if (args.size() != 1 && args.size() != 2)
            {
                ctx.server->SendServerMessage(ctx.callback.session, "[MegaVolts Online] command usage: /break or /break room_id");
                return;
            }
            uint16_t target_room_id = 0;

            if (args.size() == 1)
            {
                if (!myself_in_room)
                {
                    ctx.server->SendServerMessage(ctx.callback.session, "[MegaVolts Online] you are not in a room. Use /break room_id instead.");
                    return;
                }
                target_room_id = myself_room_id;
            }
            else if (args.size() == 2)
            {
                if (auto id = Utility::ParseNumber<uint16_t>(args[1]))
                    target_room_id = *id;
                else
                {
                    ctx.server->SendServerMessage(ctx.callback.session, fmt::format("[MegaVolts Online] invalid room ID, error: ({})", id.error()));
                    return;
                }
            }

            if (!CRoom.contains(target_room_id))
            {
                ctx.server->SendServerMessage(ctx.callback.session, "[MegaVolts Online] the specified room ID does not exist.");
                return;
            }

            DEBUGLOG(dark_cyan, "[{}] used moderation cmd ({}) on room id ({}).", nickname.c_str(), name, target_room_id);


            auto room = CRoom.get<shared_t>(target_room_id);
            auto ids = ctx.server->GetRoomSortedPlayerSessionIds(room);
            for (const auto& id : ids)
            {
                auto acc = CAccount.get<unique_t>(id);
                if (!acc->acc_info.Index || !acc->in_room || acc->room_id != room->room_id)
                {
                    acc.unlock();
                    continue;
                }
                else
                {
                    acc->in_room = false;
                    acc->slot_id = 0;
                    acc->playing = false;
                    acc->state = PlayerInfo::State::Waiting;
                    acc.unlock();

                    if (auto pss = ctx.server->GetSessionById(id))
                    {
                        pss->SendMsg(407, 0, NetEngine::Room::Leave::Ack::Result::ClosedByGm, 0); // show gm break popup
                        pss->SendMsg(141, 0, NetEngine::Room::Leave::Ack::Result::ClosedByGm, 0); // leave room ack

                    }
                }
            }
            room.unlock();
            CRoom.erase(target_room_id);
            CRoomId.erase_value(target_room_id);
            ctx.server->SendCastRoomRemoveSync(target_room_id);
            ctx.server->SetRoomIdAvailable(target_room_id);
        };

        inline static CommandRegister<BreakCommand> reg{};
    };
}
