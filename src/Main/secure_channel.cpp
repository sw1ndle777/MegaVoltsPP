
#include "secure_channel.hpp"
#include "CMainServer.h"
#include "BaseLib/Utility.h"
#include "BaseLib/CLog.h"
#include <algorithm>
#include <fstream>
#include <filesystem>
#include <vector>
#include <magic_enum.hpp>
#if defined(__linux__)
#include <unistd.h>
#endif
#include <rapidjson/document.h>
#include <rapidjson/error/en.h>

namespace Game::Anticheat {

namespace {

std::filesystem::path GetExecutableDirectory()
{
#if defined(__linux__)
    char exePath[4096]{};
    const auto len = ::readlink("/proc/self/exe", exePath, sizeof(exePath) - 1);
    if (len > 0)
    {
        exePath[len] = '\0';
        return std::filesystem::path(exePath).parent_path();
    }
#endif

    return {};
}

std::string ExtractDetectionDetail(const DetectionEvent& event)
{
    std::size_t len = 0;
    while (len < kDetectionDetailSize && event.detail[len] != '\0')
        ++len;

    return std::string(event.detail, len);
}

std::string DescribeDetectionFlag(uint32_t rawFlag)
{
    using BaseLib::AcDetection::Flag;

    const auto exactFlag = BaseLib::AcDetection::FlagFromValue(rawFlag);
    if (exactFlag != Flag::UnknownFlag || rawFlag == static_cast<uint32_t>(Flag::UnknownFlag))
    {
        const auto enumName = magic_enum::enum_name(exactFlag);
        if (!enumName.empty())
            return std::string(enumName);

        return BaseLib::AcDetection::FlagToString(exactFlag);
    }

    const auto expandedFlags = BaseLib::AcDetection::ExpandRawFlags(rawFlag);
    std::string result;
    for (const auto flag : expandedFlags)
    {
        const auto enumName = magic_enum::enum_name(flag);
        const std::string_view flagName = !enumName.empty()
            ? enumName
            : std::string_view(BaseLib::AcDetection::FlagToString(flag));

        if (!result.empty())
            result += '|';

        result.append(flagName.data(), flagName.size());
    }

    if (!result.empty())
        return result;

    return fmt::format("0x{:08X}", rawFlag);
}

} // namespace

using BaseLib::DEBUG;
using BaseLib::NONE;
using BaseLib::dark_cyan;

// =============================================================================
// Server Ed25519 signing secret key (64 bytes, expanded format).
// Generate once with:
//   uint8_t seed[32]; // fill with random
//   crypto_eddsa_key_pair(secret_key, public_key, seed);
//   → compile secret_key here, public_key in the client.
// =============================================================================
    static const uint8_t kServerSignSecret[kSignSkSize] = {
        0x3a, 0x41, 0x72, 0xd8, 0x5a, 0x81, 0x97, 0x12,
        0x85, 0x5e, 0x35, 0x76, 0xbe, 0xf4, 0xeb, 0xc7,
        0x3d, 0x34, 0x01, 0x09, 0xff, 0xc7, 0xf2, 0xbb,
        0xf1, 0x84, 0x1a, 0x9e, 0x97, 0x80, 0xc6, 0xe0,
        0x0e, 0x41, 0x3e, 0x0e, 0x94, 0x5f, 0x7d, 0x36,
        0x0c, 0x49, 0x04, 0x68, 0xd9, 0xe3, 0x18, 0x57,
        0x80, 0xdb, 0x6c, 0x48, 0xcc, 0x50, 0x53, 0xd0,
        0x6b, 0x48, 0x0b, 0x29, 0x78, 0x19, 0x26, 0x97
    };

