#pragma once
#include <BaseLib/CLogging.h>
#include <array>
#include <vector>
#include <cstdlib>

// Battle Pass (MICROPASS) server logic. Mirrors the playtime/monthly reward flow:
//   - buildAck()  : assemble the order-182 snapshot from DB (season + levels + player + mission)
//   - sendOnLogin(): called from Authorize.h to push the snapshot at login
//   - BattlePassClaim (order 183): claim current level / claim-all -> craft items + set claimed bits
//   - BattlePassReset (order 184): reroll the active mission for MP
namespace Game::Handlers
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Main;

    namespace BattlePassDetail
    {
        inline bool getBit(const std::array<uint8_t, 16>& m, uint32_t i) { return i < 100 && ((m[i >> 3] >> (i & 7)) & 1); }
        inline void setBit(std::array<uint8_t, 16>& m, uint32_t i) { if (i < 100) m[i >> 3] |= static_cast<uint8_t>(1u << (i & 7)); }

        // Pick a random mission id different from `exclude` (used for fresh state + reroll).
        inline uint32_t pickMission(const std::vector<SystemBattlePassMission>& pool, uint32_t exclude)
        {
            if (pool.empty()) return 0;
            std::vector<uint32_t> cand;
            for (const auto& m : pool) if (m.mission_id != exclude) cand.push_back(m.mission_id);
            if (cand.empty()) return pool.front().mission_id;
            return cand[static_cast<size_t>(std::rand()) % cand.size()];
        }

        // Load DB state and fill the order-182 ack. Returns false when no season is active.
        // If the player has no row (or a stale season), a fresh row is created/persisted.
        inline bool buildAck(uint32_t aid, MainBattlePassAck& ack, PlayerBattlePass* outPlayer = nullptr)
        {
            SystemBattlePassSeason season{};
            if (!BaseLib::Database->GetActiveBattlePassSeason(&season)) return false;

            std::vector<SystemBattlePassLevel> levels;
            BaseLib::Database->GetSystemBattlePassLevels(season.season, &levels);
            std::vector<SystemBattlePassMission> missions;
            BaseLib::Database->GetSystemBattlePassMissions(&missions);

            PlayerBattlePass p{};
            bool have = BaseLib::Database->GetPlayerBattlePass(aid, &p);
            if (!have || p.season != season.season)
            {
                uint8_t prem = have ? p.has_premium : 0;
                p = PlayerBattlePass{};
                p.player_account_id = aid;
                p.season = season.season;
                p.level = 1;
                p.has_premium = prem;
                p.current_mission_id = pickMission(missions, 0);

                ValidatedDbUpdates v{};
                v.aid = static_cast<int32_t>(aid);
                PlayerBattlePassPatch patch{};
                patch.season = p.season; patch.level = p.level; patch.xp = 0u;
                patch.has_premium = p.has_premium;
                patch.claimed_free = p.claimed_free; patch.claimed_premium = p.claimed_premium;
                patch.current_mission_id = p.current_mission_id;
                patch.mission_progress = 0u; patch.reset_count = 0u;
                v.player_battlepass_patches.push_back(patch);
                (void)BaseLib::Database->PersistBattlePassPatches(v);
            }
            if (outPlayer) *outPlayer = p;

            ack = MainBattlePassAck{};
            ack.season = season.season;
            uint64_t now = Utility::GetUtcTimeNow64();
            ack.days_left = (season.end_date > now) ? static_cast<uint32_t>((season.end_date - now) / 86400ull) : 0u;
            ack.level = p.level;
            ack.xp = p.xp;
            ack.has_premium = p.has_premium;
            ack.reset_count = p.reset_count;
            ack.reset_cost = season.reset_base_cost * (p.reset_count + 1);
            ack.claimed_free = p.claimed_free;
            ack.claimed_premium = p.claimed_premium;
            for (const auto& lv : levels)
            {
                if (lv.level >= 1 && lv.level <= 100)
                {
                    ack.free_items[lv.level - 1] = lv.free_item;
                    ack.premium_items[lv.level - 1] = lv.premium_item;
                }
                if (lv.level == p.level) ack.xp_required = lv.xp_required;
            }
            for (const auto& m : missions)
                if (m.mission_id == p.current_mission_id) { ack.mission_text = m.description; break; }
            return true;
        }
    }

    // Called from Authorize.h on login.
    inline void SendBattlePassOnLogin(auto session, uint32_t aid)
    {
        MainBattlePassAck ack{};
        if (!BattlePassDetail::buildAck(aid, ack)) return;
        auto data = ack.Serialize();
        session->SendMsg(182 /* BATTLEPASS_DATA */, 0, 0, 0,
                         reinterpret_cast<uint8_t*>(data.data()), data.size());
    }

    // order 183 — claim. option: 0 = single level (GetMission() = level), 1 = claim all unlocked.
    inline void BattlePassClaim(SCallbackData& callback, CMainServer* main_server)
    {
        auto session = callback.session;
        auto message = callback.message;
        if (!session || !message) return;
        auto session_id = session->GetSessionId();
        auto acc_cache = CAccount.get<unique_t>(session_id);
        auto aid = acc_cache->acc_info.Index;
        if (aid == -1) return;

        uint32_t mode = static_cast<uint32_t>(message->GetOption());   // 0 single, 1 all
        uint32_t target_level = static_cast<uint32_t>(message->GetMission());

        SystemBattlePassSeason season{};
        if (!BaseLib::Database->GetActiveBattlePassSeason(&season)) return;
        std::vector<SystemBattlePassLevel> levels;
        BaseLib::Database->GetSystemBattlePassLevels(season.season, &levels);
        PlayerBattlePass p{};
        if (!BaseLib::Database->GetPlayerBattlePass(aid, &p) || p.season != season.season) return;

        std::vector<uint32_t> item_ids;
        auto claimed_free = p.claimed_free;
        auto claimed_premium = p.claimed_premium;
        auto tryClaim = [&](const SystemBattlePassLevel& lv)
        {
            if (lv.level < 1 || lv.level > p.level) return;     // only unlocked levels
            uint32_t idx = lv.level - 1;
            if (lv.free_item && !BattlePassDetail::getBit(claimed_free, idx))
            { item_ids.push_back(lv.free_item); BattlePassDetail::setBit(claimed_free, idx); }
            if (p.has_premium && lv.premium_item && !BattlePassDetail::getBit(claimed_premium, idx))
            { item_ids.push_back(lv.premium_item); BattlePassDetail::setBit(claimed_premium, idx); }
        };
        if (mode == 1) { for (const auto& lv : levels) tryClaim(lv); }
        else { for (const auto& lv : levels) if (lv.level == target_level) tryClaim(lv); }

        if (item_ids.empty())
        {
            acc_cache.unlock();
            SendBattlePassOnLogin(session, static_cast<uint32_t>(aid));
            return;
        }

        DatabaseUpdateCtx dctx{ .sid = session_id, .aid = aid };
        auto crafted = main_server->CraftInventoryItems(acc_cache, item_ids, Items::Origin::From_Event);
        if (!crafted.has_value())
        {
            DEBUGLOG(dark_cyan, "battlepass claim: inventory full for ({})", acc_cache->acc_info.Nickname.c_str());
            return;
        }
        dctx.ops.push_back(std::move(crafted.value()));
        PlayerBattlePassPatch patch{};
        patch.claimed_free = claimed_free;
        patch.claimed_premium = claimed_premium;
        dctx.ops.push_back(patch);

        auto validated = main_server->ValidateDatabaseUpdates(acc_cache, dctx);
        if (!validated.has_value()) return;
        acc_cache.unlock();

        [[maybe_unused]] auto ignored = BaseLib::DbPool->submit_task(
            [main_server, session = std::move(callback.session), s_id = session_id,
             v = std::move(validated.value())]() mutable
        {
            if (!session) return;
            ResultDbUpdateInfo dbres;
            if (!BaseLib::Database->UpdateAccount(v, dbres).has_value()) return;
            auto nac = CAccount.get<unique_t>(s_id);
            (void)main_server->ApplyDatabaseUpdates(nac, v);
            nac.unlock();
            SendBattlePassOnLogin(session, static_cast<uint32_t>(v.aid));
        });
    }

    // order 184 — reroll the active mission for MP (cost = base * (reset_count + 1)).
    inline void BattlePassReset(SCallbackData& callback, CMainServer* main_server)
    {
        auto session = callback.session;
        auto message = callback.message;
        if (!session || !message) return;
        auto session_id = session->GetSessionId();
        auto acc_cache = CAccount.get<unique_t>(session_id);
        auto aid = acc_cache->acc_info.Index;
        if (aid == -1) return;

        SystemBattlePassSeason season{};
        if (!BaseLib::Database->GetActiveBattlePassSeason(&season)) return;
        std::vector<SystemBattlePassMission> missions;
        BaseLib::Database->GetSystemBattlePassMissions(&missions);
        PlayerBattlePass p{};
        if (!BaseLib::Database->GetPlayerBattlePass(aid, &p) || p.season != season.season) return;

        uint32_t cost = season.reset_base_cost * (p.reset_count + 1);
        uint32_t new_mission = BattlePassDetail::pickMission(missions, p.current_mission_id);

        DatabaseUpdateCtx dctx{ .sid = session_id, .aid = aid };
        using enum CurrencyType;
        if (cost > 0)
            dctx.ops.emplace_back(AccountCurrencyDelta{ .type = MP, .value = cost, .is_reward = false });
        PlayerBattlePassPatch patch{};
        patch.current_mission_id = new_mission;
        patch.mission_progress = 0u;
        patch.reset_count = p.reset_count + 1;
        dctx.ops.push_back(patch);

        auto validated = main_server->ValidateDatabaseUpdates(acc_cache, dctx);
        if (!validated.has_value())
        {
            DEBUGLOG(dark_cyan, "battlepass reset: not enough MP for ({})", acc_cache->acc_info.Nickname.c_str());
            return;
        }
        acc_cache.unlock();

        [[maybe_unused]] auto ignored = BaseLib::DbPool->submit_task(
            [main_server, session = std::move(callback.session), s_id = session_id,
             v = std::move(validated.value())]() mutable
        {
            if (!session) return;
            ResultDbUpdateInfo dbres;
            if (!BaseLib::Database->UpdateAccount(v, dbres).has_value()) return;
            auto nac = CAccount.get<unique_t>(s_id);
            (void)main_server->ApplyDatabaseUpdates(nac, v);
            nac.unlock();
            SendBattlePassOnLogin(session, static_cast<uint32_t>(v.aid));
        });
    }
}
