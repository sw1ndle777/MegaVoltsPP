#pragma once
namespace Game::Handlers
{
    using namespace BaseLib;
    using namespace NetEngine;

    inline constexpr uint32_t kCastDefaultHealthRaw = 1000;
    using CombatWeaponKind = NetEngine::Packets::Ipc::CastCombatWeaponKind;

    inline void UpdateVictimHealthAndSendCombatIpc(CCastServer* server,
        const uint16_t room_id,
        const uint16_t attacker_sid,
        const uint16_t victim_sid,
        const uint32_t new_health_raw,
        const CombatWeaponKind weapon_kind = CombatWeaponKind::Unknown)
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
        victim_acc->is_dead = kill_confirmed;
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

        NetEngine::Packets::Ipc::CastCombatHitEvent event{};
        event.room_id = room_id;
        event.attacker_sid = attacker_sid;
        event.victim_sid = victim_sid;
        event.damage_raw = previous_health_raw - new_health_raw;
        event.victim_health_raw = new_health_raw;
        event.kill_confirmed = static_cast<uint8_t>(kill_confirmed);
        event.weapon_kind = weapon_kind;

        victim_acc.unlock();

        if (kill_confirmed)
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

        DEBUGLOG(green, "CombatIpc send room=({}) attackerSid=({}) victimSid=({}) prevHp=({}) newHp=({}) dmg=({}) kill=({}) weapon=({})",
            room_id,
            attacker_sid,
            victim_sid,
            previous_health_raw,
            new_health_raw,
            event.damage_raw,
            static_cast<uint32_t>(event.kill_confirmed),
            static_cast<uint32_t>(weapon_kind));

        if (kill_confirmed)
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
