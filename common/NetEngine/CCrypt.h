#pragma once

#include <stdint.h>
#include <string.h>

#define ROTL16(x,y) ((uint16_t)((((uint16_t)(x))<<((y)&15)) | (((uint16_t)(x))>>(16-((y)&15)))))
#define ROTR16(x,y) ((uint16_t)((((uint16_t)(x))>>((y)&15)) | (((uint16_t)(x))<<(16-((y)&15)))))
#define ROTL32(x,y) ((uint32_t)((((uint32_t)(x))<<((y)&31)) | (((uint32_t)(x))>>(32-((y)&31)))))
#define ROTR32(x,y) ((uint32_t)((((uint32_t)(x))>>((y)&31)) | (((uint32_t)(x))<<(32-((y)&31)))))

namespace NetEngine
{
    namespace Cryptography
    {
        class CCrypt
        {
        public:
            bool using_new_encryption = true;

            std::uint32_t encrypt_tcp_header(std::uint32_t header)
            {
                if (!using_new_encryption) return header;
                header = ~header;  // Bitwise NOT
                header = (header << 13) | (header >> (32 - 13));  // Rotate left by 13
                header ^= 0xA5A5A5A5;  // XOR with constant
                header = ((header & 0xF0F0F0F0) >> 4) | ((header & 0x0F0F0F0F) << 4);  // Swap nibbles
                header += 0xCAFEBABE;  // Add constant for diffusion
                header = (header << 7) | (header >> (32 - 7));  // Rotate left by 7
                header ^= (header >> 16);  // XOR mix upper and lower bits
                header = ((header & 0xAAAAAAAA) >> 1) | ((header & 0x55555555) << 1);  // Swap alternating bits
                return header;
            }


            std::uint32_t decrypt_tcp_header(std::uint32_t encrypted_header)
            {
                if (!using_new_encryption) return encrypted_header;
                encrypted_header = ((encrypted_header & 0xAAAAAAAA) >> 1) | ((encrypted_header & 0x55555555) << 1);  // Reverse alternating bit swap
                encrypted_header ^= (encrypted_header >> 16);  // Reverse XOR mix
                encrypted_header = (encrypted_header >> 7) | (encrypted_header << (32 - 7));  // Rotate right by 7
                encrypted_header -= 0xCAFEBABE;  // Subtract constant
                encrypted_header = ((encrypted_header & 0xF0F0F0F0) >> 4) | ((encrypted_header & 0x0F0F0F0F) << 4);  // Reverse nibble swap
                encrypted_header ^= 0xA5A5A5A5;  // Reverse XOR with constant
                encrypted_header = (encrypted_header >> 13) | (encrypted_header << (32 - 13));  // Rotate right by 13
                encrypted_header = ~encrypted_header;  // Bitwise NOT to restore original
                return encrypted_header;
            }

            uint32_t RC5S[26];
            uint32_t RC6S[84];
            int32_t UserKey;

            void RC5KeySetup()
            {
                unsigned char K[16];
                if (!using_new_encryption)
                {
                    unsigned char Kt[16] = { 0x3d, 0x63, 0xc5, 0xa3, 0x6d, 0x9a, 0xdb, 0xa5, 0xd1, 0xb2, 0x7a, 0x17, 0xb6, 0x56, 0x2c, 0xba };//original by nq games
                    std::copy(std::begin(Kt), std::end(Kt), std::begin(K));
                }
                else
                {
                    unsigned char Kt[16] = { 0xc0, 0x58, 0x1e, 0x07, 0xd9, 0x39, 0x43, 0x12, 0x31, 0xd0, 0xce, 0x21, 0xdd, 0xaf, 0x90, 0xad };
                    std::copy(std::begin(Kt), std::end(Kt), std::begin(K));
                }
                uint32_t A, B, L[4];
                int i, j, k, l = 0;
                char UserKeyBytes[4];
                UserKeyBytes[0] = UserKey & 0xFF;
                UserKeyBytes[1] = UserKey >> 8 & 0xFF;
                UserKeyBytes[2] = UserKey >> 16 & 0xFF;
                UserKeyBytes[3] = UserKey >> 24 & 0xFF;
                for (i = 15, L[3] = 0; i >= 0; i--)
                {
                    L[i / 4] = (L[i / 4] << 8) + K[i] + UserKeyBytes[l];
                    if (++l > 3) l = 0;
                }
                for (RC5S[0] = 0x5163, i = 1; i < 26; i++)
                    RC5S[i] = RC5S[i - 1] + 0x79b9;
                for (A = B = i = j = k = 0; k < 26 * 3; k++, i = (i + 26 / 2) % 26, j = (j + 1) % 4)
                {
                    A = RC5S[i] = ROTL32(RC5S[i] + (A + B), 3);
                    B = L[j] = ROTL32(L[j] + (A + B), (A + B));
                }
            }

