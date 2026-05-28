#pragma once
#include "PenaltyCommon.h"
namespace Game::Commands
{
    struct BanCommand
    {
        static constexpr std::string_view name = "ban";
        static constexpr uint8_t required_grade = Userlist::User::Grade::GameMaster;

        static void Run(std::span<const std::string_view> args, CommandContext& ctx)
        {
            if (args.size() < 4)
            {
                ctx.server->SendServerMessage(ctx.callback.session, "[MegaVolts Online] usage: /ban <nickname> <duration> <reason>");
                return;
            }

            const auto duration = Utility::ParseHumanDuration(args[2]);
            if (!duration.has_value())
            {
                ctx.server->SendServerMessage(ctx.callback.session, std::format("[MegaVolts Online] invalid duration: {}", duration.error()));
                return;
            }

            const auto issuer = ctx.acc_cache->acc_info.Nickname;
            ctx.acc_cache.unlock();

            auto reason = JoinCommandArgs(args, 3);
            [[maybe_unused]] auto ignored = BaseLib::DbPool->submit_task([
                server = ctx.server,
                session = std::move(ctx.callback.session),
                issuer,
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
                if (auto result = BaseLib::Database->UpsertAccountBan(target->aid, until, reason); !result.has_value())
                {
                    server->SendServerMessage(session, std::format("[MegaVolts Online] failed to ban {}: {}", target->label, result.error().message));
                    return;
                }

                DisconnectOnlineAccount(server, target->aid);
                DEBUGLOG(yellow, "[{}] banned {} [{}] until {} reason=({})", issuer, target->label, target->aid, until, reason);
                server->SendServerMessage(session, std::format("[MegaVolts Online] banned {} [{}]", target->label, target->aid));
            });
        }

        inline static CommandRegister<BanCommand> reg{};
    };

    struct BanAidCommand
    {
        static constexpr std::string_view name = "banaid";
        static constexpr uint8_t required_grade = Userlist::User::Grade::GameMaster;

        static void Run(std::span<const std::string_view> args, CommandContext& ctx)
        {
            if (args.size() < 4)
            {
                ctx.server->SendServerMessage(ctx.callback.session, "[MegaVolts Online] usage: /banaid <aid> <duration> <reason>");
                return;
            }

            const auto duration = Utility::ParseHumanDuration(args[2]);
            if (!duration.has_value())
            {
                ctx.server->SendServerMessage(ctx.callback.session, std::format("[MegaVolts Online] invalid duration: {}", duration.error()));
                return;
            }

            const auto issuer = ctx.acc_cache->acc_info.Nickname;
            ctx.acc_cache.unlock();

            auto reason = JoinCommandArgs(args, 3);
            [[maybe_unused]] auto ignored = BaseLib::DbPool->submit_task([
                server = ctx.server,
                session = std::move(ctx.callback.session),
                issuer,
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
                if (auto result = BaseLib::Database->UpsertAccountBan(target->aid, until, reason); !result.has_value())
                {
                    server->SendServerMessage(session, std::format("[MegaVolts Online] failed to ban {}: {}", target->label, result.error().message));
                    return;
                }

                DisconnectOnlineAccount(server, target->aid);
                DEBUGLOG(yellow, "[{}] banned {} [{}] until {} reason=({})", issuer, target->label, target->aid, until, reason);
                server->SendServerMessage(session, std::format("[MegaVolts Online] banned {} [{}]", target->label, target->aid));
            });
        }

        inline static CommandRegister<BanAidCommand> reg{};
    };

    struct UnbanCommand
    {
        static constexpr std::string_view name = "unban";
        static constexpr uint8_t required_grade = Userlist::User::Grade::GameMaster;

        static void Run(std::span<const std::string_view> args, CommandContext& ctx)
        {
            if (args.size() != 2)
            {
                ctx.server->SendServerMessage(ctx.callback.session, "[MegaVolts Online] usage: /unban <nickname>");
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

                if (auto result = BaseLib::Database->RemoveAccountBan(target->aid); !result.has_value())
                {
                    server->SendServerMessage(session, std::format("[MegaVolts Online] failed to unban {}: {}", target->label, result.error().message));
                    return;
                }

                server->SendServerMessage(session, std::format("[MegaVolts Online] unbanned {} [{}]", target->label, target->aid));
            });
        }

        inline static CommandRegister<UnbanCommand> reg{};
    };

    struct UnbanAidCommand
    {
        static constexpr std::string_view name = "unbanaid";
        static constexpr uint8_t required_grade = Userlist::User::Grade::GameMaster;

        static void Run(std::span<const std::string_view> args, CommandContext& ctx)
        {
            if (args.size() != 2)
            {
                ctx.server->SendServerMessage(ctx.callback.session, "[MegaVolts Online] usage: /unbanaid <aid>");
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

                if (auto result = BaseLib::Database->RemoveAccountBan(target->aid); !result.has_value())
                {
                    server->SendServerMessage(session, std::format("[MegaVolts Online] failed to unban {}: {}", target->label, result.error().message));
                    return;
                }

                server->SendServerMessage(session, std::format("[MegaVolts Online] unbanned {} [{}]", target->label, target->aid));
            });
        }

        inline static CommandRegister<UnbanAidCommand> reg{};
    };
}
