#pragma once
namespace Game::Commands
{
	struct MainInfoCommand
    {
        static constexpr std::string_view name = "maininfo";
        static constexpr uint8_t required_grade = Userlist::User::Grade::GameMaster;

        static void Run(std::span<const std::string_view> args, CommandContext& ctx)
        {
            ctx.acc_cache.unlock();

#ifdef _WIN32
            HANDLE m_process_handle = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, false, GetCurrentProcessId());
            auto cpu_usage = Utility::GetCpuUsage(m_process_handle);
            auto mem_usage = static_cast<uint32_t>(Utility::GetMemoryUsage(m_process_handle));
            CloseHandle(m_process_handle);
#else
            auto cpu_usage = 0.0;
            auto mem_usage = uint32_t(0);
#endif
            auto sessions_count = static_cast<uint16_t>(ctx.server->GetSessions()->size());

            auto msg = fmt::format("[MegaVolts Online] Main Info: Sessions Online: {}, Memory Usage: {} MB, Cpu Usage: {:.2f}%",
                static_cast<uint16_t>(sessions_count),
                static_cast<uint32_t>(mem_usage),
                static_cast<double>(cpu_usage));

            ctx.server->SendServerMessage(ctx.callback.session, msg.c_str());
        };

        inline static CommandRegister<MainInfoCommand> reg{};
    };
}
