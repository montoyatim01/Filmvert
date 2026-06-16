#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "imageParams.h"

using Catch::Matchers::WithinAbs;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------
static bool allEqual(const float* arr, int n, float val) {
    for (int i = 0; i < n; ++i)
        if (arr[i] != val) return false;
    return true;
}

// ---------------------------------------------------------------------------
// Default values — these serve as a baseline. If a default changes
// accidentally, these tests catch it immediately.
// ---------------------------------------------------------------------------
TEST_CASE("imageParams default black point is all zeros", "[imageParams]") {
    imageParams p;
    CHECK(allEqual(p.blackPoint,   4, 0.0f));
    CHECK(allEqual(p.g_blackpoint, 4, 0.0f));
}

TEST_CASE("imageParams default white point is all ones", "[imageParams]") {
    imageParams p;
    CHECK(allEqual(p.whitePoint,   4, 1.0f));
    CHECK(allEqual(p.g_whitepoint, 4, 1.0f));
}

TEST_CASE("imageParams default gain/mult are all ones", "[imageParams]") {
    imageParams p;
    CHECK(allEqual(p.g_gain,  4, 1.0f));
    CHECK(allEqual(p.g_mult,  4, 1.0f));
    CHECK(allEqual(p.g_gamma, 4, 1.0f));
}

TEST_CASE("imageParams default lift/offset are all zeros", "[imageParams]") {
    imageParams p;
    CHECK(allEqual(p.g_lift,   4, 0.0f));
    CHECK(allEqual(p.g_offset, 4, 0.0f));
}

TEST_CASE("imageParams default matrix is identity", "[imageParams]") {
    imageParams p;
    for (int r = 0; r < 3; ++r)
        for (int c = 0; c < 3; ++c)
            CHECK(p.g_matrix[r][c] == (r == c ? 1.0f : 0.0f));
}

TEST_CASE("imageParams default temp/tint/saturation are zero", "[imageParams]") {
    imageParams p;
    CHECK(p.temp       == 0.0f);
    CHECK(p.tint       == 0.0f);
    CHECK(p.saturation == 0.0f);
}

TEST_CASE("imageParams default crop covers the full image", "[imageParams]") {
    imageParams p;
    CHECK(p.imageCropMinX == 0.0f);
    CHECK(p.imageCropMinY == 0.0f);
    CHECK(p.imageCropMaxX == 1.0f);
    CHECK(p.imageCropMaxY == 1.0f);
    CHECK(p.cropEnable    == 0);
}

// ---------------------------------------------------------------------------
// Reset functions
// ---------------------------------------------------------------------------
TEST_CASE("rstBP resets black point to zero", "[imageParams]") {
    imageParams p;
    p.blackPoint[0] = 0.3f;
    p.blackPoint[2] = 0.7f;
    p.rstBP();
    CHECK(allEqual(p.blackPoint, 4, 0.0f));
}

TEST_CASE("rstWP resets white point to one", "[imageParams]") {
    imageParams p;
    p.whitePoint[1] = 0.5f;
    p.whitePoint[3] = 0.1f;
    p.rstWP();
    CHECK(allEqual(p.whitePoint, 4, 1.0f));
}

TEST_CASE("rstBC resets base colour to 0.5", "[imageParams]") {
    imageParams p;
    p.baseColor[0] = 0.9f;
    p.rstBC();
    CHECK(allEqual(p.baseColor, 3, 0.5f));
}

TEST_CASE("rstBLR resets blur amount to 10", "[imageParams]") {
    imageParams p;
    p.blurAmount = 99.0f;
    p.rstBLR();
    CHECK(p.blurAmount == 10.0f);
}

TEST_CASE("rstTmp resets temperature to zero", "[imageParams]") {
    imageParams p;
    p.temp = 3500.0f;
    p.rstTmp();
    CHECK(p.temp == 0.0f);
}

TEST_CASE("rstTnt resets tint to zero", "[imageParams]") {
    imageParams p;
    p.tint = -0.5f;
    p.rstTnt();
    CHECK(p.tint == 0.0f);
}

TEST_CASE("rstSat resets saturation to zero", "[imageParams]") {
    imageParams p;
    p.saturation = 1.0f;
    p.rstSat();
    CHECK(p.saturation == 0.0f);
}

