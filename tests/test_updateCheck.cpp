#include <catch2/catch_test_macros.hpp>
#include "updateCheck.h"
#include "structs.h"
#include <stdexcept>
#include <string>

// ---------------------------------------------------------------------------
// parseVersion — valid inputs
// ---------------------------------------------------------------------------
TEST_CASE("parseVersion: simple release version", "[updateCheck]") {
    Version v = parseVersion("v1.2.3");
    CHECK(v.major == 1);
    CHECK(v.minor == 2);
    CHECK(v.patch == 3);
    CHECK(v.stage == ReleaseStage::Release);
}

TEST_CASE("parseVersion: all-zero version", "[updateCheck]") {
    Version v = parseVersion("v0.0.0");
    CHECK(v.major == 0);
    CHECK(v.minor == 0);
    CHECK(v.patch == 0);
    CHECK(v.stage == ReleaseStage::Release);
}

TEST_CASE("parseVersion: alpha suffix", "[updateCheck]") {
    Version v = parseVersion("v2.0.0a");
    CHECK(v.major == 2);
    CHECK(v.stage == ReleaseStage::Alpha);
}

TEST_CASE("parseVersion: beta suffix", "[updateCheck]") {
    Version v = parseVersion("v1.5.3b");
    CHECK(v.minor == 5);
    CHECK(v.patch == 3);
    CHECK(v.stage == ReleaseStage::Beta);
}

TEST_CASE("parseVersion: large component values", "[updateCheck]") {
    Version v = parseVersion("v65535.100.999");
    CHECK(v.major == 65535);
    CHECK(v.minor == 100);
    CHECK(v.patch == 999);
}

TEST_CASE("parseVersion: multi-digit components", "[updateCheck]") {
    Version v = parseVersion("v10.20.30");
    CHECK(v.major == 10);
    CHECK(v.minor == 20);
    CHECK(v.patch == 30);
}

// ---------------------------------------------------------------------------
// parseVersion — invalid inputs (must throw)
// ---------------------------------------------------------------------------
TEST_CASE("parseVersion: throws on missing leading 'v'", "[updateCheck]") {
    CHECK_THROWS_AS(parseVersion("1.2.3"),   std::invalid_argument);
    CHECK_THROWS_AS(parseVersion("V1.2.3"),  std::invalid_argument); // uppercase V not accepted
}

TEST_CASE("parseVersion: throws on empty string", "[updateCheck]") {
    CHECK_THROWS(parseVersion(""));
}

TEST_CASE("parseVersion: throws on missing dots", "[updateCheck]") {
    CHECK_THROWS_AS(parseVersion("v123"),    std::invalid_argument);
    CHECK_THROWS_AS(parseVersion("v1.2"),    std::invalid_argument); // only one dot
}

TEST_CASE("parseVersion: throws on non-digit component", "[updateCheck]") {
    CHECK_THROWS_AS(parseVersion("v1.x.3"),  std::invalid_argument);
    CHECK_THROWS_AS(parseVersion("va.2.3"),  std::invalid_argument);
    CHECK_THROWS_AS(parseVersion("v1.2.z"),  std::invalid_argument);
}

TEST_CASE("parseVersion: throws on unknown suffix character", "[updateCheck]") {
    CHECK_THROWS_AS(parseVersion("v1.2.3c"),   std::invalid_argument); // 'c' not valid
    CHECK_THROWS_AS(parseVersion("v1.2.3r"),   std::invalid_argument);
}

TEST_CASE("parseVersion: throws on trailing garbage after suffix", "[updateCheck]") {
    CHECK_THROWS_AS(parseVersion("v1.2.3-rc"), std::invalid_argument);
    CHECK_THROWS_AS(parseVersion("v1.2.3.4"),  std::invalid_argument);
    CHECK_THROWS_AS(parseVersion("v1.2.3a1"),  std::invalid_argument);
}

// ---------------------------------------------------------------------------
// Version::operator>
// ---------------------------------------------------------------------------
TEST_CASE("Version: newer major beats any minor/patch", "[updateCheck]") {
    Version a = parseVersion("v2.0.0");
    Version b = parseVersion("v1.9.9");
    CHECK(a > b);
    CHECK_FALSE(b > a);
}

TEST_CASE("Version: newer minor wins when major is equal", "[updateCheck]") {
    Version a = parseVersion("v1.3.0");
    Version b = parseVersion("v1.2.9");
    CHECK(a > b);
    CHECK_FALSE(b > a);
}

TEST_CASE("Version: newer patch wins when major and minor are equal", "[updateCheck]") {
    Version a = parseVersion("v1.2.4");
    Version b = parseVersion("v1.2.3");
    CHECK(a > b);
    CHECK_FALSE(b > a);
}

TEST_CASE("Version: Release > Beta > Alpha at the same numeric version", "[updateCheck]") {
    Version rel   = parseVersion("v1.0.0");
    Version beta  = parseVersion("v1.0.0b");
    Version alpha = parseVersion("v1.0.0a");
    CHECK(rel  > beta);
    CHECK(rel  > alpha);
    CHECK(beta > alpha);
    CHECK_FALSE(alpha > beta);
    CHECK_FALSE(beta  > rel);
    CHECK_FALSE(alpha > rel);
}

TEST_CASE("Version: equal versions are not greater than each other", "[updateCheck]") {
    Version a = parseVersion("v1.2.3");
    Version b = parseVersion("v1.2.3");
    CHECK_FALSE(a > b);
    CHECK_FALSE(b > a);
}

TEST_CASE("Version: newer numeric always beats older stage", "[updateCheck]") {
    // v1.2.4a (alpha) is still greater than v1.2.3 (release)
    Version newer_alpha  = parseVersion("v1.2.4a");
    Version older_release = parseVersion("v1.2.3");
    CHECK(newer_alpha > older_release);
}

// ---------------------------------------------------------------------------
// Version::versionString
// ---------------------------------------------------------------------------
TEST_CASE("versionString: release version has no suffix", "[updateCheck]") {
    Version v = parseVersion("v1.2.3");
    CHECK(v.versionString() == "1.2.3");
}

TEST_CASE("versionString: alpha version appends 'a'", "[updateCheck]") {
    Version v = parseVersion("v2.0.0a");
    CHECK(v.versionString() == "2.0.0a");
}

TEST_CASE("versionString: beta version appends 'b'", "[updateCheck]") {
    Version v = parseVersion("v1.5.0b");
    CHECK(v.versionString() == "1.5.0b");
}

TEST_CASE("versionString: all-zero release", "[updateCheck]") {
    Version v = parseVersion("v0.0.0");
    CHECK(v.versionString() == "0.0.0");
}

// ---------------------------------------------------------------------------
// Round-trip: parseVersion ↔ versionString
// ---------------------------------------------------------------------------
TEST_CASE("Round-trip: parseVersion(versionString) recovers the original version", "[updateCheck]") {
    const char* tags[] = {
        "v1.2.3", "v0.0.0", "v2.1.0a", "v10.5.99b", "v65535.0.0"
    };
    for (const char* tag : tags) {
        Version v      = parseVersion(tag);
        std::string rt = "v" + v.versionString();
        CHECK(rt == std::string(tag));
    }
}
