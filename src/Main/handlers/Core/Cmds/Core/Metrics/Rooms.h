#pragma once
namespace Game::Commands
{
    struct RoomsCommand
    {
        static constexpr std::string_view name = "rooms";
        static constexpr uint8_t required_grade = Userlist::User::Grade::Tester;

        static void Run(std::span<const std::string_view> args, CommandContext& ctx)
        {
            ctx.acc_cache.unlock();
            auto roomIds = CRoomId.get_all(shared);
            for (const auto& id : *roomIds)
            {
                auto room = CRoom.get<shared_t>(id);
                std::string msg = "";
                const auto& neutral_size = room->neutralteam_session_ids.size();
                const auto& red_size = room->redteam_session_ids.size();
                const auto& blue_size = room->blueteam_session_ids.size();
                const auto& obs_size = room->observers_session_ids.size();
                if (room->has_password)
                    msg = fmt::format("({}) - Title: {} - Password: {} - plr count N: ({}), R: ({}), B: ({}), O: ({})", room->room_id, room->title.c_str(), room->password.c_str(), neutral_size, red_size, blue_size, obs_size);
                else
                    msg = fmt::format("({}) - Title: {} - plr count N: ({}), R: ({}), B: ({}), O: ({})", room->room_id, room->title.c_str(), neutral_size, red_size, blue_size, obs_size);

                ctx.server->SendServerMessage(ctx.callback.session, msg.c_str());
            }
        };

        inline static CommandRegister<RoomsCommand> reg{};
    };
}