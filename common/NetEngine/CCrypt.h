#pragma once
#include <array>
#include <bit>
#include <type_traits>
#include <stdexcept>
#include <expected>
#include <cstring>
#pragma warning(disable: 28020) // The expression '_Param_(1)<26' is not true at this call.

namespace NetEngine
{
    template <class T>
    [[nodiscard]] constexpr T rotl(T x, int s) noexcept
        requires (std::is_unsigned_v<T>)
    {
        return std::rotl(x, s);
    }

    template <class T>
    [[nodiscard]] constexpr T rotr(T x, int s) noexcept
        requires (std::is_unsigned_v<T>)
    {
        return std::rotr(x, s);
    }
    inline static constexpr uint32_t XOR_CONST = 0xA5A5A5A5;
    inline static constexpr uint32_t ADD_CONST = 0xCAFEBABE;
    inline static constexpr uint32_t NIBBLE_MASK_HI = 0xF0F0F0F0;
    inline static constexpr uint32_t NIBBLE_MASK_LO = 0x0F0F0F0F;
    inline static constexpr uint32_t ALT_MASK_EVEN = 0xAAAAAAAA;
    inline static constexpr uint32_t ALT_MASK_ODD = 0x55555555;
    inline constexpr bool using_new_encryption = true;
    [[nodiscard]] constexpr uint32_t swap_nibbles(uint32_t x) noexcept
    {
        return ((x & NIBBLE_MASK_HI) >> 4) | ((x & NIBBLE_MASK_LO) << 4);
    }
    [[nodiscard]] constexpr uint32_t swap_alt_bits(uint32_t x) noexcept
    {
        return ((x & ALT_MASK_EVEN) >> 1) | ((x & ALT_MASK_ODD) << 1);
    }
    [[nodiscard]] constexpr uint32_t mix_rshift16(uint32_t x) noexcept { return x ^ (x >> 16); }
    [[nodiscard]] constexpr uint32_t encrypt_tcp_header(uint32_t h) noexcept
    {
        if constexpr (!using_new_encryption) return h;
        h = ~h;
        h = rotl<uint32_t>(h, 13);
        h ^= XOR_CONST;
        h = swap_nibbles(h);
        h += ADD_CONST;
        h = rotl<uint32_t>(h, 7);
        h = mix_rshift16(h);
        h = swap_alt_bits(h);
        return h;
    }
    [[nodiscard]] constexpr uint32_t decrypt_tcp_header(uint32_t e) noexcept
    {
        if constexpr (!using_new_encryption) return e;
        e = swap_alt_bits(e);
        e = mix_rshift16(e);
        e = rotr<uint32_t>(e, 7);
        e -= ADD_CONST;
        e = swap_nibbles(e);
        e ^= XOR_CONST;
        e = rotr<uint32_t>(e, 13);
        e = ~e;
        return e;
    }
    class CCrypt {
    public:
        enum class ERROR_TYPE { UNK_CHANGE_TYPE, INVALID_FUNCTIONS };
        enum class CRYPT_TYPE : uint32_t { CRYPT_NONE, CRYPT_RC5, CRYPT_RC5_SERIAL, CRYPT_RC6, CRYPT_RC6_SERIAL };
        /* Member function pointer types for setup, encrypt and decrypt */
        using PVF_SETUP = void (CCrypt::*)();
        using PBF_ENCRYPT = bool (CCrypt::*)(const uint8_t*, uint8_t*, int32_t);
        using PBF_DECRYPT = bool (CCrypt::*)(const uint8_t*, uint8_t*, int32_t);
        /* Small struct used as a manual dispatch table entry */
        struct VTab { PVF_SETUP setup; PBF_ENCRYPT enc; PBF_DECRYPT dec; };
        /* Constructor: immediately switches to the given cipher type
        and runs its setup with the provided serial key */
        CCrypt(CRYPT_TYPE eType_, int32_t Key_)
        {
            auto r = ChangeType(eType_, Key_);
            if (!r) [[unlikely]] std::unreachable();
        }
        /* Destructor (virtual in case polymorphic use is added later) */
        virtual ~CCrypt() = default;
        /* Sets a new serial key and runs the current setup function
        to rebuild the key schedule */
        void Setup(int32_t Key_) noexcept
        {
            m_iSerialKey = Key_;
            [[assume(m_pvfSetup != nullptr)]];
            (this->*m_pvfSetup)();
        }
        /* Switches cipher type (RC5/RC6), updates function pointers,
        and reinitializes with the given serial key */
        std::expected<void, ERROR_TYPE> ChangeType(CRYPT_TYPE eType_, int32_t Key_)
        {
            const auto idx = to_u(eType_);
            if (idx >= std::size(m_kVtabs)) return std::unexpected(ERROR_TYPE::UNK_CHANGE_TYPE);
            [[assume(idx < std::size(m_kVtabs))]];

            const auto& vt = m_kVtabs[idx];
            m_pvfSetup = vt.setup;
            m_pbfEncrypt = vt.enc;
            m_pbfDecrypt = vt.dec;
            if (!m_pvfSetup || !m_pbfEncrypt || !m_pbfDecrypt)
                [[unlikely]] return std::unexpected(ERROR_TYPE::INVALID_FUNCTIONS);

            [[assume(m_pvfSetup != nullptr)]];
            [[assume(m_pbfEncrypt != nullptr)]];
            [[assume(m_pbfDecrypt != nullptr)]];

            Setup(Key_);
            return {};
        }
        /* Encrypt a buffer of raw bytes using the active cipher */
        template <typename TIn, typename TOut>
        bool Encrypt(const TIn* pvIn_, TOut* pvOut_, int32_t iLen_) noexcept
        {
            [[assume(m_pbfEncrypt != nullptr)]];
            return (this->*m_pbfEncrypt)(
                reinterpret_cast<const uint8_t*>(pvIn_),
                reinterpret_cast<uint8_t*>(pvOut_),
                iLen_);
        }
        /* Decrypt a buffer of raw bytes using the active cipher */
        template <typename TIn, typename TOut>
        bool Decrypt(const TIn* pvIn_, TOut* pvOut_, int32_t iLen_) noexcept
        {
            [[assume(m_pbfDecrypt != nullptr)]];
            return (this->*m_pbfDecrypt)(
                reinterpret_cast<const uint8_t*>(pvIn_),
                reinterpret_cast<uint8_t*>(pvOut_),
                iLen_);
        }
    private:
        inline static constexpr size_t _RC5_KEY_SIZE = 16;
        inline static constexpr size_t _RC6_KEY_SIZE = 32;
        /* Return the compile-time RC5 base key (16 bytes) */
        static consteval std::array<uint8_t, _RC5_KEY_SIZE> rc5_key() noexcept
        {
            if constexpr (using_new_encryption)
                return { 0xc0, 0x58, 0x1e, 0x07, 0xd9, 0x39, 0x43, 0x12, 0x31, 0xd0, 0xce, 0x21, 0xdd, 0xaf, 0x90, 0xad };
            else
                return { 0x3d, 0x63, 0xc5, 0xa3, 0x6d, 0x9a, 0xdb, 0xa5, 0xd1, 0xb2, 0x7a, 0x17, 0xb6, 0x56, 0x2c, 0xba }; // originally by nq games
        }
        /* Return the compile-time RC6 base key (32 bytes) */
        static consteval std::array<uint8_t, _RC6_KEY_SIZE> rc6_key() noexcept
        {
            if constexpr (using_new_encryption)
                return { 0x55, 0x35, 0x34, 0xb1, 0x9f, 0x85, 0x46, 0x23, 0x46, 0x08, 0xb4, 0x75, 0xc3, 0xd4, 0x9e, 0x9c, 0x66, 0x0d, 0xab, 0x76, 0x74, 0xe7, 0x74, 0xf1, 0x35, 0x4b, 0x53, 0xc7, 0x4d, 0xe6, 0x69, 0xfe };
            else
                return { 0x76, 0xb7, 0x4b, 0x98, 0x4c, 0x5b, 0xd5, 0xe3, 0xc1, 0x92, 0x33, 0x6a, 0x7b, 0xe6, 0xcc, 0xeb, 0x17, 0x9a, 0x77, 0xbc, 0x31, 0x5d, 0xe7, 0x39, 0xa9, 0x32, 0x54, 0x88, 0x66, 0xd3, 0xce, 0x43 }; // originally by nq games
        }
        /* RC5 key expansion (S/L mixing)
           - Builds 32-bit L from 16-byte K, salted with 4 bytes from m_iSerialKey
           - Initializes S (m_usKey_RC5/m_uiKey_RC5) using RC5 magic constants P,Q
           - Performs MIX iterations to diffuse L into S (and vice-versa) */
        static void RC5_KeySetup(const uint8_t* K) noexcept
        {
            constexpr uint32_t U = 4;
            constexpr uint32_t ROT = 3;
            constexpr auto MIX = 3 * _RC5_KEYTABLES;
            std::array<uint32_t, U> L{};
            auto abyKey = std::bit_cast<std::array<uint8_t, 4>>(m_iSerialKey);
            for (auto j = 15, i = 0; j >= 0; --j, i = (i + 1) % 4)
                L[j / U] = (L[j / U] << 8) + static_cast<uint32_t>(K[j] + static_cast<int8_t>(abyKey[i]));

            m_usKey_RC5[0] = m_uiKey_RC5[0] = _RSA_P16;
            for (uint32_t j = 1; j < _RC5_KEYTABLES; ++j)
                m_usKey_RC5[j] = m_uiKey_RC5[j] = m_uiKey_RC5[j - 1] + _RSA_Q16;

            uint32_t A = 0, B = 0, j = 0, k = 0;
            for (uint32_t l = 0; l < MIX; ++l)
            {
                A = m_uiKey_RC5[j] = rotl<uint32_t>(m_uiKey_RC5[j] + A + B, ROT);
                m_usKey_RC5[j] = A;
                B = L[k] = rotl<uint32_t>(L[k] + A + B, A + B);
                j = (j + 1) % 2 * 13;
                k = (k + 1) % U;
            }
        }
        /* Build RC5 key schedule using the compile-time key */
        void RC5_Setup() noexcept
        {
            constexpr auto abyKey = rc5_key();
            [[assume(abyKey.size() == _RC5_KEY_SIZE)]]
            RC5_KeySetup(abyKey.data());
        }
        /* RC5 decryption for 4-byte blocks (two 16-bit words)
           - Copies input to output if buffers differ
           - Walks the key schedule backwards per round
           - Applies final post-whitening subtracts in the correct order (B then A) */
        bool RC5_Decrypt32(const uint8_t* piIn_, uint8_t* piOut_, int32_t iLen_) noexcept
        {
            if (piIn_ != piOut_) std::memcpy(piOut_, piIn_, iLen_);
            int32_t iRestLen = iLen_, i = 0;
            auto key = _RC5_KEYTABLES;
            while (iRestLen >= 4)
            {
                uint16_t A = static_cast<uint16_t>(*reinterpret_cast<const uint16_t*>(piIn_ + i * 2) ^ m_iSerialKey);
                uint16_t B = static_cast<uint16_t>(*reinterpret_cast<const uint16_t*>(piIn_ + (i + 1) * 2) ^ m_iSerialKey);
                for (auto k = 0; k < _RC5_ROUND; k++)
                {
                    B = rotr<uint16_t>(B - m_usKey_RC5[--key], A & 0xF) ^ A;
                    A = rotr<uint16_t>(A - m_usKey_RC5[--key], B & 0xF) ^ B;
                }
                *reinterpret_cast<uint16_t*>(piOut_ + (i + 1) * 2) = B - m_usKey_RC5[--key];
                *reinterpret_cast<uint16_t*>(piOut_ + i * 2) = A - m_usKey_RC5[--key];
                i += 2;
                iRestLen -= 4;
            }
            return true;
        }
        /* RC5 decryption for 8-byte blocks (two 32-bit words)
           - Uses 32-bit schedule m_uiKey_RC5 (pre/post-whitening differ from 16-bit path)
           - Falls back to RC5_Decrypt32 for leftover 4..7 bytes */
        bool RC5_Decrypt64(const uint8_t* piIn_, uint8_t* piOut_, int32_t iLen_) noexcept
        {
            int32_t iRestLen = iLen_, i = 0;
            while (iRestLen >= 8)
            {
                auto A = *reinterpret_cast<const uint32_t*>(piIn_ + i * 4) ^ m_iSerialKey;
                auto B = *reinterpret_cast<const uint32_t*>(piIn_ + (i + 1) * 4) ^ m_iSerialKey;
                for (auto j = static_cast<int>(_RC5_ROUND); j > 0; j--)
                {
                    B = rotr<uint32_t>(B - m_uiKey_RC5[2 * j + 1], A) ^ A;
                    A = rotr<uint32_t>(A - m_uiKey_RC5[2 * j], B) ^ B;
                }
                *reinterpret_cast<uint32_t*>(piOut_ + i * 4) = A - m_uiKey_RC5[0];
                *reinterpret_cast<uint32_t*>(piOut_ + (i + 1) * 4) = B - m_uiKey_RC5[1];
                i += 2;
                iRestLen -= 8;
            }
            return iRestLen == 0 || RC5_Decrypt32(piIn_ + (iLen_ - iRestLen), piOut_ + (iLen_ - iRestLen), iRestLen);
        }
        /* RC5 decryption dispatcher: chooses 4-byte or 8-byte path based on size */
        bool RC5_Decrypt(const uint8_t* piIn_, uint8_t* piOut_, int32_t iLen_) noexcept
        {
            return iLen_ < 8 ? RC5_Decrypt32(piIn_, piOut_, iLen_) : RC5_Decrypt64(piIn_, piOut_, iLen_);
        }
        /* RC5 encryption for 4-byte blocks (two 16-bit words)
           - Copies input to output if buffers differ (preserves tail bytes)
           - Pre-whitening: add first two 16-bit S-words
           - Round function: A = rotl(A ^ B, B&0xF) + S, then B symmetric
           - Final mask: XOR both words with serial key (low 16 bits) */
        bool RC5_Encrypt32(const uint8_t* piIn_, uint8_t* piOut_, int32_t iLen_) noexcept
        {
            if (piIn_ != piOut_) std::memcpy(piOut_, piIn_, iLen_);

            uint16_t key = 0;
            int32_t iRestLen = iLen_, i = 0;
            while (iRestLen >= 4)
            {
                auto A = *reinterpret_cast<const uint16_t*>(piIn_ + i * 2) + m_usKey_RC5[key++];
                auto B = *reinterpret_cast<const uint16_t*>(piIn_ + (i + 1) * 2) + m_usKey_RC5[key++];
                for (auto round = 0; round < _RC5_ROUND; ++round)
                {
                    A = rotl<uint16_t>(A ^ B, B & 0x0F) + m_uiKey_RC5[key++];
                    B = rotl<uint16_t>(B ^ A, A & 0x0F) + m_uiKey_RC5[key++];
                }
                *reinterpret_cast<uint16_t*>(piOut_ + i * 2) = A ^ static_cast<uint16_t>(m_iSerialKey);
                *reinterpret_cast<uint16_t*>(piOut_ + (i + 1) * 2) = B ^ static_cast<uint16_t>(m_iSerialKey);
                i += 2;
                iRestLen -= 4;
            }
            return true;
        }
        /* RC5 encryption for 8-byte blocks (two 32-bit words)
           - Pre-whitening: add S[0], S[1]
           - Round function: A = rotl(A ^ B, B) + S[2j], B = rotl(B ^ A, A) + S[2j+1]
           - Final XOR mask with full 32-bit serial key
           - Falls back to RC5_Encrypt32 for leftover 4..7 bytes */
        bool RC5_Encrypt64(const uint8_t* piIn_, uint8_t* piOut_, int32_t iLen_) noexcept
        {
            int32_t iRestLen = iLen_, i = 0;
            while (iRestLen >= 8)
            {
                auto A = *reinterpret_cast<const uint32_t*>(piIn_ + i * 4) + m_uiKey_RC5[0];
                auto B = *reinterpret_cast<const uint32_t*>(piIn_ + (i + 1) * 4) + m_uiKey_RC5[1];
                for (auto j = 1; j <= static_cast<int>(_RC5_ROUND); j++)
                {
                    A = rotl<uint32_t>(A ^ B, B) + m_uiKey_RC5[2 * j];
                    B = rotl<uint32_t>(B ^ A, A) + m_uiKey_RC5[2 * j + 1];
                }
                *reinterpret_cast<uint32_t*>(piOut_ + i * 4) = A ^ static_cast<uint32_t>(m_iSerialKey);
                *reinterpret_cast<uint32_t*>(piOut_ + (i + 1) * 4) = B ^ static_cast<uint32_t>(m_iSerialKey);
                i += 2;
                iRestLen -= 8;
            }
            return iRestLen == 0 || RC5_Encrypt32(piIn_ + (iLen_ - iRestLen), piOut_ + (iLen_ - iRestLen), iRestLen);
        }
        /* RC5 encryption dispatcher: chooses 4-byte or 8-byte path based on size */
        bool RC5_Encrypt(const uint8_t* piIn_, uint8_t* piOut_, int32_t iLen_) noexcept
        {
            return iLen_ < 8 ? RC5_Encrypt32(piIn_, piOut_, iLen_) : RC5_Encrypt64(piIn_, piOut_, iLen_);
        }
        /* RC6 key expansion
           - Packs variable-length input key into L (c words), salted with serial bytes
           - Initializes S with 32-bit P/Q
           - Performs MIX iterations over S and L */
        void RC6_KeySetup(const uint8_t* ucInKey_, uint16_t usKeyLen_) noexcept
        {
            constexpr uint32_t U = 4;
            constexpr uint32_t ROT = 3;
            const uint32_t c = (usKeyLen_ + U - 1) / U;
            auto MIX = 3 * std::max(c, _RC6_KEYTABLES);
            std::array<uint32_t, 8> L{};
            auto abyKey = std::bit_cast<std::array<std::uint8_t, 4>>(m_iSerialKey);
            for (auto j = static_cast<int32_t>(usKeyLen_) - 1, i = 0; j >= 0; j--, i = (i + 1) % 4)
                L[j / U] = (L[j / U] << 8) + static_cast<uint32_t>(ucInKey_[j] + static_cast<int8_t>(abyKey[i]));

            m_uiKey_RC6[0] = _RSA_P32;
            for (auto j = 1; j < _RC6_KEYTABLES; j++)
                m_uiKey_RC6[j] = m_uiKey_RC6[j - 1] + _RSA_Q32;

            uint32_t A = 0, B = 0, j = 0, k = 0;
            for (auto s = 1u; s <= MIX; s++)
            {
                A = m_uiKey_RC6[j] = rotl<uint32_t>(m_uiKey_RC6[j] + A + B, ROT);
                B = L[k] = rotl<uint32_t>(L[k] + A + B, A + B);
                j = (j + 1) % _RC6_KEYTABLES;
                k = (k + 1) % c;
            }
        }
        /* Build RC6 key schedule using the compile-time key */
        void RC6_Setup() noexcept
        {
            constexpr auto abyKey = rc6_key();
            [[assume(abyKey.size() == _RC6_KEY_SIZE)]];
            RC6_KeySetup(abyKey.data(), 32);
        }
        /* RC6 decryption of 16-byte blocks (four 32-bit words)
           - Removes post-whitening for A/C, then iterates rounds backward
           - Each round rotates the state (D<-C<-B<-A), computes t/u from B/D,
             then reverses C and A transforms with rotr and S-keys
           - Writes back with final subtracts on B,D. Falls back to RC5 for short tails */
        bool RC6_Decrypt(const uint8_t* piIn_, uint8_t* piOut_, int32_t iLen_) noexcept
        {
            int32_t iRestLen = iLen_, i = 0;
            while (iRestLen >= 16)
            {
                auto A = (*reinterpret_cast<const uint32_t*>(piIn_ + i * 4) ^ m_iSerialKey) - m_uiKey_RC6[_RC6_R22];
                auto B = *reinterpret_cast<const uint32_t*>(piIn_ + (i + 1) * 4) ^ m_iSerialKey;
                auto C = (*reinterpret_cast<const uint32_t*>(piIn_ + (i + 2) * 4) ^ m_iSerialKey) - m_uiKey_RC6[_RC6_R23];
                auto D = *reinterpret_cast<const uint32_t*>(piIn_ + (i + 3) * 4) ^ m_iSerialKey;
                for (auto k = _RC6_ROUND; k > 0; k--)
                {
                    auto tmp = D;
                    D = C;
                    C = B;
                    B = A;
                    A = tmp;
                    auto u = rotl<uint32_t>(D * (2 * D + 1), _RC6_LGW);
                    auto t = rotl<uint32_t>(B * (2 * B + 1), _RC6_LGW);
                    C = rotr<uint32_t>(C - m_uiKey_RC6[2 * k + 1], t) ^ u;
                    A = rotr<uint32_t>(A - m_uiKey_RC6[2 * k], u) ^ t;
                }
                *reinterpret_cast<uint32_t*>(piOut_ + i * 4) = A;
                *reinterpret_cast<uint32_t*>(piOut_ + (i + 1) * 4) = B - m_uiKey_RC6[0];
                *reinterpret_cast<uint32_t*>(piOut_ + (i + 2) * 4) = C;
                *reinterpret_cast<uint32_t*>(piOut_ + (i + 3) * 4) = D - m_uiKey_RC6[1];
                i += 4;
                iRestLen -= 16;
            }
            return iRestLen == 0 || (RC5_Setup(), RC5_Decrypt(piIn_ + (iLen_ - iRestLen), piOut_ + (iLen_ - iRestLen), iRestLen));
        }
        /* RC6 encryption of 16-byte blocks (four 32-bit words)
           - Pre-whitening on B and D (S[0], S[1])
           - Each round computes t/u from B/D, mixes into A and C with rotl and S-keys,
             then rotates (A->B->C->D->A)
           - Final post-whitening/XOR with serial for A/C; raw XOR for B/D
           - Falls back to RC5 for short tails */
        bool RC6_Encrypt(const uint8_t* piIn_, uint8_t* piOut_, int32_t iLen_) noexcept
        {
            int32_t iRestLen = iLen_, i = 0;
            while (iRestLen >= 16)
            {
                auto A = *reinterpret_cast<const uint32_t*>(piIn_ + i * 4);
                auto B = *reinterpret_cast<const uint32_t*>(piIn_ + (i + 1) * 4) + m_uiKey_RC6[0];
                auto C = *reinterpret_cast<const uint32_t*>(piIn_ + (i + 2) * 4);
                auto D = *reinterpret_cast<const uint32_t*>(piIn_ + (i + 3) * 4) + m_uiKey_RC6[1];
                for (auto k = 1; k <= _RC6_ROUND; k++)
                {
                    auto t = rotl<uint32_t>(B * (2 * B + 1), _RC6_LGW);
                    auto u = rotl<uint32_t>(D * (2 * D + 1), _RC6_LGW);
                    A = rotl<uint32_t>(A ^ t, u) + m_uiKey_RC6[2 * k];
                    C = rotl<uint32_t>(C ^ u, t) + m_uiKey_RC6[2 * k + 1];
                    auto tmp = A;
                    A = B;
                    B = C;
                    C = D;
                    D = tmp;
                }
                *reinterpret_cast<uint32_t*>(piOut_ + i * 4) = (A + m_uiKey_RC6[_RC6_R22]) ^ static_cast<uint32_t>(m_iSerialKey);
                *reinterpret_cast<uint32_t*>(piOut_ + (i + 1) * 4) = B ^ static_cast<uint32_t>(m_iSerialKey);
                *reinterpret_cast<uint32_t*>(piOut_ + (i + 2) * 4) = (C + m_uiKey_RC6[_RC6_R23]) ^ static_cast<uint32_t>(m_iSerialKey);
                *reinterpret_cast<uint32_t*>(piOut_ + (i + 3) * 4) = D ^ static_cast<uint32_t>(m_iSerialKey);
                i += 4;
                iRestLen -= 16;
            }
            return iRestLen == 0 || (RC5_Setup(), RC5_Encrypt(piIn_ + (iLen_ - iRestLen), piOut_ + (iLen_ - iRestLen), iRestLen));
        }
        PVF_SETUP m_pvfSetup = nullptr; /* current setup function */
        PBF_ENCRYPT m_pbfEncrypt = nullptr; /* current encrypt function */
        PBF_DECRYPT m_pbfDecrypt = nullptr; /* current decrypt function */
        /* Key schedule storage for RC5 and RC6 (shared across all instances) */
        constinit inline static std::array<uint16_t, 26> m_usKey_RC5{};
        constinit inline static std::array<uint32_t, 26> m_uiKey_RC5{};
        constinit inline static std::array<uint32_t, 84> m_uiKey_RC6{};
        /* Serial key: mixed into key expansion and used for XOR masks */
        inline static int32_t m_iSerialKey = 0;
        /* Cipher parameters */
        inline static constexpr uint32_t _RC5_KEYTABLES = 26;
        inline static constexpr uint32_t _RC5_ROUND = 12;
        inline static constexpr uint32_t _RC6_KEYTABLES = 84;
        inline static constexpr uint32_t _RC6_LGW = 5;
        inline static constexpr uint32_t _RC6_R22 = 82;
        inline static constexpr uint32_t _RC6_R23 = 83;
        inline static constexpr uint32_t _RC6_ROUND = 40;
        inline static constexpr uint16_t _RSA_P16 = 20835;
        inline static constexpr uint32_t _RSA_P32 = 3084996963;
        inline static constexpr uint16_t _RSA_Q16 = 31161;
        inline static constexpr uint32_t _RSA_Q32 = 2654435769;
        /* Static dispatch table: maps CRYPT_TYPE to setup/encrypt/decrypt functions */
        static constexpr VTab m_kVtabs[] =
        {
            /* CRYPT_NONE       */ { &CCrypt::RC5_Setup,  &CCrypt::RC5_Encrypt,  &CCrypt::RC5_Decrypt },
            /* CRYPT_RC5        */ { &CCrypt::RC5_Setup,  &CCrypt::RC5_Encrypt,  &CCrypt::RC5_Decrypt },
            /* CRYPT_RC5_SERIAL */ { &CCrypt::RC5_Setup,  &CCrypt::RC5_Encrypt,  &CCrypt::RC5_Decrypt },
            /* CRYPT_RC6        */ { &CCrypt::RC6_Setup,  &CCrypt::RC6_Encrypt,  &CCrypt::RC6_Decrypt },
            /* CRYPT_RC6_SERIAL */ { &CCrypt::RC6_Setup,  &CCrypt::RC6_Encrypt,  &CCrypt::RC6_Decrypt },
        };
    };
}

//#endif