    static const uint8_t kServerSignPubkey[kKeySize] = {
        0x0e, 0x41, 0x3e, 0x0e, 0x94, 0x5f, 0x7d, 0x36,
        0x0c, 0x49, 0x04, 0x68, 0xd9, 0xe3, 0x18, 0x57,
        0x80, 0xdb, 0x6c, 0x48, 0xcc, 0x50, 0x53, 0xd0,
        0x6b, 0x48, 0x0b, 0x29, 0x78, 0x19, 0x26, 0x97
    };

// =============================================================================
// ServerSecureChannel
// =============================================================================

ServerSecureChannel::ServerSecureChannel() {
    memset(ephemeralSecret_, 0, kKeySize);
    memset(ephemeralPubkey_, 0, kKeySize);
}

ServerSecureChannel::~ServerSecureChannel() {
    crypto_wipe(ephemeralSecret_, kKeySize);
}

ServerSecureChannel::ServerSecureChannel(ServerSecureChannel&& other) noexcept {
    memcpy(ephemeralSecret_, other.ephemeralSecret_, kKeySize);
    memcpy(ephemeralPubkey_, other.ephemeralPubkey_, kKeySize);
    crypto_wipe(other.ephemeralSecret_, kKeySize);
    memset(other.ephemeralPubkey_, 0, kKeySize);
}

ServerSecureChannel& ServerSecureChannel::operator=(ServerSecureChannel&& other) noexcept {
    if (this != &other) {
        crypto_wipe(ephemeralSecret_, kKeySize);
        memcpy(ephemeralSecret_, other.ephemeralSecret_, kKeySize);
        memcpy(ephemeralPubkey_, other.ephemeralPubkey_, kKeySize);
        crypto_wipe(other.ephemeralSecret_, kKeySize);
        memset(other.ephemeralPubkey_, 0, kKeySize);
    }
    return *this;
}

// =============================================================================
// Phase 1 — Build ServerKeyPayload for ConnectAck
// =============================================================================
bool ServerSecureChannel::buildConnectPayload(const ConnectAckData& connectData,
                                               ServerKeyPayload& outPayload)
{
    // 1. Generate ephemeral X25519 keypair
    Utility::SecureRandomBlake2b::Generator rng;
    rng.NextBytes(ephemeralSecret_, kKeySize);
    crypto_x25519_public_key(ephemeralPubkey_, ephemeralSecret_);

    // 2. Fill payload
    memcpy(outPayload.sign_pubkey, kServerSignPubkey, kKeySize);
    memcpy(outPayload.x25519_pubkey, ephemeralPubkey_, kKeySize);
    memcpy(&outPayload.connect_data, &connectData, kConnectDataSize);

    // 3. Ed25519 sign (x25519_pk || connect_data)
    //    These are contiguous starting at outPayload.x25519_pubkey
    crypto_eddsa_sign(outPayload.signature, kServerSignSecret,
                      outPayload.x25519_pubkey, kSignedMsgSize);

    return true;
}

// =============================================================================
// Phase 2 — Decrypt auth packet from client
// =============================================================================
bool ServerSecureChannel::decryptAuthPacket(const uint8_t* data, uint32_t data_size,
                                            SecureAuthPayload& out,
                                            uint8_t* sessionKeyOut)
{
    if (data_size < sizeof(SecureAuthWireData))
    {
        DEBUGLOG(dark_cyan, "[SecureChannel] size mismatch: got={} expected={}", data_size, sizeof(SecureAuthWireData));
        return false;
    }

    auto wire = reinterpret_cast<const SecureAuthWireData*>(data);

    // Debug: dump client pubkey, server pubkey, AD header
    auto hexBytes = [](const uint8_t* d, size_t n) {
        std::string s;
        for (size_t i = 0; i < n; ++i)
            s += fmt::format("{:02x}", d[i]);
        return s;
    };
    DEBUGLOG(dark_cyan, "[SecureChannel] client_pk: {}", hexBytes(wire->client_pubkey, kKeySize));
    DEBUGLOG(dark_cyan, "[SecureChannel] server_pk: {}", hexBytes(ephemeralPubkey_, kKeySize));
    DEBUGLOG(dark_cyan, "[SecureChannel] mac:       {}", hexBytes(wire->mac, kMacSize));
    DEBUGLOG(dark_cyan, "[SecureChannel] cipherLen: {}", sizeof(SecureAuthPayload));

    // 1. DH(server_ephemeral_sk, client_ephemeral_pk) → raw shared secret
    uint8_t rawShared[kKeySize];
    crypto_x25519(rawShared, ephemeralSecret_, wire->client_pubkey);
    DEBUGLOG(dark_cyan, "[SecureChannel] rawDH:     {}", hexBytes(rawShared, kKeySize));

    // 2. Session key: BLAKE2b keyed hash (same label as client)
    uint8_t sessionKey[kKeySize];
    const char* label = "MegaGuard-AEAD-Key-v1";
    crypto_blake2b_keyed(sessionKey, kKeySize,
                         rawShared, kKeySize,
                         reinterpret_cast<const uint8_t*>(label), 21);
    crypto_wipe(rawShared, kKeySize);
    DEBUGLOG(dark_cyan, "[SecureChannel] sessKey:   {}", hexBytes(sessionKey, kKeySize));

    // 3. Session nonce: BLAKE2b(client_ephemeral_pk || server_ephemeral_pk), first 24 bytes
    uint8_t nonceInput[kKeySize * 2];
    memcpy(nonceInput, wire->client_pubkey, kKeySize);
    memcpy(nonceInput + kKeySize, ephemeralPubkey_, kKeySize);

    uint8_t fullHash[32];
    crypto_blake2b(fullHash, 32, nonceInput, sizeof(nonceInput));

    uint8_t nonce[kNonceSize];
    memcpy(nonce, fullHash, kNonceSize);
    crypto_wipe(nonceInput, sizeof(nonceInput));
    crypto_wipe(fullHash, sizeof(fullHash));
    DEBUGLOG(dark_cyan, "[SecureChannel] nonce:     {}", hexBytes(nonce, kNonceSize));

    // 4. AEAD decrypt with command header as additional data
    int result = crypto_aead_unlock(
        reinterpret_cast<uint8_t*>(&out),
        wire->mac, sessionKey, nonce,
        nullptr, 0,
        wire->ciphertext, sizeof(SecureAuthPayload));

    DEBUGLOG(dark_cyan, "[SecureChannel] aead_unlock result: {}", result);

    if (result == 0 && sessionKeyOut)
        memcpy(sessionKeyOut, sessionKey, kKeySize);

    crypto_wipe(sessionKey, kKeySize);
    crypto_wipe(nonce, kNonceSize);
    crypto_wipe(ephemeralSecret_, kKeySize); // no longer needed after decryption

    return result == 0;
}

// =============================================================================
// SecureChannelStore
// =============================================================================

void SecureChannelStore::store(uint16_t sessionId, ServerSecureChannel&& channel) {
    std::unique_lock lock(mutex_);
    channels_.insert_or_assign(sessionId, std::move(channel));
}

bool SecureChannelStore::retrieve(uint16_t sessionId, ServerSecureChannel& out) {
    std::unique_lock lock(mutex_);
    auto it = channels_.find(sessionId);
    if (it == channels_.end()) return false;
    out = std::move(it->second);
    channels_.erase(it);
    return true;
}

void SecureChannelStore::remove(uint16_t sessionId) {
    std::unique_lock lock(mutex_);
    channels_.erase(sessionId);
}

// =============================================================================
// HeartbeatManager
// =============================================================================

void HeartbeatManager::startSession(uint16_t sid, const uint8_t sessionKey[kKeySize],
                                     asio::io_context& io, Game::CMainServer* server)
{
    {
        std::unique_lock lock(mutex_);
        auto& state = sessions_[sid];
        memcpy(state.sessionKey, sessionKey, kKeySize);
        state.retryCount = 0;
        state.awaitingResponse = false;
        state.pendingChallenges.clear();
    }
    DEBUGLOG(dark_cyan, "sid=({}) heartbeat session started", sid);
    scheduleNextChallenge(sid, io, server, kInitialDelayMs);
}

void HeartbeatManager::stopSession(uint16_t sid)
{
    std::unique_lock lock(mutex_);
    auto it = sessions_.find(sid);
    if (it != sessions_.end()) {
        if (it->second.timer) it->second.timer->cancel();
        crypto_wipe(it->second.sessionKey, kKeySize);
        sessions_.erase(it);
    }
}

bool HeartbeatManager::onResponse(uint16_t sid, const uint8_t* data, uint32_t dataSize,
                                   asio::io_context& io, Game::CMainServer* server,
                                   std::vector<DetectionEvent>* outEvents)
{
    constexpr auto wireSize = sizeof(HeartbeatWireData<HeartbeatResponse>);
    if (dataSize < wireSize) {
        DEBUGLOG(dark_cyan, "sid=({}) heartbeat response too small: got={} need={}", sid, dataSize, wireSize);
        return false;
    }

    std::unique_lock lock(mutex_);
    auto it = sessions_.find(sid);
    if (it == sessions_.end()) return false;
    auto& state = it->second;

    if (!state.awaitingResponse && state.pendingChallenges.empty()) {
        DEBUGLOG(dark_cyan, "sid=({}) heartbeat response received but not awaiting one", sid);
        return false;
    }

    // Decrypt the response
    auto wire = reinterpret_cast<const HeartbeatWireData<HeartbeatResponse>*>(data);
    HeartbeatResponse response{};
    int result = crypto_aead_unlock(
        reinterpret_cast<uint8_t*>(&response),
        wire->mac, state.sessionKey, wire->nonce,
        nullptr, 0,
        wire->ciphertext, sizeof(HeartbeatResponse));

    if (result != 0) {
        DEBUGLOG(dark_cyan, "sid=({}) heartbeat response decryption failed", sid);
        return false;
    }

    // Find matching challenge in the pending set
    auto matchIt = std::find_if(state.pendingChallenges.begin(), state.pendingChallenges.end(),
        [&](const PendingChallenge& pc) { return pc.challenge_id == response.challenge_id; });

    if (matchIt == state.pendingChallenges.end()) {
        DEBUGLOG(dark_cyan, "sid=({}) heartbeat challenge ID 0x{:016X} not in pending set ({}  pending)",
                 sid, response.challenge_id, state.pendingChallenges.size());
        return false;
    }

    // Verify challenge answer against the matched challenge data
    if (!verifyChallengeAnswer(state.sessionKey, matchIt->challenge_data, response.challenge_answer)) {
        DEBUGLOG(dark_cyan, "sid=({}) heartbeat challenge answer verification failed", sid);
        return false;
    }

    // Remove the matched challenge (and any older ones before it to prevent replay)
    state.pendingChallenges.erase(state.pendingChallenges.begin(), matchIt + 1);

    // Any valid response resets retry state and cancels timeout
    if (state.awaitingResponse) {
        if (state.timer) state.timer->cancel();
        state.awaitingResponse = false;
        state.retryCount = 0;
    }

    const std::size_t eventCount =
        (response.event_count < kMaxQueuedEvents) ? response.event_count : kMaxQueuedEvents;

    // Schedule next challenge if no more pending challenges remain
    const bool shouldScheduleNext = state.pendingChallenges.empty();

    DEBUGLOG(dark_cyan, "sid=({}) heartbeat response accepted, challenge=0x{:016X} events={} pending={}",
             sid, response.challenge_id, static_cast<uint32_t>(eventCount),
             state.pendingChallenges.size());

    // Log detection events
    for (std::size_t i = 0; i < eventCount; ++i) {
        const auto detail = ExtractDetectionDetail(response.events[i]);
        const auto flagName = DescribeDetectionFlag(response.events[i].flag);
        if (detail.empty())
        {
            DEBUGLOG(dark_cyan, "sid=({}) detection event: flag={} ts={} extra=0x{:08X}",
                     sid, flagName, response.events[i].timestamp, response.events[i].extra);
        }
        else
        {
            DEBUGLOG(dark_cyan, "sid=({}) detection event: flag={} ts={} extra=0x{:08X} detail='{}'",
                     sid, flagName, response.events[i].timestamp, response.events[i].extra, detail);
        }
    }

    if (outEvents && eventCount > 0) {
        outEvents->reserve(outEvents->size() + eventCount);
        for (std::size_t i = 0; i < eventCount; ++i)
            outEvents->push_back(response.events[i]);
    }

    lock.unlock();

    if (shouldScheduleNext)
        scheduleNextChallenge(sid, io, server, kChallengeIntervalMs);

    return true;
}

void HeartbeatManager::sendChallenge(uint16_t sid, asio::io_context& io, Game::CMainServer* server)
{
    std::unique_lock lock(mutex_);
    auto it = sessions_.find(sid);
    if (it == sessions_.end()) return;
    auto& state = it->second;

    // Generate random challenge and add to pending set
    Utility::SecureRandomBlake2b::Generator rng;
    PendingChallenge pending{};
    rng.NextBytes(reinterpret_cast<uint8_t*>(&pending.challenge_id), sizeof(uint64_t));
    rng.NextBytes(pending.challenge_data, 32);

    // Evict oldest if at capacity
    while (state.pendingChallenges.size() >= kMaxPendingChallenges)
        state.pendingChallenges.pop_front();

    state.pendingChallenges.push_back(pending);
    state.awaitingResponse = true;

    // Build plaintext challenge
    HeartbeatChallenge challenge{};
    challenge.challenge_id = pending.challenge_id;
    memcpy(challenge.challenge_data, pending.challenge_data, 32);

    // Encrypt into wire format
    HeartbeatWireData<HeartbeatChallenge> wire{};
    rng.NextBytes(wire.nonce, kNonceSize);
    crypto_aead_lock(wire.ciphertext, wire.mac, state.sessionKey, wire.nonce,
                     nullptr, 0,
                     reinterpret_cast<const uint8_t*>(&challenge), sizeof(HeartbeatChallenge));

    auto challengeId = pending.challenge_id;
    lock.unlock();

    // Send on packet 81 (INFO_SECURITY_TOOLS) with extra=1
    if (auto session = server->GetSessionById(sid)) {
        session->SendMsg(81, 0, 1, 0,
                         reinterpret_cast<uint8_t*>(&wire), static_cast<uint16_t>(sizeof(wire)));
        DEBUGLOG(dark_cyan, "sid=({}) heartbeat challenge sent, id=0x{:016X}", sid, challengeId);
    }

    scheduleTimeout(sid, io, server);
}

void HeartbeatManager::scheduleTimeout(uint16_t sid, asio::io_context& io, Game::CMainServer* server)
{
    auto timer = std::make_shared<asio::steady_timer>(io, std::chrono::milliseconds(kChallengeTimeoutMs));

    {
        std::unique_lock lock(mutex_);
        auto it = sessions_.find(sid);
        if (it == sessions_.end()) return;
        it->second.timer = timer;
    }

    timer->async_wait([this, timer, sid, &io, server](const asio::error_code& ec) {
        if (ec) return; // timer was cancelled (response arrived in time)

        std::unique_lock lock(mutex_);
        auto it = sessions_.find(sid);
        if (it == sessions_.end()) return;
        auto& state = it->second;

        if (!state.awaitingResponse) return;

        state.retryCount++;
        DEBUGLOG(dark_cyan, "sid=({}) heartbeat timeout, retry {}/{} pending={}",
                 sid, state.retryCount, kMaxRetries, state.pendingChallenges.size());

        if (state.retryCount >= kMaxRetries) {
            lock.unlock();
            DEBUGLOG(dark_cyan, "sid=({}) heartbeat max retries reached, disconnecting", sid);
            server->DisconnectPlayer(sid, Game::Disconnect::Reason::DataError);
            stopSession(sid);
            return;
        }

        // Send a new challenge; old ones remain in pendingChallenges
        state.awaitingResponse = false;
        lock.unlock();
        sendChallenge(sid, io, server);
    });
}

void HeartbeatManager::scheduleNextChallenge(uint16_t sid, asio::io_context& io,
                                              Game::CMainServer* server, uint32_t delayMs)
{
    auto timer = std::make_shared<asio::steady_timer>(io, std::chrono::milliseconds(delayMs));

    {
        std::unique_lock lock(mutex_);
        auto it = sessions_.find(sid);
        if (it == sessions_.end()) return;
        it->second.timer = timer;
    }

    timer->async_wait([this, timer, sid, &io, server](const asio::error_code& ec) {
        if (ec) return; // cancelled (session stopped)
        sendChallenge(sid, io, server);
    });
}

bool HeartbeatManager::verifyChallengeAnswer(const uint8_t sessionKey[kKeySize],
                                              const uint8_t challengeData[32],
                                              const uint8_t answer[32])
{
    uint8_t expected[32];
    crypto_blake2b_keyed(expected, 32, sessionKey, kKeySize, challengeData, 32);

    // Constant-time comparison
    uint8_t diff = 0;
    for (int i = 0; i < 32; ++i)
        diff |= expected[i] ^ answer[i];

    crypto_wipe(expected, 32);
    return diff == 0;
}

// =============================================================================
// FileIntegrityConfig — load expected hashes from file_integrity.json
// =============================================================================
FileIntegrityConfig FileIntegrityConfig::Load(const char* path)
{
    namespace fs = std::filesystem;
    FileIntegrityConfig cfg;

    fs::path requestedPath = (path && *path) ? fs::path(path) : fs::path("file_integrity.json");
    std::vector<fs::path> candidates;
    candidates.reserve(3);
    candidates.emplace_back(requestedPath);

    if (requestedPath.is_relative())
    {
        std::error_code cwdEc;
        const auto cwd = fs::current_path(cwdEc);
        if (!cwdEc)
            candidates.emplace_back(cwd / requestedPath);

        const auto exeDir = GetExecutableDirectory();
        if (!exeDir.empty())
            candidates.emplace_back(exeDir / requestedPath);
    }

    std::ifstream file;
    fs::path loadedPath;
    for (const auto& candidate : candidates)
    {
        file.open(candidate, std::ios::in | std::ios::binary);
        if (file.is_open())
        {
            loadedPath = candidate;
            break;
        }

        file.clear();
    }

    if (!file.is_open())
    {
        std::error_code cwdEc;
        const auto cwd = fs::current_path(cwdEc);
        std::string tried;
        for (size_t i = 0; i < candidates.size(); ++i)
        {
            tried += candidates[i].string();
            if (i + 1 < candidates.size())
                tried += ", ";
        }

        DEBUGLOG(
            dark_cyan,
            "file_integrity.json not found (requested='{}', cwd='{}', tried=[{}]) — integrity checking disabled",
            requestedPath.string(),
            cwdEc ? std::string("<unknown>") : cwd.string(),
            tried);
        return cfg;
    }

    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();

    rapidjson::Document doc;
    doc.Parse(content.c_str());
    if (doc.HasParseError())
    {
        DEBUGLOG(fmt::color::red,
                 "file_integrity.json parse error at offset {} ({}) in '{}' — integrity checking disabled",
                 doc.GetErrorOffset(), rapidjson::GetParseError_En(doc.GetParseError()), loadedPath.string());
        return cfg;
    }

    if (!doc.IsObject())
    {
        DEBUGLOG(fmt::color::red,
                 "file_integrity.json root is not an object in '{}' — integrity checking disabled",
                 loadedPath.string());
        return cfg;
    }

    if (doc.HasMember("enabled") && doc["enabled"].IsBool())
        cfg.enabled = doc["enabled"].GetBool();

    if (!cfg.enabled)
    {
        DEBUGLOG(dark_cyan, "file_integrity.json loaded from '{}' but checking is disabled", loadedPath.string());
        return cfg;
    }

    if (!doc.HasMember("files") || !doc["files"].IsObject())
    {
        DEBUGLOG(fmt::color::red,
                 "file_integrity.json missing 'files' object in '{}' — integrity checking disabled",
                 loadedPath.string());
        cfg.enabled = false;
        return cfg;
    }

    const auto& files = doc["files"];
    auto hexToByte = [](char c) -> uint8_t {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'a' && c <= 'f') return 10 + c - 'a';
        if (c >= 'A' && c <= 'F') return 10 + c - 'A';
        return 0;
    };

    for (uint32_t i = 0; i < kIntegrityFileCount; ++i)
    {
        const char* name = IntegrityFileToString(static_cast<IntegrityFile>(i));
        if (!files.HasMember(name)) continue;
        const auto& val = files[name];
        if (!val.IsString()) continue;

        std::string hex = val.GetString();
        if (hex.empty() || hex.size() != 64) continue; // 32 bytes = 64 hex chars

        for (size_t b = 0; b < 32; ++b)
            cfg.expected_hashes[i][b] = (hexToByte(hex[b * 2]) << 4) | hexToByte(hex[b * 2 + 1]);

        cfg.has_hash[i] = true;
    }

    uint32_t count = 0;
    for (auto h : cfg.has_hash) if (h) count++;
    DEBUGLOG(dark_cyan, "file_integrity.json loaded from '{}': {} of {} files have expected hashes",
             loadedPath.string(), count, kIntegrityFileCount);
    return cfg;
}

} // namespace Game::Anticheat
