#include "CCastServer.h"
#include "BaseLib/Utility.h"
#include "BaseLib/CDatabase.h"
#include "NetEngine/CMessage.h"

#include "handlers/Gameplay/Core/Ping.h"
#include "handlers/Gameplay/Core/Node/Authorize.h"
#include "handlers/Gameplay/Core/Node/CastQuit.h"

#include "handlers/Gameplay/User/Core/EffectBehaviour.h"
#include "handlers/Gameplay/User/Core/Movement.h"
#include "handlers/Gameplay/User/Core/Nickname.h"
#include "handlers/Gameplay/User/Core/Radio.h"
#include "handlers/Gameplay/User/Core/Reload.h"
#include "handlers/Gameplay/User/Core/Respawn.h"
#include "handlers/Gameplay/User/Core/TickRequest.h"

#include "handlers/Gameplay/User/Item/Use.h"
#include "handlers/Gameplay/User/Item/Kitdrop/Get.h"
#include "handlers/Gameplay/User/Item/Kitdrop/Pickup.h"

#include "handlers/Gameplay/User/Mod/Bomb/Drop.h"
#include "handlers/Gameplay/User/Mod/Bomb/Idk.h"
#include "handlers/Gameplay/User/Mod/Bomb/State.h"

#include "handlers/Gameplay/User/Weapon/AttackGatling.h"
#include "handlers/Gameplay/User/Weapon/AttackMelee.h"
#include "handlers/Gameplay/User/Weapon/AttackProjectile.h"
#include "handlers/Gameplay/User/Weapon/AttackRifle.h"
#include "handlers/Gameplay/User/Weapon/AttackShotgun.h"
#include "handlers/Gameplay/User/Weapon/AttackSniper.h"
#include "handlers/Gameplay/User/Weapon/WarmupGatling.h"

#include "handlers/Gameplay/Host/Core/Modinfo.h"
#include "handlers/Gameplay/Host/Core/Respawn.h"
#include "handlers/Gameplay/Host/Core/Status.h"
#include "handlers/Gameplay/Host/Core/TickUpdate.h"
#include "handlers/Gameplay/Host/Core/GiveWeapon.h"

#include "handlers/Gameplay/Host/Item/Kitdrop/Info.h"
#include "handlers/Gameplay/Host/Item/Kitdrop/Pickup.h"
#include "handlers/Gameplay/Host/Item/Kitdrop/Spawn.h"
#include "handlers/Gameplay/Host/Item/Use.h"

#include "handlers/Gameplay/Host/Mod/Bomb/State.h"

#include "handlers/Gameplay/Host/Mod/Match/End.h"
#include "handlers/Gameplay/Host/Mod/Match/Leave.h"
#include "handlers/Gameplay/Host/Mod/Match/Load.h"
#include "handlers/Gameplay/Host/Mod/Match/Score.h"
#include "handlers/Gameplay/Host/Mod/Match/Start.h"

#include "handlers/Gameplay/Host/Npc/Attack.h"
#include "handlers/Gameplay/Host/Npc/Movement.h"
#include "handlers/Gameplay/Host/Npc/Projectile.h"
#include "handlers/Gameplay/Host/Npc/Spawn.h"

#include "handlers/Gameplay/Host/Weapon/AttackGatling.h"
#include "handlers/Gameplay/Host/Weapon/AttackMelee.h"
#include "handlers/Gameplay/Host/Weapon/AttackProjectile.h"
#include "handlers/Gameplay/Host/Weapon/AttackRifle.h"
#include "handlers/Gameplay/Host/Weapon/AttackShotgun.h"
#include "handlers/Gameplay/Host/Weapon/AttackSniper.h"
#include "handlers/Gameplay/Host/Weapon/ImpactProjectile.h"
#include "handlers/Gameplay/Host/Weapon/WarmupGatling.h"

#include "handlers/Plaza/Join.h"
#include "handlers/Plaza/Leave.h"

#include "handlers/Room/Create.h"
#include "handlers/Room/Join.h"
#include "handlers/Room/Leave.h"

#include "handlers/Core/Ipc/MainAuthorize.h"
#include "handlers/Core/Ipc/MainDisconnect.h"
#include "handlers/Core/Ipc/MainHostChange.h"
#include "handlers/Core/Ipc/MainRoomSync.h"
#include "handlers/Core/Ipc/MainPlayerHealth.h"
#include "handlers/Core/Ipc/MainPingAssure.h"
#include "handlers/Core/Ipc/MainServerInfo.h"
#include "handlers/Core/Ipc/MainTpToProj.h"
#include "handlers/Core/Connect.h"
#include "handlers/Core/Disconnect.h"
#include "handlers/Core/Ipc.h"

namespace Game
{
    CCache<boost::unordered_flat_set<uint16_t>> g_tp_to_proj_sids;
    CCache<boost::unordered_flat_map<uint16_t, Player>> CAccount;
    CCache<boost::unordered_flat_map<uint16_t, Room>> CRoom;
    CCache<boost::unordered_flat_map<uint16_t, RoomProjectiles>> CRoomProjectiles;
    CCache<boost::unordered_flat_map<uint16_t, Plaza>> CPlaza;
    CCache<boost::unordered_flat_map<uint64_t, uint16_t>> CAuthKey;

