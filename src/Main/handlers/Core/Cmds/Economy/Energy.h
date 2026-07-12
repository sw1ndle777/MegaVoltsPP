#pragma once
#include "EconomyCommon.h"
namespace Game::Commands
{
    struct EnergyCommand
    {
        static constexpr std::string_view name = "energy";
        static constexpr uint8_t required_grade = Userlist::User::Grade::Tester;

        static void Run(std::span<const std::string_view> args, CommandContext& ctx)
        {
            if (args.size() < 2 || args.size() > 3)
            {
                ctx.server->SendServerMessage(ctx.callback.session, "[MegaVolts Online] usage: /energy <amount> [nickname]");
                return;
            }

            auto amount = Utility::ParseNumber<uint32_t>(args[1]);
            if (!amount.has_value())
            {
                ctx.server->SendServerMessage(ctx.callback.session, fmt::format("[MegaVolts Online] invalid amount: {}", amount.error()));
                return;
            }

            bool targeting_other = args.size() == 3;
            std::string target_nickname;
            uint16_t target_sid = ctx.callback.session->GetSessionId();

            if (targeting_other)
            {
                target_nickname = std::string(args[2]);
                auto target = FindOnlinePlayerByNickname(target_nickname);
                if (!target.has_value())
                {
                    ctx.server->SendServerMessage(ctx.callback.session, fmt::format("[MegaVolts Online] player '{}' is not online", target_nickname));
                    return;
                }
                target_sid = target->sid;
                target_nickname = target->nickname;

                // Targeting yourself by name resolves to our own session. Reuse the
                // already-held unique lock below instead of re-acquiring it, which would
                // self-deadlock the non-recursive per-account shared_mutex.
                if (target_sid == ctx.callback.session->GetSessionId())
                    targeting_other = false;
            }

            auto target_acc = targeting_other ? CAccount.get<unique_t>(target_sid) : std::move(ctx.acc_cache);
            if (!target_acc || target_acc->acc_info.Index == -1)
            {
                ctx.server->SendServerMessage(ctx.callback.session, "[MegaVolts Online] target player not found");
                return;
            }

            auto current = target_acc->acc_info.Energy;
            auto desired = *amount;
            if (current == desired)
            {
                ctx.server->SendServerMessage(ctx.callback.session, fmt::format("[MegaVolts Online] Energy is already {}", desired));
                return;
            }

            DatabaseUpdateCtx dctx{ .sid = target_sid, .aid = target_acc->acc_info.Index };
            using enum CurrencyType;
            if (desired > current)
                dctx.ops.emplace_back(AccountCurrencyDelta{ .type = ENERGY, .value = desired - current, .is_reward = true });
            else
                dctx.ops.emplace_back(AccountCurrencyDelta{ .type = ENERGY, .value = current - desired, .is_reward = false });

            auto validated = ctx.server->ValidateDatabaseUpdates(target_acc, dctx, false, true);
            if (!validated.has_value())
            {
                ctx.server->SendServerMessage(ctx.callback.session, "[MegaVolts Online] failed to update Energy");
                DEBUGLOG(red, "ValidateDatabaseUpdates failed for energy cmd: {}", static_cast<int>(validated.error()));
                return;
            }

            auto energy_before = current;
            auto target_aid = target_acc->acc_info.Index;
            auto target_nick = targeting_other ? target_nickname : std::string(target_acc->acc_info.Nickname);
            target_acc.unlock();
            if (targeting_other)
                ctx.acc_cache.unlock();

            [[maybe_unused]] auto ignored = BaseLib::DbPool->submit_task([
                server = ctx.server,
                session = std::move(ctx.callback.session),
                t_sid = target_sid,
                t_aid = target_aid,
                t_nick = std::move(target_nick),
                energy_before,
                desired,
                v = std::move(validated.value())
            ]() mutable
            {
                if (!session) return;
                ResultDbUpdateInfo dbres;
                if (!BaseLib::Database->UpdateAccount(v, dbres).has_value()) return;
                auto new_acc = CAccount.get<unique_t>(t_sid);
                if (!new_acc) return;
                auto applied = server->ApplyDatabaseUpdates(new_acc, v);
                if (!applied.has_value()) return;

                LogContext log_ctx;
                CurrencyLogEntry log;
                log.aid = t_aid;
                log.currency_type = CurrencyLog::Type::Energy;
                log.amount = static_cast<int32_t>(desired) - static_cast<int32_t>(energy_before);
                log.before_value = energy_before;
                log.after_value = new_acc->acc_info.Energy;
                log.source_type = CurrencyLog::SourceType::Admin;
                log_ctx.currency_logs.push_back(log);
                BaseLib::Database->PersistLogs(log_ctx);

                if (auto target_session = server->GetSessionById(t_sid))
                    SendAccountInfoRefresh(server, target_session.get(), new_acc);

                server->SendServerMessage(session, fmt::format("[MegaVolts Online] set Energy to {} for {}", new_acc->acc_info.Energy, t_nick));
            });
        }

        inline static CommandRegister<EnergyCommand> reg{};
    };
}
