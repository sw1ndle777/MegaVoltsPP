#pragma once
namespace Game::Handlers
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Main;

    inline void Channels(SCallbackData& callback, CMainServer* main_server)
    {
        auto session = callback.session;
        auto message = callback.message;
        if (!session || !message) return;

        //std::shared_lock lock(session->GetMutex());

        std::vector<MainServerInfo> server_infos;
        for (uint32_t i = 1; i < 2; i++)
        {
            MainServerInfo server_info;
            server_info.serverId = i;
            server_info.channel1 = ChannelInfo::Status::Busy;
            server_info.channel2 = ChannelInfo::Status::Busy;
            server_info.channel3 = ChannelInfo::Status::Busy;
            server_info.channel4 = ChannelInfo::Status::Busy;
            server_info.channel5 = ChannelInfo::Status::Busy;
            server_info.channel6 = ChannelInfo::Status::Busy;
            server_infos.push_back(server_info);
        }

        session->SendMsg(23, 0, 0, static_cast<uint8_t>(server_infos.size()), reinterpret_cast<uint8_t*>(server_infos.data()), static_cast<uint16_t>(server_infos.size() * sizeof(MainServerInfo)));

        DEBUGLOG(dark_cyan, "sid=({}) received ({})'s servers channel info", session->GetSessionId(), server_infos.size());
    }
}