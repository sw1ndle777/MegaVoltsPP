#pragma once
namespace Game::Commands
{
    struct ClearInventoryCommand
    {
        static constexpr std::string_view name = "clearinv";
        static constexpr uint8_t required_grade = Userlist::User::Grade::Tester;

        static void Run(std::span<const std::string_view> args, CommandContext& ctx)
        {

            DatabaseUpdateCtx dctx{ .sid = ctx.callback.session->GetSessionId(), .aid = ctx.acc_cache->acc_info.Index };
            std::vector<ItemSerialInfo> items_to_delete;
            for (const auto& item : ctx.acc_cache->inventory_items)
            {
                items_to_delete.push_back(item.item_info.serial_info);
                dctx.ops.emplace_back(ItemDeleteCtx{ .serials = {item.item_info.serial_info} });
            }
            ctx.server->SendServerMessage(ctx.callback.session, std::format("[MegaVolts Online] gonna delete ({}) item(s) from ({})", items_to_delete.size(), ctx.acc_cache->acc_info.Nickname.c_str()).c_str());

            auto validated = ctx.server->ValidateDatabaseUpdates(ctx.acc_cache, dctx); // bypass inv limit check
            if (!validated.has_value())
            {
                DEBUGLOG(red, "ValidateDatabaseUpdates failed for [{}] [{}]: {}", ctx.acc_cache->acc_info.Index, ctx.acc_cache->acc_info.Nickname.c_str(), static_cast<int>(validated.error()));
                return;
            }

            ctx.acc_cache.unlock();

            [[maybe_unused]] auto ignored = BaseLib::DbPool->submit_task([ctx,
                session = std::move(ctx.callback.session),
                items_to_delete = std::move(items_to_delete),
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
                    for (auto& item : items_to_delete)
                    {
                        auto deleteItemData = MainDeleteItemAck({ item }).Serialize();
                        session->SendMsg(89, 0, 1, 0, reinterpret_cast<uint8_t*>(deleteItemData.data()), deleteItemData.size());
                    }
                    ctx.server->SendServerMessage(session, fmt::format("[MegaVolts Online] deleted ({}) item(s) from ({})", items_to_delete.size(), new_acc_cache->acc_info.Nickname.c_str()).c_str());
                });
        };

        inline static CommandRegister<ClearInventoryCommand> reg{};
    };
}