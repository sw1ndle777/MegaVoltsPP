#pragma once
#include <monocypher.h>
#include <shared_mutex>
#include <array>
#include <fstream>
#include <utility>
#include <boost_unordered.hpp>
#include <asio.hpp>
#include "NetEngine/Packets/PacketStruct.h"
#include "NetEngine/Packets/PacketData.h"
#include "NetEngine/Protocols/BaseProtocol.h"
#include <BaseLib/Platform.h>

namespace Game { class CMainServer; }

namespace Game::Anticheat {

inline constexpr uint32_t kKeySize    = 32;
inline constexpr uint32_t kNonceSize  = 24;
inline constexpr uint32_t kMacSize    = 16;
inline constexpr uint32_t kSigSize    = 64;
inline constexpr uint32_t kSignSkSize = 64; // Ed25519 expanded secret key
inline constexpr uint32_t kHwidSize   = 32; // BLAKE2b-256 of hardware fingerprint

// â”€â”€ ConnectAck plaintext â€” encrypted inside ServerKeyPayload â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
#pragma pack(push, 1)
struct ConnectAckData {
    int32_t      cryptoKey;
    NetEngine::Packets::Core::UniqueId uniqueId;
};
#pragma pack(pop)

inline constexpr uint32_t kConnectDataSize = sizeof(ConnectAckData); // 8

// â”€â”€ ServerKeyPayload â€” sent to client in ConnectAck â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
// Signed message = (x25519_pk || mac || encrypted_data), contiguous in memory.
inline constexpr uint32_t kSignedMsgSize = kKeySize + kConnectDataSize;

#pragma pack(push, 1)
struct ServerKeyPayload {
    uint8_t sign_pubkey[kKeySize];                // 32 â€” Ed25519 verify key
    uint8_t signature[kSigSize];                  // 64 â€” Ed25519(sign_sk, signed message)
    // â”€â”€ signed message starts here â”€â”€
    uint8_t x25519_pubkey[kKeySize];              // 32 â€” Server ephemeral X25519 DH key
    ConnectAckData connect_data;                   //  8 â€” plaintext {cryptoKey, uniqueId}
    // â”€â”€ signed message ends here â”€â”€
};
#pragma pack(pop)

// â”€â”€ File integrity payload â€” BLAKE2b-256 hashes of game files â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
inline constexpr uint32_t kIntegrityFileCount = 33; // 2 data + 31 release

enum class IntegrityFile : uint32_t
{
    MapDat = 0,
    CgdDip,
    Bdvid32,
    Cudart32,
    D3DX10d_39,
    D3dx10_37,
    D3dx10_39,
    D3dx10_40,
    D3dx10_41,
    D3dx10_42,
    D3dx9d_39,
    D3dx9_31,
    D3dx9_33,
    D3DX9_37,
    D3dx9_38,
    D3DX9_39,
    D3DX9_40,
    D3DX9_41,
    D3DX9_42,
    Gdiplus,
    Megaguard,
    MegaVoltsExe,
    VC90CRTManifest,
    Mss32,
    Msvcm90,
    Msvcp90,
    Msvcr90,
    NxCharacter,
    PhysXCooking,
    PhysXCore,
    PhysXDevice,
    PhysXLoader,
    SteamApi,
    Count // must equal kIntegrityFileCount
};
static_assert(std::to_underlying(IntegrityFile::Count) == kIntegrityFileCount);

inline constexpr const char* IntegrityFileToString(IntegrityFile f)
{
    constexpr const char* names[kIntegrityFileCount] = {
        "Data/map.dat",
        "Data/cgd.dip",
        "Release/bdvid32.dll",
        "Release/cudart32_30_9.dll",
        "Release/D3DX10d_39.dll",
        "Release/d3dx10_37.dll",
        "Release/d3dx10_39.dll",
        "Release/d3dx10_40.dll",
        "Release/d3dx10_41.dll",
        "Release/d3dx10_42.dll",
        "Release/D3dx9d_39.dll",
        "Release/d3dx9_31.dll",
        "Release/d3dx9_33.dll",
        "Release/D3DX9_37.dll",
        "Release/d3dx9_38.dll",
        "Release/D3DX9_39.dll",
        "Release/D3DX9_40.dll",
        "Release/D3DX9_41.dll",
        "Release/D3DX9_42.dll",
        "Release/gdiplus.dll",
        "Release/megaguard.dll",
        "Release/MegaVolts.exe",
        "Release/Microsoft.VC90.CRT.manifest",
        "Release/mss32.dll",
        "Release/msvcm90.dll",
        "Release/msvcp90.dll",
        "Release/msvcr90.dll",
        "Release/NxCharacter.dll",
        "Release/PhysXCooking.dll",
        "Release/PhysXCore.dll",
        "Release/PhysXDevice.dll",
        "Release/PhysXLoader.dll",
        "Release/steam_api.dll",
    };
    auto idx = std::to_underlying(f);
    return (idx < kIntegrityFileCount) ? names[idx] : "Unknown";
}

#pragma pack(push, 1)
struct FileIntegrityPayload {
    uint8_t hashes[kIntegrityFileCount][32];
};
#pragma pack(pop)

// â”€â”€ File integrity expected hashes loaded from file_integrity.json â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
struct FileIntegrityConfig {
    bool enabled = false;
    std::array<std::array<uint8_t, 32>, kIntegrityFileCount> expected_hashes{};
    std::array<bool, kIntegrityFileCount> has_hash{}; // true if hash was specified in JSON

