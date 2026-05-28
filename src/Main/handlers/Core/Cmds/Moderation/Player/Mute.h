#pragma once
#include "PenaltyCommon.h"
namespace Game::Commands
{
    struct MuteCommand
    {
        static constexpr std::string_view name = "mute";
        static constexpr uint8_t required_grade = Userlist::User::Grade::GameMaster;

        static void Run(std::span<const std::string_view> args, CommandContext& ctx)
        {
            if (args.size() < 4)
            {
                ctx.server->SendServerMessage(ctx.callback.session, "[MegaVolts Online] usage: /mute <nickname> <duration> <reason>");
                return;
            }

            const auto duration = Utility::ParseHumanDuration(args[2]);
            if (!duration.has_value())
            {
                ctx.server->SendServerMessage(ctx.callback.session, std::format("[MegaVolts Online] invalid duration: {}", duration.error()));
                return;
            }

            ctx.acc_cache.unlock();
            auto reason = JoinCommandArgs(args, 3);
            [[maybe_unused]] auto ignored = BaseLib::DbPool->submit_task([
                server = ctx.server,
                session = std::move(ctx.callback.session),
                nickname = std::string(args[1]),
                duration = *duration,
                reason = std::move(reason)
            ]() mutable
            {
                if (!session) return;
                const auto target = ResolvePenaltyTargetByNickname(nickname);
                if (!target.has_value())
                {
                    server->SendServerMessage(session, std::format("[MegaVolts Online] {}", target.error()));
                    return;
                }

                const auto until = Utility::GetUtcTimeNow64() + static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::seconds>(duration).count());
                if (auto result = BaseLib::Database->SetAccountMutedUntil(target->aid, until); !result.has_value())
                {
                    server->SendServerMessage(session, std::format("[MegaVolts Online] failed to mute {}: {}", target->label, result.error().message));
                    return;
                }

                SetOnlineMutedUntil(target->aid, until);
                DEBUGLOG(yellow, "muted {} [{}] until {} reason=({})", target->label, target->aid, until, reason);
                server->SendServerMessage(session, std::format("[MegaVolts Online] muted {} [{}]", target->label, target->aid));
            });
        }

        inline static CommandRegister<MuteCommand> reg{};
    };

    struct MuteAidCommand
    {
        static constexpr std::string_view name = "muteaid";
        static constexpr uint8_t required_grade = Userlist::User::Grade::GameMaster;

        static void Run(std::span<const std::string_view> args, CommandContext& ctx)
        {
            if (args.size() < 4)
            {
                ctx.server->SendServerMessage(ctx.callback.session, "[MegaVolts Online] usage: /muteaid <aid> <duration> <reason>");
                return;
            }

            const auto duration = Utility::ParseHumanDuration(args[2]);
            if (!duration.has_value())
            {
                ctx.server->SendServerMessage(ctx.callback.session, std::format("[MegaVolts Online] invalid duration: {}", duration.error()));
                return;
            }

            ctx.acc_cache.unlock();
            auto reason = JoinCommandArgs(args, 3);
            [[maybe_unused]] auto ignored = BaseLib::DbPool->submit_task([
                server = ctx.server,
                session = std::move(ctx.callback.session),
                aid_arg = std::string(args[1]),
                duration = *duration,
                reason = std::move(reason)
            ]() mutable
            {
                if (!session) return;
                const auto target = ResolvePenaltyTargetByAid(aid_arg);
                if (!target.has_value())
                {
                    server->SendServerMessage(session, std::format("[MegaVolts Online] {}", target.error()));
                    return;
                }

                const auto until = Utility::GetUtcTimeNow64() + static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::seconds>(duration).count());
                if (auto result = BaseLib::Database->SetAccountMutedUntil(target->aid, until); !result.has_value())
                {
                    server->SendServerMessage(session, std::format("[MegaVolts Online] failed to mute {}: {}", target->label, result.error().message));
                    return;
                }

                SetOnlineMutedUntil(target->aid, until);
                DEBUGLOG(yellow, "muted {} [{}] until {} reason=({})", target->label, target->aid, until, reason);
                server->SendServerMessage(session, std::format("[MegaVolts Online] muted {} [{}]", target->label, target->aid));
            });
        }

        inline static CommandRegister<MuteAidCommand> reg{};
    };

    struct UnmuteCommand
    {
        static constexpr std::string_view name = "unmute";
        static constexpr uint8_t required_grade = Userlist::User::Grade::GameMaster;

        static void Run(std::span<const std::string_view> args, CommandContext& ctx)
        {
            if (args.size() != 2)
            {
                ctx.server->SendServerMessage(ctx.callback.session, "[MegaVolts Online] usage: /unmute <nickname>");
                return;
            }

            ctx.acc_cache.unlock();
            [[maybe_unused]] auto ignored = BaseLib::DbPool->submit_task([
                server = ctx.server,
                session = std::move(ctx.callback.session),
                nickname = std::string(args[1])
            ]() mutable
            {
                if (!session) return;
                const auto target = ResolvePenaltyTargetByNickname(nickname);
                if (!target.has_value())
                {
                    server->SendServerMessage(session, std::format("[MegaVolts Online] {}", target.error()));
                    return;
                }

                if (auto result = BaseLib::Database->SetAccountMutedUntil(target->aid, 0); !result.has_value())
                {
                    server->SendServerMessage(session, std::format("[MegaVolts Online] failed to unmute {}: {}", target->label, result.error().message));
                    return;
                }

                SetOnlineMutedUntil(target->aid, 0);
                server->SendServerMessage(session, std::format("[MegaVolts Online] unmuted {} [{}]", target->label, target->aid));
            });
        }

        inline static CommandRegister<UnmuteCommand> reg{};
    };

    struct UnmuteAidCommand
    {
        static constexpr std::string_view name = "unmuteaid";
        static constexpr uint8_t required_grade = Userlist::User::Grade::GameMaster;

        static void Run(std::span<const std::string_view> args, CommandContext& ctx)
        {
            if (args.size() != 2)
            {
                ctx.server->SendServerMessage(ctx.callback.session, "[MegaVolts Online] usage: /unmuteaid <aid>");
                return;
            }

            ctx.acc_cache.unlock();
            [[maybe_unused]] auto ignored = BaseLib::DbPool->submit_task([
                server = ctx.server,
                session = std::move(ctx.callback.session),
                aid_arg = std::string(args[1])
            ]() mutable
            {
                if (!session) return;
                const auto target = ResolvePenaltyTargetByAid(aid_arg);
                if (!target.has_value())
                {
                    server->SendServerMessage(session, std::format("[MegaVolts Online] {}", target.error()));
                    return;
                }

                if (auto result = BaseLib::Database->SetAccountMutedUntil(target->aid, 0); !result.has_value())
                {
                    server->SendServerMessage(session, std::format("[MegaVolts Online] failed to unmute {}: {}", target->label, result.error().message));
                    return;
                }

                SetOnlineMutedUntil(target->aid, 0);
                server->SendServerMessage(session, std::format("[MegaVolts Online] unmuted {} [{}]", target->label, target->aid));
            });
        }

        inline static CommandRegister<UnmuteAidCommand> reg{};
    };
}
