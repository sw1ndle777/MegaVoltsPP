#pragma once
namespace Game::Commands
{
    using CommandFunc = std::function<void(const std::vector<std::string>&,
        const SCallbackData&,
        AccCacheResource&,
        CMainServer*)>;

    struct Command { CommandFunc func; uint8_t required_grade; };

    inline auto& CommandsCache()
    {
        static boost::unordered_flat_map<std::string, Command> cmds;
        return cmds; // init on first use
    }

    inline void Register(std::string_view name, CommandFunc func, uint8_t required_grade)
    {
        CommandsCache()[std::string(name)] = Command{ std::move(func), required_grade };
    }

    inline bool Execute(std::string_view name,
        const std::vector<std::string>& args,
        const SCallbackData& callback,
        AccCacheResource& acc_cache,
        CMainServer* main_server)
    {
        auto it = CommandsCache().find(std::string(name));
        if (it == CommandsCache().end()) return false;
        if (acc_cache->acc_info.Grade < it->second.required_grade) return false;
        it->second.func(args, callback, acc_cache, main_server);
        return true;
    }

    inline std::vector<std::string> ListCommands(uint8_t grade)
    {
        std::vector<std::string> out;
        out.push_back("[MegaVolts Online] Available commands:\n");
        for (const auto& [n, c] : CommandsCache()) if (grade >= c.required_grade) out.push_back(fmt::format("/{}", n));
        std::ranges::sort(out.begin() + 1, out.end());
        return out;
    }

    struct CommandContext { const SCallbackData& callback; AccCacheResource& acc_cache; CMainServer* server; };

    template <typename T>
    concept CommandImpl = requires(std::span<const std::string_view> a, CommandContext & c)
    {
        { T::Run(a, c) } -> std::same_as<void>;
        { T::required_grade } -> std::convertible_to<uint8_t>;
        { T::name } -> std::convertible_to<std::string_view>;
    };

    template <typename Impl>
        requires CommandImpl<Impl>
    inline void LegacyAdapter(const std::vector<std::string>& rawArgs,
        const SCallbackData& cb,
        AccCacheResource& acc,
        CMainServer* server)
    {
        std::vector<std::string_view> views; views.reserve(rawArgs.size());
        for (const auto& s : rawArgs) views.emplace_back(s);
        CommandContext ctx{ cb, acc, server };
        Impl::Run(std::span<const std::string_view>(views.data(), views.size()), ctx);
    }

    template <typename Impl>
    struct CommandRegister
    {
        CommandRegister() { Register(Impl::name, &LegacyAdapter<Impl>, Impl::required_grade); }
    };

    inline void Init() {}
}