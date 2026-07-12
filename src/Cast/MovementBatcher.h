#pragma once
#include <array>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <functional>
#include <memory>
#include <mutex>
#include <asio.hpp>
#include <boost_unordered.hpp>

namespace Game
{
    // Per-room movement batcher.
    //
    // Accumulates the latest movement entry per source player and emits them on a
    // fixed-rate flush as a single multi-entry cmd-322, instead of one packet per
    // received USER_MOVE. This turns the rebroadcast from O(N^2) packets/sec
    // (N players x send-rate x N-1 recipients) into O(N) (one packet per recipient
    // per flush, carrying every mover's entry).
    //
    // Client constraints this respects (verified in IDA on the MegaVolts Alpha shared
    // client; see memory note movement-batch-client-constraints):
    //  * Framing is a true stream reassembler (CTcpConnector::Read), so concatenating
    //    packets is safe, but each individual packet must stay <= 2047B total
    //    (8B header + payload) => payload (tick + entries) <= 2039B; we split otherwise.
    //  * OTHER_MOVE reads ONE front tick per packet (payload[0]) and applies
    //    waypoint.timestamp = tick/100 to EVERY entry in that packet. So the front tick
    //    MUST be uniform across the packet and monotonic across flushes. We use the max
    //    source matchTick in the packet, clamped to never decrease per room. Echoing a
    //    single player's matchTick into a multi-entry packet (the ToyBattleHQ bug)
    //    corrupts every other player's interpolation pacing -> rubberband/lag.
    //  * A GLOBAL per-packet stall clock (sub_549930) trips when movement packets gap
    //    >100ms, so the flush must be fixed-rate and well under that (128 Hz ~ 7.8ms).
    //
    // An "entry" here is exactly the cmd-322 response struct WITHOUT its leading 4-byte
    // tick (SpecificInfo + position + direction + [bullets] + rotation + [jump]); the
    // client strides entries by the SpecificInfo enable flags, so variable lengths
    // (20/24/28/32) concatenate transparently.
    class MovementBatcher
    {
    public:
        // entries = concatenated per-player entry bodies (no per-entry tick).
        using FlushFn = std::function<void(uint16_t room_id, uint32_t tick,
                                           const uint8_t* entries, uint16_t entries_len,
                                           uint8_t count)>;

        static constexpr uint16_t kMaxEntryLen   = 32;            // "complete" variant
        static constexpr uint16_t kMaxPayload    = 2039;          // 2047 - 8B headers
        static constexpr uint16_t kMaxEntriesLen = kMaxPayload - 4; // minus front tick

        void Start(asio::io_context& io, std::chrono::microseconds interval, FlushFn fn)
        {
            m_interval = interval;
            m_flush = std::move(fn);
            m_timer = std::make_unique<asio::steady_timer>(io);
            m_running.store(true, std::memory_order_release);
            m_next = std::chrono::steady_clock::now() + m_interval;
            arm();
        }

        void Stop()
        {
            m_running.store(false, std::memory_order_release);
            if (m_timer)
            {
                try { m_timer->cancel(); } catch (...) {}
            }
        }

        // Called from packet-handler threads. Latest sample per (room, sid) wins.
        void Submit(uint16_t room_id, uint16_t sid, const uint8_t* entry, uint8_t len, uint32_t matchTick)
        {
            if (len == 0 || len > kMaxEntryLen) return;
            std::lock_guard<std::mutex> lk(m_mtx);
            auto& room = m_rooms[room_id];
            auto& e = room.entries[sid];
            e.len = len;
            e.matchTick = matchTick;
            std::memcpy(e.bytes.data(), entry, len);
        }

        // Optional: drop a room's pending state on teardown (not required for safety;
        // stale entries flush at most once more, then stop as submits cease).
        void DropRoom(uint16_t room_id)
        {
            std::lock_guard<std::mutex> lk(m_mtx);
            m_rooms.erase(room_id);
        }

    private:
        struct Entry
        {
            std::array<uint8_t, kMaxEntryLen> bytes{};
            uint32_t matchTick{};
            uint8_t len{};
        };
        struct RoomBatch
        {
            boost::unordered_flat_map<uint16_t, Entry> entries;
            uint32_t lastTick{}; // monotonic front-tick high-water mark
        };

        void arm()
        {
            if (!m_running.load(std::memory_order_acquire) || !m_timer) return;
            m_timer->expires_at(m_next);
            m_timer->async_wait([this](const asio::error_code& ec)
            {
                if (ec || !m_running.load(std::memory_order_acquire)) return;
                flush();
                // Fixed cadence, drift-free; if we fell behind, resync to now.
                const auto now = std::chrono::steady_clock::now();
                m_next += m_interval;
                if (m_next < now) m_next = now + m_interval;
                arm();
            });
        }

        void flush()
        {
            // Drain under lock into a local snapshot, then build + emit WITHOUT the lock
            // (never call the broadcast callback while holding the accumulator mutex).
            boost::unordered_flat_map<uint16_t, RoomBatch> drained;
            {
                std::lock_guard<std::mutex> lk(m_mtx);
                if (m_rooms.empty()) return;
                for (auto& [rid, rb] : m_rooms)
                {
                    if (rb.entries.empty()) continue;
                    RoomBatch snap;
                    snap.lastTick = rb.lastTick;
                    snap.entries = std::move(rb.entries);
                    rb.entries.clear();
                    drained.emplace(rid, std::move(snap));
                }
            }

            for (auto& [rid, rb] : drained)
            {
                const uint32_t newLast = emitRoom(rid, rb);
                std::lock_guard<std::mutex> lk(m_mtx);
                auto it = m_rooms.find(rid);
                if (it != m_rooms.end() && newLast > it->second.lastTick)
                    it->second.lastTick = newLast;
            }
        }

        // Emits one or more packets for a room, splitting at the 2039B / 255-entry cap.
        // Returns the highest tick emitted (for monotonic clamping).
        uint32_t emitRoom(uint16_t rid, RoomBatch& rb)
        {
            uint8_t buf[kMaxEntriesLen];
            uint16_t used = 0;
            uint8_t count = 0;
            uint32_t maxTick = rb.lastTick;
            uint32_t lastEmitted = rb.lastTick;

            auto emit = [&]()
            {
                if (count == 0) return;
                uint32_t tick = maxTick;
                if (tick < lastEmitted) tick = lastEmitted; // never go backward
                if (m_flush) m_flush(rid, tick, buf, used, count);
                lastEmitted = tick;
                used = 0;
                count = 0;
                maxTick = lastEmitted;
            };

            for (auto& [sid, e] : rb.entries)
            {
                if (e.len == 0) continue;
                if (used + e.len > kMaxEntriesLen || count == 255)
                    emit();
                std::memcpy(buf + used, e.bytes.data(), e.len);
                used += e.len;
                ++count;
                if (e.matchTick > maxTick) maxTick = e.matchTick;
            }
            emit();
            return lastEmitted;
        }

        std::mutex m_mtx;
        boost::unordered_flat_map<uint16_t, RoomBatch> m_rooms;
        std::unique_ptr<asio::steady_timer> m_timer;
        std::chrono::microseconds m_interval{ std::chrono::microseconds(7813) };
        std::chrono::steady_clock::time_point m_next{};
        FlushFn m_flush;
        std::atomic<bool> m_running{ false };
    };
}
