#pragma once
namespace Game
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Main;

    namespace Handlers
    {
        inline void ChannelsInfo(SCallbackData& callback, CMainServer* main_server)
        {
            std::shared_lock lock(callback.session->GetMutex());
            CSession* session = callback.session;

            std::vector<MainServerInfo> server_infos;
            for (std::uint32_t i = 1; i < 2; i++)
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

            CMessage frontServerInfoAckMessage = CMessage(session->GetEncryptionKey());
            frontServerInfoAckMessage.SetSession(session->GetSessionId());
            frontServerInfoAckMessage.SetCommand(23, 0, 0, static_cast<std::uint8_t>(server_infos.size()));
            frontServerInfoAckMessage.SetData(reinterpret_cast<uint8_t*>(server_infos.data()), static_cast<std::uint16_t>(server_infos.size() * sizeof(MainServerInfo)));
            session->Send(frontServerInfoAckMessage);

            BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "session id: ({}) received ({})'s servers channel info", session->GetSessionId(), server_infos.size());
        }
    }
    
}