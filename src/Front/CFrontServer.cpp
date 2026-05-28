#include "CFrontServer.h"
#include "BaseLib/Utility.h"
#include "BaseLib/CDatabase.h"
#include "NetEngine/Packets/PacketStruct.h"
#include "NetEngine/Packets/PacketData.h"

#include "handlers/Player/Core/Authorize.h"
#include "handlers/Player/Core/Channels.h"
#include "handlers/Player/Core/Reconnect.h"

#include "handlers/Core/Connect.h"
#include "handlers/Core/Disconnect.h"
#include "handlers/Core/Ipc/MainAidOnline.h"
#include "handlers/Core/Ipc/MainDisconnect.h"
#include "handlers/Core/Ipc.h"


namespace Game
{    
	CCache<boost::unordered_flat_map<int32_t, Player>> CAccount;
    CCache<boost::unordered_flat_set<uint64_t>> CAuthKeys;
    NetEngine::RateLimit::IdentitySnapshot CFrontServer::BuildPacketRateLimitIdentitySnapshot(const SCallbackData& callback)
    {
        NetEngine::RateLimit::IdentitySnapshot snapshot{};
        if (!callback.session)
            return snapshot;

        snapshot.sid = callback.session->GetSessionId();
        snapshot.ip = callback.session->GetIpAddress();

        auto accounts = CAccount.get_all(shared);
        for (const auto& [aid, player] : *accounts)
        {
            if (player.sid != snapshot.sid)
                continue;

            snapshot.aid = aid;
            break;
        }

        return snapshot;
    }
    CFrontServer::CFrontServer()
    {
        using namespace Game::Handlers;
        using enum EOrder;
        this->OnNewSession(std::bind(&ServerConnect, std::placeholders::_1, this));
        this->OnSessionDisconnected(std::bind(&ServerDisconnect, std::placeholders::_1, this));
        this->OnIpcMessage(std::bind(&ServerIpc, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, this));
        this->BindPacketHandler<&Authorize>(this, AUTH_AUTHORIZE);
        this->BindPacketHandler<&Channels>(this, INFO_SERVER);
        this->BindPacketHandler<&Reconnect>(this, AUTH_RETRY);
    }
    CFrontServer::~CFrontServer(){}
}
