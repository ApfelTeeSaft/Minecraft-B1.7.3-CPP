#include "uuid.hpp"
#include "util/types.hpp"
#include <sstream>
#include <iomanip>
#include <random>
#include <cstring>

// Simple MD5 implementation for deterministic UUID generation
namespace {

using namespace mcserver;  // For u8, u32, etc.

// MD5 context
struct MD5Context {
    u32 state[4];
    u32 count[2];
    u8 buffer[64];
};

// MD5 basic functions
#define F(x, y, z) (((x) & (y)) | ((~x) & (z)))
#define G(x, y, z) (((x) & (z)) | ((y) & (~z)))
#define H(x, y, z) ((x) ^ (y) ^ (z))
#define I(x, y, z) ((y) ^ ((x) | (~z)))

#define ROTATE_LEFT(x, n) (((x) << (n)) | ((x) >> (32-(n))))

#define FF(a, b, c, d, x, s, ac) { \
    (a) += F((b), (c), (d)) + (x) + (u32)(ac); \
    (a) = ROTATE_LEFT((a), (s)); \
    (a) += (b); \
}
#define GG(a, b, c, d, x, s, ac) { \
    (a) += G((b), (c), (d)) + (x) + (u32)(ac); \
    (a) = ROTATE_LEFT((a), (s)); \
    (a) += (b); \
}
#define HH(a, b, c, d, x, s, ac) { \
    (a) += H((b), (c), (d)) + (x) + (u32)(ac); \
    (a) = ROTATE_LEFT((a), (s)); \
    (a) += (b); \
}
#define II(a, b, c, d, x, s, ac) { \
    (a) += I((b), (c), (d)) + (x) + (u32)(ac); \
    (a) = ROTATE_LEFT((a), (s)); \
    (a) += (b); \
}

void MD5Init(MD5Context* context) {
    context->count[0] = context->count[1] = 0;
    context->state[0] = 0x67452301;
    context->state[1] = 0xefcdab89;
    context->state[2] = 0x98badcfe;
    context->state[3] = 0x10325476;
}

