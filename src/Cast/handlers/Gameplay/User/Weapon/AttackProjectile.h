#pragma once
namespace Game::Handlers
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Cast;
    inline void UserAttackProjectile(SCallbackData& callback, CCastServer* server)
    {
        auto session = callback.session;
        auto message = callback.message;
        if (!session || !message) return;

        auto hostSid = message->GetSession();
        auto sid = session->GetSessionId();
		auto projectileType = message->GetOption();

		if (projectileType != 1 && projectileType != 2)
        {
            auto order = magic_enum::enum_cast<EOrder>(u16_cast(message->GetOrder())).value_or(EOrder::NONE);
            auto orderName = magic_enum::enum_name(order);
            DEBUGLOG(yellow, "({}): invalid projectileType=({}) from sid=({})", orderName, projectileType, sid);
            return;
        }

        server->Forward(hostSid, sid, *message);
    }
}

namespace NetEngine
{
    template <>
    struct PacketRateLimitPolicy<&Game::Handlers::UserAttackProjectile>
    {
        inline static const std::optional<RateLimit::Rule> value = RateLimit::Rule{
            .enabled = true,
            .bucket_scope = RateLimit::IdentityScope::Session,
            .max_packets = 20,
            .window = std::chrono::seconds{ 2 },
            .bucket_scope_resolver = [](const SCallbackData&, const RateLimit::IdentitySnapshot& identity)
            {
                return identity.aid > 0 ? RateLimit::IdentityScope::Aid : RateLimit::IdentityScope::Session;
            },
            .on_limit = [](RateLimit::ActionContext& ctx)
            {
                if (ctx.identity.aid > 0)
                    ctx.CooldownAid(std::chrono::seconds{ 2 });
                else
                    ctx.CooldownSession(std::chrono::seconds{ 2 });
            },
        };
    };
}