            void RC6KeySetup()
            {
                unsigned char K[32];
                if (!using_new_encryption)
                {
                    unsigned char Kt[32] = { 0x76, 0xb7, 0x4b, 0x98, 0x4c, 0x5b, 0xd5, 0xe3, 0xc1, 0x92, 0x33, 0x6a, 0x7b, 0xe6, 0xcc, 0xeb, 0x17, 0x9a, 0x77, 0xbc, 0x31, 0x5d, 0xe7, 0x39, 0xa9, 0x32, 0x54, 0x88, 0x66, 0xd3, 0xce, 0x43 };//original by nq games
                    std::copy(std::begin(Kt), std::end(Kt), std::begin(K));
                }
                else
                {
                    unsigned char Kt[32] = { 0x55, 0x35, 0x34, 0xb1, 0x9f, 0x85, 0x46, 0x23, 0x46, 0x08, 0xb4, 0x75, 0xc3, 0xd4, 0x9e, 0x9c, 0x66, 0x0d, 0xab, 0x76, 0x74, 0xe7, 0x74, 0xf1, 0x35, 0x4b, 0x53, 0xc7, 0x4d, 0xe6, 0x69, 0xfe };
                    std::copy(std::begin(Kt), std::end(Kt), std::begin(K));
                }
                uint32_t A, B, L[8];
                int i, j, k, l = 0;
                char UserKeyBytes[4];
                UserKeyBytes[0] = UserKey & 0xFF;
                UserKeyBytes[1] = UserKey >> 8 & 0xFF;
                UserKeyBytes[2] = UserKey >> 16 & 0xFF;
                UserKeyBytes[3] = UserKey >> 24 & 0xFF;
                for (i = 31, L[7] = 0; i >= 0; i--)
                {
                    L[i / 4] = (L[i / 4] << 8) + K[i] + UserKeyBytes[l];
                    if (++l > 3) l = 0;
                }
                for (RC6S[0] = 0xb7e15163, i = 1; i < 84; i++)
                    RC6S[i] = RC6S[i - 1] + 0x9e3779b9;
                for (A = B = i = j = k = 0; k < 84 * 3; k++, i = (i + 1) % 84, j = (j + 1) % 8)
                {
                    A = RC6S[i] = ROTL32(RC6S[i] + A + B, 3);
                    B = L[j] = ROTL32(L[j] + A + B, A + B);
                }
            }

            void KeySetup(int32_t key = 0) {
                UserKey = key;
                RC5KeySetup();
                RC6KeySetup();
            }

            CCrypt(int32_t key = 0)
            {
                KeySetup(key);
            }

            void RC5Encrypt32(const void* source, void* destination, int size)
            {
                uint16_t A, B, * src = (uint16_t*)source, * dst = (uint16_t*)destination;
                int i, j;
                if (source != destination && size % 4) memcpy((char*)destination + size - size % 4, (char*)source + size - size % 4, size % 4);
                for (j = size / 4; j > 0; j--, src += 2, dst += 2)
                {
                    A = src[0] + RC5S[0];
                    B = src[1] + RC5S[1];
                    for (i = 1; i <= 12; i++)
                    {
                        A = ROTL16(A ^ B, B) + RC5S[2 * i];
                        B = ROTL16(B ^ A, A) + RC5S[2 * i + 1];
                    }
                    dst[0] = A ^ UserKey;
                    dst[1] = B ^ UserKey;
                }
            }

            void RC5Decrypt32(const void* source, void* destination, int size)
            {
                uint16_t A, B, * src = (uint16_t*)source, * dst = (uint16_t*)destination;
                int i, j;
                if (source != destination && size % 4) memcpy((char*)destination + size - size % 4, (char*)source + size - size % 4, size % 4);
                for (j = size / 4; j > 0; j--, src += 2, dst += 2)
                {
                    A = src[0] ^ UserKey;
                    B = src[1] ^ UserKey;
                    for (i = 12; i > 0; i--)
                    {
                        B = ROTR16(B - RC5S[2 * i + 1], A) ^ A;
                        A = ROTR16(A - RC5S[2 * i], B) ^ B;
                    }
                    dst[0] = A - RC5S[0];
                    dst[1] = B - RC5S[1];
                }
            }

