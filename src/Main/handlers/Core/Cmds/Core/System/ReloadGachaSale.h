#pragma once
namespace Game::Commands
{
    struct ReloadGachaSaleCommand
    {
        static constexpr std::string_view name = "reloadgachasale";
        static constexpr uint8_t required_grade = Userlist::User::Grade::GameMaster;

        static void Run(std::span<const std::string_view> args, CommandContext& ctx)
        {
            ctx.acc_cache.unlock();
            CGachaponSale.clear();
            CGachaponSaleInfo.clear();

            [[maybe_unused]] auto ignored = BaseLib::DbPool->submit_task([ctx, session = std::move(ctx.callback.session)
            ]() mutable
                {
                    if (!session) return;
                   
                    auto gachapon_sales = BaseLib::Database->GetGachaponSalesInfo();
                    for (auto& sale : gachapon_sales)
                    {
                        if (CGachaponSaleInfo.contains(sale.gachapon_id))
                            continue;

                        CGachaponSaleInfo.insert(sale.gachapon_id, sale);
                        CGachaponSale.emplace_back(sale.gachapon_id);
                    }
                    ctx.server->SendServerMessage(ctx.callback.session, fmt::format("[MegaVolts Online] {} gachapon sales info reloaded", gachapon_sales.size()).c_str());
                });
        };

        inline static CommandRegister<ReloadGachaSaleCommand> reg{};
    };
}