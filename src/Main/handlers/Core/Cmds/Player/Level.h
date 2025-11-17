#pragma once
namespace Game::Commands
{
    struct LevelCommand
    {
        static constexpr std::string_view name = "level";
        static constexpr uint8_t required_grade = Userlist::User::Grade::Tester;

        static void Run(std::span<const std::string_view> args, CommandContext& ctx)
        {
            if (args.size() != 2)
            {
                ctx.server->SendServerMessage(ctx.callback.session, "[MegaVolts Online] command usage: /level new_level (0-100)");
                return;
            }
            uint32_t lvl = 0;
            if (auto number = Utility::ParseNumber<uint32_t>(args[1]))
            {
                lvl = *number;
                if(lvl > 100)
                {
                    ctx.server->SendServerMessage(ctx.callback.session, "[MegaVolts Online] invalid level, must be between (0-100)");
                    return;
				}
            } 
            else
            {
                ctx.server->SendServerMessage(ctx.callback.session, fmt::format("[MegaVolts Online] invalid level, error: ({})", number.error()));
                return;
            }

            DatabaseUpdateCtx dctx{ .sid = ctx.callback.session->GetSessionId(), .aid = ctx.acc_cache->acc_info.Index };

            auto gi = CGradesInfo.get<shared_t>(lvl + 1);
            if (gi->Grade)
                dctx.ops.emplace_back(AccountInfoPatch{ .experience = gi->Exp, .level = lvl });
            else
            {
                ctx.server->SendServerMessage(ctx.callback.session, "[MegaVolts Online] invalid level");
                return;
            }

            auto validated = ctx.server->ValidateDatabaseUpdates(ctx.acc_cache, dctx);
            if (!validated.has_value())
            {
                DEBUGLOG(red, "ValidateDatabaseUpdates failed for [{}] [{}]: {}", ctx.acc_cache->acc_info.Index, ctx.acc_cache->acc_info.Nickname.c_str(), static_cast<int>(validated.error()));
                return;
            }

            ctx.acc_cache.unlock();

            [[maybe_unused]] auto ignored = BaseLib::DbPool->submit_task([ctx, session = std::move(ctx.callback.session),
                v = std::move(validated.value())
            ]() mutable
                {
                    if (!session) return;
                    ResultDbUpdateInfo dbres;
                    if (!BaseLib::Database->UpdateAccount(v, dbres).has_value()) return;
                    auto new_acc_cache = CAccount.get<unique_t>(session->GetSessionId());
                    auto applied = ctx.server->ApplyDatabaseUpdates(new_acc_cache, v);
                    if (!applied.has_value())
                    {
                        DEBUGLOG(red, "ApplyDatabaseUpdates failed for [{}] [{}]: {}", new_acc_cache->acc_info.Index, new_acc_cache->acc_info.Nickname.c_str(), static_cast<int>(applied.error()));
                        return;
                    }
					ctx.server->SendServerMessage(session, fmt::format("[MegaVolts Online] Your level has been changed to ({})", v.acc_info_patches[0].level.value()));
                });
        };

        inline static CommandRegister<LevelCommand> reg{};
    };
}