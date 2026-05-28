#pragma once
namespace Game::Commands
{
    struct RoomAidsCommand
    {
        static constexpr std::string_view name = "roomaids";
        static constexpr uint8_t required_grade = Userlist::User::Grade::Moderator;

        static void Run(std::span<const std::string_view> /*args*/, CommandContext& ctx)
        {
            if (!ctx.acc_cache->in_room || !CRoom.contains(ctx.acc_cache->room_id))
            {
                ctx.server->SendServerMessage(ctx.callback.session, "[MegaVolts Online] You are not in a room.");
                ctx.acc_cache.unlock();
                return;
            }

            auto room = CRoom.get<shared_t>(ctx.acc_cache->room_id);
            ctx.acc_cache.unlock();

            auto ids = ctx.server->GetRoomSortedPlayerSessionIds(room);
            for (const auto sid : ids)
            {
                auto acc = CAccount.get<shared_t>(sid);
                if (!acc || acc->acc_info.Index <= 0)
                    continue;

                ctx.server->SendServerMessage(ctx.callback.session,
                    std::format("[MegaVolts Online] {} -> aid={}, sid={}", acc->acc_info.Nickname, acc->acc_info.Index, acc->session_id));
            }
        }

        inline static CommandRegister<RoomAidsCommand> reg{};
    };
}
