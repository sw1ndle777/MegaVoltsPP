#pragma once
namespace Game::Handlers
{
    using namespace BaseLib;
    using namespace NetEngine;

    inline constexpr uint32_t kCastDefaultHealthRaw = 1000;
    using CombatWeaponKind = NetEngine::Packets::Ipc::CastCombatWeaponKind;

    // A kill is attributed to whoever last damaged the victim within this many seconds
    // before the (host-authoritative) death. Older damage is treated as a suicide /
    // environmental death and no kill is credited to anyone.
    inline constexpr uint64_t kKillAttributionWindowSeconds = 10;

    // Send an authoritative "kill" to Main (reuses CastCombatHitEvent with kill_confirmed=1
    // and zero damage) and bump the attacker's kill streak. Called at the death event so the
    // Cast count tracks real deaths — letting Main's end-match mismatch check flag tampering.
    inline void CreditKill(CCastServer* server, const uint16_t room_id,
        const uint16_t attacker_sid, const uint16_t victim_sid,
        const CombatWeaponKind weapon_kind = CombatWeaponKind::Unknown)
    {
        if (!server || !room_id || !attacker_sid || !victim_sid || attacker_sid == victim_sid)
            return;

        NetEngine::Packets::Ipc::CastCombatHitEvent event{};
        event.room_id = room_id;
        event.attacker_sid = attacker_sid;
        event.victim_sid = victim_sid;
        event.damage_raw = 0;
        event.victim_health_raw = 0;
        event.kill_confirmed = 1;
        event.weapon_kind = weapon_kind;

        if (auto attacker_acc = CAccount.get<unique_t>(attacker_sid))
        {
            if (attacker_acc->current_kill_streak < UINT16_MAX)
                ++attacker_acc->current_kill_streak;
            attacker_acc->highest_kill_streak = std::max(attacker_acc->highest_kill_streak, attacker_acc->current_kill_streak);
            event.attacker_current_kill_streak = attacker_acc->current_kill_streak;
            event.attacker_highest_kill_streak = attacker_acc->highest_kill_streak;
            attacker_acc.unlock();
        }

        DEBUGLOG(green, "CombatIpc credit-kill room=({}) attackerSid=({}) victimSid=({}) streak=({})",
            room_id, attacker_sid, victim_sid, static_cast<uint32_t>(event.attacker_current_kill_streak));
        server->SendMainIpc(PacketIds::Ipc::CastToMainMatchCombatHit, Utility::ToVector(event));
    }

