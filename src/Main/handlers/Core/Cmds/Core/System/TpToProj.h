#pragma once
namespace Game::Commands
{
    struct TpToProjCommand
    {
        static constexpr std::string_view name = "tptoproj";
        static constexpr uint8_t required_grade = Userlist::User::Grade::Moderator;

        static void Run(std::span<const std::string_view> args, CommandContext& ctx)
        {
            auto parse_state = [](std::string_view value) -> std::optional<bool>
            {
                if (value == "1" || value == "on" || value == "true" || value == "enable" || value == "enabled") return true;
                if (value == "0" || value == "off" || value == "false" || value == "disable" || value == "disabled") return false;
                return std::nullopt;
            };

            auto enabled_sids_read = Game::g_tp_to_proj_sids.get_all(shared);
            bool new_state = enabled_sids_read->find(ctx.acc_cache->session_id) == enabled_sids_read->end();
            enabled_sids_read.unlock();
            if (args.size() >= 2)
            {
                auto parsed = parse_state(args[1]);
                if (!parsed.has_value())
                {
                    ctx.server->SendServerMessage(ctx.callback.session, "[MegaVolts Online] usage: /tptoproj [on|off]");
                    return;
                }
                new_state = *parsed;
            }

            auto enabled_sids_write = Game::g_tp_to_proj_sids.get_all(unique);
            if (new_state)
                enabled_sids_write->insert(ctx.acc_cache->session_id);
            else
                enabled_sids_write->erase(ctx.acc_cache->session_id);
            enabled_sids_write.unlock();

            NetEngine::Packets::Ipc::MainToCastTpToProjToggle payload{ ctx.acc_cache->session_id, static_cast<uint8_t>(new_state ? 1 : 0) };
            ctx.server->SendCastIpc(PacketIds::Ipc::MainToCastTpToProjToggle, Utility::ToVector(payload));
            ctx.server->SendServerMessage(ctx.callback.session,
                std::format("[MegaVolts Online] tptoproj {} for sid {}", new_state ? "enabled" : "disabled", ctx.acc_cache->session_id));
        }

        inline static CommandRegister<TpToProjCommand> reg{};
    };
}
