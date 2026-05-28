#include "CCastServer.h"
#include "BaseLib/Utility.h"
#include "BaseLib/CDatabase.h"

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
    CCastServer::~CCastServer()
    {
        if (m_positionFlushTimer)
            m_positionFlushTimer->cancel();
    }

    void CCastServer::StartPositionFlushTimer()
    {
        if (!IsBatchPositionsEnabled()) return;
        m_positionFlushTimer = std::make_shared<asio::steady_timer>(GetIoContext());
        auto tick = [this]() {
            auto self = m_positionFlushTimer;
            auto loop = [this, self](auto& loop_ref) -> void {
                FlushPendingPositions();
                self->expires_after(std::chrono::milliseconds(100));
                self->async_wait([this, &loop_ref](const asio::error_code& ec) {
                    if (!ec) loop_ref(loop_ref);
                });
            };
            loop(loop);
        };
        tick();
    }

    void CCastServer::FlushPendingPositions()
    {
        static constexpr size_t header_size = 8;
        static constexpr size_t max_data = 2047 - header_size - sizeof(uint32_t);

        auto all_rooms = CRoom.get_all(shared_t{});
        std::vector<uint16_t> ids_with_pending;
        for (auto& [id, room] : *all_rooms)
        {
            if (!room.pending_positions.empty())
                ids_with_pending.push_back(id);
        }
        all_rooms.unlock();

        for (auto room_id : ids_with_pending)
        {
            auto room = CRoom.get<unique_t>(room_id);
            if (!room || room->pending_positions.empty()) continue;

            auto pending = std::move(room->pending_positions);
            room->pending_positions.clear();
            auto players = room->players_session_id;
            auto tick = room->room_tick++;
            room.unlock();

            size_t index = 0;
            while (index < pending.size())
            {
                std::vector<uint8_t> batch_buf;
                batch_buf.reserve(max_data);
                batch_buf.resize(sizeof(uint32_t));
                std::memcpy(batch_buf.data(), &tick, sizeof(tick));

                uint8_t count = 0;
                while (index < pending.size())
                {
                    auto& entry = pending[index];
                    if (batch_buf.size() + entry.size() > max_data) break;
                    batch_buf.insert(batch_buf.end(), entry.begin(), entry.end());
                    ++count;
                    ++index;
                }

                CMessage msg;
                msg.SetCommand(322, 0, 0, count);
                msg.SetData(batch_buf.data(), static_cast<uint16_t>(batch_buf.size()));
                Broadcast(players, msg);
            }
        }
    }
   
}
