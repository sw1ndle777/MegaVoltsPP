#pragma once
namespace Game::Commands
{
    struct InvisCommand
    {
        static constexpr std::string_view name = "invis";
        static constexpr uint8_t required_grade = Userlist::User::Grade::Tester;

        static void Run(std::span<const std::string_view> args, CommandContext& ctx)
        {
            auto sid = ctx.callback.session->GetSessionId();
            bool new_state = !ctx.acc_cache->is_invisible;
            ctx.acc_cache->is_invisible = new_state;

            auto aid = ctx.acc_cache->acc_info.Index;
            bool in_plaza = ctx.acc_cache->in_plaza;
            auto plaza_id = ctx.acc_cache->plaza_id;

            if (in_plaza && CPlaza.contains(plaza_id))
            {
                auto plaza = CPlaza.get<unique_t>(plaza_id);
                if (new_state)
                {
                    auto my_uid = NetEngine::Packets::Core::UniqueId(sid, 1).data;
                    for (const auto& id : plaza->session_ids)
                    {
                        if (id == sid) continue;
                        if (auto pss = ctx.server->GetSessionById(id))
                            pss->SendMsg(425, 0, 0, 1, reinterpret_cast<uint8_t*>(&my_uid), sizeof(my_uid));
                    }
                }
                else
                {
                    // Reuse the caller's already-held unique lock. Re-fetching the same
                    // account via CAccount.get<shared_t>(sid) would re-lock our own entry
                    // on a non-recursive shared_mutex -> permanent self-deadlock (and it
                    // freezes all account inserts/erases, taking down logins on every server).
                    auto uid = ctx.acc_cache->uid;
                    auto voice = ctx.acc_cache->voice_id;
                    auto info1 = ctx.server->GetRoomUserPlayerInfo1(ctx.acc_cache);
                    auto info2 = ctx.server->GetRoomUserPlayerInfo2(ctx.acc_cache);
                    auto equiped_items = ctx.server->GetEquippedItems(ctx.acc_cache);
                    auto equipack = NetEngine::Packets::Main::PlazaEquipInfoAck(
                        ctx.acc_cache->acc_info.Nickname, ctx.acc_cache->uid, equiped_items, info1, info2);
                    for (const auto& id : plaza->session_ids)
                    {
                        if (id == sid) continue;
                        if (auto pss = ctx.server->GetSessionById(id))
                        {
                            pss->SendMsg(424, 0, 0, 1, reinterpret_cast<uint8_t*>(&equipack), sizeof(equipack));
                            pss->SendMsg(314, 0, 0, voice, reinterpret_cast<uint8_t*>(&uid), sizeof(uid));
                        }
                    }
                }
                plaza.unlock();
            }

            ctx.acc_cache.unlock();

            [[maybe_unused]] auto ignored = BaseLib::DbPool->submit_task([aid, new_state]()
            {
                BaseLib::Database->SetPlayerMiscInvisible(aid, new_state);
            });

            ctx.server->SendServerMessage(ctx.callback.session,
                fmt::format("[MegaVolts Online] invisibility {}", new_state ? "ON" : "OFF"));
        }

        inline static CommandRegister<InvisCommand> reg{};
    };
}