    CCache<std::vector<uint16_t>> CRoomId;
    CCache<std::vector<uint16_t>> CPartyId;

    NetEngine::RateLimit::IdentitySnapshot CCastServer::BuildPacketRateLimitIdentitySnapshot(const SCallbackData& callback)
    {
        NetEngine::RateLimit::IdentitySnapshot snapshot{};
        if (!callback.session)
            return snapshot;

        snapshot.sid = callback.session->GetSessionId();
        snapshot.ip = callback.session->GetIpAddress();

        auto acc = CAccount.get<shared_t>(snapshot.sid);
        if (!acc)
            return snapshot;

        snapshot.aid = acc->account_id;
        snapshot.hwid = acc->hwid;
        return snapshot;
    }

    CCastServer::CCastServer()
    {
        using namespace Game::Handlers;
        using enum EOrder;
        this->OnNewSession(std::bind(&ServerConnect, std::placeholders::_1, this));
        this->OnSessionDisconnected(std::bind(&ServerDisconnect, std::placeholders::_1, this));
        this->OnIpcMessage(std::bind(&ServerIpcMessage, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, this));
        this->BindPacketHandler<&Ping>(this, ID_PING);
        this->BindPacketHandler<&UserEffectBehaviour>(this, USER_EFFECT_BEHAVIOUR);
        this->BindPacketHandler<&UserTickRequest>(this, USER_SERVER_TICK);
        this->BindPacketHandler<&HostItemKitdropInfo>(this, ITEM_KITDROP_INFO);
        this->BindPacketHandler<&UserItemKitdropGet>(this, ITEM_KITDROP_GET);
        this->BindPacketHandler<&UserItemPickup>(this, ITEM_PICKUP);
        this->BindPacketHandler<&UserItemUse>(this, ITEM_USE);
        this->BindPacketHandler<&RoomJoin>(this, ROOM_JOIN);
        this->BindPacketHandler<&UserWarmupGatling>(this, USER_WARMUP_GATLING);
        this->BindPacketHandler<&UserAttackGatling>(this, USER_ATTACK_GATLING);
        this->BindPacketHandler<&UserAttackMelee>(this, USER_ATTACK_MELEE);
        this->BindPacketHandler<&UserAttackProjectile>(this, USER_ATTACK_PROJECTILE);
        this->BindPacketHandler<&UserAttackRifle>(this, USER_ATTACK_RIFLE);
        this->BindPacketHandler<&UserAttackShotgun>(this, USER_ATTACK_SHOTGUN);
        this->BindPacketHandler<&UserAttackSniper>(this, USER_ATTACK_SNIPER);
        this->BindPacketHandler<&UserModBombState>(this, USER_MOD_BOMB_STATE);
        this->BindPacketHandler<&UserModBombDrop>(this, USER_MOD_BOMB_DROP);
        this->BindPacketHandler<&UserModBombIdk>(this, USER_MOD_BOMB_IDK);
        this->BindPacketHandler<&UserRespawn>(this, USER_RESPAWN);

    #if defined(RELEASE_1_0_3)
        this->BindPacketHandler<&PlazaJoin>(this, USER_PLAZA_JOIN);
        this->BindPacketHandler<&PlazaLeave>(this, USER_PLAZA_LEAVE);
    #endif

        this->BindPacketHandler<&NodeAuthorize>(this, NODE_AUTHORIZE);
        this->BindPacketHandler<&NodeCastQuit>(this, NODE_CAST_QUIT);

        this->BindPacketHandler<&HostModMatchEnd>(this, MOD_END);
        this->BindPacketHandler<&HostModMatchScore>(this, MOD_SCORE);
        this->BindPacketHandler<&HostModMatchLeave>(this, MOD_LEAVE);
        this->BindPacketHandler<&HostModMatchLoad>(this, MOD_LOAD);
        this->BindPacketHandler<&HostModMatchStart>(this, MOD_OTHER_START);
		this->BindPacketHandler<&HostModRoundEnd>(this, MOD_ROUND_END);

        this->BindPacketHandler<&HostItemKitdropSpawn>(this, HOST_KITDROP_SPAWN); // spawn pickups
        this->BindPacketHandler<&HostGiveWeapon>(this, HOST_GIVE_WEAPON);
        this->BindPacketHandler<&HostItemKitdropPickup>(this, HOST_KITDROP_PICKUP);
        this->BindPacketHandler<&HostItemUse>(this, HOST_ITEM_USE);
        this->BindPacketHandler<&HostImpactProjectile>(this, HOST_IMPACT_PROJECTILE);//bazooka impact
        this->BindPacketHandler<&HostWarmupGatling>(this, HOST_WARMUP_GATLING);//gatling fire
        this->BindPacketHandler<&HostAttackGatling>(this, HOST_ATTACK_GATLING);//gatling bullet
        this->BindPacketHandler<&HostAttackMelee>(this, HOST_ATTACK_MELEE);//melee
        this->BindPacketHandler<&HostAttackRifle>(this, HOST_ATTACK_RIFLE);//rifle
        this->BindPacketHandler<&HostAttackShotgun>(this, HOST_ATTACK_SHOTGUN);//shotgun
        this->BindPacketHandler<&HostAttackSniper>(this, HOST_ATTACK_SNIPER);//sniper
      
        this->BindPacketHandler<&HostModBombState>(this, HOST_MOD_BOMB_STATE);//host broadcast bomb plant/defuse state
        this->BindPacketHandler<&HostAttackProjectile>(this, HOST_ATTACK_PROJECTILE);//bazokoka fire
		this->BindPacketHandler<&UserRadio>(this, OTHER_RADIO); // idk maybe other player radio smth
        // PVE_NPC_STATE 274
        this->BindPacketHandler<&UserReload>(this, USER_RELOAD);
        this->BindPacketHandler<&HostRespawn>(this, HOST_RESPAWN);
        this->BindPacketHandler<&RoomCreate>(this, ROOM_CREATE_CAST);
        this->BindPacketHandler<&RoomLeave>(this, ROOM_LEAVE_CAST);
        this->BindPacketHandler<&UserMovement>(this, USER_MOVE);

        this->BindPacketHandler<&NpcMovement>(this, HOST_NPC_MOVE);
        this->BindPacketHandler<&UserNickname>(this, USER_NICKNAME);
        this->BindPacketHandler<&HostStatus>(this, HOST_OTHER_STATUS); // CExPlayerUpdate
        this->BindPacketHandler<&HostModinfo>(this, HOST_MODINFO_CAST); // unfinished struct
        this->BindPacketHandler<&NpcProjectile>(this, HOST_NPC_PROJECTILE);
        this->BindPacketHandler<&NpcSpawn>(this, HOST_NPC_SPAWN);
        this->BindPacketHandler<&NpcAttack>(this, HOST_NPC_ATTACK);
        this->BindPacketHandler<&HostTickUpdate>(this, HOST_SERVER_TICK);

    }
    CCastServer::~CCastServer() {}

