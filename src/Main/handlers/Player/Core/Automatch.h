#pragma once
namespace Game::Handlers
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Main;
    using namespace NetEngine::Room::Mode;
    std::vector<Index> GetModesFromMask(uint32_t mask)
    {
        std::vector<Index> result;
        for (uint8_t i = 0; i <= Index::BossBattle; i++)
            if (mask & (1 << i))
                result.push_back(static_cast<Index>(i));

        return result;
    }
    struct AutomatchAck
    {
        uint16_t room_id;
        uint16_t channel_id;
        AutomatchAck(const uint16_t room_id = 0, const uint16_t channel_id = 0)
        {
            std::memset(this, 0, sizeof(AutomatchAck));
            this->room_id = room_id;
            this->channel_id = channel_id;
        }
    };

    inline void Automatch(SCallbackData& callback, CMainServer* main_server)
    {
        auto session = callback.session;
        auto message = callback.message;
        if (!session || !message) return;
        //std::shared_lock lock(session->GetMutex());
        auto sid = session->GetSessionId();
        auto acc = CAccount.get<unique_t>(sid);
        auto aid = acc->acc_info.Index;
        if (aid == -1) return;

        auto mod_pool = *reinterpret_cast<uint32_t*>(message->GetData());
        auto pool = GetModesFromMask(mod_pool);
        std::array<bool, 12> match_mod{};
        for (auto m : pool)
            match_mod[static_cast<uint8_t>(m)] = true;

        //std::shared_lock room_ids_lock(main_server->GetRoomIdsMutex());
        auto room_ids = CRoomId.get_all(shared);
        for (auto i = 0; i < room_ids->size(); i++)
        {
            auto room = CRoom.get<shared_t>(room_ids->at(i));
            if (room->title.empty() || room->has_password || room->ModeIndex > BossBattle || !match_mod[room->ModeIndex]) continue;
            auto room_mod_id = (uint16_t)room->ModeIndex;
            DEBUGLOG(red, "found match mod: ({})", room_mod_id);
            auto res = AutomatchAck(room->room_id, room->channel_id);
            room.unlock();
            room_ids.unlock();
            session->SendMsg(169, 0, 0, 1, reinterpret_cast<uint8_t*>(&res), sizeof(res));
            return;
        }
        room_ids.unlock();
        session->SendMsg(169, 0, 2, 1);
    }
}