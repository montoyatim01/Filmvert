#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <vector>
#include <cmath>
#include "utils.h"

using Catch::Matchers::WithinAbs;
using Catch::Matchers::WithinRel;

// ---------------------------------------------------------------------------
// iDivUp — ceiling integer division
// ---------------------------------------------------------------------------
TEST_CASE("iDivUp divides exactly", "[utils]") {
    CHECK(iDivUp(9, 3)  == 3);
    CHECK(iDivUp(0, 3)  == 0);
    CHECK(iDivUp(6, 6)  == 1);
    CHECK(iDivUp(12, 4) == 3);
}

TEST_CASE("iDivUp rounds up when there is a remainder", "[utils]") {
    CHECK(iDivUp(10, 3) == 4);
    CHECK(iDivUp(1,  5) == 1);
    CHECK(iDivUp(7,  4) == 2);
    CHECK(iDivUp(11, 4) == 3);
}

// ---------------------------------------------------------------------------
// swapBytes16 / swapBytes32 — endian helpers
// ---------------------------------------------------------------------------
TEST_CASE("swapBytes16 reverses byte order", "[utils]") {
    CHECK(swapBytes16(0x1234) == 0x3412);
    CHECK(swapBytes16(0xFF00) == 0x00FF);
    CHECK(swapBytes16(0x0001) == 0x0100);
}

TEST_CASE("swapBytes16 is its own inverse", "[utils]") {
    uint16_t values[] = {0xABCD, 0x0000, 0xFFFF, 0x1234};
    for (uint16_t v : values)
        CHECK(swapBytes16(swapBytes16(v)) == v);
}

TEST_CASE("swapBytes32 reverses byte order", "[utils]") {
    CHECK(swapBytes32(0x12345678) == 0x78563412);
    CHECK(swapBytes32(0xFF000000) == 0x000000FF);
    CHECK(swapBytes32(0x00000001) == 0x01000000);
}

TEST_CASE("swapBytes32 is its own inverse", "[utils]") {
    uint32_t values[] = {0xDEADBEEF, 0x00000000, 0xFFFFFFFF, 0x12345678};
    for (uint32_t v : values)
        CHECK(swapBytes32(swapBytes32(v)) == v);
}

// ---------------------------------------------------------------------------
// Luma — ACES AP1 luminance coefficients
// Coefficients: R=0.2722287168, G=0.6740817658, B=0.0536895174
// ---------------------------------------------------------------------------
TEST_CASE("Luma returns 0 for black", "[utils]") {
    REQUIRE_THAT(Luma(0.0f, 0.0f, 0.0f), WithinAbs(0.0f, 1e-6f));
}

TEST_CASE("Luma returns ~1 for white", "[utils]") {
    // Coefficients sum to 1.0
    REQUIRE_THAT(Luma(1.0f, 1.0f, 1.0f), WithinAbs(1.0f, 1e-5f));
}

TEST_CASE("Luma weights each channel correctly", "[utils]") {
    REQUIRE_THAT(Luma(1.0f, 0.0f, 0.0f), WithinAbs(0.2722287168f, 1e-6f));
    REQUIRE_THAT(Luma(0.0f, 1.0f, 0.0f), WithinAbs(0.6740817658f, 1e-6f));
    REQUIRE_THAT(Luma(0.0f, 0.0f, 1.0f), WithinAbs(0.0536895174f, 1e-6f));
}

TEST_CASE("Luma is linear in each channel", "[utils]") {
    float half_r = Luma(0.5f, 0.0f, 0.0f);
    float full_r = Luma(1.0f, 0.0f, 0.0f);
    REQUIRE_THAT(half_r, WithinAbs(full_r * 0.5f, 1e-6f));
}

// ---------------------------------------------------------------------------
// ap0_to_ap1 — colour space matrix
// ---------------------------------------------------------------------------
TEST_CASE("ap0_to_ap1 does not produce NaN or Inf", "[utils]") {
    float in[3]  = {0.5f, 0.5f, 0.5f};
    float out[3] = {0.0f, 0.0f, 0.0f};
    ap0_to_ap1(in, out);
    for (int i = 0; i < 3; ++i) {
        CHECK(std::isfinite(out[i]));
    }
}

TEST_CASE("ap0_to_ap1 maps black to black", "[utils]") {
    float in[3]  = {0.0f, 0.0f, 0.0f};
    float out[3] = {1.0f, 1.0f, 1.0f};
    ap0_to_ap1(in, out);
    for (int i = 0; i < 3; ++i)
        REQUIRE_THAT(out[i], WithinAbs(0.0f, 1e-6f));
}

TEST_CASE("ap0_to_ap1 equal-energy white maps to near-white in AP1", "[utils]") {
    // AP0 equal-energy white (1,1,1) should map close to AP1 white
    float in[3]  = {1.0f, 1.0f, 1.0f};
    float out[3] = {0.0f, 0.0f, 0.0f};
    ap0_to_ap1(in, out);
    for (int i = 0; i < 3; ++i)
        REQUIRE_THAT(out[i], WithinAbs(1.0f, 0.05f));
}

// ---------------------------------------------------------------------------
// isPointInBox — winding-number polygon test
// ---------------------------------------------------------------------------
static bool inUnitSquare(unsigned int x, unsigned int y) {
    // Square with corners (0,0),(10,0),(10,10),(0,10)
    unsigned int xs[] = {0, 10, 10, 0};
    unsigned int ys[] = {0, 0,  10, 10};
    return isPointInBox(x, y, xs, ys);
}

TEST_CASE("isPointInBox detects interior point", "[utils]") {
    CHECK(inUnitSquare(5, 5) == true);
    CHECK(inUnitSquare(1, 1) == true);
    CHECK(inUnitSquare(9, 9) == true);
}

TEST_CASE("isPointInBox rejects exterior point", "[utils]") {
    CHECK(inUnitSquare(11, 5)  == false);
    CHECK(inUnitSquare(5,  11) == false);
    CHECK(inUnitSquare(20, 20) == false);
}

// ---------------------------------------------------------------------------
// computeKernels — Gaussian kernel
// ---------------------------------------------------------------------------
// Helper: compute the kernel size the same way computeKernels() does internally
static int kernelSizeFor(float strength) {
    int ks = (int)(strength * KERNELSIZE) + 1;
    return ks % 2 == 0 ? ks + 1 : ks;
}

TEST_CASE("computeKernels weights sum to 1", "[utils]") {
    const float strength = 5.0f;
    int ks = kernelSizeFor(strength);
    std::vector<float> kernels(ks, 0.0f);
    computeKernels(strength, kernels.data());

    float sum = 0.0f;
    for (int i = 0; i < ks; ++i)
        sum += kernels[i];

    REQUIRE_THAT(sum, WithinAbs(1.0f, 1e-5f));
}

TEST_CASE("computeKernels weights are non-negative", "[utils]") {
    const float strength = 3.0f;
    int ks = kernelSizeFor(strength);
    std::vector<float> kernels(ks, 0.0f);
    computeKernels(strength, kernels.data());

    for (int i = 0; i < ks; ++i)
        CHECK(kernels[i] >= 0.0f);
}
