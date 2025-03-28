#include "CFrontServer.h"
#include "BaseLib/Utility.h"
#include "BaseLib/CDatabase.h"
#include "NetEngine/Packets/PacketStruct.h"
#include "NetEngine/Packets/PacketData.h"
#include "handlers/ServerIpcMessage.h"
#include "handlers/ServerConnect.h"
#include "handlers/ServerDisconnect.h"
#include "handlers/LoginAuth.h"
#include "handlers/LoginReconnect.h"
#include "handlers/ChannelsInfo.h"

namespace Game
{    
    std::shared_mutex players_cache_mutex;
    boost::unordered_flat_map<uint64_t, Player> players_cache;
    CFrontServer::CFrontServer()
    {
        this->OnNewSession(std::bind(&Game::Handlers::ServerConnect, std::placeholders::_1, this));
        this->OnSessionDisconnected(std::bind(&Game::Handlers::ServerDisconnect, std::placeholders::_1, this));
        this->OnIpcMessage(std::bind(&Game::Handlers::ServerIpcMessage, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, this));
        this->On(22, std::bind(&Game::Handlers::LoginAuth, std::placeholders::_1, this));
        this->On(23, std::bind(&Game::Handlers::ChannelsInfo, std::placeholders::_1, this));
        this->On(25, std::bind(&Game::Handlers::LoginReconnect, std::placeholders::_1, this));
    }
    CFrontServer::~CFrontServer(){}
}