void MD5Transform(u32 state[4], const u8 block[64]) {
    u32 a = state[0], b = state[1], c = state[2], d = state[3];
    u32 x[16];

    for (int i = 0, j = 0; i < 16; ++i, j += 4) {
        x[i] = ((u32)block[j]) | (((u32)block[j+1]) << 8) |
               (((u32)block[j+2]) << 16) | (((u32)block[j+3]) << 24);
    }

    // Round 1
    FF(a, b, c, d, x[0], 7, 0xd76aa478); FF(d, a, b, c, x[1], 12, 0xe8c7b756);
    FF(c, d, a, b, x[2], 17, 0x242070db); FF(b, c, d, a, x[3], 22, 0xc1bdceee);
    FF(a, b, c, d, x[4], 7, 0xf57c0faf); FF(d, a, b, c, x[5], 12, 0x4787c62a);
    FF(c, d, a, b, x[6], 17, 0xa8304613); FF(b, c, d, a, x[7], 22, 0xfd469501);
    FF(a, b, c, d, x[8], 7, 0x698098d8); FF(d, a, b, c, x[9], 12, 0x8b44f7af);
    FF(c, d, a, b, x[10], 17, 0xffff5bb1); FF(b, c, d, a, x[11], 22, 0x895cd7be);
    FF(a, b, c, d, x[12], 7, 0x6b901122); FF(d, a, b, c, x[13], 12, 0xfd987193);
    FF(c, d, a, b, x[14], 17, 0xa679438e); FF(b, c, d, a, x[15], 22, 0x49b40821);

    // Round 2
    GG(a, b, c, d, x[1], 5, 0xf61e2562); GG(d, a, b, c, x[6], 9, 0xc040b340);
    GG(c, d, a, b, x[11], 14, 0x265e5a51); GG(b, c, d, a, x[0], 20, 0xe9b6c7aa);
    GG(a, b, c, d, x[5], 5, 0xd62f105d); GG(d, a, b, c, x[10], 9, 0x02441453);
    GG(c, d, a, b, x[15], 14, 0xd8a1e681); GG(b, c, d, a, x[4], 20, 0xe7d3fbc8);
    GG(a, b, c, d, x[9], 5, 0x21e1cde6); GG(d, a, b, c, x[14], 9, 0xc33707d6);
    GG(c, d, a, b, x[3], 14, 0xf4d50d87); GG(b, c, d, a, x[8], 20, 0x455a14ed);
    GG(a, b, c, d, x[13], 5, 0xa9e3e905); GG(d, a, b, c, x[2], 9, 0xfcefa3f8);
    GG(c, d, a, b, x[7], 14, 0x676f02d9); GG(b, c, d, a, x[12], 20, 0x8d2a4c8a);

    // Round 3
    HH(a, b, c, d, x[5], 4, 0xfffa3942); HH(d, a, b, c, x[8], 11, 0x8771f681);
    HH(c, d, a, b, x[11], 16, 0x6d9d6122); HH(b, c, d, a, x[14], 23, 0xfde5380c);
    HH(a, b, c, d, x[1], 4, 0xa4beea44); HH(d, a, b, c, x[4], 11, 0x4bdecfa9);
    HH(c, d, a, b, x[7], 16, 0xf6bb4b60); HH(b, c, d, a, x[10], 23, 0xbebfbc70);
    HH(a, b, c, d, x[13], 4, 0x289b7ec6); HH(d, a, b, c, x[0], 11, 0xeaa127fa);
    HH(c, d, a, b, x[3], 16, 0xd4ef3085); HH(b, c, d, a, x[6], 23, 0x04881d05);
    HH(a, b, c, d, x[9], 4, 0xd9d4d039); HH(d, a, b, c, x[12], 11, 0xe6db99e5);
    HH(c, d, a, b, x[15], 16, 0x1fa27cf8); HH(b, c, d, a, x[2], 23, 0xc4ac5665);

    // Round 4
    II(a, b, c, d, x[0], 6, 0xf4292244); II(d, a, b, c, x[7], 10, 0x432aff97);
    II(c, d, a, b, x[14], 15, 0xab9423a7); II(b, c, d, a, x[5], 21, 0xfc93a039);
    II(a, b, c, d, x[12], 6, 0x655b59c3); II(d, a, b, c, x[3], 10, 0x8f0ccc92);
    II(c, d, a, b, x[10], 15, 0xffeff47d); II(b, c, d, a, x[1], 21, 0x85845dd1);
    II(a, b, c, d, x[8], 6, 0x6fa87e4f); II(d, a, b, c, x[15], 10, 0xfe2ce6e0);
    II(c, d, a, b, x[6], 15, 0xa3014314); II(b, c, d, a, x[13], 21, 0x4e0811a1);
    II(a, b, c, d, x[4], 6, 0xf7537e82); II(d, a, b, c, x[11], 10, 0xbd3af235);
    II(c, d, a, b, x[2], 15, 0x2ad7d2bb); II(b, c, d, a, x[9], 21, 0xeb86d391);

    state[0] += a;
    state[1] += b;
    state[2] += c;
    state[3] += d;
}

void MD5Update(MD5Context* context, const u8* input, u32 inputLen) {
    u32 i, index, partLen;

    index = (u32)((context->count[0] >> 3) & 0x3F);

    if ((context->count[0] += ((u32)inputLen << 3)) < ((u32)inputLen << 3))
        context->count[1]++;
    context->count[1] += ((u32)inputLen >> 29);

    partLen = 64 - index;

    if (inputLen >= partLen) {
        std::memcpy(&context->buffer[index], input, partLen);
        MD5Transform(context->state, context->buffer);

        for (i = partLen; i + 63 < inputLen; i += 64)
            MD5Transform(context->state, &input[i]);

        index = 0;
    } else {
        i = 0;
    }

    std::memcpy(&context->buffer[index], &input[i], inputLen - i);
}

