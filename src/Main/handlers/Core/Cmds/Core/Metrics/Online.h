#pragma once
namespace Game::Commands
{
    struct OnlineCommand
    {
        static constexpr std::string_view name = "online";
        static constexpr uint8_t required_grade = Userlist::User::Grade::Tester;

        static void Run(std::span<const std::string_view> args, CommandContext& ctx)
        {
            ctx.acc_cache.unlock();
            auto sessions_list = ctx.server->GetSessions();


            auto sidsOnline = CSid.get_all(shared);
           

            // debug info
            for (const auto& sid : *sidsOnline)
            {
                auto acc = CAccount.get<shared_t>(sid);
                const auto& is_playing = acc->playing ? "Yes" : "No";
                const auto& in_room = acc->in_room;
                const auto& state = acc->state;
                std::string msg = "";
                msg = fmt::format("({}) SessionID: {} - Grade: {}, Slot: {}, Playing: {}, State: {}, Ping: {}",
                    acc->acc_info.Nickname.c_str(), sid, acc->acc_info.Grade, acc->slot_id, is_playing, acc->state, acc->ping);
                if (acc->in_room) msg += fmt::format(", roomId={}", acc->room_id);

                acc.unlock();
                ctx.server->SendServerMessage(ctx.callback.session, msg.c_str());
            }

            for (auto& sid : *sessions_list)
                ctx.server->SendServerMessage(ctx.callback.session, std::format("sid online: {}", sid.first).c_str());

            ctx.server->SendServerMessage(ctx.callback.session, fmt::format("[MegaVolts Online] Players Online: {}, Sessions Size: {}", sidsOnline->size(), sessions_list->size()).c_str());
        };

        inline static CommandRegister<OnlineCommand> reg{};
    };
}