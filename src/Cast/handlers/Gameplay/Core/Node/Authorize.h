#pragma once
namespace Game::Handlers
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Cast;
    inline void NodeAuthorize(SCallbackData& callback, CCastServer* server)
    {
        auto session = callback.session;
        auto message = callback.message;
        if (!session || !message) return;
		auto req = message->GetData<CastConnectionReq*>();
        req->UniqueId.session = session->GetSessionId();
        server->SendMainIpc(PacketIds::Ipc::CastToMainAcknowledgeAuthPlayer, Utility::ToVector(*req));
		PACKETLOG(REQ, NODE_AUTHORIZE, "sid=({}) key=({}) serverId=({})", session->GetSessionId(), static_cast<uint64_t>(req->Authkey), static_cast<uint32_t>(req->UniqueId.server));
    }
}