#pragma once
namespace Game
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets;

    namespace Handlers
    {
        inline void ChannelsInfo(SCallbackData& callback, CFrontServer* front_server)
        {
            auto session = callback.session;
            if (!session) return;
            std::shared_lock lock(session->GetMutex());
            std::vector<Front::FrontServerInfo> server_infos;
            for (uint32_t i = 1; i < 2; i++)
            {
                Front::FrontServerInfo server_info;
                server_info.serverId = i;
                server_info.channel1 = ChannelInfo::Status::Busy;
                server_info.channel2 = ChannelInfo::Status::Busy;
                server_info.channel3 = ChannelInfo::Status::Busy;
                server_info.channel4 = ChannelInfo::Status::Busy;
                server_info.channel5 = ChannelInfo::Status::Busy;
                server_info.channel6 = ChannelInfo::Status::Busy;
                server_infos.push_back(server_info);
            }
            auto server_info_size = static_cast<uint8_t>(server_infos.size());
            session->SendMsg(23, 0, 0, server_info_size, reinterpret_cast<uint8_t*>(server_infos.data()), static_cast<uint16_t>(server_info_size * sizeof(Front::FrontServerInfo)));
            EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "sent ({}) channels info", server_infos.size());
        }
    }
    
}