#pragma once
namespace Game::Commands
{
    struct EffectCurrCommand
    {
        static constexpr std::string_view name = "effectcurr";
        static constexpr uint8_t required_grade = Userlist::User::Grade::Moderator;

        static void Run(std::span<const std::string_view> /*args*/, CommandContext& ctx)
        {
            constexpr uint8_t kSetType = 25;

            const auto selected_char = static_cast<uint8_t>(ctx.acc_cache->acc_info.SelectedCharacter);
            std::vector<BaseLib::Item> equipped;
            equipped.reserve(18);

            for (const auto& item : ctx.acc_cache->inventory_items)
            {
                if (item.is_equipped == 1 && item.character_id == selected_char)
                    equipped.push_back(item);
            }

            auto send_effect_info = [&](const std::string& label, uint32_t effect_id)
            {
                auto effect_info = CEffectInfo.get<shared_t>(effect_id);
                if (!effect_info->id)
                {
                    ctx.server->SendServerMessage(ctx.callback.session,
                        std::format("[MegaVolts Online] {} effectInfo id={} was not found.", label, effect_id));
                    return;
                }

                ctx.server->SendServerMessage(ctx.callback.session,
                    std::format("[MegaVolts Online] {} effectInfo key={} value={}",
                        label,
                        effect_info->key,
                        effect_info->valueA));
            };

            auto send_item_effects = [&](std::string_view label, uint32_t item_id)
            {
                auto item_info = CItemsInfo.get<shared_t>(item_id);
                if (!item_info->Id)
                {
                    ctx.server->SendServerMessage(ctx.callback.session,
                        std::format("[MegaVolts Online] {} part item_id={} was not found in item info.", label, item_id));
                    return;
                }

                bool found_effect = false;
                auto send_item_effect = [&](uint32_t effect_id, uint32_t /*target*/, uint8_t effect_slot)
                {
                    if (!effect_id)
                        return;

                    found_effect = true;
                    send_effect_info(std::format("{} part item_id={} effect{}", label, item_id, effect_slot), effect_id);
                };

                send_item_effect(item_info->EffectId1, item_info->EffectTarget1, 1);
                send_item_effect(item_info->EffectId2, item_info->EffectTarget2, 2);
                send_item_effect(item_info->EffectId3, item_info->EffectTarget3, 3);

                if (!found_effect)
                {
                    ctx.server->SendServerMessage(ctx.callback.session,
                        std::format("[MegaVolts Online] {} part item_id={} has no effect info.", label, item_id));
                }
            };

            const auto set_item_id = ctx.server->GetItemByType(equipped, kSetType).item_info.item_number.item_id;
            if (set_item_id && ctx.server->IsItemSet(set_item_id))
            {
                auto set_info = CSetItemsInfo.get<shared_t>(set_item_id);
                if (!set_info->Id)
                {
                    ctx.server->SendServerMessage(ctx.callback.session,
                        std::format("[MegaVolts Online] set item_id={} was not found in set item info.", set_item_id));
                    return;
                }

                ctx.server->SendServerMessage(ctx.callback.session,
                    std::format("[MegaVolts Online] set item_id={}", set_item_id));

                bool found_set_effect = false;
                auto send_set_effect = [&](std::string_view label, uint32_t effect_id)
                {
                    if (!effect_id || effect_id == UINT32_MAX)
                        return;

                    found_set_effect = true;
                    send_effect_info(std::format("set {}", label), effect_id);
                };

                send_set_effect("hair", set_info->Hair);
                send_set_effect("face", set_info->Face);
                send_set_effect("upper", set_info->Upper);
                send_set_effect("under", set_info->Under);
                send_set_effect("pants", set_info->Pants);
                send_set_effect("shirt", set_info->Arms);
                send_set_effect("boots", set_info->Boots);
                send_set_effect("glasses", set_info->AccessoryA);

                if (!found_set_effect)
                {
                    ctx.server->SendServerMessage(ctx.callback.session,
                        std::format("[MegaVolts Online] set item_id={} has no set-part effect info.", set_item_id));
                }

                return;
            }

            bool found_part = false;
            auto send_part_effect = [&](uint8_t type, std::string_view label)
            {
                const auto item_id = ctx.server->GetItemByType(equipped, type).item_info.item_number.item_id;
                if (!item_id)
                    return;

                found_part = true;
                send_item_effects(label, item_id);
            };

            send_part_effect(0, "hair");
            send_part_effect(1, "face");
            send_part_effect(2, "upper");
            send_part_effect(3, "under");
            send_part_effect(4, "pants");
            send_part_effect(5, "shirt");
            send_part_effect(6, "boots");
            send_part_effect(7, "glasses");

            if (!found_part)
            {
                ctx.server->SendServerMessage(ctx.callback.session,
                    "[MegaVolts Online] no equipped costume parts or set found for the current character.");
            }
        }

        inline static CommandRegister<EffectCurrCommand> reg{};
    };
}