    static FileIntegrityConfig Load(const char* path = "file_integrity.json");
};

inline FileIntegrityConfig g_fileIntegrityConfig;

// â”€â”€ Auth payload â€” encrypted contents of the authorize packet from client â”€â”€â”€â”€â”€
#pragma pack(push, 1)
struct SecureAuthPayload {
    uint32_t auth_key;
    uint32_t auth_key2;
    uint32_t server_id;
    uint8_t  netVersion1;
    uint8_t  netVersion2;
    uint8_t  netVersion3;
    uint8_t  netVersion4;
    uint8_t  hwid[kHwidSize];
    FileIntegrityPayload integrity;
};
#pragma pack(pop)

// â”€â”€ Wire layout of auth data as received by server (data-only, no cmd header) â”€
#pragma pack(push, 1)
struct SecureAuthWireData {
    uint8_t client_pubkey[kKeySize]; // 32 â€” client ephemeral X25519 public key
    uint8_t mac[kMacSize];           // 16 â€” AEAD auth tag
    uint8_t ciphertext[sizeof(SecureAuthPayload)]; // encrypted payload
};
#pragma pack(pop)

// =============================================================================
// ServerSecureChannel â€” per-session server-side key management
// =============================================================================
// Phase 1 (Connect):  buildConnectPayload()  â†’ ServerKeyPayload sent to client
// Phase 2 (Authorize): decryptAuthPacket()   â†’ SecureAuthPayload from client
// =============================================================================
class ServerSecureChannel {
public:
    ServerSecureChannel();
    ~ServerSecureChannel();

    ServerSecureChannel(ServerSecureChannel&& other) noexcept;
    ServerSecureChannel& operator=(ServerSecureChannel&& other) noexcept;
    ServerSecureChannel(const ServerSecureChannel&) = delete;
    ServerSecureChannel& operator=(const ServerSecureChannel&) = delete;

    // Phase 1: Build the ServerKeyPayload to send in ConnectAck.
    // Generates an ephemeral X25519 keypair, encrypts connectData with
    // DH(ephemeral_sk, client_static_pk), signs the result with Ed25519.
    bool buildConnectPayload(const ConnectAckData& connectData,
                             ServerKeyPayload& outPayload);