    inline void UpdateVictimHealthAndSendCombatIpc(CCastServer* server,
        const uint16_t room_id,
        const uint16_t attacker_sid,
        const uint16_t victim_sid,
        const uint32_t new_health_raw,
        const CombatWeaponKind weapon_kind = CombatWeaponKind::Unknown,
        const uint32_t bodypart = 0,
        const uint8_t hit_variant = 0xFF)
    {
        if (!server || !room_id || !attacker_sid || !victim_sid)
        {
            DEBUGLOG(yellow, "CombatIpc skip invalid args room=({}) attackerSid=({}) victimSid=({}) hp=({}) weapon=({})",
                room_id,
                attacker_sid,
                victim_sid,
                new_health_raw,
                static_cast<uint32_t>(weapon_kind));
            return;
        }

        auto victim_acc = CAccount.get<unique_t>(victim_sid);
        if (!victim_acc)
        {
            DEBUGLOG(yellow, "CombatIpc skip victim missing room=({}) attackerSid=({}) victimSid=({}) hp=({}) weapon=({})",
                room_id,
                attacker_sid,
                victim_sid,
                new_health_raw,
                static_cast<uint32_t>(weapon_kind));
            return;
        }

        const auto previous_health_raw = victim_acc->combat_health;
        const auto had_previous_health = victim_acc->combat_health_known;
        const auto kill_confirmed = previous_health_raw > 0 && new_health_raw == 0;
        victim_acc->current_health = new_health_raw;
        victim_acc->health = new_health_raw;
        victim_acc->combat_health = new_health_raw;
        victim_acc->combat_health_known = true;
        // Death STATE is host-authoritative (Status.h m_bIsDead). We intentionally do NOT
        // set is_dead from Cast-computed health here — Cast health is for ADR/damage only.
        if (kill_confirmed)
            victim_acc->current_kill_streak = 0;

        if (attacker_sid == victim_sid)
        {
            DEBUGLOG(yellow, "CombatIpc skip self-hit room=({}) attackerSid=({}) victimSid=({}) prevHp=({}) newHp=({}) weapon=({})",
                room_id,
                attacker_sid,
                victim_sid,
                previous_health_raw,
                new_health_raw,
                static_cast<uint32_t>(weapon_kind));
            return;
        }

        if (previous_health_raw == 0)
        {
            DEBUGLOG(yellow, "CombatIpc skip zero previous hp room=({}) attackerSid=({}) victimSid=({}) newHp=({}) weapon=({}) hadPrev=({})",
                room_id,
                attacker_sid,
                victim_sid,
                new_health_raw,
                static_cast<uint32_t>(weapon_kind),
                had_previous_health ? "true" : "false");
            return;
        }

        if (previous_health_raw <= new_health_raw)
        {
            DEBUGLOG(yellow, "CombatIpc skip non-damage room=({}) attackerSid=({}) victimSid=({}) prevHp=({}) newHp=({}) weapon=({}) hadPrev=({})",
                room_id,
                attacker_sid,
                victim_sid,
                previous_health_raw,
                new_health_raw,
                static_cast<uint32_t>(weapon_kind),
                had_previous_health ? "true" : "false");
            return;
        }

        if (!had_previous_health)
        {
            DEBUGLOG(yellow, "CombatIpc skip first hit cache warmup room=({}) attackerSid=({}) victimSid=({}) weapon=({}) hp=({})",
                room_id,
                attacker_sid,
                victim_sid,
                static_cast<uint32_t>(weapon_kind),
                new_health_raw);
            return;
        }

        // Record who last damaged this victim for kill attribution at the (host-authoritative)
        // death. If the client already reported health hitting 0, credit the kill here too,
        // deduped so the death's Status packet won't double-count it.
        victim_acc->last_attacker_sid = attacker_sid;
        victim_acc->last_damage_time = Utility::GetUtcTimeNow64();
        const bool credit_kill = kill_confirmed && !victim_acc->kill_credited_this_death;
        if (credit_kill)
            victim_acc->kill_credited_this_death = true;

        NetEngine::Packets::Ipc::CastCombatHitEvent event{};
        event.room_id = room_id;
        event.attacker_sid = attacker_sid;
        event.victim_sid = victim_sid;
        event.damage_raw = previous_health_raw - new_health_raw;
        event.victim_health_raw = new_health_raw;
        event.kill_confirmed = static_cast<uint8_t>(credit_kill);
        event.weapon_kind = weapon_kind;
        event.bodypart = static_cast<uint8_t>(bodypart);
        event.hit_variant = hit_variant;

        victim_acc.unlock();

        if (credit_kill)
        {
            auto attacker_acc = CAccount.get<unique_t>(attacker_sid);
            if (attacker_acc)
            {
                if (attacker_acc->current_kill_streak < UINT16_MAX)
                    ++attacker_acc->current_kill_streak;

                attacker_acc->highest_kill_streak = std::max(attacker_acc->highest_kill_streak, attacker_acc->current_kill_streak);
                event.attacker_current_kill_streak = attacker_acc->current_kill_streak;
                event.attacker_highest_kill_streak = attacker_acc->highest_kill_streak;
            }
        }

        DEBUGLOG(green, "CombatIpc send room=({}) attackerSid=({}) victimSid=({}) prevHp=({}) newHp=({}) dmg=({}) kill=({}) weapon=({}) bodypart=({})",
            room_id,
            attacker_sid,
            victim_sid,
            previous_health_raw,
            new_health_raw,
            event.damage_raw,
            static_cast<uint32_t>(event.kill_confirmed),
            static_cast<uint32_t>(weapon_kind),
            bodypart);

        if (credit_kill)
        {
            DEBUGLOG(green,
                "CombatIpc streak room=({}) attackerSid=({}) current=({}) highest=({}) victimSid=({})",
                room_id,
                attacker_sid,
                static_cast<uint32_t>(event.attacker_current_kill_streak),
                static_cast<uint32_t>(event.attacker_highest_kill_streak),
                victim_sid);
        }
        server->SendMainIpc(PacketIds::Ipc::CastToMainMatchCombatHit, Utility::ToVector(event));
    }
}
