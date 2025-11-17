#pragma once
namespace Game::Commands
{
    struct HelpCommand
    {
        static constexpr std::string_view name = "?";
        static constexpr uint8_t required_grade = Userlist::User::Grade::Tester;

        static void Run(std::span<const std::string_view> /*args*/, CommandContext& ctx)
        {
            const auto list = ListCommands(ctx.acc_cache->acc_info.Grade);
            for (const auto& line : list)
                ctx.server->SendServerMessage(ctx.callback.session, line);
        }

        inline static CommandRegister<HelpCommand> reg{};
    };
}