    // Phase 2: Decrypt the auth packet received from the client.
    // data / data_size : raw bytes from message->GetData() / GetDataSize()
    // If sessionKeyOut is not null, the derived session key is copied there on success.
    bool decryptAuthPacket(const uint8_t* data, uint32_t data_size,
                           SecureAuthPayload& out,
                           uint8_t* sessionKeyOut = nullptr);

private:
    uint8_t ephemeralSecret_[kKeySize]{};
    uint8_t ephemeralPubkey_[kKeySize]{};
};

// =============================================================================
// SecureChannelStore â€” thread-safe storage for pending channels
// =============================================================================
// Stores a ServerSecureChannel between Connect and Authorize, keyed by session id.
// retrieve() moves the channel out and erases the entry.
// =============================================================================
class SecureChannelStore {
public:
    void store(uint16_t sessionId, ServerSecureChannel&& channel);
    bool retrieve(uint16_t sessionId, ServerSecureChannel& out);
    void remove(uint16_t sessionId);

private:
    std::shared_mutex mutex_;
    boost::unordered_flat_map<uint16_t, ServerSecureChannel> channels_;
};

inline SecureChannelStore g_secureChannels;

// =============================================================================
// Heartbeat â€” periodic challenge/response anti-cheat system
// =============================================================================

inline constexpr uint32_t kDetectionBitsSize          = 16;
inline constexpr uint32_t kDetectionDetailSize        = 64;
inline constexpr uint32_t kHeartbeatPacketLimit       = 1432;
inline constexpr uint32_t kHeartbeatEncryptedOverhead = kNonceSize + kMacSize;
inline constexpr uint32_t kHeartbeatTransportOverhead =
    sizeof(NetEngine::Protocols::STcpPacketHeader) +
    sizeof(NetEngine::Protocols::SCommandHeader) +
    kHeartbeatEncryptedOverhead;
inline constexpr uint32_t kHeartbeatFixedResponseBytes =
    sizeof(uint64_t) + 32 + kDetectionBitsSize + sizeof(uint8_t) + 3;
inline constexpr int32_t kHeartbeatEventBudget =
    static_cast<int32_t>(kHeartbeatPacketLimit) -
    static_cast<int32_t>(kHeartbeatTransportOverhead) -
    static_cast<int32_t>(kHeartbeatFixedResponseBytes);

static_assert(kHeartbeatEventBudget > 0,
              "Heartbeat packet overhead exceeds the encrypted transport budget");

#pragma pack(push, 1)
struct DetectionEvent {
    uint32_t flag;
    uint32_t timestamp;
    uint32_t extra;
    char     detail[kDetectionDetailSize];
};

static_assert(sizeof(DetectionEvent) == sizeof(uint32_t) * 3 + kDetectionDetailSize,
              "DetectionEvent size mismatch");

inline constexpr uint32_t kMaxQueuedEvents = static_cast<uint32_t>(
    kHeartbeatEventBudget / static_cast<int32_t>(sizeof(DetectionEvent)));

static_assert(kMaxQueuedEvents > 0,
              "DetectionEvent is too large for the heartbeat packet budget");

struct HeartbeatChallenge {
    uint64_t challenge_id;
    uint8_t  challenge_data[32];
};

struct HeartbeatResponse {
    uint64_t       challenge_id;
    uint8_t        challenge_answer[32];
    uint8_t        detection_bits[kDetectionBitsSize];
    uint8_t        event_count;
    uint8_t        _pad[3];
    DetectionEvent events[kMaxQueuedEvents];
};
#pragma pack(pop)

static_assert(
    sizeof(NetEngine::Protocols::STcpPacketHeader) +
    sizeof(NetEngine::Protocols::SCommandHeader) +
    kHeartbeatEncryptedOverhead + sizeof(HeartbeatResponse) <= kHeartbeatPacketLimit,
    "Heartbeat response exceeds the encrypted transport budget");

// Wire data for encrypted heartbeat messages (nonce + mac + ciphertext, no header)
template<typename T>
struct HeartbeatWireData {
    uint8_t nonce[kNonceSize];
    uint8_t mac[kMacSize];
    uint8_t ciphertext[sizeof(T)];
};

// =============================================================================
// HeartbeatManager â€” server-side challenge/response scheduler
// =============================================================================
// After auth, call startSession() to begin periodic challenges.
// The Authorize handler routes heartbeat responses to onResponse().
// On disconnect, call stopSession() to clean up timers/keys.
//
// Retry logic: send challenge â†’ wait kChallengeTimeoutMs â†’ if no response,
// resend up to kMaxRetries times, then disconnect the player.
// =============================================================================
class HeartbeatManager {
public:
    static constexpr uint32_t kChallengeTimeoutMs  = 10000; // 10 s per attempt
    static constexpr uint32_t kChallengeIntervalMs = 30000; // 30 s between challenges
    static constexpr uint32_t kMaxRetries          = 3;
    static constexpr uint32_t kInitialDelayMs      = 5000;  // first challenge 5 s after auth

    void startSession(uint16_t sid, const uint8_t sessionKey[kKeySize],
                      asio::io_context& io, Game::CMainServer* server);
    void stopSession(uint16_t sid);

    // Returns true if the response was valid. Caller should disconnect on false.
    // If outEvents is non-null, detected events are copied there on success.
    bool onResponse(uint16_t sid, const uint8_t* data, uint32_t dataSize,
                    asio::io_context& io, Game::CMainServer* server,
                    std::vector<DetectionEvent>* outEvents = nullptr);

private:
    struct SessionState {
        uint8_t  sessionKey[kKeySize]{};
        uint64_t currentChallengeId{0};
        uint8_t  currentChallengeData[32]{};
        uint32_t retryCount{0};
        bool     awaitingResponse{false};
        bool     allowQueuedResponses{false};
        std::shared_ptr<asio::steady_timer> timer;
    };

    void sendChallenge(uint16_t sid, asio::io_context& io, Game::CMainServer* server);
    void scheduleTimeout(uint16_t sid, asio::io_context& io, Game::CMainServer* server);
    void scheduleNextChallenge(uint16_t sid, asio::io_context& io,
                               Game::CMainServer* server, uint32_t delayMs);
    static bool verifyChallengeAnswer(const uint8_t sessionKey[kKeySize],
                                      const uint8_t challengeData[32],
                                      const uint8_t answer[32]);

    std::shared_mutex mutex_;
    boost::unordered_flat_map<uint16_t, SessionState> sessions_;
};

inline HeartbeatManager g_heartbeatManager;

} // namespace Game::Anticheat
