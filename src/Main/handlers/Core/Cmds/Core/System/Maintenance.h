#pragma once
namespace Game::Commands
{
    struct MaintenanceCommand
    {
        static constexpr std::string_view name = "maintenance";
        static constexpr uint8_t required_grade = Userlist::User::Grade::GameMaster;

        static void Run(std::span<const std::string_view> args, CommandContext& ctx)
        {
            ctx.acc_cache.unlock();

            auto sessions = ctx.server->GetSessions();

            std::vector<uint16_t> ids;
            ids.reserve(sessions->size());
            for (const auto& id : std::views::keys(*sessions))
                ids.push_back(id);

            sessions.unlock();

            for (auto& id : ids)
                ctx.server->DisconnectPlayer(id, Disconnect::Reason::Deny);

			DEBUGLOG(dark_cyan, "Disconnected ({}) players for maintenance.", ids.size());
        };

        inline static CommandRegister<MaintenanceCommand> reg{};
    };
}