void MD5Final(u8 digest[16], MD5Context* context) {
    u8 bits[8];
    u32 index, padLen;

    for (int i = 0, j = 0; i < 2; ++i, j += 4) {
        bits[j] = (u8)(context->count[i] & 0xff);
        bits[j+1] = (u8)((context->count[i] >> 8) & 0xff);
        bits[j+2] = (u8)((context->count[i] >> 16) & 0xff);
        bits[j+3] = (u8)((context->count[i] >> 24) & 0xff);
    }

    index = (u32)((context->count[0] >> 3) & 0x3f);
    padLen = (index < 56) ? (56 - index) : (120 - index);

    static u8 PADDING[64] = {
        0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    };

    MD5Update(context, PADDING, padLen);
    MD5Update(context, bits, 8);

    for (int i = 0, j = 0; i < 4; ++i, j += 4) {
        digest[j] = (u8)(context->state[i] & 0xff);
        digest[j+1] = (u8)((context->state[i] >> 8) & 0xff);
        digest[j+2] = (u8)((context->state[i] >> 16) & 0xff);
        digest[j+3] = (u8)((context->state[i] >> 24) & 0xff);
    }
}

std::array<u8, 16> MD5Hash(const std::string& str) {
    MD5Context ctx;
    MD5Init(&ctx);
    MD5Update(&ctx, reinterpret_cast<const u8*>(str.data()), static_cast<u32>(str.size()));

    std::array<u8, 16> result;
    MD5Final(result.data(), &ctx);
    return result;
}

} // anonymous namespace

namespace mcserver {

UUID::UUID() {
    bytes_.fill(0);
}

UUID::UUID(const std::array<u8, 16>& bytes) : bytes_(bytes) {}

UUID UUID::from_string(const std::string& str) {
    // Generate deterministic UUID from string using MD5
    auto hash = MD5Hash(str);

    // Set version to 3 (name-based, MD5)
    hash[6] = (hash[6] & 0x0F) | 0x30;
    // Set variant to RFC 4122
    hash[8] = (hash[8] & 0x3F) | 0x80;

    return UUID(hash);
}

UUID UUID::random() {
    static std::random_device rd;
    static std::mt19937_64 gen(rd());
    static std::uniform_int_distribution<unsigned int> dist(0, 255);

    std::array<u8, 16> bytes;
    for (auto& byte : bytes) {
        byte = static_cast<u8>(dist(gen));
    }

    // Set version to 4 (random)
    bytes[6] = (bytes[6] & 0x0F) | 0x40;
    // Set variant to RFC 4122
    bytes[8] = (bytes[8] & 0x3F) | 0x80;

    return UUID(bytes);
}

UUID UUID::from_formatted_string(const std::string& str) {
    std::array<u8, 16> bytes;
    bytes.fill(0);

    // Parse format: xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx
    if (str.length() != 36) {
        return UUID(bytes);  // Return zero UUID on invalid format
    }

    int byte_idx = 0;
    for (size_t i = 0; i < str.length() && byte_idx < 16; i++) {
        if (str[i] == '-') continue;

        char hex[3] = {str[i], str[i+1], 0};
        bytes[byte_idx++] = static_cast<u8>(std::strtol(hex, nullptr, 16));
        i++;  // Skip second hex digit
    }

    return UUID(bytes);
}

std::string UUID::to_string() const {
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');

    for (size_t i = 0; i < 16; i++) {
        oss << std::setw(2) << static_cast<int>(bytes_[i]);
        if (i == 3 || i == 5 || i == 7 || i == 9) {
            oss << '-';
        }
    }

    return oss.str();
}

std::string UUID::to_filename() const {
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');

    for (size_t i = 0; i < 16; i++) {
        oss << std::setw(2) << static_cast<int>(bytes_[i]);
    }

    return oss.str();
}

} // namespace mcserver
