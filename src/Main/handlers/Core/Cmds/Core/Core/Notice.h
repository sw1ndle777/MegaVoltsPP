#pragma once
namespace Game::Commands
{
    struct NoticeCommand
    {
        static constexpr std::string_view name = "!";
        static constexpr uint8_t required_grade = Userlist::User::Grade::GameMaster;

        static void Run(std::span<const std::string_view> args, CommandContext& ctx)
        {
            const std::string nickname = ctx.acc_cache->acc_info.Nickname;
            ctx.acc_cache.unlock();

            if (args.size() != 2)
            {
                ctx.server->SendServerMessage(ctx.callback.session, "[MegaVolts Online] command usage: /! msg (512 max chars)");
                return;
            }

            const auto msg_sv = args[1];
            if (msg_sv.empty() || msg_sv.size() > 512)
            { 
                ctx.server->SendServerMessage(ctx.callback.session, "[MegaVolts Online] command usage: /! msg (512 max chars)");
                return;
            }

            std::string msg(msg_sv); // own it before broadcasting

            auto sidsOnline = CSid.get_all(shared);
            for (const auto& sid : *sidsOnline)
            {
                if (auto pss = ctx.server->GetSessionById(sid))
                {
                    pss->SendMsg(
                        402, 0, 10, 0,
                        reinterpret_cast<uint8_t*>(msg.data()),
                        static_cast<uint16_t>(msg.size()));
                }
            }
        };

        inline static CommandRegister<NoticeCommand> reg{};
    };
}