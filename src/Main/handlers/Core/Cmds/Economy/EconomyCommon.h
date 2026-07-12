#pragma once
namespace Game::Commands
{
    struct OnlineTarget
    {
        uint16_t sid{};
        int32_t aid{};
        std::string nickname{};
    };

    [[nodiscard]] inline std::optional<OnlineTarget> FindOnlinePlayerByNickname(std::string_view nickname)
    {
        // IMPORTANT: only take the map-level shared lock, never a per-entry lock.
        // The chat-command dispatch already holds a unique lock on the *caller's* own
        // account entry. get_by_filter<>() would lock the matched entry too, and if the
        // match is the caller (targeting yourself by name) that re-locks an entry we
        // already hold unique on a non-recursive shared_mutex -> permanent self-deadlock
        // that also wedges every CAccount insert/erase (logins/disconnects). We only need
        // to read copyable fields, so the map lock alone is sufficient and deadlock-free.
        const auto wanted = Utility::ToLowercase(nickname);
        auto accounts = CAccount.get_all(BaseLib::shared);
        for (const auto& entry : *accounts)
        {
            const auto& candidate = entry.second;
            if (candidate.acc_info.Index > 0 &&
                Utility::ToLowercase(candidate.acc_info.Nickname) == wanted)
                return OnlineTarget{ candidate.session_id, candidate.acc_info.Index, candidate.acc_info.Nickname };
        }
        return std::nullopt;
    }

    inline void SendCurrencyPacket(CSession* session, AccCacheResource& acc)
    {
        NetEngine::Packets::Main::MainCurrencyUpdateAck cur{
            acc->acc_info.RockTokens,
            acc->acc_info.MicroPoints,
            acc->acc_info.Coins
        };
        session->SendMsg(307, 0, 1, 0, reinterpret_cast<uint8_t*>(&cur), sizeof(cur));
    }

    inline void SendAccountInfoRefresh(CMainServer* server, CSession* session, AccCacheResource& acc)
    {
        auto clan_name = std::string{};
        uint32_t logo_front = 0, logo_back = 0;
        if (acc->acc_info.ClanId && CClan.contains(acc->acc_info.ClanId))
        {
            auto clan = CClan.get<shared_t>(acc->acc_info.ClanId);
            clan_name = clan->clan_name;
            logo_front = clan->logo_front;
            logo_back = clan->logo_back;
        }
        auto msg = server->CraftAccInfoAck(acc, acc->server_id, clan_name.c_str(), logo_front, logo_back);
        session->SendMsg(413, 0, 1, 1, reinterpret_cast<uint8_t*>(&msg), sizeof(msg));
    }

    inline void SendInventoryRefresh(CMainServer* server, CSession* session, AccCacheResource& acc)
    {
        using namespace NetEngine::Packets::Main;
        std::vector<BaseLib::Item> items;

        if (acc->acc_info.Coupons > 0)
        {
            InventoryItemInfo coupon_info = { {1000000, acc->acc_info.Coupons},
                Utility::GetUnixEpoch(), ItemSerialInfo(0, 0, 0, 0, Utility::GetUnixEpoch()), 0, 0 };
            BaseLib::Item coupon_item = { coupon_info, acc->acc_info.Coupons, 0, 0 };
            items.push_back(coupon_item);
        }

        for (const auto& item : acc->inventory_items)
            items.push_back(item);

        if (items.empty())
        {
            session->SendMsg(77, 0, 6, 0);
            return;
        }

        constexpr uint32_t max_packet_size = 1440;
        constexpr uint32_t full_header_size = 8;
        constexpr uint32_t split_size = (max_packet_size - full_header_size) / sizeof(InventoryItemInfo);
        uint32_t total_fragments = static_cast<uint32_t>((items.size() + split_size - 1) / split_size);

        for (uint32_t i = 0; i < total_fragments; i++)
        {
            auto items_batch = server->GetTransformStockItems(items, i, split_size);
            if (!items_batch.empty())
                session->SendMsg(77, 0, (i == 0) ? 37 : 0,
                    static_cast<uint8_t>(items_batch.size()),
                    reinterpret_cast<uint8_t*>(items_batch.data()),
                    items_batch.size() * sizeof(InventoryItemInfo));
        }
    }
}
