#pragma once
namespace Game::Handlers
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Cast;
    inline void UserAttackSniper(SCallbackData& callback, CCastServer* server)
    {
        auto session = callback.session;
        auto message = callback.message;
        if (!session || !message) return;

        auto hostSid = message->GetSession();
        auto sid = session->GetSessionId();
        server->Forward(hostSid, sid, *message);
    }
}

namespace NetEngine
{
    template <>
    struct PacketRateLimitPolicy<&Game::Handlers::UserAttackSniper>
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