    void CCastServer::StartMovementBatcher(uint32_t hz)
    {
        if (hz == 0) hz = 128;
        const auto interval = std::chrono::microseconds(1'000'000ull / hz);
        m_moveBatcher.Start(GetIoContext(), interval,
            [this](uint16_t room_id, uint32_t tick, const uint8_t* entries, uint16_t len, uint8_t count)
            {
                std::vector<uint16_t> ids;
                if (auto room = CRoom.get<shared_t>(room_id))
                {
                    ids = room->players_session_id;
                    room.unlock();
                }
                if (ids.empty()) return;

                // payload = [front tick (4)] [concatenated entries], <= 2039B (enforced
                // by the batcher's split). One shared tick drives all entries' interp.
                uint8_t payload[MovementBatcher::kMaxPayload];
                std::memcpy(payload, &tick, sizeof(tick));
                std::memcpy(payload + sizeof(tick), entries, len);

                CMessage msg;
                msg.SetCommand(322, 0, 0, count); // order=322, mission=0, extra=0, option=count
                msg.SetData(payload, static_cast<uint16_t>(sizeof(tick) + len));
                Broadcast(ids, msg);

                // --- DIAGNOSTIC: flush throughput (remove after verifying). ---
                // packets/sec  = cmd-322 actually sent per recipient-set
                // entries/sec  = total movement entries carried (should ~= [MoveRate])
                // maxEntries   = biggest single batch (== concurrent movers in a room)
                // A healthy populated room: entries >> packets, [SendQ] stays ~0.
                {
                    static std::atomic<uint32_t> s_pkts{ 0 }, s_entries{ 0 }, s_maxCount{ 0 };
                    static std::atomic<int64_t>  s_lastLogMs{ 0 };
                    s_pkts.fetch_add(1, std::memory_order_relaxed);
                    s_entries.fetch_add(count, std::memory_order_relaxed);
                    for (auto cur = s_maxCount.load(std::memory_order_relaxed);
                         count > cur && !s_maxCount.compare_exchange_weak(cur, count, std::memory_order_relaxed); ) {}
                    const auto nowMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::steady_clock::now().time_since_epoch()).count();
                    auto last = s_lastLogMs.load(std::memory_order_relaxed);
                    if (nowMs - last >= 1000 && s_lastLogMs.compare_exchange_strong(last, nowMs))
                    {
                        const auto p = s_pkts.exchange(0, std::memory_order_relaxed);
                        const auto e = s_entries.exchange(0, std::memory_order_relaxed);
                        const auto m = s_maxCount.exchange(0, std::memory_order_relaxed);
                        (void)p; (void)e; (void)m;
                        // TEMP: silenced to isolate combat logs (restore by uncommenting).
                        //DEBUGLOG(dark_green, "[FlushRate] cmd-322 packets/sec=({}) entries/sec=({}) maxEntries/pkt=({})", p, e, m);
                    }
                }
            });
    }
}