            void RC5Encrypt64(const void* source, void* destination, int size)
            {
                uint32_t A, B, * src = (uint32_t*)source, * dst = (uint32_t*)destination;
                int i, j;
                for (j = size / 8; j > 0; j--, src += 2, dst += 2)
                {
                    A = src[0] + RC5S[0];
                    B = src[1] + RC5S[1];
                    for (i = 1; i <= 12; i++)
                    {
                        A = ROTL32(A ^ B, B) + RC5S[2 * i];
                        B = ROTL32(B ^ A, A) + RC5S[2 * i + 1];
                    }
                    dst[0] = A ^ UserKey;
                    dst[1] = B ^ UserKey;
                }
                RC5Encrypt32((uint32_t*)source + (size - size % 8) / 4, (uint32_t*)destination + (size - size % 8) / 4, size % 8);
            }

            void RC5Decrypt64(const void* source, void* destination, int size)
            {
                uint32_t A, B, * src = (uint32_t*)source, * dst = (uint32_t*)destination;
                int i, j;
                for (j = size / 8; j > 0; j--, src += 2, dst += 2)
                {
                    A = src[0] ^ UserKey;
                    B = src[1] ^ UserKey;
                    for (i = 12; i > 0; i--)
                    {
                        B = ROTR32(B - RC5S[2 * i + 1], A) ^ A;
                        A = ROTR32(A - RC5S[2 * i], B) ^ B;
                    }
                    dst[0] = A - RC5S[0];
                    dst[1] = B - RC5S[1];
                }
                RC5Decrypt32((uint32_t*)source + (size - size % 8) / 4, (uint32_t*)destination + (size - size % 8) / 4, size % 8);
            }

            void RC6Encrypt128(const void* source, void* destination, int size)
            {
                uint32_t A, B, C, D, t, u, x, * src = (uint32_t*)source, * dst = (uint32_t*)destination;
                int i, j;
                for (j = size / 16; j > 0; j--, src += 4, dst += 4)
                {
                    A = src[0];
                    B = src[1] + RC6S[0];
                    C = src[2];
                    D = src[3] + RC6S[1];
                    for (i = 2; i <= 2 * 40; i += 2)
                    {
                        t = ROTL32(B * (2 * B + 1), 5);
                        u = ROTL32(D * (2 * D + 1), 5);
                        A = ROTL32(A ^ t, u) + RC6S[i];
                        C = ROTL32(C ^ u, t) + RC6S[i + 1];
                        x = A;
                        A = B;
                        B = C;
                        C = D;
                        D = x;
                    }
                    dst[0] = (A + RC6S[2 * 40 + 2]) ^ UserKey;
                    dst[1] = B ^ UserKey;
                    dst[2] = (C + RC6S[2 * 40 + 3]) ^ UserKey;
                    dst[3] = D ^ UserKey;
                }
                RC5Encrypt64((uint32_t*)source + (size - size % 16) / 4, (uint32_t*)destination + (size - size % 16) / 4, size % 16);
            }

            void RC6Decrypt128(const void* source, void* destination, int size)
            {
                uint32_t A, B, C, D, t, u, x, * src = (uint32_t*)source, * dst = (uint32_t*)destination;
                int i, j;
                for (j = size / 16; j > 0; j--, src += 4, dst += 4)
                {
                    A = (src[0] ^ UserKey) - RC6S[2 * 40 + 2];
                    B = src[1] ^ UserKey;
                    C = (src[2] ^ UserKey) - RC6S[2 * 40 + 3];
                    D = src[3] ^ UserKey;
                    for (i = 2 * 40; i >= 2; i -= 2)
                    {
                        x = D;
                        D = C;
                        C = B;
                        B = A;
                        A = x;
                        u = ROTL32(D * (2 * D + 1), 5);
                        t = ROTL32(B * (2 * B + 1), 5);
                        C = ROTR32(C - RC6S[i + 1], t) ^ u;
                        A = ROTR32(A - RC6S[i], u) ^ t;
                    }
                    dst[0] = A;
                    dst[1] = B - RC6S[0];
                    dst[2] = C;
                    dst[3] = D - RC6S[1];
                }
                RC5Decrypt64((uint32_t*)source + (size - size % 16) / 4, (uint32_t*)destination + (size - size % 16) / 4, size % 16);
            }
        };
    }
}

//#endif