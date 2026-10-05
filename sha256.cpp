#include <iostream>
#include <cstdint>
#include <vector>
#include <array>
#include <iomanip>
#include <sstream>
#include <stdexcept>

#include "sha256.h"

using namespace std;

// rotate right
uint32_t rotr(uint32_t x, uint32_t n)
{
    return (x >> n) | (x << (32 - n));
}

// Choose
uint32_t Ch(uint32_t x, uint32_t y, uint32_t z)
{
    return (x & y) ^ (~x & z);
}

// Majority
uint32_t Maj(uint32_t x, uint32_t y, uint32_t z)
{
    return (x & y) ^ (y & z) ^ (x & z);
}
uint32_t Sigma0(uint32_t x)
{
    return rotr(x, 2) ^ rotr(x, 13) ^ rotr(x, 22);
}

uint32_t Sigma1(uint32_t x)
{
    return rotr(x, 6) ^ rotr(x, 11) ^ rotr(x, 25);
}

vector<uint8_t> padMessage(const string &inp)
{
    vector<uint8_t> message(inp.begin(), inp.end());
    uint64_t original_bit_len = static_cast<uint64_t>(inp.size()) * 8;
    message.push_back(0x80);
    while (message.size() % 64 != 56)
    {
        message.push_back(0x00);
    }
    for (int i = 7; i >= 0; i--)
    {
        uint8_t byte = static_cast<uint8_t>((original_bit_len >> (i * 8)) & 0xFF);
        message.push_back(byte);
    }
    return message;
}

uint32_t sigma0(uint32_t x)
{
    return rotr(x, 7) ^
           rotr(x, 18) ^
           (x >> 3);
}

uint32_t sigma1(uint32_t x)
{
    return rotr(x, 17) ^
           rotr(x, 19) ^
           (x >> 10);
}

vector<uint32_t> createMessageSchedule(const vector<uint8_t> &block)
{
    if (block.size() != 64)
    {
        throw runtime_error("Block must contain exactly 64 bytes");
    }

    vector<uint32_t> W(64);
    for (int i = 0; i < 64; i += 4)
    {
        uint32_t word_32 = (static_cast<uint32_t>(block[i]) << 24) |
                           (static_cast<uint32_t>(block[i + 1]) << 16) |
                           (static_cast<uint32_t>(block[i + 2]) << 8) |
                           (static_cast<uint32_t>(block[i + 3]));
        W[i / 4] = word_32;
    }
    for (int i = 16; i < 64; i++)
    {
        W[i] = sigma1(W[i - 2]) + W[i - 7] + sigma0(W[i - 15]) + W[i - 16];
    }
    return W;
}

const uint32_t K[64] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5,
    0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
    0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
    0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
    0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
    0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3,
    0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5,
    0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
    0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2};

array<uint32_t, 8> compress(const vector<uint32_t> &W, array<uint32_t, 8> H)
{
    uint32_t a = H[0];
    uint32_t b = H[1];
    uint32_t c = H[2];
    uint32_t d = H[3];
    uint32_t e = H[4];
    uint32_t f = H[5];
    uint32_t g = H[6];
    uint32_t h = H[7];

    for (int i = 0; i < 64; i++)
    {
        uint32_t T1 =
            h + Sigma1(e) + Ch(e, f, g) + K[i] + W[i];

        uint32_t T2 =
            Sigma0(a) + Maj(a, b, c);

        h = g;
        g = f;
        f = e;
        e = d + T1;
        d = c;
        c = b;
        b = a;
        a = T1 + T2;
    }

    H[0] += a;
    H[1] += b;
    H[2] += c;
    H[3] += d;
    H[4] += e;
    H[5] += f;
    H[6] += g;
    H[7] += h;

    return H;
}

string hashToHex(const array<uint32_t, 8> &H)
{
    stringstream ss;
    for (int i = 0; i < 8; i++)
    {
        ss << hex << setw(8) << setfill('0') << H[i];
    }
    return ss.str();
}

string sha256(const string &input)
{
    auto message = padMessage(input);
    size_t total_bytes = message.size();
    array<uint32_t, 8> H = {
        0x6a09e667,
        0xbb67ae85,
        0x3c6ef372,
        0xa54ff53a,
        0x510e527f,
        0x9b05688c,
        0x1f83d9ab,
        0x5be0cd19};

    for (int i = 0; i < total_bytes; i += 64)
    {
        vector<uint8_t> block(message.begin() + i, message.begin() + i + 64);
        vector<uint32_t> W = createMessageSchedule(block);
        H = compress(W, H);
    }
    return hashToHex(H);
}
