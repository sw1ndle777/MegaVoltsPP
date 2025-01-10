#include "CCastServer.h"
#include "BaseLib/Utility.h"
#include "BaseLib/CDatabase.h"
#include "BaseLib/CThreadPool.h"

#include "handlers/ConnectionAuth.h"
#include "handlers/CreateRoom.h"

#include "handlers/HostAbility.h"
#include "handlers/HostExplosives.h"
#include "handlers/HostExplosivesDamage.h"
#include "handlers/HostPickupItem.h"
#include "handlers/HostRespawn.h"
#include "handlers/HostSpawnAmmo.h"
#include "handlers/HostSyncTick.h"
#include "handlers/HostTurnZombie.h"
#include "handlers/HostUpdateTick.h"
#include "handlers/HostWeaponAttack.h"

#include "handlers/LeaveRoom.h"

#include "handlers/NpcsAttack.h"
#include "handlers/NpcsMovement.h"
#include "handlers/NpcsProjectiles.h"
#include "handlers/NpcsSpawn.h"

#include "handlers/PingPong.h"

#include "handlers/PlayerAbility.h"
#include "handlers/PlayerCrash.h"
#include "handlers/PlayerEndMatch.h"
#include "handlers/PlayerJoinRoom.h"
#include "handlers/PlayerJoinPlaza.h"
#include "handlers/PlayerLeaveMatch.h"
#include "handlers/PlayerLeavePlaza.h"
#include "handlers/PlayerLoadMatch.h"
#include "handlers/PlayerMovement.h"
#include "handlers/PlayerNicknames.h"
#include "handlers/PlayerPickupItem.h"
#include "handlers/PlayerReload.h"
#include "handlers/PlayerRespawn.h"
#include "handlers/PlayerScoreMatch.h"
#include "handlers/PlayerStartMatch.h"
#include "handlers/PlayerSyncTick.h"
#include "handlers/PlayerUnknown1.h"
#include "handlers/PlayerUnknown2.h"
#include "handlers/PlayerUnknown3.h"
#include "handlers/PlayerUnknown4.h"
#include "handlers/PlayerUnknown5.h"
#include "handlers/PlayerUnknown6.h"
#include "handlers/PlayerUnknown7.h"
#include "handlers/PlayerUnknown8.h"
#include "handlers/PlayerSpawnBomb.h"
#include "handlers/PlayerWeaponAttack.h"

#include "handlers/ServerConnect.h"
#include "handlers/ServerDisconnect.h"
#include "handlers/ServerIpcMessage.h"

namespace Game
{

