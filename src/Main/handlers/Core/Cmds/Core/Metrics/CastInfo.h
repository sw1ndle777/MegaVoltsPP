#pragma once
namespace Game::Commands
{
    struct CastInfoCommand
    {
        static constexpr std::string_view name = "castinfo";
        static constexpr uint8_t required_grade = Userlist::User::Grade::GameMaster;

        static void Run(std::span<const std::string_view> args, CommandContext& ctx)
        {
            ctx.server->SendCastIpc(PacketIds::Ipc::MainToCastReqServerInfo, Utility::ToVector(ctx.acc_cache->acc_info.AuthKey));
            ctx.acc_cache.unlock();
        };

        inline static CommandRegister<CastInfoCommand> reg{};
    };
}