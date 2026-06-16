#include "metaUtils.h"
#include "logger.h"
#include "zlib.h"

static const std::string B64_CHARS =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

std::string base64Encode(const std::string& input) {
    std::string out;
    int val = 0, valb = -6;
    for (unsigned char c : input) {
        val = (val << 8) + c;
        valb += 8;
        while (valb >= 0) {
            out.push_back(B64_CHARS[(val >> valb) & 0x3F]);
            valb -= 6;
        }
    }
    if (valb > -6) out.push_back(B64_CHARS[((val << 8) >> (valb + 8)) & 0x3F]);
    while (out.size() % 4) out.push_back('=');
    return out;
}

std::string base64Decode(const std::string& input) {
    std::string out;
    std::vector<int> T(256, -1);
    for (int i = 0; i < 64; i++) T[B64_CHARS[i]] = i;
    int val = 0, valb = -8;
    for (unsigned char c : input) {
        if (T[c] == -1) break;
        val = (val << 6) + T[c];
        valb += 6;
        if (valb >= 0) {
            out.push_back(char((val >> valb) & 0xFF));
            valb -= 8;
        }
    }
    return out;
}

std::string compressAndEncode(const std::string& input) {

    if (input.empty())
        return "";
    uLongf compressedSize = compressBound(input.size());
    std::string compressed(compressedSize, '\0');

    int result = compress2(
        reinterpret_cast<Bytef*>(compressed.data()), &compressedSize,
        reinterpret_cast<const Bytef*>(input.data()), input.size(),
        Z_BEST_COMPRESSION
    );

    if (result != Z_OK) {
        LOG_WARN("Roll metadata compression failed: {}", result);
        return "";
    }

    compressed.resize(compressedSize);
    return base64Encode(compressed);
}

std::string decodeAndDecompress(const std::string& input, size_t maxOutputSize) {
    std::string compressed = base64Decode(input);

    std::string decompressed(maxOutputSize, '\0');
    uLongf decompressedSize = maxOutputSize;

    int result = uncompress(
        reinterpret_cast<Bytef*>(decompressed.data()), &decompressedSize,
        reinterpret_cast<const Bytef*>(compressed.data()), compressed.size()
    );

    if (result != Z_OK) {
        LOG_WARN("Roll metadata decompression failed: {}", result);
        return "";
    }

    decompressed.resize(decompressedSize);
    return decompressed;
}
