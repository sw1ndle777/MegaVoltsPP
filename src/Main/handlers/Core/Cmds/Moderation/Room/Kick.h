#pragma once
namespace Game::Commands
{
    struct KickCommand
    {
        static constexpr std::string_view name = "kick";
        static constexpr uint8_t required_grade = Userlist::User::Grade::GameMaster;

        static void Run(std::span<const std::string_view> args, CommandContext& ctx)
        {
            const std::string my_nickname = ctx.acc_cache->acc_info.Nickname;
            
            ctx.acc_cache.unlock();

            if (args.size() != 2)
            {
                ctx.server->SendServerMessage(ctx.callback.session, "[MegaVolts Online] command usage: /kick nickname (2-16)");
                return;
            }

            const auto& target_nickname = args[1];

            if (target_nickname.size() < 2 || target_nickname.size() > 16)
            {
                ctx.server->SendServerMessage(ctx.callback.session, "[MegaVolts Online] command usage: /kick nickname (2-16)");
                return;
            }
            if (my_nickname == target_nickname)
            {
                ctx.server->SendServerMessage(ctx.callback.session, "[MegaVolts Online] {}, you can't kick yourself");
                return;
            }

            DEBUGLOG(dark_cyan, "[{}] used moderation cmd ({}).", my_nickname.c_str(), name);

            auto player = CAccount.get_by_filter<shared_t>([&](const auto& /*id*/, auto& player) {
                return Utility::ToLowercase(player.acc_info.Nickname) == Utility::ToLowercase(target_nickname);
                });
            auto sid = player->session_id;
            auto roomId = player->room_id;
            auto teamId = player->team_id;
            if (!player->acc_info.Index) return;
            if (!player->in_room || !CRoom.contains(roomId))
            {
                ctx.server->SendServerMessage(ctx.callback.session, std::format("[MegaVolts Online] player {} is not in any room.", player->acc_info.Nickname.c_str()).c_str());
                return;
            }
            player.unlock();

            DEBUGLOG(dark_cyan, "[{}] used moderation cmd ({}) on player ({}).", my_nickname.c_str(), name, target_nickname);

            auto room = CRoom.get<unique_t>(roomId);
            ctx.server->NewRemoveRoomPlayer(room, sid, teamId, NetEngine::Room::Leave::Ack::Result::KickedByGm, true);
        };

        inline static CommandRegister<KickCommand> reg{};
    };
}