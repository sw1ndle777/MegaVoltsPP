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
            std::shared_lock lock(callback.session->GetMutex());
            CSession* session = callback.session;
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

            CMessage frontServerInfoAckMessage = CMessage(session->GetEncryptionKey());
            frontServerInfoAckMessage.SetSession(session->GetSessionId());
            frontServerInfoAckMessage.SetCommand(0x17, 0x00, 0, static_cast<uint8_t>(server_infos.size()));
            frontServerInfoAckMessage.SetData(reinterpret_cast<uint8_t*>(server_infos.data()), static_cast<uint16_t>(server_infos.size() * sizeof(Front::FrontServerInfo)));

            session->Send(frontServerInfoAckMessage);
            EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "sent ({}) channels info", server_infos.size());
        }
    }
    
}