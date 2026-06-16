#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "imageParams.h"
#include <nlohmann/json.hpp>
#include <array>

using Catch::Matchers::WithinAbs;

// ---------------------------------------------------------------------------
// isIdentity
// ---------------------------------------------------------------------------
TEST_CASE("imageCurve: default construction is identity", "[imageCurve]") {
    imageCurve c;
    CHECK(c.isIdentity());
}

TEST_CASE("imageCurve: modifying py breaks identity", "[imageCurve]") {
    imageCurve c;
    c.py[2] = 0.6f; // push midpoint up
    CHECK_FALSE(c.isIdentity());
}

TEST_CASE("imageCurve: modifying px breaks identity", "[imageCurve]") {
    imageCurve c;
    c.px[1] = 0.30f; // shift second control point
    CHECK_FALSE(c.isIdentity());
}

TEST_CASE("imageCurve: single-epsilon change is caught by memcmp", "[imageCurve]") {
    imageCurve c;
    c.py[0] = 0.001f; // tiny but nonzero deviation at black point
    CHECK_FALSE(c.isIdentity());
}

// ---------------------------------------------------------------------------
// reset
// ---------------------------------------------------------------------------
TEST_CASE("imageCurve: reset restores identity from a modified state", "[imageCurve]") {
    imageCurve c;
    c.py[0] = 0.1f;
    c.py[2] = 0.7f;
    c.px[3] = 0.8f;
    c.reset();
    CHECK(c.isIdentity());
}

TEST_CASE("imageCurve: reset on an already-identity curve is a no-op", "[imageCurve]") {
    imageCurve c;
    c.reset();
    CHECK(c.isIdentity());
}

// ---------------------------------------------------------------------------
// operator== / operator!=
// ---------------------------------------------------------------------------
TEST_CASE("imageCurve: two default curves are equal", "[imageCurve]") {
    imageCurve a, b;
    CHECK(a == b);
    CHECK_FALSE(a != b);
}

TEST_CASE("imageCurve: modified py makes curves unequal", "[imageCurve]") {
    imageCurve a, b;
    b.py[2] = 0.7f;
    CHECK(a != b);
    CHECK_FALSE(a == b);
}

TEST_CASE("imageCurve: modified px makes curves unequal", "[imageCurve]") {
    imageCurve a, b;
    b.px[1] = 0.35f;
    CHECK(a != b);
}

TEST_CASE("imageCurve: resetting a modified curve restores equality", "[imageCurve]") {
    imageCurve a, b;
    b.py[3] = 0.8f;
    REQUIRE(a != b);
    b.reset();
    CHECK(a == b);
}

TEST_CASE("imageCurve: two independently modified curves with same values are equal", "[imageCurve]") {
    imageCurve a, b;
    a.py[2] = 0.65f;
    b.py[2] = 0.65f;
    CHECK(a == b);
}

// ---------------------------------------------------------------------------
// JSON round-trip — single imageCurve
// ---------------------------------------------------------------------------
TEST_CASE("imageCurve JSON: to_json produces an object with 'px' and 'py' keys", "[imageCurve][json]") {
    imageCurve c;
    nlohmann::json j = c;
    CHECK(j.contains("px"));
    CHECK(j.contains("py"));
}

TEST_CASE("imageCurve JSON: 'px' and 'py' arrays have exactly 5 elements", "[imageCurve][json]") {
    imageCurve c;
    nlohmann::json j = c;
    CHECK(j["px"].size() == 5);
    CHECK(j["py"].size() == 5);
}

TEST_CASE("imageCurve JSON: round-trip of the identity curve", "[imageCurve][json]") {
    imageCurve original;
    nlohmann::json j    = original;
    imageCurve restored = j.get<imageCurve>();
    CHECK(original == restored);
}

TEST_CASE("imageCurve JSON: round-trip of a modified S-curve", "[imageCurve][json]") {
    imageCurve original;
    original.py[1] = 0.05f;
    original.py[2] = 0.55f;
    original.py[3] = 0.95f;
    nlohmann::json j    = original;
    imageCurve restored = j.get<imageCurve>();
    CHECK(original == restored);
}

TEST_CASE("imageCurve JSON: round-trip of a curve with non-default px positions", "[imageCurve][json]") {
    imageCurve original;
    original.px[1] = 0.30f;
    original.px[3] = 0.80f;
    nlohmann::json j    = original;
    imageCurve restored = j.get<imageCurve>();
    CHECK(original == restored);
}

TEST_CASE("imageCurve JSON: missing keys fall back to default identity values", "[imageCurve][json]") {
    // An empty JSON object should deserialise to the identity curve (default member init)
    nlohmann::json j    = nlohmann::json::object();
    imageCurve restored = j.get<imageCurve>();
    CHECK(restored.isIdentity());
}

// ---------------------------------------------------------------------------
// JSON round-trip — std::array<imageCurve, 4> (the type used in imageParams)
// ---------------------------------------------------------------------------
TEST_CASE("std::array<imageCurve,4> JSON: serialises to a JSON array of 4 elements", "[imageCurve][json]") {
    std::array<imageCurve, 4> curves;
    nlohmann::json j = curves;
    REQUIRE(j.is_array());
    CHECK(j.size() == 4);
}

TEST_CASE("std::array<imageCurve,4> JSON: round-trip preserves all curves", "[imageCurve][json]") {
    std::array<imageCurve, 4> original;
    original[0].py[2] = 0.40f; // darken master
    original[1].py[3] = 0.85f; // lighten red highlights
    // [2] and [3] remain identity

    nlohmann::json j              = original;
    auto restored                 = j.get<std::array<imageCurve, 4>>();

    for (int i = 0; i < 4; ++i)
        CHECK(original[i] == restored[i]);
}

TEST_CASE("std::array<imageCurve,4> JSON: empty curve entries fall back to identity", "[imageCurve][json]") {
    // Four empty JSON objects — from_json should leave each curve at its default
    nlohmann::json j = nlohmann::json::array();
    for (int i = 0; i < 4; ++i)
        j.push_back(nlohmann::json::object());

    auto curves = j.get<std::array<imageCurve, 4>>();
    for (int i = 0; i < 4; ++i)
        CHECK(curves[i].isIdentity());
}
