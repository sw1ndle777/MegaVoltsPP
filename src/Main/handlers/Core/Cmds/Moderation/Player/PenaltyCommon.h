#pragma once
namespace Game::Commands
{
    struct PenaltyTarget
    {
        int32_t aid{ 0 };
        std::string label{};
        std::optional<uint16_t> sid{};
    };

    [[nodiscard]] inline std::string JoinCommandArgs(std::span<const std::string_view> args, const size_t start_index)
    {
        std::string joined;
        for (size_t i = start_index; i < args.size(); ++i)
        {
            if (!joined.empty())
                joined += ' ';
            joined += args[i];
        }
        return joined;
    }

    [[nodiscard]] inline std::expected<PenaltyTarget, std::string> ResolvePenaltyTargetByAid(std::string_view aid_arg)
    {
        const auto parsed_aid = Utility::ParseNumber<int32_t>(aid_arg);
        if (!parsed_aid.has_value() || *parsed_aid <= 0)
            return std::unexpected("invalid aid");

        const auto aid = *parsed_aid;
        PenaltyTarget target{ .aid = aid, .label = std::format("aid {}", aid) };

        if (CAidSid.contains(aid))
        {
            auto sid = CAidSid.get<shared_t>(aid);
            if (sid && *sid)
            {
                target.sid = *sid;
                auto acc = CAccount.get<shared_t>(*sid);
                if (acc && acc->acc_info.Index == aid)
                    target.label = std::format("{} [{}]", acc->acc_info.Nickname, aid);
            }
            return target;
        }

        const auto exists = BaseLib::Database->AccountExists(aid);
        if (!exists.has_value())
            return std::unexpected(exists.error().message);
        if (!*exists)
            return std::unexpected("aid not found");

        return target;
    }

    [[nodiscard]] inline std::expected<PenaltyTarget, std::string> ResolvePenaltyTargetByNickname(std::string_view nickname)
    {
        if (nickname.empty())
            return std::unexpected("nickname is empty");

        auto player = CAccount.get_by_filter<shared_t>([&](const auto& /*id*/, auto& candidate)
        {
            return candidate.acc_info.Index > 0 && Utility::ToLowercase(candidate.acc_info.Nickname) == Utility::ToLowercase(nickname);
        });
        if (player && player->acc_info.Index > 0)
        {
            return PenaltyTarget{
                .aid = player->acc_info.Index,
                .label = player->acc_info.Nickname,
                .sid = player->session_id,
            };
        }

        const auto aid = BaseLib::Database->GetAccountIdByNickname(nickname);
        if (!aid.has_value())
            return std::unexpected(aid.error().message);

        return PenaltyTarget{
            .aid = *aid,
            .label = std::string(nickname),
            .sid = std::nullopt,
        };
    }

    inline void SetOnlineMutedUntil(const int32_t aid, const uint64_t muted_until)
    {
        if (!CAidSid.contains(aid))
            return;

        auto sid = CAidSid.get<shared_t>(aid);
        if (!sid || !*sid)
            return;

        auto acc = CAccount.get<unique_t>(*sid);
        if (!acc || acc->acc_info.Index != aid)
            return;

        acc->acc_info.MutedUntil = muted_until;
    }

    inline void DisconnectOnlineAccount(CMainServer* server, const int32_t aid)
    {
        if (!server || !CAidSid.contains(aid))
            return;

        auto sid = CAidSid.get<shared_t>(aid);
        if (sid && *sid)
            server->DisconnectPlayer(*sid, Disconnect::Reason::Deny);
    }
}
