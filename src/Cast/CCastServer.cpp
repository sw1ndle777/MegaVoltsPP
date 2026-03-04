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
#include "handlers/Core/Ipc/MainPingAssure.h"
#include "handlers/Core/Ipc/MainServerInfo.h"
#include "handlers/Core/Connect.h"
#include "handlers/Core/Disconnect.h"
#include "handlers/Core/Ipc.h"

namespace Game
{
    CCache<boost::unordered_flat_map<uint16_t, Player>> CAccount;
    CCache<boost::unordered_flat_map<uint16_t, Room>> CRoom;
    CCache<boost::unordered_flat_map<uint16_t, Plaza>> CPlaza;
    CCache<boost::unordered_flat_map<uint64_t, uint16_t>> CAuthKey;

    CCache<std::vector<uint16_t>> CRoomId;
    CCache<std::vector<uint16_t>> CPartyId;

    CCastServer::CCastServer()
    {
        using namespace Game::Handlers;
        using enum EOrder;
        this->OnNewSession(std::bind(&ServerConnect, std::placeholders::_1, this));
        this->OnSessionDisconnected(std::bind(&ServerDisconnect, std::placeholders::_1, this));
        this->OnIpcMessage(std::bind(&ServerIpcMessage, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, this));
        this->On(ID_PING, std::bind(&Ping, std::placeholders::_1, this));
        this->On(USER_EFFECT_BEHAVIOUR, std::bind(&UserEffectBehaviour, std::placeholders::_1, this));
        this->On(USER_SERVER_TICK, std::bind(&UserTickRequest, std::placeholders::_1, this));
        this->On(ITEM_KITDROP_INFO, std::bind(&HostItemKitdropInfo, std::placeholders::_1, this));
        this->On(ITEM_KITDROP_GET, std::bind(&UserItemKitdropGet, std::placeholders::_1, this));
        this->On(ITEM_PICKUP, std::bind(&UserItemPickup, std::placeholders::_1, this));
        this->On(ITEM_USE, std::bind(&UserItemUse, std::placeholders::_1, this));
        this->On(ROOM_JOIN, std::bind(&RoomJoin, std::placeholders::_1, this));
        this->On(USER_WARMUP_GATLING, std::bind(&UserWarmupGatling, std::placeholders::_1, this));
        this->On(USER_ATTACK_GATLING, std::bind(&UserAttackGatling, std::placeholders::_1, this));
        this->On(USER_ATTACK_MELEE, std::bind(&UserAttackMelee, std::placeholders::_1, this));
        this->On(USER_ATTACK_PROJECTILE, std::bind(&UserAttackProjectile, std::placeholders::_1, this));
        this->On(USER_ATTACK_RIFLE, std::bind(&UserAttackRifle, std::placeholders::_1, this));
        this->On(USER_ATTACK_SHOTGUN, std::bind(&UserAttackShotgun, std::placeholders::_1, this));
        this->On(USER_ATTACK_SNIPER, std::bind(&UserAttackSniper, std::placeholders::_1, this));
        this->On(USER_MOD_BOMB_STATE, std::bind(&UserModBombState, std::placeholders::_1, this));
        this->On(USER_MOD_BOMB_DROP, std::bind(&UserModBombDrop, std::placeholders::_1, this));
        this->On(USER_MOD_BOMB_IDK, std::bind(&UserModBombIdk, std::placeholders::_1, this));
        this->On(USER_RESPAWN, std::bind(&UserRespawn, std::placeholders::_1, this));

    #if defined(RELEASE_1_0_3)
        this->On(USER_PLAZA_JOIN, std::bind(&PlazaJoin, std::placeholders::_1, this));
        this->On(USER_PLAZA_LEAVE, std::bind(&PlazaLeave, std::placeholders::_1, this));
    #endif

        this->On(NODE_AUTHORIZE, std::bind(&NodeAuthorize, std::placeholders::_1, this));
        this->On(NODE_CAST_QUIT, std::bind(&NodeCastQuit, std::placeholders::_1, this));

        this->On(MOD_END, std::bind(&HostModMatchEnd, std::placeholders::_1, this));
        this->On(MOD_SCORE, std::bind(&HostModMatchScore, std::placeholders::_1, this));
        this->On(MOD_LEAVE, std::bind(&HostModMatchLeave, std::placeholders::_1, this));
        this->On(MOD_LOAD, std::bind(&HostModMatchLoad, std::placeholders::_1, this));
        this->On(MOD_OTHER_START, std::bind(&HostModMatchStart, std::placeholders::_1, this));

        //this->On(259, std::bind(&PlayerUnknown8, std::placeholders::_1, this));// idk maybe observer rounds smth prolly has to be empty

        this->On(HOST_KITDROP_SPAWN, std::bind(&HostItemKitdropSpawn, std::placeholders::_1, this)); // spawn pickups
        this->On(HOST_GIVE_WEAPON, std::bind(&HostGiveWeapon, std::placeholders::_1, this));
        this->On(HOST_KITDROP_PICKUP, std::bind(&HostItemKitdropPickup, std::placeholders::_1, this));
        this->On(HOST_ITEM_USE, std::bind(&HostItemUse, std::placeholders::_1, this));
        this->On(HOST_IMPACT_PROJECTILE, std::bind(&HostImpactProjectile, std::placeholders::_1, this));//bazooka impact
        this->On(HOST_WARMUP_GATLING, std::bind(&HostWarmupGatling, std::placeholders::_1, this));//gatling fire
        this->On(HOST_ATTACK_GATLING, std::bind(&HostAttackGatling, std::placeholders::_1, this));//gatling bullet
        this->On(HOST_ATTACK_MELEE, std::bind(&HostAttackMelee, std::placeholders::_1, this));//melee
        this->On(HOST_ATTACK_RIFLE, std::bind(&HostAttackRifle, std::placeholders::_1, this));//rifle
        this->On(HOST_ATTACK_SHOTGUN, std::bind(&HostAttackShotgun, std::placeholders::_1, this));//shotgun
        this->On(HOST_ATTACK_SNIPER, std::bind(&HostAttackSniper, std::placeholders::_1, this));//sniper
     
        this->On(HOST_MOD_BOMB_STATE, std::bind(&HostModBombState, std::placeholders::_1, this));//host broadcast bomb plant/defuse state
        this->On(HOST_ATTACK_PROJECTILE, std::bind(&HostAttackProjectile, std::placeholders::_1, this));//bazokoka fire
        // OTHER_USER_RADIO 273
        // PVE_NPC_STATE 274
        this->On(USER_RELOAD, std::bind(&UserReload, std::placeholders::_1, this));
        this->On(HOST_RESPAWN, std::bind(&HostRespawn, std::placeholders::_1, this));
        this->On(ROOM_CREATE_CAST, std::bind(&RoomCreate, std::placeholders::_1, this));
        this->On(ROOM_LEAVE_CAST, std::bind(&RoomLeave, std::placeholders::_1, this));
        // USER_RADIO 280
        this->On(USER_MOVE, std::bind(&UserMovement, std::placeholders::_1, this));

        this->On(HOST_NPC_MOVE, std::bind(&NpcMovement, std::placeholders::_1, this));
        this->On(USER_NICKNAME, std::bind(&UserNickname, std::placeholders::_1, this));
        this->On(HOST_OTHER_STATUS, std::bind(&HostStatus, std::placeholders::_1, this)); // CExPlayerUpdate
        this->On(HOST_MODINFO_CAST, std::bind(&HostModinfo, std::placeholders::_1, this)); // unfinished struct
        this->On(HOST_NPC_PROJECTILE, std::bind(&NpcProjectile, std::placeholders::_1, this));
        this->On(HOST_NPC_SPAWN, std::bind(&NpcSpawn, std::placeholders::_1, this));
        this->On(HOST_NPC_ATTACK, std::bind(&NpcAttack, std::placeholders::_1, this));
        this->On(HOST_SERVER_TICK, std::bind(&HostTickUpdate, std::placeholders::_1, this));

    }
    CCastServer::~CCastServer(){}
   
}
