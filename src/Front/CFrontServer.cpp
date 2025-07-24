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
		using namespace NetEngine::PacketId::Front;
        this->OnNewSession(std::bind(&Handlers::ServerConnect, std::placeholders::_1, this));
        this->OnSessionDisconnected(std::bind(&Handlers::ServerDisconnect, std::placeholders::_1, this));
        this->OnIpcMessage(std::bind(&Handlers::ServerIpcMessage, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, this));
        this->On(22, std::bind(&Handlers::LoginAuth, std::placeholders::_1, this));
        this->On(23, std::bind(&Handlers::ChannelsInfo, std::placeholders::_1, this));
        this->On(25, std::bind(&Handlers::LoginReconnect, std::placeholders::_1, this));
    }
    CFrontServer::~CFrontServer(){}
}