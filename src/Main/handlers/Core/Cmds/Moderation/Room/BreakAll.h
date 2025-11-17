#pragma once
namespace Game::Commands
{
    struct BreakAllCommand
    {
        static constexpr std::string_view name = "breakall";
        static constexpr uint8_t required_grade = Userlist::User::Grade::GameMaster;

        static void Run(std::span<const std::string_view> args, CommandContext& ctx)
        {
			DEBUGLOG(dark_cyan, "[{}] used moderation cmd ({}).", ctx.acc_cache->acc_info.Nickname, name);
            ctx.acc_cache.unlock();

            auto room_ids = CRoomId.get_all(shared);
            for (auto& room_id : *room_ids)
            {
                auto room = CRoom.get<shared_t>(room_id);
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
                            pss->SendMsg(141, 0, NetEngine::Room::Leave::Ack::Result::ClosedByGm, 0); // Leave room ack

                        }
                    }
                }
                room.unlock();
                CRoom.erase(room_id);
                CRoomId.erase_value(room_id);
                ctx.server->SetRoomIdAvailable(room_id);
            }
            ctx.server->SendServerMessage(ctx.callback.session, "[MegaVolts Online] successfully broke all rooms.");
        };

        inline static CommandRegister<BreakAllCommand> reg{};
    };
}