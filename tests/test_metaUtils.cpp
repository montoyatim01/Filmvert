#include <catch2/catch_test_macros.hpp>
#include "metaUtils.h"
#include <string>
#include <vector>

// Forward declarations for functions that are compiled in via MISC_SRCS
// but not exported through metaUtils.h (they are implementation details).
std::string base64Encode(const std::string& input);
std::string base64Decode(const std::string& input);

// saniJsonString lives in exifUtils.cpp, part of IMAGE_SRCS.
std::string saniJsonString(const std::string& input);

// ---------------------------------------------------------------------------
// saniJsonString
// ---------------------------------------------------------------------------
TEST_CASE("saniJsonString: clean JSON object is returned unchanged", "[saniJsonString]") {
    CHECK(saniJsonString(R"({"key":"value"})") == R"({"key":"value"})");
}

TEST_CASE("saniJsonString: clean JSON array is returned unchanged", "[saniJsonString]") {
    CHECK(saniJsonString("[1,2,3]") == "[1,2,3]");
}

TEST_CASE("saniJsonString: strips arbitrary prefix before '{'", "[saniJsonString]") {
    CHECK(saniJsonString(R"(JUNKPREFIX{"a":1})") == R"({"a":1})");
}

TEST_CASE("saniJsonString: strips base64-like prefix before '['", "[saniJsonString]") {
    CHECK(saniJsonString("ABC123dGVzdA==[1,2,3]") == "[1,2,3]");
}

TEST_CASE("saniJsonString: returns empty string when no '{' or '[' is found", "[saniJsonString]") {
    CHECK(saniJsonString("no json here") == "");
    CHECK(saniJsonString("")             == "");
    CHECK(saniJsonString("ABCDEFG")      == "");
}

TEST_CASE("saniJsonString: first '{' wins over a later '['", "[saniJsonString]") {
    // Both are present — the function should start at whichever comes first
    CHECK(saniJsonString("prefix{obj}[arr]") == "{obj}[arr]");
}

TEST_CASE("saniJsonString: first '[' wins when it comes before '{'", "[saniJsonString]") {
    CHECK(saniJsonString("prefix[arr]{obj}") == "[arr]{obj}");
}

TEST_CASE("saniJsonString: leading whitespace is stripped (whitespace before '{')", "[saniJsonString]") {
    // Spaces are not '{' or '[', so they get stripped
    CHECK(saniJsonString("   {\"key\":1}") == "{\"key\":1}");
}

// ---------------------------------------------------------------------------
// base64Encode / base64Decode
// ---------------------------------------------------------------------------
TEST_CASE("base64Encode: empty string produces empty string", "[base64]") {
    CHECK(base64Encode("") == "");
}

TEST_CASE("base64Decode: empty string produces empty string", "[base64]") {
    CHECK(base64Decode("") == "");
}

TEST_CASE("base64Encode: output length is always a multiple of 4", "[base64]") {
    for (int len = 1; len <= 12; ++len) {
        std::string input(len, 'A');
        CHECK(base64Encode(input).size() % 4 == 0);
    }
}

TEST_CASE("base64 round-trip: encode then decode returns the original string", "[base64]") {
    const std::vector<std::string> cases = {
        "hello",
        "Hello, World!",
        "The quick brown fox jumps over the lazy dog",
        std::string(1, '\x00'),        // single null byte
        std::string(3, '\xFF'),        // all-high bytes
        std::string(100, 'X'),         // longer run
        std::string(255, '\x01'),      // near-full byte range
    };
    for (const auto& s : cases)
        CHECK(base64Decode(base64Encode(s)) == s);
}

TEST_CASE("base64 round-trip: boundary lengths 1 through 6 all work", "[base64]") {
    // Lengths 1, 2, 3 require one, two, or zero '=' pads; 4-6 repeat the cycle
    for (int len = 1; len <= 6; ++len) {
        std::string input(len, static_cast<char>(len * 37)); // arbitrary non-zero content
        CHECK(base64Decode(base64Encode(input)) == input);
    }
}

TEST_CASE("base64 round-trip: binary content with all byte values 0-255", "[base64]") {
    std::string all_bytes;
    for (int i = 0; i < 256; ++i)
        all_bytes.push_back(static_cast<char>(i));
    CHECK(base64Decode(base64Encode(all_bytes)) == all_bytes);
}

// ---------------------------------------------------------------------------
// compressAndEncode / decodeAndDecompress
// ---------------------------------------------------------------------------
TEST_CASE("compressAndEncode: non-empty input produces non-empty output", "[compress]") {
    CHECK_FALSE(compressAndEncode("hello world").empty());
}

TEST_CASE("compressAndEncode: empty input produces empty string", "[compress]") {
    // compress2 on 0 bytes returns Z_OK but produces no data
    CHECK(compressAndEncode("").empty());
}

TEST_CASE("compress round-trip: short JSON-like string", "[compress]") {
    std::string original = R"({"key":"value","number":42})";
    std::string encoded  = compressAndEncode(original);
    std::string decoded  = decodeAndDecompress(encoded, original.size() * 4);
    CHECK(decoded == original);
}

TEST_CASE("compress round-trip: highly compressible string", "[compress]") {
    std::string original(4096, 'A'); // trivially compressible run of identical bytes
    std::string encoded  = compressAndEncode(original);
    std::string decoded  = decodeAndDecompress(encoded, original.size() * 2);
    CHECK(decoded == original);
    // Sanity check: the encoded form should be much smaller than the original
    CHECK(encoded.size() < original.size());
}

TEST_CASE("compress round-trip: realistic imageParams JSON", "[compress]") {
    std::string original =
        R"({"imageParams":{"g_gain":[1.0,1.0,1.0,1.0],"g_gamma":[1.0,1.0,1.0,1.0],)"
        R"("curves":[{"px":[0,0.25,0.5,0.75,1],"py":[0,0.25,0.5,0.75,1]},)"
        R"({"px":[0,0.25,0.5,0.75,1],"py":[0,0.1,0.5,0.9,1]}]}})";
    std::string encoded  = compressAndEncode(original);
    std::string decoded  = decodeAndDecompress(encoded, 4096);
    CHECK(decoded == original);
}

TEST_CASE("compress round-trip: single character", "[compress]") {
    std::string original = "X";
    std::string encoded  = compressAndEncode(original);
    std::string decoded  = decodeAndDecompress(encoded, 64);
    CHECK(decoded == original);
}

TEST_CASE("decodeAndDecompress: garbage input returns empty string", "[compress]") {
    // Random data that is not valid compressed+base64 content
    std::string result = decodeAndDecompress("not_valid_compressed_data", 1024);
    CHECK(result.empty());
}

TEST_CASE("decodeAndDecompress: maxOutputSize is enforced", "[compress]") {
    std::string original(1024, 'Z');
    std::string encoded  = compressAndEncode(original);
    // Request with a buffer far too small — zlib returns Z_BUF_ERROR
    std::string result = decodeAndDecompress(encoded, 10);
    CHECK(result.empty());
}
