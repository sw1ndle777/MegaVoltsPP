#pragma once
namespace Game::Commands
{
    struct RegisterAccountCommand
    {
        static constexpr std::string_view name = "registeracc";
        static constexpr uint8_t required_grade = Userlist::User::Grade::Tester;

        static void Run(std::span<const std::string_view> args, CommandContext& ctx)
        {
            if (args.size() < 4)
            {
                ctx.server->SendServerMessage(ctx.callback.session, "[MegaVolts Online] Usage: /registeracc <username> <password> <nickname>");
                return;
            }

            std::string username(args[1]);
            std::string password(args[2]);
            std::string nickname(args[3]);

            // Validate lengths
            if (username.length() < 2 || username.length() > 24)
            {
                ctx.server->SendServerMessage(ctx.callback.session, "[MegaVolts Online] Username must be 2-24 characters.");
                return;
            }
            if (password.length() < 2 || password.length() > 24)
            {
                ctx.server->SendServerMessage(ctx.callback.session, "[MegaVolts Online] Password must be 2-24 characters.");
                return;
            }
            if (nickname.length() < 1 || nickname.length() > 16)
            {
                ctx.server->SendServerMessage(ctx.callback.session, "[MegaVolts Online] Nickname must be 1-16 characters.");
                return;
            }

            [[maybe_unused]] auto ignored = BaseLib::DbPool->submit_task([
                server = ctx.server,
                session = std::move(ctx.callback.session),
                username = std::move(username),
                password = std::move(password),
                nickname = std::move(nickname)
            ]() mutable
                {
                    if (!session) return;

                    thread_local Utility::SecureRandomBlake2b::Generator rng;
                    uint64_t salt_part1 = rng.GenerateAuthKey();
                    uint64_t salt_part2 = rng.GenerateAuthKey();
                    uint8_t salt[16];
                    std::memcpy(salt, &salt_part1, 8);
                    std::memcpy(salt + 8, &salt_part2, 8);

                    // Hash password
                    uint8_t hash[32];
                    if (!Utility::HashPassword(password, salt, hash))
                    {
                        server->SendServerMessage(session, "[MegaVolts Online] Failed to hash password.");
                        return;
                    }

                    // Base64 encode salt and hash
                    std::string salt_b64 = Utility::Base64::to_base64(std::string_view(reinterpret_cast<char*>(salt), 16));
                    std::string hash_b64 = Utility::Base64::to_base64(std::string_view(reinterpret_cast<char*>(hash), 32));

                    // Insert into database
                    auto result = BaseLib::Database->InsertAccount(username, hash_b64, salt_b64, nickname);
                    if (result.has_value())
                        server->SendServerMessage(session, std::format("[MegaVolts Online] Account '{}' created successfully!", username).c_str());
                    else
                        server->SendServerMessage(session, std::format("[MegaVolts Online] Failed to create account: {}", result.error().message).c_str());
                });
        }

        inline static CommandRegister<RegisterAccountCommand> reg{};
    };
}
