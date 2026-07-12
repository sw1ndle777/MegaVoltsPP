#pragma once
#include <algorithm>
#include <mutex>
#include <optional>
#include <unordered_map>
#include <vector>

// =============================================================================
// Player<->player Trade (orders 190-199). Mirrors the MegaGuard client
// trade_handler protocol: orders are symmetric both directions, the result code
// rides in CommandHeader.extra, and item serials sit at body+8.
//
// Flow:
//   A clicks Trade -> 191 (by target name).
//   server resolves target B, relays 191 to B carrying A's MainTradePlayerInfo.
//   B's client auto-accepts -> 192 (carries A's account id it just received).
//   server pairs A<->B, sends 192 (A's invite answered, B's info) to A and
//   190 (open, A's info) to B -> both trade windows open.
//   Either side 194/195 to offer/withdraw an item -> relayed to the partner.
//   196 lock / 197 confirm relayed; when BOTH have confirmed -> 199 to both.
//   198 cancel (or a confirm completing) ends the trade for both sides.
//
// NOTE: the actual inventory transfer at 199 is intentionally NOT done here yet
// (irreversible DB mutation; landed separately once the flow is verified in
// game). Today 199 just closes both windows after a clean handshake.
// =============================================================================
namespace Game::Handlers
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Main;

    // Result codes carried in CommandHeader.extra (mirror the client TradeExtra).
    namespace TradeResult
    {
        constexpr uint8_t Success         = 1;
        constexpr uint8_t Declined        = 31;
        constexpr uint8_t CannotOrOffline = 47;
    }

    // ── In-memory trade state ────────────────────────────────────────────────
    // Self-contained, guarded by its own mutex. One entry per participant; both
    // entries reference each other. Keyed by account id (aid).
    struct TradeEntry
    {
        int32_t                 partner_aid{ -1 };
        Packets::Core::UniqueId partner_uid{};
        std::vector<Packets::Main::ItemSerialInfo> offered; // serials this side put up
        bool                    locked{ false };
        bool                    confirmed{ false };
    };

    class CTradeRegistry
    {
    public:
        static CTradeRegistry& Instance() { static CTradeRegistry r; return r; }

        void RegisterInvite(int32_t inviter_aid, Packets::Core::UniqueId inviter_uid, int32_t invitee_aid)
        {
            std::scoped_lock lk(m_mutex);
            m_pending[invitee_aid] = { inviter_aid, inviter_uid };
        }

        // Validate + consume a pending invite. Returns the inviter's uid if the
        // accept matches an outstanding invite from that exact inviter.
        std::optional<Packets::Core::UniqueId> TakeInvite(int32_t invitee_aid, int32_t inviter_aid)
        {
            std::scoped_lock lk(m_mutex);
            auto it = m_pending.find(invitee_aid);
            if (it == m_pending.end() || it->second.first != inviter_aid) return std::nullopt;
            auto uid = it->second.second;
            m_pending.erase(it);
            return uid;
        }

        void Begin(int32_t a, Packets::Core::UniqueId a_uid, int32_t b, Packets::Core::UniqueId b_uid)
        {
            std::scoped_lock lk(m_mutex);
            EndLocked(a);  // drop any stale state on either side first
            EndLocked(b);
            TradeEntry ea; ea.partner_aid = b; ea.partner_uid = b_uid; m_active[a] = std::move(ea);
            TradeEntry eb; eb.partner_aid = a; eb.partner_uid = a_uid; m_active[b] = std::move(eb);
        }

        std::optional<TradeEntry> Get(int32_t aid)
        {
            std::scoped_lock lk(m_mutex);
            auto it = m_active.find(aid);
            if (it == m_active.end()) return std::nullopt;
            return it->second;
        }

        // An offer change voids any prior lock/confirm on BOTH sides.
        void AddOffer(int32_t aid, Packets::Main::ItemSerialInfo serial)
        {
            std::scoped_lock lk(m_mutex);
            auto it = m_active.find(aid);
            if (it == m_active.end()) return;
            auto& list = it->second.offered;
            if (std::none_of(list.begin(), list.end(), [&](auto& s) { return s.data == serial.data; }))
                list.push_back(serial);
            ClearConfirmsLocked(aid);
        }
        void RemoveOffer(int32_t aid, Packets::Main::ItemSerialInfo serial)
        {
            std::scoped_lock lk(m_mutex);
            auto it = m_active.find(aid);
            if (it == m_active.end()) return;
            auto& list = it->second.offered;
            list.erase(std::remove_if(list.begin(), list.end(), [&](auto& s) { return s.data == serial.data; }), list.end());
            ClearConfirmsLocked(aid);
        }
        void SetLocked(int32_t aid)    { std::scoped_lock lk(m_mutex); if (auto it = m_active.find(aid); it != m_active.end()) it->second.locked = true; }
        void SetConfirmed(int32_t aid) { std::scoped_lock lk(m_mutex); if (auto it = m_active.find(aid); it != m_active.end()) it->second.confirmed = true; }

        bool BothConfirmed(int32_t aid)
        {
            std::scoped_lock lk(m_mutex);
            auto it = m_active.find(aid);
            if (it == m_active.end()) return false;
            auto pit = m_active.find(it->second.partner_aid);
            return it->second.confirmed && pit != m_active.end() && pit->second.confirmed;
        }

        void End(int32_t aid) { std::scoped_lock lk(m_mutex); EndLocked(aid); }

    private:
        void ClearConfirmsLocked(int32_t aid)
        {
            if (auto it = m_active.find(aid); it != m_active.end())
            {
                it->second.locked = it->second.confirmed = false;
                if (auto pit = m_active.find(it->second.partner_aid); pit != m_active.end())
                    pit->second.locked = pit->second.confirmed = false;
            }
        }
        void EndLocked(int32_t aid)
        {
            auto it = m_active.find(aid);
            if (it == m_active.end()) return;
            int32_t partner = it->second.partner_aid;
            m_active.erase(it);
            if (partner >= 0) m_active.erase(partner);
        }

        std::mutex m_mutex;
        std::unordered_map<int32_t, std::pair<int32_t, Packets::Core::UniqueId>> m_pending; // invitee_aid -> (inviter_aid, inviter_uid)
        std::unordered_map<int32_t, TradeEntry> m_active;                                   // aid -> entry (both sides present)
    };

    // ── Item swap at completion (order 199) ──────────────────────────────────
    // Resolve the item_ids for a set of offered serials from an account's live
    // inventory (read under a shared lock).
    inline std::vector<uint32_t> ResolveOfferedItemIds(
        const std::vector<Packets::Main::ItemSerialInfo>& offered, int32_t aid, uint16_t sid)
    {
        std::vector<uint32_t> ids;
        auto acc = CAccount.get<shared_t>(sid);
        if (acc->acc_info.Index != aid) return ids;
        for (const auto& s : offered)
            for (const auto& it : acc->inventory_items)
                if (it.item_info.serial_info.data == s.data) { ids.push_back(it.item_info.item_number.item_id); break; }
        return ids;
    }

    // Apply one side of the swap: delete this account's offered serials and grant it
    // the partner's offered item_ids. Mirrors the GM /item (add) + /clearinv (delete)
    // DB paths: build a DatabaseUpdateCtx, validate, then UpdateAccount +
    // ApplyDatabaseUpdates off the DbPool, finally ack the client (89 delete, 99 add).
    inline void ApplyTradeSide(CMainServer* main_server, Packets::Core::UniqueId uid, int32_t aid,
        const std::vector<Packets::Main::ItemSerialInfo>& myOffered,
        const std::vector<uint32_t>& receivedItemIds)
    {
        auto session = main_server->GetSessionById(uid.session);
        if (!session) return;
        if (myOffered.empty() && receivedItemIds.empty()) return;

        DatabaseUpdateCtx dctx{ .sid = uid.session, .aid = aid };
        for (const auto& s : myOffered)
            dctx.ops.emplace_back(ItemDeleteCtx{ .serials = { s } });

        auto acc = CAccount.get<unique_t>(uid.session);
        if (acc->acc_info.Index != aid) return;
        if (!receivedItemIds.empty())
        {
            auto crafted = main_server->CraftInventoryItems(acc, receivedItemIds, NetEngine::Items::Origin::From_Game);
            if (!crafted.has_value())
            {
                DEBUGLOG(red, "trade swap: craft failed for [{}]: {}", aid, static_cast<int>(crafted.error()));
                return;
            }
            dctx.ops.push_back(crafted.value());
        }
        auto validated = main_server->ValidateDatabaseUpdates(acc, dctx, true);
        if (!validated.has_value())
        {
            DEBUGLOG(red, "trade swap: validate failed for [{}]: {}", aid, static_cast<int>(validated.error()));
            return;
        }
        acc.unlock();

        auto deleted = myOffered;
        [[maybe_unused]] auto ignored = BaseLib::DbPool->submit_task(
            [main_server, session, v = std::move(validated.value()), deleted = std::move(deleted)]() mutable
            {
                if (!session) return;
                ResultDbUpdateInfo dbres;
                if (!BaseLib::Database->UpdateAccount(v, dbres).has_value()) return;
                auto na = CAccount.get<unique_t>(session->GetSessionId());
                auto applied = main_server->ApplyDatabaseUpdates(na, v);
                if (!applied.has_value())
                {
                    DEBUGLOG(red, "trade swap: apply failed for [{}]: {}", na->acc_info.Index, static_cast<int>(applied.error()));
                    return;
                }
                for (const auto& s : deleted)
                {
                    auto d = MainDeleteItemAck({ s }).Serialize();
                    session->SendMsg(89, 0, 1, 0, reinterpret_cast<uint8_t*>(d.data()), d.size());
                }
                if (!v.items_added.empty())
                {
                    std::vector<ShopItem> shop;
                    for (const auto& item : v.items_added)
                    {
                        auto ii = CItemsInfo.get<shared_t>(item.item_info.item_number.item_id);
                        shop.push_back({ { item.item_info.item_number.item_id, ii->Stock }, ItemExpire::Type::Unused, item.item_info.serial_info });
                    }
                    session->SendMsg(99, 0, 37, static_cast<uint8_t>(shop.size()),
                        reinterpret_cast<uint8_t*>(shop.data()), shop.size() * sizeof(ShopItem));
                }
            });
    }

    // ── 191: A invites a player by name ──────────────────────────────────────
    inline void TradeRequest(SCallbackData& callback, CMainServer* main_server)
    {
        auto session = callback.session;
        auto message = callback.message;
        if (!session || !message) return;

        auto sid = session->GetSessionId();
        int32_t aid; uint32_t my_char; Packets::Core::UniqueId my_uid; std::string my_nick;
        {
            auto acc = CAccount.get<shared_t>(sid);
            aid      = acc->acc_info.Index;
            my_char  = acc->acc_info.SelectedCharacter;
            my_uid   = acc->uid;
            my_nick  = acc->acc_info.Nickname;
        }
        if (aid == -1) return;

        auto req = reinterpret_cast<MainTradeInviteReq*>(message->GetData());
        std::string target = Utility::ReadMicrovoltsString(req->target_name, sizeof(req->target_name));
        if (target.empty() || Utility::ToLowercase(target) == Utility::ToLowercase(my_nick))
            return;

        int32_t target_aid = -1; Packets::Core::UniqueId target_uid{};
        {
            auto target_acc = CAccount.get_by_filter<shared_t>([&](const auto& /*id*/, auto& player) {
                return Utility::ToLowercase(player.acc_info.Nickname) == Utility::ToLowercase(target);
                });
            if (target_acc->acc_info.Index)
            {
                target_aid = target_acc->acc_info.Index;
                target_uid = target_acc->uid;
            }
        }
        if (target_aid == -1)
        {
            DEBUGLOG(yellow, "trade: ({}) invited offline/unknown player ({})", my_nick.c_str(), target.c_str());
            session->SendMsg(static_cast<uint16_t>(EOrder::TRADE_ACCEPT), 0, TradeResult::CannotOrOffline, 0);
            return;
        }

        CTradeRegistry::Instance().RegisterInvite(aid, my_uid, target_aid);

        MainTradePlayerInfo me(static_cast<uint32_t>(aid), my_char, my_nick.c_str());
        if (auto ts = main_server->GetSessionById(target_uid.session))
        {
            ts->SendMsg(static_cast<uint16_t>(EOrder::TRADE_REQUEST), 0, TradeResult::Success, 0,
                reinterpret_cast<uint8_t*>(&me), sizeof(me));
            DEBUGLOG(green, "trade: ({}) [{}] invited ({}) [{}]", my_nick.c_str(), aid, target.c_str(), target_aid);
        }
    }

    // ── 192: B accepts a pending invite (carries the inviter's account id) ────
    inline void TradeAccept(SCallbackData& callback, CMainServer* main_server)
    {
        auto session = callback.session;
        auto message = callback.message;
        if (!session || !message) return;

        auto sid = session->GetSessionId();
        int32_t aid; uint32_t my_char; Packets::Core::UniqueId my_uid; std::string my_nick;
        {
            auto acc = CAccount.get<shared_t>(sid);
            aid     = acc->acc_info.Index;
            my_char = acc->acc_info.SelectedCharacter;
            my_uid  = acc->uid;
            my_nick = acc->acc_info.Nickname;
        }
        if (aid == -1) return;

        auto req = reinterpret_cast<MainTradeAcceptReq*>(message->GetData());
        int32_t inviter_aid = req->inviter_id;

        auto inviter_uid = CTradeRegistry::Instance().TakeInvite(aid, inviter_aid);
        if (!inviter_uid.has_value()) return; // no matching outstanding invite

        auto inviter_session = main_server->GetSessionById(inviter_uid->session);
        if (!inviter_session) return;          // inviter went offline

        uint32_t inviter_char = 0; std::string inviter_nick;
        {
            auto inv_acc = CAccount.get<shared_t>(inviter_uid->session);
            if (inv_acc->acc_info.Index == inviter_aid)
            {
                inviter_char = inv_acc->acc_info.SelectedCharacter;
                inviter_nick = inv_acc->acc_info.Nickname;
            }
        }

        CTradeRegistry::Instance().Begin(inviter_aid, *inviter_uid, aid, my_uid);

        // Inviter (A): "your invite was answered" + B's player record (with B's name).
        MainTradePlayerInfo b_info(static_cast<uint32_t>(aid), my_char, my_nick.c_str());
        inviter_session->SendMsg(static_cast<uint16_t>(EOrder::TRADE_ACCEPT), 0, TradeResult::Success, 0,
            reinterpret_cast<uint8_t*>(&b_info), sizeof(b_info));

        // Accepter (B): open the dialog with A's player record (with A's name).
        MainTradePlayerInfo a_info(static_cast<uint32_t>(inviter_aid), inviter_char, inviter_nick.c_str());
        session->SendMsg(static_cast<uint16_t>(EOrder::TRADE_OPEN), 0, TradeResult::Success, 0,
            reinterpret_cast<uint8_t*>(&a_info), sizeof(a_info));

        DEBUGLOG(green, "trade: pair opened {} <-> {}", inviter_aid, aid);
    }

    // ── 194/195: offer / withdraw an item, relayed to the partner ────────────
    inline void TradeItemChange(SCallbackData& callback, CMainServer* main_server)
    {
        auto session = callback.session;
        auto message = callback.message;
        if (!session || !message) return;

        auto sid = session->GetSessionId();
        int32_t aid; uint32_t item_id = 0;
        auto req = reinterpret_cast<MainTradeItemReq*>(message->GetData());
        const auto serial = req->serial;
        {
            auto acc = CAccount.get<shared_t>(sid);
            aid = acc->acc_info.Index;
            for (auto& it : acc->inventory_items)
                if (it.item_info.serial_info.data == serial.data) { item_id = it.item_info.item_number.item_id; break; }
        }
        if (aid == -1) return;

        auto& reg = CTradeRegistry::Instance();
        auto st = reg.Get(aid);
        if (!st.has_value()) return;

        const uint16_t order = message->GetOrder();
        if (order == static_cast<uint16_t>(EOrder::TRADE_ITEM_ADD)) reg.AddOffer(aid, serial);
        else                                                        reg.RemoveOffer(aid, serial);

        MainTradeItemInfo info(static_cast<uint32_t>(aid), serial, item_id);
        if (auto ps = main_server->GetSessionById(st->partner_uid.session))
            ps->SendMsg(order, 0, TradeResult::Success, 0, reinterpret_cast<uint8_t*>(&info), sizeof(info));
    }

    // ── 196: a side locked its offer ─────────────────────────────────────────
    inline void TradeLock(SCallbackData& callback, CMainServer* main_server)
    {
        auto session = callback.session;
        if (!session || !callback.message) return;
        auto sid = session->GetSessionId();
        int32_t aid;
        { auto acc = CAccount.get<shared_t>(sid); aid = acc->acc_info.Index; }
        if (aid == -1) return;

        auto& reg = CTradeRegistry::Instance();
        auto st = reg.Get(aid);
        if (!st.has_value()) return;
        reg.SetLocked(aid);
        if (auto ps = main_server->GetSessionById(st->partner_uid.session))
            ps->SendMsg(static_cast<uint16_t>(EOrder::TRADE_LOCK), 0, TradeResult::Success, 0);
    }

    // ── 197: a side confirmed; when both have, complete the trade ────────────
    inline void TradeConfirm(SCallbackData& callback, CMainServer* main_server)
    {
        auto session = callback.session;
        if (!session || !callback.message) return;
        auto sid = session->GetSessionId();
        int32_t aid; Packets::Core::UniqueId my_uid;
        { auto acc = CAccount.get<shared_t>(sid); aid = acc->acc_info.Index; my_uid = acc->uid; }
        if (aid == -1) return;

        auto& reg = CTradeRegistry::Instance();
        auto st = reg.Get(aid);
        if (!st.has_value()) return;
        reg.SetConfirmed(aid);

        if (reg.BothConfirmed(aid))
        {
            const int32_t partner_aid = st->partner_aid;
            const auto     partner_uid = st->partner_uid;

            // Snapshot both sides' offered serials, then end the trade state.
            auto meE = reg.Get(aid);
            auto partnerE = reg.Get(partner_aid);
            std::vector<Packets::Main::ItemSerialInfo> myOffered      = meE      ? meE->offered      : std::vector<Packets::Main::ItemSerialInfo>{};
            std::vector<Packets::Main::ItemSerialInfo> partnerOffered = partnerE ? partnerE->offered : std::vector<Packets::Main::ItemSerialInfo>{};
            reg.End(aid);

            // Resolve item_ids from each owner's live inventory BEFORE mutating, then
            // apply: each side loses what it offered and gains what the partner offered.
            auto myItemIds      = ResolveOfferedItemIds(myOffered,      aid,         my_uid.session);
            auto partnerItemIds = ResolveOfferedItemIds(partnerOffered, partner_aid, partner_uid.session);
            ApplyTradeSide(main_server, my_uid,      aid,         myOffered,      partnerItemIds);
            ApplyTradeSide(main_server, partner_uid, partner_aid, partnerOffered, myItemIds);

            session->SendMsg(static_cast<uint16_t>(EOrder::TRADE_COMPLETE), 0, TradeResult::Success, 0);
            if (auto ps = main_server->GetSessionById(partner_uid.session))
                ps->SendMsg(static_cast<uint16_t>(EOrder::TRADE_COMPLETE), 0, TradeResult::Success, 0);
            DEBUGLOG(green, "trade: complete {} <-> {} (swap: {}<-{} items)", aid, partner_aid, partnerItemIds.size(), myItemIds.size());
        }
        else if (auto ps = main_server->GetSessionById(st->partner_uid.session))
        {
            ps->SendMsg(static_cast<uint16_t>(EOrder::TRADE_CONFIRM), 0, TradeResult::Success, 0);
        }
    }

    // ── 198: a side cancelled / closed the window ────────────────────────────
    inline void TradeCancel(SCallbackData& callback, CMainServer* main_server)
    {
        auto session = callback.session;
        if (!session || !callback.message) return;
        auto sid = session->GetSessionId();
        int32_t aid;
        { auto acc = CAccount.get<shared_t>(sid); aid = acc->acc_info.Index; }
        if (aid == -1) return;

        auto& reg = CTradeRegistry::Instance();
        auto st = reg.Get(aid);
        if (!st.has_value()) return;
        auto partner_uid = st->partner_uid;
        reg.End(aid);

        session->SendMsg(static_cast<uint16_t>(EOrder::TRADE_CANCEL), 0, TradeResult::Success, 0);
        if (auto ps = main_server->GetSessionById(partner_uid.session))
            ps->SendMsg(static_cast<uint16_t>(EOrder::TRADE_CANCEL), 0, TradeResult::Success, 0);
        DEBUGLOG(yellow, "trade: cancelled by {} (partner {})", aid, st->partner_aid);
    }
}