TEST_CASE("rstANA resets BP, WP and min/max points", "[imageParams]") {
    imageParams p;
    p.blackPoint[0] = 0.2f;
    p.whitePoint[0] = 0.8f;
    p.minX = 0.1f; p.minY = 0.2f;
    p.maxX = 0.9f; p.maxY = 0.8f;
    p.rstANA();
    CHECK(allEqual(p.blackPoint, 4, 0.0f));
    CHECK(allEqual(p.whitePoint, 4, 1.0f));
    CHECK(p.minX == 0.0f);
    CHECK(p.minY == 0.0f);
    CHECK(p.maxX == 0.0f);
    CHECK(p.maxY == 0.0f);
}

TEST_CASE("rst_gGain resets grade gain to one", "[imageParams]") {
    imageParams p;
    for (int i = 0; i < 4; ++i) p.g_gain[i] = 2.5f;
    p.rst_gGain();
    CHECK(allEqual(p.g_gain, 4, 1.0f));
}

TEST_CASE("rst_gMul resets grade multiplier to one", "[imageParams]") {
    imageParams p;
    for (int i = 0; i < 4; ++i) p.g_mult[i] = 0.0f;
    p.rst_gMul();
    CHECK(allEqual(p.g_mult, 4, 1.0f));
}

TEST_CASE("rst_g_Gam resets grade gamma to one", "[imageParams]") {
    imageParams p;
    for (int i = 0; i < 4; ++i) p.g_gamma[i] = 0.5f;
    p.rst_g_Gam();
    CHECK(allEqual(p.g_gamma, 4, 1.0f));
}

TEST_CASE("rst_gLft resets lift to zero", "[imageParams]") {
    imageParams p;
    for (int i = 0; i < 4; ++i) p.g_lift[i] = 0.5f;
    p.rst_gLft();
    CHECK(allEqual(p.g_lift, 4, 0.0f));
}

TEST_CASE("rst_gOft resets offset to zero", "[imageParams]") {
    imageParams p;
    for (int i = 0; i < 4; ++i) p.g_offset[i] = 1.0f;
    p.rst_gOft();
    CHECK(allEqual(p.g_offset, 4, 0.0f));
}

TEST_CASE("rst_r_Matrix resets red matrix row to (1,0,0)", "[imageParams]") {
    imageParams p;
    p.g_matrix[0][0] = 0.0f;
    p.g_matrix[0][1] = 0.5f;
    p.g_matrix[0][2] = 0.5f;
    p.rst_r_Matrix();
    CHECK(p.g_matrix[0][0] == 1.0f);
    CHECK(p.g_matrix[0][1] == 0.0f);
    CHECK(p.g_matrix[0][2] == 0.0f);
}

TEST_CASE("rst_g_Matrix resets green matrix row to (0,1,0)", "[imageParams]") {
    imageParams p;
    p.g_matrix[1][0] = 0.5f;
    p.g_matrix[1][1] = 0.0f;
    p.g_matrix[1][2] = 0.5f;
    p.rst_g_Matrix();
    CHECK(p.g_matrix[1][0] == 0.0f);
    CHECK(p.g_matrix[1][1] == 1.0f);
    CHECK(p.g_matrix[1][2] == 0.0f);
}

TEST_CASE("rst_b_Matrix resets blue matrix row to (0,0,1)", "[imageParams]") {
    imageParams p;
    p.g_matrix[2][0] = 0.5f;
    p.g_matrix[2][1] = 0.5f;
    p.g_matrix[2][2] = 0.0f;
    p.rst_b_Matrix();
    CHECK(p.g_matrix[2][0] == 0.0f);
    CHECK(p.g_matrix[2][1] == 0.0f);
    CHECK(p.g_matrix[2][2] == 1.0f);
}

// ---------------------------------------------------------------------------
// Equality operator
// ---------------------------------------------------------------------------
TEST_CASE("Two default imageParams are equal", "[imageParams]") {
    imageParams a, b;
    CHECK(a == b);
    CHECK_FALSE(a != b);
}

TEST_CASE("Modifying a grading value makes params unequal", "[imageParams]") {
    imageParams a, b;
    b.g_gain[0] = 2.0f;
    CHECK(a != b);
    CHECK_FALSE(a == b);
}

TEST_CASE("Resetting a modified param restores equality", "[imageParams]") {
    imageParams a, b;
    b.g_gain[0] = 2.0f;
    REQUIRE(a != b);
    b.rst_gGain();
    CHECK(a == b);
}

TEST_CASE("Modifying non-grading fields does not affect equality", "[imageParams]") {
    // Fields like cropEnable, rotation, OCIO settings are intentionally
    // excluded from operator== (only grade params matter for re-render)
    imageParams a, b;
    b.cropEnable = 1;
    b.rotation   = 6;
    CHECK(a == b);  // still equal — these don't trigger a re-render
}
