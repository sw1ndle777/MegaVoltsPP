#pragma once
namespace Game::Handlers
{
    using namespace BaseLib;
    using namespace NetEngine;

    [[nodiscard]] inline std::string_view ToCombatWeaponLabel(const NetEngine::Packets::Ipc::CastCombatWeaponKind weapon_kind)
    {
        using enum NetEngine::Packets::Ipc::CastCombatWeaponKind;

        switch (weapon_kind)
        {
        case Melee: return "melee";
        case Rifle: return "rifle";
        case Shotgun: return "shotgun";
        case Sniper: return "sniper";
        case Gatling: return "gatling";
        case Bazooka: return "bazooka";
        case Grenade: return "grenade";
        default: return "unknown";
        }
    }

    inline void IpcCastCombatStats(const std::vector<uint8_t>& payload, CMainServer* main_server)
    {
        if (payload.size() < sizeof(NetEngine::Packets::Ipc::CastCombatHitEvent))
        {
            DEBUGLOG(red, "IpcCastCombatStats payload size too small: {}", payload.size());
            return;
        }

        const auto hit = Utility::FromVector<NetEngine::Packets::Ipc::CastCombatHitEvent>(payload);
        DEBUGLOG(dark_cyan,
            "IpcCastCombatStats recv room=({}) attackerSid=({}) victimSid=({}) dmg=({}) hp=({}) kill=({}) weapon=({}) streak=({}) bestStreak=({}) payloadSize=({})",
            hit.room_id,
            hit.attacker_sid,
            hit.victim_sid,
            hit.damage_raw,
            hit.victim_health_raw,
            static_cast<uint32_t>(hit.kill_confirmed),
            static_cast<uint32_t>(hit.weapon_kind),
            static_cast<uint32_t>(hit.attacker_current_kill_streak),
            static_cast<uint32_t>(hit.attacker_highest_kill_streak),
            payload.size());
        if (!hit.room_id || !hit.attacker_sid || !hit.victim_sid || hit.attacker_sid == hit.victim_sid || !hit.damage_raw)
        {
            DEBUGLOG(yellow,
                "IpcCastCombatStats skip invalid hit room=({}) attackerSid=({}) victimSid=({}) dmg=({})",
                hit.room_id,
                hit.attacker_sid,
                hit.victim_sid,
                hit.damage_raw);
            return;
        }
        if (!CRoom.contains(hit.room_id))
        {
            DEBUGLOG(yellow, "IpcCastCombatStats skip missing room room=({})", hit.room_id);
            return;
        }

        auto room = CRoom.get<unique_t>(hit.room_id);
        if (!room)
        {
            DEBUGLOG(yellow, "IpcCastCombatStats skip missing room room=({})", hit.room_id);
            return;
        }

        auto attacker = CAccount.get<shared_t>(hit.attacker_sid);
        auto victim = CAccount.get<shared_t>(hit.victim_sid);
        if (!attacker || !victim)
        {
            DEBUGLOG(yellow,
                "IpcCastCombatStats skip missing player attackerExists=({}) victimExists=({}) attackerSid=({}) victimSid=({})",
                attacker ? "true" : "false",
                victim ? "true" : "false",
                hit.attacker_sid,
                hit.victim_sid);
            return;
        }
        if (!attacker->in_room || !victim->in_room || attacker->room_id != hit.room_id || victim->room_id != hit.room_id)
        {
            DEBUGLOG(yellow,
                "IpcCastCombatStats skip room mismatch attackerInRoom=({}) victimInRoom=({}) attackerRoom=({}) victimRoom=({}) expectedRoom=({})",
                attacker->in_room ? "true" : "false",
                victim->in_room ? "true" : "false",
                attacker->room_id,
                victim->room_id,
                hit.room_id);
            return;
        }

        const bool any_player_playing = attacker->playing || victim->playing;
        if (!room->is_playing)
        {
            if (!any_player_playing)
            {
                DEBUGLOG(yellow,
                    "IpcCastCombatStats skip room not playing room=({}) attackerPlaying=({}) victimPlaying=({})",
                    hit.room_id,
                    attacker->playing ? "true" : "false",
                    victim->playing ? "true" : "false");
                return;
            }

            DEBUGLOG(yellow,
                "IpcCastCombatStats room=({}) was marked not playing while active players are fighting, accepting hit and restoring playing state",
                hit.room_id);
            room->is_playing = true;
        }

        attacker.unlock();
        victim.unlock();

        if (auto victim_update = CAccount.get<unique_t>(hit.victim_sid))
        {
            const auto max_health = victim_update->max_health;
            victim_update->current_health = max_health ? std::min(hit.victim_health_raw, max_health) : hit.victim_health_raw;
        }

		auto attacker_acc = CAccount.get<shared_t>(hit.attacker_sid);
		auto attacker_nick = attacker_acc ? attacker_acc->acc_info.Nickname : "???";
		attacker_acc.unlock();

		auto victim_acc = CAccount.get<shared_t>(hit.victim_sid);
		auto victim_nick = victim_acc ? victim_acc->acc_info.Nickname : "???";
		victim_acc.unlock();

        auto& stats = room->match_combat_stats[hit.attacker_sid];
        stats.damage_dealt_raw += hit.damage_raw;
        if (hit.kill_confirmed)
        {
            ++stats.packet_kills;
            stats.highest_kill_streak = std::max<uint32_t>(stats.highest_kill_streak, hit.attacker_highest_kill_streak);
        }

		DEBUGLOG(green, "[Combat Logs] Room {}, {} (sid {}) [{}] hit {} (sid {}) for {} dmg, ({} hp left) - K: {} - BestStreak: {}",
                        hit.room_id, attacker_nick, hit.attacker_sid, ToCombatWeaponLabel(hit.weapon_kind), victim_nick, hit.victim_sid, hit.damage_raw, hit.victim_health_raw, stats.packet_kills, stats.highest_kill_streak);
    }
}
