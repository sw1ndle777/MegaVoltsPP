#pragma once
#include <BaseLib/CLogging.h>

namespace Game::Handlers
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Main;
    
    boost::unordered_flat_map<uint32_t, std::pair<uint32_t, uint32_t>> boss_rewards =
    {
        {0, {4801002, 50}}, // bronze 50%
        {1, {4801001, 35}}, // silver 35%
        {2, {4801000, 10}}, // gold 10%
        {3, {4801003, 5}}   // diamond 5%
    };
    
    uint32_t get_random_boss_reward()
    {
        uint32_t total_weight = 0;
        for (const auto& [key, reward] : boss_rewards)
            total_weight += reward.second;

        auto random_value = Utility::Random::CustomGen(1, total_weight);

        uint32_t cumulative_weight = 0;
        for (const auto& [key, reward] : boss_rewards)
        {
            cumulative_weight += reward.second;
            if (random_value <= cumulative_weight)
                return reward.first;
        }
        return boss_rewards.begin()->second.first;
    }
    
    using Utility::CPacketTask::PriorityLevel;

    struct Leader { uint32_t sid = 0; int32_t pts = 0; };
    struct LeaderDerived
    {
        int32_t base_mvp = 0;
        int32_t kd_rank = 0;
        int32_t heads = 0;
        int32_t assists = 0;
        int32_t streak = 0;
        int32_t boom = 0;
        int32_t ctb_cap = 0;
        int32_t bomb_won = 0;
        int32_t arms_pts = 0;
        int32_t zombie = 0;
    };
    
    using ClientInfoVariant = std::variant<
        MainRoomEndMatchClientInfo,
        MainRoomEndMatchClientBossBattleInfo
    >;
    
    struct ClientInfo
    {
        uint32_t unique_id;
        ClientInfoVariant info;
    };
    
    using EndMatchResponse = std::variant<
        MainRoomEndMatchResponse,
        MainRoomEndMatchResponseBossBattle
    >;

    // ========== ADD THIS TRACKING STRUCTURE ==========
    struct PlayerRewardTracking
    {
        uint16_t sid;
        int32_t aid;
        uint32_t mp_reward;
        uint32_t exp_reward;
        uint32_t energy_reward;
        uint32_t reward_item_id;
        uint64_t mp_before;
        uint64_t energy_before;
        bool is_boss_battle;
        bool is_tutorial;
        bool is_story;
        uint32_t story_item_id;
        uint32_t tutorial_item_id;
        bool had_level_up;
        std::optional<ShopItem> level_up_item;
    };
    
    struct EndMatchCtx
    {
        CMainServer* main;
        Utility::CPacketTask::BucketQueue& packets;
        uint16_t order{ 0 };
        uint8_t mission{ 0 }, extra{ 0 }, option{ 0 };
        AccCacheResource& host;
        uint8_t* data;
        uint32_t dataSize;
        std::vector<DatabaseUpdateCtx>& dctxs;
        std::vector<uint16_t>& all_ss;
        std::vector<BossItem>& pve_rewards;
        boost::unordered_flat_map<uint32_t, ClientInfo>& cli;
        uint32_t room_id = 0;
        Leader& ffa_winner, & most_captures, & most_wonrounds, & best_kd;
        Leader& mvp, & entryFragger, & bullseye, & support, & bomba;
        Leader& best_zombie, & best_arms;
        std::vector<PlayerRewardTracking>& tracking; // ← ADD THIS

        [[nodiscard]] uint64_t now64() const { return Utility::GetUtcTimeNow64(); }
        auto bump_score(Leader& L, uint32_t sid, int32_t pts) { if (pts > L.pts) { L.sid = sid; L.pts = pts; } }
        template<class... Ids>
        auto room_exists() const { return CRoom.contains(room_id); }
        auto same_room(AccCacheResource& acc) const { return acc->in_room && acc->room_id == room_id; }
        auto all_players(RoomCacheResource& room) const { return main->GetRoomSortedPlayerSessionIds(room); }
        auto playing_players(RoomCacheResource& room) const { return main->GetRoomSortedPlayerPlayingWithoutObserverSessionIds(room); }
        auto is_cw(RoomCacheResource& room) const { return room->is_clan_room; }
        auto clanId1(RoomCacheResource& room) const { return room->clan_id_1; }
        auto clanId2(RoomCacheResource& room) const { return room->clan_id_2; }
        auto is_pve(RoomCacheResource& room) const { return room->ModeIndex == NetEngine::Room::Mode::Index::BossBattle; }
        auto enough_playtime(AccCacheResource& acc, uint64_t playtime) const
        {
            DEBUGLOG(dark_cyan, "player playtime seconds: ({}), required min seconds: ({})", playtime, main->GetPlaytimeMinSeconds());
            return playtime >= main->GetPlaytimeMinSeconds();
        }
        auto is_observer(AccCacheResource& acc) const { return acc->team_id == NetEngine::Team::IdType::Observer; }
        auto no_kill(MainRoomEndMatchClientInfo& info) const { return info.total_kills == 0 && info.assists == 0 && info.deaths == 0; }
        auto no_rewards(RoomCacheResource& room, AccCacheResource& acc, ClientInfo& info, uint64_t playtime) const
        {
            if (is_observer(acc) || !enough_playtime(acc, playtime))
            {
                DEBUGLOG(dark_cyan, "no rewards for observer or not enough playtime");
                return true;
            }
            if (is_pve(room))
            {
                auto* boss = std::get_if<MainRoomEndMatchClientBossBattleInfo>(&info.info);
                DEBUGLOG(dark_cyan, "pve no rewards check, boss state: ({})", boss->pve007_state);
                return !boss || boss->pve007_state == 5;
            }
            DEBUGLOG(dark_cyan, "pvp no rewards check");
            auto* pvp = std::get_if<MainRoomEndMatchClientInfo>(&info.info);
            return pvp && no_kill(*pvp);
        }
        auto add_bonuses(AccCacheResource& acc, uint32_t& exp, uint32_t& point) const
        {
            auto extra_procent_exp = 0.f, extra_procent_point = 0.f;
            for (auto& item : acc->inventory_items)
            {
                if (item.is_equipped == 1 && item.character_id == static_cast<uint8_t>(acc->acc_info.SelectedCharacter))
                {
                    auto item_info = CItemsInfo.get<shared_t>(item.item_info.item_number.item_id);
                    auto bonus_ef_id = item_info->BonusEffectId;
                    if (bonus_ef_id)
                    {
                        auto bonus_ef_info = CEffectInfo.get<shared_t>(bonus_ef_id);
                        DEBUGLOG(dark_cyan, "found effect info key({}) value({}) for itemid({})",
                            bonus_ef_info->key, bonus_ef_info->valueA, static_cast<uint32_t>(item.item_info.item_number.item_id));

                        (bonus_ef_info->key == 122) ? extra_procent_exp += bonus_ef_info->valueA / 10.f : 0;
                        (bonus_ef_info->key == 123) ? extra_procent_point += bonus_ef_info->valueA / 10.f : 0;
                    }
                }
            }
            extra_procent_exp /= 100.f;
            extra_procent_point /= 100.f;
            DEBUGLOG(dark_cyan, "player before have exp: ({}), point: ({})", exp, point);
            DEBUGLOG(dark_cyan, "player benefit of bonus exp procent: ({}) and bonus point procent: ({})", extra_procent_exp, extra_procent_point);
            exp += static_cast<uint32_t>(extra_procent_exp * exp);
            point += static_cast<uint32_t>(extra_procent_point * point);
            DEBUGLOG(dark_cyan, "player now will get exp: ({}), point: ({})", exp, point);
            if (acc->acc_info.PCRoom > 1)
            {
                DEBUGLOG(dark_cyan, "pcroom state with bonus exp and point enabled");
                exp += std::ceil(0.28 * exp);
                point += std::floor(0.47 * point);
            }
        }
        auto calc_xpmp(RoomCacheResource& room, AccCacheResource& acc, const MainRoomEndMatchClientInfo& m) const
        {
            auto gamemode = room->ModeIndex;
            auto ri = CRewardsInfo.get<shared_t>(gamemode);
            uint32_t exp = ri->ExpBase, pt = ri->PointBase;
            auto add_basic = [&]()
            {
                exp += (m.total_kills * ri->ExpKill) + (m.deaths * ri->ExpDeath) + (m.assists * ri->ExpAssist);
                pt += (m.total_kills * ri->PointKill) - (m.deaths * ri->PointDeath) + (m.assists * ri->PointAssist);
            };
            auto add_ctb = [&]()
            {
                auto TeamCaps = m.missionWin, MyCaps = m.mission;
                exp += (TeamCaps * ri->ExpMission) + (MyCaps * ri->ExpMissionWin);
                pt += (TeamCaps * ri->PointMission) + (MyCaps * ri->PointMissionWin);
            };
            auto add_bmb = [&]()
            {
                auto RoundWin = m.missionWin;
                exp += (RoundWin * ri->ExpMissionWin);
                pt += (RoundWin * ri->PointMissionWin);
            };
            auto add_zombie = [&]()
            {
                auto ZombiKill = m.killstreak, Infected = m.melee_kills, Survived = m.missionWin;
                exp += (ZombiKill * ri->ExpModeKill) + (Infected * ri->ExpKill) + (Survived * ri->ExpMissionWin);
                pt += (ZombiKill * ri->PointModeKill) + (Infected * ri->PointKill) + (Survived * ri->PointMissionWin);
            };
            auto add_armsrace = [&]()
            {
                auto Mission = m.mission;
                exp += (Mission * ri->ExpMission);
                pt += (Mission * ri->PointMission);
            };
            auto normalize = [&]()
            {
                exp = std::max(ri->ExpBase, exp);
                pt = std::max(ri->PointBase, pt);
                exp = std::min(ri->ExpMax, exp);
                pt = std::min(ri->ExpMax, pt);
            };
            switch (gamemode)
            {
            case NetEngine::Room::Mode::Index::CLAN_Elimination:
            case NetEngine::Room::Mode::Index::CLAN_TeamDeathMatch:
            case NetEngine::Room::Mode::Index::TeamDeathMatch:
            case NetEngine::Room::Mode::Index::FreeForAll:
            case NetEngine::Room::Mode::Index::ItemMatch:
            case NetEngine::Room::Mode::Index::Elimination:
            case NetEngine::Room::Mode::Index::SuperItemMatch:
            {
                add_basic();
                normalize();
                break;
            }
            case NetEngine::Room::Mode::Index::CLAN_CaptureTheBattery:
            case NetEngine::Room::Mode::Index::CaptureTheBattery:
            {
                add_basic();
                add_ctb();
                normalize();
                break;
            }
            case NetEngine::Room::Mode::Index::BombBattle:
            {
                add_basic();
                add_bmb();
                normalize();
                break;
            }
            case NetEngine::Room::Mode::Index::ZombieMode:
            {
                add_basic();
                add_zombie();
                normalize();
                break;
            }
            case NetEngine::Room::Mode::Index::ArmsRace:
            {
                add_basic();
                add_armsrace();
                normalize();
                break;
            }
            default:
            {
                DEBUGLOG(dark_cyan, "unknown mod id, no reward");
                break;
            }
            }
            add_bonuses(acc, exp, pt);
            return std::make_pair(exp, pt);
        }
        EndMatchResponse get_rewards(RoomCacheResource& room, AccCacheResource& acc, ClientInfo& info, bool no_reward, uint32_t& outMpReward, uint32_t& outExpReward, uint32_t& outEnergyReward, uint32_t& outItemReward) const
        {
            const auto tmpMp = acc->acc_info.MicroPoints;
            const auto tmpExp = acc->acc_info.Experience;
            const auto is_boss = is_pve(room);
            if (is_boss)
            {
                outItemReward = no_reward ? 0 : get_random_boss_reward();
                return MainRoomEndMatchResponseBossBattle(tmpMp, tmpExp, outItemReward);
            }
            const auto* pvp = std::get_if<MainRoomEndMatchClientInfo>(&info.info);
            MainRoomEndMatchResponse r{};
            auto tmpEnergy = acc->earnt_battery;
            auto capacityLeft = acc->acc_info.MaximumEnergy - acc->acc_info.Energy;
            if (tmpEnergy > capacityLeft) tmpEnergy = capacityLeft;
            acc->earnt_battery = 0;
            outEnergyReward = 0;
            r.total_mp = tmpMp;
            r.total_xp = tmpExp;
            r.unique_id = info.unique_id;
            if (no_reward) return r;
            auto [exp, pt] = calc_xpmp(room, acc, *pvp);
            outEnergyReward = tmpEnergy;
            r.melee_kills = pvp->melee_kills;
            r.rifle_kills = pvp->rifle_kills;
            r.shotgun_kills = pvp->shotgun_kills;
            r.sniper_kills = pvp->sniper_kills;
            r.gatling_kills = pvp->gatling_kills;
            r.bazooka_kills = pvp->bazooka_kills;
            r.grenade_kills = pvp->grenade_kills;
            r.killstreak = pvp->killstreak;
            r.total_kills = pvp->total_kills;
            r.deaths = pvp->deaths;
            r.headshots = pvp->headshots;
            r.assists = pvp->assists;
            r.mission = pvp->mission;
            r.missionWin = pvp->missionWin;
            r.unknown3 = pvp->unknown3;
            r.unknown4 = pvp->unknown4;
            r.total_mp += pt;
            r.total_xp += exp;
            outMpReward = pt;
            outExpReward = exp;
            return r;
        }
        DatabaseUpdateCtx make_dctx(AccCacheResource& acc) const { return DatabaseUpdateCtx{ .sid = acc->session_id, .aid = acc->acc_info.Index }; }
    };

    inline void StoryMod(EndMatchCtx& ctx)
    {
        auto story_episode = *reinterpret_cast<uint8_t*>(ctx.data);
        auto current_story = ctx.host->acc_info.Story;
        if (current_story + 1 != story_episode) return;
        
        // ========== TRACK STORY REWARD ==========
        PlayerRewardTracking track;
        track.sid = ctx.host->session_id;
        track.aid = ctx.host->acc_info.Index;
        track.is_story = true;
        track.story_item_id = 4600100;
        track.mp_before = ctx.host->acc_info.MicroPoints;
        track.energy_before = ctx.host->acc_info.Energy;
        
        auto dctx = ctx.make_dctx(ctx.host);
        dctx.ops.emplace_back(AccountInfoPatch{ .story = story_episode });
        
        if (auto crafted = ctx.main->CraftInventoryItems(ctx.host, { 4600100 }, Items::Origin::From_Game); crafted.has_value())
        {
            auto& item = crafted.value().items[0];
            auto item_info = CItemsInfo.get<shared_t>(item.item_info.item_number.item_id);
            ShopItem new_item = { {item.item_info.item_number.item_id, item_info->Stock}, ItemExpire::Type::Unused, item.item_info.serial_info };
            ctx.packets.enqueue(ctx.host->session_id, NetEngine::Protocols::SCommandHeader(66, 3, 51, 0), reinterpret_cast<uint8_t*>(&new_item), sizeof(ShopItem), PriorityLevel::Highest);
            dctx.ops.push_back(crafted.value());
            ctx.dctxs.push_back(std::move(dctx));
            ctx.tracking.push_back(std::move(track));
        }
        else
            DEBUGLOG(red, "CraftInventoryItems failed for player [{}] [{}]: {}",
                ctx.host->acc_info.Index, ctx.host->acc_info.Nickname.c_str(),
                static_cast<int>(crafted.error()));

        ctx.host.unlock();
    }
    
    inline void Tutorial(EndMatchCtx& ctx)
    {
        if (ctx.host->acc_info.Tutorial) return;
        
        // ========== TRACK TUTORIAL REWARD ==========
        PlayerRewardTracking track;
        track.sid = ctx.host->session_id;
        track.aid = ctx.host->acc_info.Index;
        track.is_tutorial = true;
        track.tutorial_item_id = 4500000;
        track.mp_before = ctx.host->acc_info.MicroPoints;
        track.energy_before = ctx.host->acc_info.Energy;
        track.mp_reward = 20000;
        
        auto dctx = ctx.make_dctx(ctx.host);
        dctx.ops.emplace_back(AccountInfoPatch{ .bTutorial = true });
        
        if (auto crafted = ctx.main->CraftInventoryItems(ctx.host, { 4500000 }, Items::Origin::From_Game); crafted.has_value())
        {
            auto& item = crafted.value().items[0];
            auto item_info = CItemsInfo.get<shared_t>(item.item_info.item_number.item_id);
            ShopItem new_item = { {item.item_info.item_number.item_id, item_info->Stock}, ItemExpire::Type::Unused, item.item_info.serial_info };
            ctx.packets.enqueue(ctx.host->session_id, NetEngine::Protocols::SCommandHeader(99, 0, 37, 0), reinterpret_cast<uint8_t*>(&new_item), sizeof(ShopItem), PriorityLevel::Highest);
            dctx.ops.push_back(crafted.value());
            
            using enum CurrencyType;
            dctx.ops.emplace_back(AccountCurrencyDelta{ .type = MP, .value = 20000, .is_reward = true });
            MainCurrencyUpdateAck currency_update_data = { ctx.host->acc_info.RockTokens, ctx.host->acc_info.MicroPoints + 20000, ctx.host->acc_info.Coins };
            ctx.packets.enqueue(ctx.host->session_id, NetEngine::Protocols::SCommandHeader(307, 0, 0, 0), reinterpret_cast<uint8_t*>(&currency_update_data), sizeof(currency_update_data), PriorityLevel::Highest);
            ctx.dctxs.push_back(std::move(dctx));
            ctx.tracking.push_back(std::move(track));
        }
        else
            DEBUGLOG(red, "CraftInventoryItems failed for player [{}] [{}]: {}",
                ctx.host->acc_info.Index, ctx.host->acc_info.Nickname.c_str(),
                static_cast<int>(crafted.error()));
        
        ctx.host.unlock();
    }
    
    inline void SingleWave(EndMatchCtx& ctx)
    {
        auto req = reinterpret_cast<SingleWaveEndReq*>(ctx.data);
        auto end_single_wave_time = Utility::GetUtcTimeNowInSeconds();
        auto playtime_seconds = end_single_wave_time - ctx.host->match_loaded_time;
        auto dctx = ctx.make_dctx(ctx.host);
        auto is_hard = req->type == 2;
        dctx.ops.emplace_back(AccountInfoPatch{
            .sw_high_score = std::max(ctx.host->acc_info.SingleWaveHighScore, req->score),
            .sw_highest_wave = std::max(ctx.host->acc_info.SingleWaveHighestWave, req->stage),
            .sw_last_update = ctx.now64()
        });
        ctx.dctxs.push_back(std::move(dctx));
        ctx.host.unlock();
    }
    
    inline void SpGameModes(EndMatchCtx& ctx)
    {
        switch (ctx.mission)
        {
        case 0: Tutorial(ctx); break;
        case 1: SingleWave(ctx); break;
        case 2: StoryMod(ctx); break;
        default: break;
        }
    }

    inline void MpGameModes(EndMatchCtx& ctx, uint32_t& gamemodeOut)
    {
        if (!ctx.room_exists() || !ctx.same_room(ctx.host)) return;
        auto room = CRoom.get<unique_t>(ctx.host->room_id);
        ctx.host.unlock();
        gamemodeOut = room->ModeIndex;
        auto req = reinterpret_cast<MainRoomEndMatchScoreClientInfo*>(ctx.data);
        auto blue_win = req->blue_score > req->red_score;
        auto draw = req->blue_score == req->red_score;
        auto is_cw = ctx.is_cw(room);
        auto cId1 = ctx.clanId1(room);
        auto cId2 = ctx.clanId2(room);
        auto all_players = ctx.all_players(room);
        auto playing_players = ctx.playing_players(room);
        auto end_time = Utility::GetUtcTimeNowInSeconds();
        auto is_zombie = room->ModeIndex == NetEngine::Room::Mode::Index::ZombieMode;
        auto is_ffa = room->ModeIndex == NetEngine::Room::Mode::Index::FreeForAll;
        auto is_bomb = room->ModeIndex == NetEngine::Room::Mode::Index::BombBattle;
        auto is_arms_race = room->ModeIndex == NetEngine::Room::Mode::Index::ArmsRace;
        auto is_ctb = room->ModeIndex == NetEngine::Room::Mode::Index::CaptureTheBattery || room->ModeIndex == NetEngine::Room::Mode::Index::CLAN_CaptureTheBattery;
        auto is_bossbattle = room->ModeIndex == NetEngine::Room::Mode::Index::BossBattle;
        boost::unordered_flat_map<uint16_t, LeaderDerived> derived;

        DEBUGLOG(dark_cyan, "now handle mod id: ({})", static_cast<uint32_t>(room->ModeIndex));
        if (is_cw)
            DEBUGLOG(dark_cyan, "clanwar between ({}) and ({})", cId1, cId2);

        // parse client infos
        for (auto i = 0; i < ctx.option; i++)
        {
            auto boss_i = reinterpret_cast<MainRoomEndMatchClientBossBattleInfo*>(ctx.data + sizeof(MainRoomEndMatchClientBossBattleInfo) * i);
            auto em_i = reinterpret_cast<MainRoomEndMatchClientInfo*>(ctx.data + sizeof(MainRoomEndMatchClientInfo) * i + sizeof(MainRoomEndMatchScoreClientInfo));
            auto uuid = is_bossbattle ? boss_i->unique_id : em_i->unique_id;
            auto cu_i = NetEngine::Packets::Core::UniqueId(uuid);
            const auto cu_session = static_cast<uint16_t>(cu_i.session);
            if (ctx.cli.find(cu_session) != ctx.cli.end()) continue;
            ctx.cli.insert({ cu_session, ClientInfo{.unique_id = uuid, .info = is_bossbattle ? ClientInfoVariant{*boss_i} : ClientInfoVariant{*em_i} } });
            if (cu_i.session == room->host_session_id) continue;
            ctx.packets.enqueue(cu_session, NetEngine::Protocols::SCommandHeader(ctx.order, 0, 0, ctx.option), ctx.data, ctx.dataSize, PriorityLevel::High);
        }

        if (!is_bossbattle)
        {
            // calculate derived stats and leaders
            for (const auto& id : playing_players)
            {
                auto it = ctx.cli.find(id);
                if (it == ctx.cli.end()) continue;
                auto& info = std::get<MainRoomEndMatchClientInfo>(it->second.info);
                auto acc = CAccount.get<unique_t>(id);
                if (!acc->acc_info.Index) { acc.unlock(); continue; }
                if (!ctx.same_room(acc)) { acc.unlock(); continue; }
                auto play_time = end_time - acc->match_loaded_time;
                auto no_rewards = ctx.no_rewards(room, acc, ctx.cli[id], play_time);
                if (no_rewards) { acc.unlock(); continue; }
                DEBUGLOG(dark_cyan, "({}) end info melee: ({}) rifle: ({}) shotgun: ({}) sniper: ({}) gatling: ({}) bazooka: ({}) grenade: ({}) killstreak: ({}) kills: ({}) deaths: ({}) hs: ({}) assist: ({}) mission: ({}) missionWin: ({})",
                    acc->acc_info.Nickname,
                    info.melee_kills,
                    info.rifle_kills,
                    info.shotgun_kills,
                    info.sniper_kills,
                    info.gatling_kills,
                    info.bazooka_kills,
                    info.grenade_kills,
                    info.killstreak,
                    info.total_kills,
                    info.deaths,
                    info.headshots,
                    info.assists,
                    info.mission,
                    info.missionWin);
                acc.unlock();
                LeaderDerived d{};
                d.base_mvp = info.total_kills * 2 + info.assists;
                auto hs_percentage = info.total_kills == 0 ? 0.f : (static_cast<float>(info.headshots) / static_cast<float>(info.total_kills)) * 100.f;
                d.heads = static_cast<int32_t>(hs_percentage);
                d.assists = info.assists;
                d.streak = info.killstreak;
                d.boom = info.bazooka_kills + info.grenade_kills;
                d.ctb_cap = info.mission;
                d.bomb_won = info.missionWin;
                d.arms_pts = info.mission;
                (info.deaths == 0) ?
                    d.kd_rank = std::numeric_limits<int32_t>::max() :
                    d.kd_rank = static_cast<int32_t>((static_cast<int64_t>(info.total_kills) * 10000) / info.deaths);

                if (is_zombie)
                    d.zombie = info.killstreak * 3 + info.melee_kills + info.missionWin;

                if (is_ffa) ctx.bump_score(ctx.ffa_winner, id, info.total_kills);
                if (is_ctb) ctx.bump_score(ctx.most_captures, id, d.ctb_cap);
                if (is_bomb) ctx.bump_score(ctx.most_wonrounds, id, d.bomb_won);
                if (is_arms_race) ctx.bump_score(ctx.best_arms, id, d.arms_pts);
                if (!is_zombie)
                {
                    ctx.bump_score(ctx.bomba, id, d.boom);
                    ctx.bump_score(ctx.support, id, d.assists);
                    ctx.bump_score(ctx.bullseye, id, d.heads);
                    ctx.bump_score(ctx.entryFragger, id, d.streak);
                    ctx.bump_score(ctx.best_kd, id, d.kd_rank);
                }
                else
                    ctx.bump_score(ctx.best_zombie, id, d.zombie);
                derived.emplace(id, std::move(d));
            }
        }

        const std::pair<uint16_t, int32_t> role_bonuses[] =
        {
            {ctx.best_kd.sid, 25},
            {ctx.entryFragger.sid, 20},
            {ctx.bullseye.sid, 15},
            {ctx.support.sid, 10},
            {ctx.bomba.sid, 5},
            {ctx.ffa_winner.sid, 25},
            {ctx.most_captures.sid, 25},
            {ctx.most_wonrounds.sid, 25},
            {ctx.best_zombie.sid, 25},
            {ctx.best_arms.sid, 25},
        };
        
        // calculate rewards and queue db updates
        for (const auto& id : playing_players)
        {
            DEBUGLOG(dark_cyan, "now handle player sid=({})", id);
            if (ctx.cli.find(id) == ctx.cli.end()) continue;
            auto acc = CAccount.get<unique_t>(id);
            if (!acc->acc_info.Index) { acc.unlock(); continue; }
            DEBUGLOG(dark_cyan, "found acc cache for player sid=({})", id);
            if (!ctx.same_room(acc)) { acc.unlock(); continue; }
            DEBUGLOG(dark_cyan, "player sid=({}) is in same room", id);
            auto my_uid = NetEngine::Packets::Core::UniqueId(id, 1).data;
            auto& info = ctx.cli[id];
            auto play_time = end_time - acc->match_loaded_time;
            auto no_rewards = ctx.no_rewards(room, acc, info, play_time);
            uint32_t mp_reward = 0, exp_reward = 0, energy_reward = 0, wins = 0, loses = 0, draws = 0, new_level = 0, reward_item = 0;
            
            // ========== CAPTURE BEFORE VALUES ==========
            PlayerRewardTracking track;
            track.sid = id;
            track.aid = acc->acc_info.Index;
            track.mp_before = acc->acc_info.MicroPoints;
            track.energy_before = acc->acc_info.Energy;
            track.is_boss_battle = is_bossbattle;
            track.is_tutorial = false;
            track.is_story = false;
            track.had_level_up = false;
            
            auto resp = ctx.get_rewards(room, acc, info, no_rewards, mp_reward, exp_reward, energy_reward, reward_item);
            auto won = acc->team_id == NetEngine::Team::IdType::Blue ? blue_win : !blue_win;

            track.mp_reward = mp_reward;
            track.exp_reward = exp_reward;
            track.energy_reward = energy_reward;
            track.reward_item_id = reward_item;

            auto dctx = ctx.make_dctx(acc);
            using enum CurrencyType;
            
            if (!no_rewards && !is_bossbattle)
            {
                if (ctx.ffa_winner.sid == id && is_ffa) won = true;
                const auto it = derived.find(id);
                if (it != derived.end())
                {
                    auto points = it->second.base_mvp;
                    for (auto [sid, bonus] : role_bonuses) if (id == sid) points += bonus;
                    if (points > ctx.mvp.pts) { ctx.mvp.pts = points; ctx.mvp.sid = id; }
                }

                if (energy_reward)
                    dctx.ops.emplace_back(AccountCurrencyDelta{ .type = ENERGY, .value = energy_reward, .is_reward = true });
                if (mp_reward)
                    dctx.ops.emplace_back(AccountCurrencyDelta{ .type = MP, .value = mp_reward, .is_reward = true });
                if (exp_reward)
                {
                    auto level_up = ctx.main->ProcessLevelUp(acc, exp_reward, dctx);
                    if (!level_up.has_value())
                    {
                        DEBUGLOG(red, "ProcessLevelUp failed for player [{}] [{}]: {}", acc->acc_info.Index, acc->acc_info.Nickname.c_str(), static_cast<int>(level_up.error()));
                        return;
                    }
                    auto& level_up_info = level_up.value();
                    auto& ri = level_up_info.reward_item;
                    new_level = level_up_info.new_level;
                    
                    if (ri.has_value())
                    {
                        track.had_level_up = true;
                        track.level_up_item = ri.value();
                        ctx.packets.enqueue(id, NetEngine::Protocols::SCommandHeader(99, 0, 37, 0), reinterpret_cast<uint8_t*>(&ri.value()), sizeof(ShopItem), PriorityLevel::Highest);
                    }

                    if (level_up_info.level_up)
                    {
                        for (auto& other_sid : all_players)
                        {
                            if (other_sid == id) continue;
                            ctx.packets.enqueue(other_sid, NetEngine::Protocols::SCommandHeader(311, 0, 0, static_cast<uint8_t>(new_level + 1)), reinterpret_cast<uint8_t*>(&my_uid), sizeof(my_uid), PriorityLevel::Highest);
                        }
                    }
                }
                const auto* pvp = std::get_if<MainRoomEndMatchResponse>(&resp);

                if (is_zombie)
                    dctx.ops.emplace_back(AccountInfoPatch{
                        .deaths = acc->acc_info.Deaths + pvp->deaths,
                        .headshots = acc->acc_info.Headshots + pvp->headshots,
                        .zombie_kills = acc->acc_info.ZombieKills + pvp->killstreak,
                        .infections = acc->acc_info.Infections + pvp->melee_kills
                    });
                else
                    dctx.ops.emplace_back(AccountInfoPatch{
                        .kills = acc->acc_info.Kills + pvp->total_kills,
                        .deaths = acc->acc_info.Deaths + pvp->deaths,
                        .assists = acc->acc_info.Assists + pvp->assists,
                        .headshots = acc->acc_info.Headshots + pvp->headshots,
                        .highest_kill_streak = std::max<uint32_t>(acc->acc_info.HighestKillStreak, pvp->killstreak),
                        .melee_kills = acc->acc_info.MeleeKills + pvp->melee_kills,
                        .rifle_kills = acc->acc_info.RifleKills + pvp->rifle_kills,
                        .shotgun_kills = acc->acc_info.ShotgunKills + pvp->shotgun_kills,
                        .sniper_kills = acc->acc_info.SniperKills + pvp->sniper_kills,
                        .gatling_kills = acc->acc_info.GatlingKills + pvp->gatling_kills,
                        .bazooka_kills = acc->acc_info.BazookaKills + pvp->bazooka_kills,
                        .grenade_kills = acc->acc_info.GrenadeKills + pvp->grenade_kills,
                    });

                if (draw)
                    dctx.ops.emplace_back(AccountInfoPatch{ .draws = acc->acc_info.Draws + 1 });
                if (won)
                    dctx.ops.emplace_back(AccountInfoPatch{ .wins = acc->acc_info.Wins + 1 });
                else
                    dctx.ops.emplace_back(AccountInfoPatch{ .loses = acc->acc_info.Loses + 1 });

                if (is_cw)
                {
                    uint32_t clan_contribution = won ? 100 : 0;
                    if (id == ctx.mvp.sid) clan_contribution += 50;
                    if (it != derived.end())
                    {
                        clan_contribution += it->second.base_mvp;
                        for (auto [sid, bonus] : role_bonuses) if (id == sid) clan_contribution += bonus;
                    }
                    dctx.ops.emplace_back(AccountInfoPatch{
                        .clan_kills = acc->acc_info.ClanKills + pvp->total_kills,
                        .clan_deaths = acc->acc_info.ClanDeaths + pvp->deaths,
                        .clan_assists = acc->acc_info.ClanAssists + pvp->assists,
                        .clan_contribution = acc->acc_info.ClanContribution + clan_contribution
                    });

                    if (draw)
                        dctx.ops.emplace_back(AccountInfoPatch{ .clan_draws = acc->acc_info.ClanDraws + 1 });
                    if (won)
                        dctx.ops.emplace_back(AccountInfoPatch{ .clan_wins = acc->acc_info.ClanWins + 1 });
                    else
                        dctx.ops.emplace_back(AccountInfoPatch{ .clan_loses = acc->acc_info.ClanLoses + 1 });
                }
                dctx.ops.emplace_back(AccountInfoPatch{ .play_time = acc->acc_info.PlayTime + play_time });
            }

            std::visit([&](const auto& msg)
            {
                using T = std::decay_t<decltype(msg)>;
                ctx.packets.enqueue(id, NetEngine::Protocols::SCommandHeader(ctx.order, 0, 1, 0),
                    reinterpret_cast<const uint8_t*>(&msg),
                    sizeof(T),
                    PriorityLevel::High);
            }, resp);
            
            if (reward_item && is_bossbattle)
            {
                if (auto crafted = ctx.main->CraftInventoryItems(acc, { reward_item }, Items::Origin::From_Game); crafted.has_value())
                {
                    auto& item = crafted.value().items[0];
                    auto item_info = CItemsInfo.get<shared_t>(item.item_info.item_number.item_id);
                    ShopItem new_item = { {item.item_info.item_number.item_id, item_info->Stock}, ItemExpire::Type::Unused, item.item_info.serial_info };
                    ctx.packets.enqueue(id, NetEngine::Protocols::SCommandHeader(99, 0, 37, 0), reinterpret_cast<uint8_t*>(&new_item), sizeof(ShopItem), PriorityLevel::Highest);
                    dctx.ops.push_back(crafted.value());
                    ctx.pve_rewards.push_back({ my_uid, reward_item });
                }
                else
                    DEBUGLOG(red, "CraftInventoryItems failed for player [{}] [{}]: {}",
                        acc->acc_info.Index, acc->acc_info.Nickname.c_str(),
                        static_cast<int>(crafted.error()));
            }
            
            acc->playing = false;
#if defined(RELEASE_1_0_3)
            acc->state = room->host_session_id == id ? PlayerInfo::State::HostReady : PlayerInfo::State::Waiting;
#else
            acc->state = room->host_session_id == id ? PlayerInfo::State::PlayerReady : PlayerInfo::State::Waiting;
#endif

            static constexpr std::size_t kMaxEquippedSlots = 17;
            const auto sel_char = static_cast<uint8_t>(acc->acc_info.SelectedCharacter);
            std::vector<BaseLib::Item> equipped;
            equipped.reserve(kMaxEquippedSlots);
            std::copy_if(acc->inventory_items.begin(), acc->inventory_items.end(), std::back_inserter(equipped), [&](const BaseLib::Item& it)
            {
                return it.is_equipped == 1 && it.character_id == sel_char;
            });
            auto item_id_of = [&](uint8_t type) { return ctx.main->GetItemByType(equipped, type).item_info.item_number.item_id; };
            const uint32_t set_item_id = item_id_of(25);
            auto setinfo = CSetItemsInfo.get<shared_t>(set_item_id);
            auto fallback = [&](uint32_t direct, uint32_t set_field_value)
            {
                return direct ? direct : setinfo->Id;
            };

            const auto hair = item_id_of(0);
            const auto face = item_id_of(1);
            const auto upper = item_id_of(2);
            const auto under = item_id_of(3);
            const auto skirt = item_id_of(4);
            const auto gloves = item_id_of(5);
            const auto boots = item_id_of(6);
            const auto accH = item_id_of(7);
            const auto accW = item_id_of(8);
            const auto accB = item_id_of(9);

            auto EquippedMelee = EquipItemNumber(item_id_of(10), 10);
            auto EquippedRifle = EquipItemNumber(item_id_of(11), 11);
            auto EquippedShotgun = EquipItemNumber(item_id_of(12), 12);
            auto EquippedSniper = EquipItemNumber(item_id_of(13), 13);
            auto EquippedGatling = EquipItemNumber(item_id_of(14), 14);
            auto EquippedBazooka = EquipItemNumber(item_id_of(15), 15);
            auto EquippedGrenade = EquipItemNumber(item_id_of(16), 16);

            auto EquippedHair = EquipItemNumber(fallback(hair, setinfo->Hair), 0);
            auto EquippedFace = EquipItemNumber(fallback(face, setinfo->Face), 1);
            auto EquippedUpper = EquipItemNumber(fallback(upper, setinfo->Upper), 2);
            auto EquippedUnder = EquipItemNumber(fallback(under, setinfo->Under), 3);
            auto EquippedSkirt = EquipItemNumber(fallback(skirt, setinfo->Pants), 4);
            auto EquippedGloves = EquipItemNumber(fallback(gloves, setinfo->Arms), 5);
            auto EquippedBoots = EquipItemNumber(fallback(boots, setinfo->Boots), 6);
            auto EquippedHeadAcc = EquipItemNumber(fallback(accH, setinfo->AccessoryA), 7);
            auto EquippedWaistAcc = EquipItemNumber(fallback(accW, setinfo->AccessoryB), 8);
            auto EquippedBackAcc = EquipItemNumber(fallback(accB, setinfo->AccessoryC), 9);

            const auto* pvp = std::get_if<MainRoomEndMatchClientInfo>(&info.info);
            const uint32_t kills = (!pvp || is_zombie || is_bossbattle) ? 0 : pvp->total_kills;
            const uint32_t deaths = (!pvp || is_bossbattle) ? 0 : pvp->deaths;
            const uint32_t assists = (!pvp || is_zombie || is_bossbattle) ? 0 : pvp->assists;
            const uint32_t headshots = (!pvp || is_bossbattle) ? 0 : pvp->headshots;
            const uint32_t streak = (!pvp || is_zombie || is_bossbattle) ? 0 : pvp->killstreak;
            const uint32_t melee_k = (!pvp || is_zombie || is_bossbattle) ? 0 : pvp->melee_kills;
            const uint32_t rifle_k = (!pvp || is_zombie || is_bossbattle) ? 0 : pvp->rifle_kills;
            const uint32_t shotgun_k = (!pvp || is_zombie || is_bossbattle) ? 0 : pvp->shotgun_kills;
            const uint32_t sniper_k = (!pvp || is_zombie || is_bossbattle) ? 0 : pvp->sniper_kills;
            const uint32_t gatling_k = (!pvp || is_zombie || is_bossbattle) ? 0 : pvp->gatling_kills;
            const uint32_t bazooka_k = (!pvp || is_zombie || is_bossbattle) ? 0 : pvp->bazooka_kills;
            const uint32_t grenade_k = (!pvp || is_zombie || is_bossbattle) ? 0 : pvp->grenade_kills;
            const uint32_t zombie_k = (is_zombie && pvp) ? pvp->killstreak : 0;
            const uint32_t infections = (is_zombie && pvp) ? pvp->melee_kills : 0;

            dctx.ops.emplace_back(MatchInfoHistoryAdd
            {
                .Sid = id,
                .Aid = acc->acc_info.Index,
                .IsHost = room->host_session_id == id,
                .IsDraw = draw,
                .IsClanMatch = is_cw,
                .PlayTime = static_cast<uint32_t>(play_time),
                .Level = acc->acc_info.Level,
                .Experience = exp_reward,
                .Energy = energy_reward,
                .MicroPoints = mp_reward,
                .room_index = room->room_id,
                .redscore = req->red_score,
                .bluescore = req->blue_score,
                .team_id = acc->team_id,
                .room_mode = room->ModeIndex,
                .room_map = room->MapIndex,
                .SelectedCharacter = acc->acc_info.SelectedCharacter,
                .Kills = kills,
                .Deaths = deaths,
                .Assists = assists,
                .Headshots = headshots,
                .HighestKillStreak = streak,
                .MeleeKills = melee_k,
                .RifleKills = rifle_k,
                .ShotgunKills = shotgun_k,
                .SniperKills = sniper_k,
                .GatlingKills = gatling_k,
                .BazookaKills = bazooka_k,
                .GrenadeKills = grenade_k,
                .ZombieKills = zombie_k,
                .Infections = infections,
                .MatchEndTime = end_time,
                .Hair = EquippedHair.item_id,
                .Face = EquippedFace.item_id,
                .Upper = EquippedUpper.item_id,
                .Under = EquippedUnder.item_id,
                .Skirt = EquippedSkirt.item_id,
                .Gloves = EquippedGloves.item_id,
                .Boots = EquippedBoots.item_id,
                .HeadAcc = EquippedHeadAcc.item_id,
                .WaistAcc = EquippedWaistAcc.item_id,
                .BackAcc = EquippedBackAcc.item_id,
                .Melee = EquippedMelee.item_id,
                .Rifle = EquippedRifle.item_id,
                .Shotgun = EquippedShotgun.item_id,
                .Sniper = EquippedSniper.item_id,
                .Gatling = EquippedGatling.item_id,
                .Bazooka = EquippedBazooka.item_id,
                .Grenade = EquippedGrenade.item_id,
                .IsItemReward = is_bossbattle,
                .reward_item = reward_item,
                .IsMvp = (id == ctx.mvp.sid),
                .IsEntryFragger = (id == ctx.entryFragger.sid),
                .IsBullseye = (id == ctx.bullseye.sid),
                .IsSupport = (id == ctx.support.sid),
                .IsBomba = (id == ctx.bomba.sid),
            });

            ctx.dctxs.push_back(std::move(dctx));
            ctx.tracking.push_back(std::move(track)); // ← SAVE TRACKING

            acc.unlock();
        }

        room->is_playing = false;
        room->is_kick_vote_running = false;
        room->kicked.clear();
        room->voters.clear();
        room->voteKickers.clear();

        if (is_bossbattle)
        {
            for (const auto& id : playing_players)
            {
                std::vector<BossItem> others_rewards;
                auto curr_uid = NetEngine::Packets::Core::UniqueId(id, 1).data;
                std::copy_if(ctx.pve_rewards.begin(), ctx.pve_rewards.end(), std::back_inserter(others_rewards), [curr_uid](const BossItem& item) { return item.unique_id != curr_uid; });
                auto otherRewards = MainBossBattleEndMatchResultAck(others_rewards).Serialize();
                ctx.packets.enqueue(id, NetEngine::Protocols::SCommandHeader(ctx.order, 0, 41, 0), reinterpret_cast<uint8_t*>(otherRewards.data()), otherRewards.size(), PriorityLevel::Normal);
            }
        }

        for (const auto& id : all_players)
            ctx.packets.enqueue(id, NetEngine::Protocols::SCommandHeader(256, 0, 33, 0), PriorityLevel::Normal);

        ctx.all_ss = std::move(all_players);
    }
    
    inline void MatchEnd(SCallbackData& callback, CMainServer* main_server)
    {
        auto session = callback.session;
        auto message = callback.message;
        if (!session || !message) return;

        CServer* server = callback.server;
        auto session_id = session->GetSessionId();
        auto acc_cache = CAccount.get<unique_t>(session_id);
        auto acc_index = acc_cache->acc_info.Index;
        if (acc_index == -1) return;
        auto extra = message->GetExtra();
        auto mission = message->GetMission();
        auto option = message->GetOption();
        auto order = message->GetOrder();
        
        std::vector<DatabaseUpdateCtx> dctxs;
        Utility::CPacketTask::BucketQueue packets;
        std::vector<uint16_t> all_ss;
        std::vector<BossItem> pve_rewards;
        boost::unordered_flat_set<uint32_t> pu_ss;
        boost::unordered_flat_set<uint32_t> won_ss;
        boost::unordered_flat_map<uint32_t, MainRoomEndMatchResponse> rpi;
        boost::unordered_flat_map<uint32_t, ClientInfo> cli;
        std::vector<PlayerRewardTracking> tracking; // ← ADD TRACKING VECTOR

        Leader ffa_winner{}, most_captures{}, most_wonrounds{}, best_kd{};
        Leader mvp{}, entryFragger{}, bullseye{}, support{}, bomba{};
        Leader best_zombie{}, best_arms{};
        
        EndMatchCtx ctx
        {
            .main = main_server,
            .packets = packets,
            .order = order,
            .mission = mission,
            .extra = extra,
            .option = option,
            .host = acc_cache,
            .data = message->GetData(),
            .dataSize = message->GetDataSize(),
            .dctxs = dctxs,
            .all_ss = all_ss,
            .pve_rewards = pve_rewards,
            .cli = cli,
            .room_id = acc_cache->room_id,
            .ffa_winner = ffa_winner,
            .most_captures = most_captures,
            .most_wonrounds = most_wonrounds,
            .best_kd = best_kd,
            .mvp = mvp,
            .entryFragger = entryFragger,
            .bullseye = bullseye,
            .support = support,
            .bomba = bomba,
            .best_zombie = best_zombie,
            .best_arms = best_arms,
            .tracking = tracking // ← PASS TRACKING
        };
        
        auto bSinglePlayer = extra == 6;
        uint32_t gameMode = 0;
        bSinglePlayer ? SpGameModes(ctx) : MpGameModes(ctx, gameMode);
        
        std::vector<ValidatedDbUpdates> validated_vec;
        validated_vec.resize(dctxs.size());
        
        for (auto& d : dctxs)
        {
            auto acc = CAccount.get<unique_t>(d.sid);
            if (acc->acc_info.Index != d.aid) { acc.unlock(); continue; }
            auto validated = main_server->ValidateDatabaseUpdates(acc, d);
            if (!validated.has_value())
            {
                DEBUGLOG(red, "ValidateDatabaseUpdates failed for [{}] [{}]: {}", acc->acc_info.Index, acc->acc_info.Nickname.c_str(), static_cast<int>(validated.error()));
                return;
            }
            validated_vec.push_back(std::move(validated.value()));
            acc.unlock();
        }
        
        [[maybe_unused]] auto ignored = BaseLib::DbPool->submit_task([main_server,
            session = std::move(callback.session),
            validated_vec = std::move(validated_vec),
            tracking = std::move(tracking), // ← MOVE TRACKING
            bSinglePlayer = bSinglePlayer,
            gameMode = gameMode,
            packets = std::move(packets),
            all_ss = std::move(all_ss),
            mvp = mvp,
            entryFragger = entryFragger,
            bullseye = bullseye,
            support = support,
            bomba = bomba
        ]() mutable
        {
            if (!session) return;
            
            std::vector<ResultDbUpdateInfo> dbres;
            if (!BaseLib::Database->UpdateAccounts(validated_vec, dbres).has_value()) return;
            
            std::string mvp_msg = "", entry_msg = "", bullseye_msg = "", support_msg = "", bomba_msg = "";
            auto is_boss = gameMode == NetEngine::Room::Mode::Index::BossBattle;
            
            // ========== BUILD AND PERSIST LOGS ==========
            for (size_t idx = 0; idx < validated_vec.size(); ++idx)
            {
                auto& v = validated_vec[idx];
                auto acc = CAccount.get<unique_t>(v.sid);
                auto applied = main_server->ApplyDatabaseUpdates(acc, v);
                if (!applied.has_value())
                {
                    DEBUGLOG(red, "ApplyDatabaseUpdates failed for [{}] [{}]: {}", acc->acc_info.Index, acc->acc_info.Nickname.c_str(), static_cast<int>(applied.error()));
                    acc.unlock();
                    return;
                }

                // Find matching tracking entry
                auto track_it = std::find_if(tracking.begin(), tracking.end(), [&](const PlayerRewardTracking& t) {
                    return t.sid == v.sid && t.aid == v.aid;
                });

                if (track_it != tracking.end())
                {
                    auto& track = *track_it;
                    LogContext log_ctx;

                    // Log Energy reward (from battery pickups during match)
                    if (track.energy_reward > 0)
                    {
                        CurrencyLogEntry energy_log;
                        energy_log.aid = track.aid;
                        energy_log.currency_type = CurrencyLog::Type::Energy;
                        energy_log.amount = static_cast<int32_t>(track.energy_reward);
                        energy_log.before_value = track.energy_before;
                        energy_log.after_value = acc->acc_info.Energy;
                        energy_log.source_type = CurrencyLog::SourceType::MatchReward;
                        log_ctx.currency_logs.push_back(energy_log);
                    }

                    // Log MP reward (match points)
                    if (track.mp_reward > 0)
                    {
                        CurrencyLogEntry mp_log;
                        mp_log.aid = track.aid;
                        mp_log.currency_type = CurrencyLog::Type::MP;
                        mp_log.amount = static_cast<int32_t>(track.mp_reward);
                        mp_log.before_value = track.mp_before;
                        mp_log.after_value = acc->acc_info.MicroPoints;
                        mp_log.source_type = CurrencyLog::SourceType::MatchReward;
                        log_ctx.currency_logs.push_back(mp_log);
                    }

                    // Log boss battle item reward
                    if (track.is_boss_battle && track.reward_item_id > 0)
                    {
                        for (const auto& added : v.items_added)
                        {
                            if (added.item_info.item_number.item_id == track.reward_item_id)
                            {
                                ItemLogEntry item_log;
                                item_log.aid = track.aid;
                                item_log.action_type = ItemLog::ActionType::Added;
                                item_log.item_id = track.reward_item_id;
                                item_log.serial_info = added.item_info.serial_info.data;
                                item_log.origin_type = ItemLog::OriginType::BossBattle;
                                item_log.item_type = static_cast<ItemLog::ItemType>(added.item_type);
                                log_ctx.item_logs.push_back(item_log);
                                break;
                            }
                        }
                    }

                    // Log tutorial item reward
                    if (track.is_tutorial && track.tutorial_item_id > 0)
                    {
                        for (const auto& added : v.items_added)
                        {
                            if (added.item_info.item_number.item_id == track.tutorial_item_id)
                            {
                                ItemLogEntry item_log;
                                item_log.aid = track.aid;
                                item_log.action_type = ItemLog::ActionType::Added;
                                item_log.item_id = track.tutorial_item_id;
                                item_log.serial_info = added.item_info.serial_info.data;
                                item_log.origin_type = ItemLog::OriginType::Tutorial;
                                item_log.item_type = static_cast<ItemLog::ItemType>(added.item_type);
                                log_ctx.item_logs.push_back(item_log);
                                break;
                            }
                        }

                        // Log tutorial MP reward
                        if (track.mp_reward > 0)
                        {
                            CurrencyLogEntry mp_log;
                            mp_log.aid = track.aid;
                            mp_log.currency_type = CurrencyLog::Type::MP;
                            mp_log.amount = static_cast<int32_t>(track.mp_reward);
                            mp_log.before_value = track.mp_before;
                            mp_log.after_value = acc->acc_info.MicroPoints;
                            mp_log.source_type = CurrencyLog::SourceType::Tutorial;
                            log_ctx.currency_logs.push_back(mp_log);
                        }
                    }

                    // Log story item reward
                    if (track.is_story && track.story_item_id > 0)
                    {
                        for (const auto& added : v.items_added)
                        {
                            if (added.item_info.item_number.item_id == track.story_item_id)
                            {
                                ItemLogEntry item_log;
                                item_log.aid = track.aid;
                                item_log.action_type = ItemLog::ActionType::Added;
                                item_log.item_id = track.story_item_id;
                                item_log.serial_info = added.item_info.serial_info.data;
                                item_log.origin_type = ItemLog::OriginType::Story;
                                item_log.item_type = static_cast<ItemLog::ItemType>(added.item_type);
                                log_ctx.item_logs.push_back(item_log);
                                break;
                            }
                        }
                    }

                    // Log level up item reward
                    if (track.had_level_up && track.level_up_item.has_value())
                    {
                        auto& lvl_item = track.level_up_item.value();
                        for (const auto& added : v.items_added)
                        {
                            if (added.item_info.item_number.item_id == lvl_item.item_number.item_id)
                            {
                                ItemLogEntry item_log;
                                item_log.aid = track.aid;
                                item_log.action_type = ItemLog::ActionType::Added;
                                item_log.item_id = lvl_item.item_number.item_id;
                                item_log.serial_info = added.item_info.serial_info.data;
                                item_log.origin_type = ItemLog::OriginType::LevelUp;
                                item_log.item_type = static_cast<ItemLog::ItemType>(added.item_type);
                                log_ctx.item_logs.push_back(item_log);
                                break;
                            }
                        }
                    }

                    // Persist logs for this player
                    if (!log_ctx.empty())
                    {
                        auto log_result = BaseLib::Database->PersistLogs(log_ctx);
                        if (!log_result.has_value())
                        {
                            DEBUGLOG(red, "Failed to persist match end logs for player [{}]: {}",
                                acc->acc_info.Nickname.c_str(),
                                log_result.error().message);
                        }
                    }
                }

                // Build MVP/role messages
                if (!bSinglePlayer && !is_boss)
                {
                    if (mvp.sid == v.sid && mvp.pts)
                        mvp_msg = fmt::format("MVP: {} {}pts", acc->acc_info.Nickname, mvp.pts);
                    if (entryFragger.sid == v.sid && entryFragger.pts)
                        entry_msg = fmt::format("ENTRY FRAGGER: {} {} Killstreak", acc->acc_info.Nickname, entryFragger.pts);
                    if (bullseye.sid == v.sid && bullseye.pts)
                        bullseye_msg = fmt::format("THE BULLSEYE: {} {:.2f}% Headshots", acc->acc_info.Nickname, static_cast<float>(bullseye.pts) / 100.f);
                    if (support.sid == v.sid && support.pts)
                        support_msg = fmt::format("THE SUPPORT: {} {} Assists", acc->acc_info.Nickname, support.pts);
                    if (bomba.sid == v.sid && bomba.pts)
                        bomba_msg = fmt::format("BOMBA: {} {} Explosive kills", acc->acc_info.Nickname, bomba.pts);
                }
                
                acc.unlock();
            }
            
            // flush all packets
            for (auto& b : packets.buckets_)
            {
                for (auto& p : b)
                {
                    if (auto session = main_server->GetSessionById(p.sid))
                    {
                        DEBUGLOG(dark_cyan, "sent end match packet ({},{},{},{}) to player sid=({})", static_cast<int>(p.cmd.order), static_cast<int>(p.cmd.mission), static_cast<int>(p.cmd.extra), static_cast<int>(p.cmd.option), p.sid);
                        session->SendMsg(p.cmd, p.data.empty() ? nullptr : reinterpret_cast<uint8_t*>(p.data.data()), p.data.size());
                    }
                }
            }

            packets.clear();
            
            if (!bSinglePlayer && !is_boss)
            {
                for (auto& sid : all_ss)
                {
                    if (auto session = main_server->GetSessionById(sid))
                    {
                        if (!mvp_msg.empty())
                            main_server->SendServerMessage(session.get(), mvp_msg);
                        if (!entry_msg.empty())
                            main_server->SendServerMessage(session.get(), entry_msg);
                        if (!bullseye_msg.empty())
                            main_server->SendServerMessage(session.get(), bullseye_msg);
                        if (!support_msg.empty())
                            main_server->SendServerMessage(session.get(), support_msg);
                        if (!bomba_msg.empty())
                            main_server->SendServerMessage(session.get(), bomba_msg);
                    }
                }
            }
        });
    }
}