    //asio::strand<asio::io_context::executor_type> global_strand;
    //asio::strand<asio::io_context::executor_type> players_strand;
    //asio::strand<asio::io_context::executor_type> rooms_strand;
    //asio::strand<asio::io_context::executor_type> plazas_strand;
    std::shared_mutex players_cache_mutex;
    std::shared_mutex rooms_cache_mutex;
    std::shared_mutex plaza_cache_mutex;
    std::shared_mutex room_ids_mutex;
    std::shared_mutex plaza_ids_mutex;
    boost::unordered_flat_map<std::uint16_t, Player> players_cache;
    boost::unordered_flat_map<std::uint16_t, Room> rooms_cache;
    boost::unordered_flat_map<std::uint16_t, Plaza> plaza_cache;
    /*
    std::unordered_map<std::uint16_t, Player> players_cache;
    std::unordered_map<std::uint16_t, Room> rooms_cache;
    std::unordered_map<std::uint16_t, Plaza> plaza_cache;
    */
    std::vector<std::uint32_t> room_ids;  
    std::vector<std::uint32_t> plaza_ids;
    RECT rc = { 0 };
    CCastServer::CCastServer()
    {
        this->OnNewSession(std::bind(&Game::Handlers::ServerConnect, std::placeholders::_1, this));
        this->OnSessionDisconnected(std::bind(&Game::Handlers::ServerDisconnect, std::placeholders::_1, this));
        this->OnIpcMessage(std::bind(&Game::Handlers::ServerIpcMessage, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, this));
        this->On(71, std::bind(&Game::Handlers::PingPong, std::placeholders::_1, this));
        this->On(78, std::bind(&Game::Handlers::PlayerUnknown8, std::placeholders::_1, this));// idk maybe observer rounds smth prolly has to be empty
        this->On(79, std::bind(&Game::Handlers::PlayerSyncTick, std::placeholders::_1, this));
        this->On(90, std::bind(&Game::Handlers::PlayerUnknown3, std::placeholders::_1, this));//ctb battery respawn
        this->On(92, std::bind(&Game::Handlers::PlayerSpawnBomb, std::placeholders::_1, this));
        this->On(94, std::bind(&Game::Handlers::PlayerPickupItem, std::placeholders::_1, this)); //pickup bomb capsule zombie item
        this->On(96, std::bind(&Game::Handlers::PlayerPickupItem, std::placeholders::_1, this));//hp ammo pickup
        this->On(102, std::bind(&Game::Handlers::PlayerAbility, std::placeholders::_1, this));
        this->On(140, std::bind(&Game::Handlers::PlayerJoinRoom, std::placeholders::_1, this));
        this->On(146, std::bind(&Game::Handlers::PlayerWeaponAttack, std::placeholders::_1, this));//gatling fire
        this->On(147, std::bind(&Game::Handlers::PlayerWeaponAttack, std::placeholders::_1, this));//gatling bullet
        this->On(149, std::bind(&Game::Handlers::PlayerWeaponAttack, std::placeholders::_1, this));//melee
        this->On(151, std::bind(&Game::Handlers::PlayerWeaponAttack, std::placeholders::_1, this));//bazooka
        this->On(152, std::bind(&Game::Handlers::PlayerWeaponAttack, std::placeholders::_1, this));//rifle
        this->On(153, std::bind(&Game::Handlers::PlayerWeaponAttack, std::placeholders::_1, this));//shotgun
        this->On(154, std::bind(&Game::Handlers::PlayerWeaponAttack, std::placeholders::_1, this));//sniper
        this->On(155, std::bind(&Game::Handlers::PlayerUnknown6, std::placeholders::_1, this));//bomb battle
        this->On(156, std::bind(&Game::Handlers::PlayerUnknown7, std::placeholders::_1, this));//bomb battle
        this->On(165, std::bind(&Game::Handlers::PlayerUnknown4, std::placeholders::_1, this));//ctb bomb batle
        this->On(166, std::bind(&Game::Handlers::PlayerRespawn, std::placeholders::_1, this));

    #if defined(RELEASE_1_0_3)
        this->On(175, std::bind(&Game::Handlers::PlayerJoinPlaza, std::placeholders::_1, this));
        this->On(176, std::bind(&Game::Handlers::PlayerLeavePlaza, std::placeholders::_1, this));
    #endif

        this->On(252, std::bind(&Game::Handlers::ConnectionAuth, std::placeholders::_1, this));

        this->On(253, std::bind(&Game::Handlers::PlayerCrash, std::placeholders::_1, this));
        this->On(254, std::bind(&Game::Handlers::PlayerEndMatch, std::placeholders::_1, this));
        this->On(255, std::bind(&Game::Handlers::PlayerScoreMatch, std::placeholders::_1, this));
        this->On(256, std::bind(&Game::Handlers::PlayerLeaveMatch, std::placeholders::_1, this));
        this->On(257, std::bind(&Game::Handlers::PlayerLoadMatch, std::placeholders::_1, this));
        this->On(258, std::bind(&Game::Handlers::PlayerStartMatch, std::placeholders::_1, this));

        //this->On(259, std::bind(&Game::Handlers::PlayerUnknown8, std::placeholders::_1, this));// idk maybe observer rounds smth prolly has to be empty

        this->On(260, std::bind(&Game::Handlers::HostSpawnAmmo, std::placeholders::_1, this));
        this->On(261, std::bind(&Game::Handlers::HostTurnZombie, std::placeholders::_1, this));
        this->On(262, std::bind(&Game::Handlers::HostPickupItem, std::placeholders::_1, this));
        this->On(263, std::bind(&Game::Handlers::HostAbility, std::placeholders::_1, this));
        this->On(264, std::bind(&Game::Handlers::HostExplosivesDamage, std::placeholders::_1, this));//bazooka impact
        this->On(265, std::bind(&Game::Handlers::HostWeaponAttack, std::placeholders::_1, this));//gatling fire
        this->On(266, std::bind(&Game::Handlers::HostWeaponAttack, std::placeholders::_1, this));//gatling bullet
        this->On(267, std::bind(&Game::Handlers::HostWeaponAttack, std::placeholders::_1, this));//melee
        this->On(268, std::bind(&Game::Handlers::HostWeaponAttack, std::placeholders::_1, this));//rifle
        this->On(269, std::bind(&Game::Handlers::HostWeaponAttack, std::placeholders::_1, this));//shotgun
        this->On(270, std::bind(&Game::Handlers::HostWeaponAttack, std::placeholders::_1, this));//sniper
     
        this->On(271, std::bind(&Game::Handlers::PlayerUnknown5, std::placeholders::_1, this));//bomb battle maybe objects?
        this->On(272, std::bind(&Game::Handlers::HostExplosives, std::placeholders::_1, this));//bazokoka fire
        this->On(275, std::bind(&Game::Handlers::PlayerReload, std::placeholders::_1, this));
        this->On(276, std::bind(&Game::Handlers::HostRespawn, std::placeholders::_1, this));
        this->On(277, std::bind(&Game::Handlers::CreateRoom, std::placeholders::_1, this));
        this->On(279, std::bind(&Game::Handlers::LeaveRoom, std::placeholders::_1, this));

        this->On(281, std::bind(&Game::Handlers::PlayerMovement, std::placeholders::_1, this));
        this->On(282, std::bind(&Game::Handlers::NpcsMovement, std::placeholders::_1, this));
        this->On(284, std::bind(&Game::Handlers::PlayerNicknames, std::placeholders::_1, this));

        this->On(306, std::bind(&Game::Handlers::PlayerUnknown2, std::placeholders::_1, this));

        this->On(309, std::bind(&Game::Handlers::HostSyncTick, std::placeholders::_1, this));
        this->On(326, std::bind(&Game::Handlers::NpcsProjectiles, std::placeholders::_1, this));
        this->On(328, std::bind(&Game::Handlers::NpcsSpawn, std::placeholders::_1, this));
        this->On(331, std::bind(&Game::Handlers::NpcsAttack, std::placeholders::_1, this));

        this->On(408, std::bind(&Game::Handlers::HostUpdateTick, std::placeholders::_1, this));

    }
    CCastServer::~CCastServer(){}
   
}