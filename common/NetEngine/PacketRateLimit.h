#pragma once

#include <chrono>
#include <cstdint>
#include <functional>
#include <magic_enum.hpp>
#include <optional>
#include <string>

#include "Constants.h"

namespace NetEngine
{
    class CServer;
    class CSession;
    struct SCallbackData;

    namespace RateLimit
    {
        enum class IdentityScope : uint8_t
        {
            Session = 0,
            Ip = 1,
            Hwid = 2,
            Aid = 3,
        };

        enum class Event : uint8_t
        {
            LimitExceeded = 0,
            CooldownActive = 1,
            BlacklistActive = 2,
        };

        struct IdentitySnapshot
        {
            uint16_t sid{};
            int32_t aid{ -1 };
            std::string ip{};
            std::string hwid{};
        };

        struct ActionContext
        {
            CServer* server{};
            SCallbackData* callback{};
            CSession* session{};
            Event event{ Event::LimitExceeded };
            uint16_t order{};
            IdentitySnapshot identity{};
            std::chrono::milliseconds retry_after{};
            uint32_t packet_count{};

            [[nodiscard]] uint16_t OrderId() const noexcept { return order; }
            [[nodiscard]] std::string OrderName() const;
            [[nodiscard]] bool HasIdentity(IdentityScope scope) const;
            [[nodiscard]] std::string IdentityValue(IdentityScope scope) const;

            void ApplyOrderCooldown(IdentityScope scope, std::chrono::milliseconds duration) const;
            void Blacklist(IdentityScope scope, std::chrono::milliseconds duration) const;
            [[nodiscard]] uint32_t AddStrike(IdentityScope scope, std::chrono::milliseconds window) const;
            void Disconnect() const;

            void CooldownSession(std::chrono::milliseconds duration) const { ApplyOrderCooldown(IdentityScope::Session, duration); }
            void CooldownIp(std::chrono::milliseconds duration) const { ApplyOrderCooldown(IdentityScope::Ip, duration); }
            void CooldownHwid(std::chrono::milliseconds duration) const { ApplyOrderCooldown(IdentityScope::Hwid, duration); }
            void CooldownAid(std::chrono::milliseconds duration) const { ApplyOrderCooldown(IdentityScope::Aid, duration); }

            void BlacklistSession(std::chrono::milliseconds duration) const { Blacklist(IdentityScope::Session, duration); }
            void BlacklistIp(std::chrono::milliseconds duration) const { Blacklist(IdentityScope::Ip, duration); }
            void BlacklistHwid(std::chrono::milliseconds duration) const { Blacklist(IdentityScope::Hwid, duration); }
            void BlacklistAid(std::chrono::milliseconds duration) const { Blacklist(IdentityScope::Aid, duration); }

            [[nodiscard]] uint32_t StrikeSession(std::chrono::milliseconds window) const { return AddStrike(IdentityScope::Session, window); }
            [[nodiscard]] uint32_t StrikeIp(std::chrono::milliseconds window) const { return AddStrike(IdentityScope::Ip, window); }
            [[nodiscard]] uint32_t StrikeHwid(std::chrono::milliseconds window) const { return AddStrike(IdentityScope::Hwid, window); }
            [[nodiscard]] uint32_t StrikeAid(std::chrono::milliseconds window) const { return AddStrike(IdentityScope::Aid, window); }
        };

        struct Rule
        {
            bool enabled{ false };
            IdentityScope bucket_scope{ IdentityScope::Session };
            uint32_t max_packets{ 0 };
            std::chrono::milliseconds window{};
            std::function<IdentityScope(const SCallbackData&, const IdentitySnapshot&)> bucket_scope_resolver{};
            std::function<uint32_t(const SCallbackData&, const IdentitySnapshot&)> max_packets_resolver{};
            std::function<std::chrono::milliseconds(const SCallbackData&, const IdentitySnapshot&)> window_resolver{};
            std::function<void(ActionContext&)> on_limit{};
            std::function<void(ActionContext&)> on_rejected{};
        };
    }

    template <auto HandlerFn>
    struct PacketRateLimitPolicy
    {
        inline static const std::optional<RateLimit::Rule> value = std::nullopt;
    };
}
