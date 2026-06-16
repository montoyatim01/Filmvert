#include <catch2/catch_test_macros.hpp>
#include "image.h"

// ---------------------------------------------------------------------------
// Helpers — build a minimal image struct for a given scenario
// ---------------------------------------------------------------------------
static image makeImage(unsigned int w, unsigned int h, unsigned int ch,
                       unsigned int rw = 0, unsigned int rh = 0) {
    image img;
    img.width     = w;
    img.height    = h;
    img.nChannels = ch;
    img.rawWidth  = rw > 0 ? rw : w;
    img.rawHeight = rh > 0 ? rh : h;
    img.rndrW     = w;
    img.rndrH     = h;
    img.fullIm    = false;
    return img;
}

// ---------------------------------------------------------------------------
// allocProcBuf / delProcBuf
// ---------------------------------------------------------------------------
TEST_CASE("allocProcBuf allocates a non-null buffer", "[imageBuffers]") {
    image img = makeImage(100, 100, 3);
    img.allocProcBuf();
    REQUIRE(img.procImgData != nullptr);
    img.delProcBuf();
}

TEST_CASE("allocProcBuf does not re-allocate if buffer already exists", "[imageBuffers]") {
    image img = makeImage(100, 100, 3);
    img.allocProcBuf();
    float* first = img.procImgData;
    img.allocProcBuf();  // second call — must be a no-op
    CHECK(img.procImgData == first);
    img.delProcBuf();
}

TEST_CASE("delProcBuf sets pointer to null", "[imageBuffers]") {
    image img = makeImage(50, 50, 3);
    img.allocProcBuf();
    img.delProcBuf();
    CHECK(img.procImgData == nullptr);
}

TEST_CASE("delProcBuf is safe to call on a null buffer", "[imageBuffers]") {
    image img = makeImage(50, 50, 3);
    REQUIRE(img.procImgData == nullptr);
    REQUIRE_NOTHROW(img.delProcBuf());
}

// ---------------------------------------------------------------------------
// allocateTmpBuf / clearTmpBuf
// ---------------------------------------------------------------------------
TEST_CASE("allocateTmpBuf allocates a non-null buffer for RGB image", "[imageBuffers]") {
    image img = makeImage(200, 100, 3);
    img.allocateTmpBuf();
    REQUIRE(img.tmpOutData != nullptr);
    img.clearTmpBuf();
}

TEST_CASE("allocateTmpBuf does not re-allocate if buffer already exists", "[imageBuffers]") {
    image img = makeImage(200, 100, 3);
    img.allocateTmpBuf();
    float* first = img.tmpOutData;
    img.allocateTmpBuf();
    CHECK(img.tmpOutData == first);
    img.clearTmpBuf();
}

TEST_CASE("clearTmpBuf sets pointer to null and frees memory", "[imageBuffers]") {
    image img = makeImage(200, 100, 3);
    img.allocateTmpBuf();
    img.clearTmpBuf();
    CHECK(img.tmpOutData == nullptr);
}

TEST_CASE("clearTmpBuf is safe when buffer is null", "[imageBuffers]") {
    image img = makeImage(200, 100, 3);
    REQUIRE(img.tmpOutData == nullptr);
    REQUIRE_NOTHROW(img.clearTmpBuf());
}

// ---------------------------------------------------------------------------
// allocateTmpBuf — aliasing case (nChannels==4 && fullIm)
// This covers the double-free bug fixed in clearBuffers().
// ---------------------------------------------------------------------------
TEST_CASE("allocateTmpBuf aliases procImgData when nChannels==4 and fullIm", "[imageBuffers]") {
    image img = makeImage(100, 100, 4);
    img.fullIm = true;
    img.allocProcBuf();
    img.allocateTmpBuf();

    // tmpOutData must point to the same block as procImgData
    CHECK(img.tmpOutData == img.procImgData);

    // clearTmpBuf must NOT free the shared block, but it should null the pointer
    img.clearTmpBuf();
    CHECK(img.tmpOutData  == nullptr);  // pointer cleared
    CHECK(img.procImgData != nullptr);  // underlying allocation still valid
    img.delProcBuf();
}

TEST_CASE("clearBuffers does not double-free when tmpOutData aliases procImgData", "[imageBuffers]") {
    image img = makeImage(100, 100, 4);
    img.fullIm    = true;
    img.rawWidth  = 100;
    img.rawHeight = 100;
    img.rawImgData = new float[100 * 100 * 4];

    img.allocProcBuf();
    img.allocateTmpBuf();

    REQUIRE(img.tmpOutData == img.procImgData);

    // Must not crash (double-free would be UB / SIGABRT with ASan)
    REQUIRE_NOTHROW(img.clearBuffers());

    CHECK(img.procImgData == nullptr);
    CHECK(img.tmpOutData  == nullptr);
    CHECK(img.rawImgData  == nullptr);
}

// ---------------------------------------------------------------------------
// allocateTmpBuf — buffer is large enough for rndrW x rndrH
// This covers the size-mismatch bug where a crop made rndrW*rndrH
// larger than rawWidth*rawHeight, causing trimForSave() to go OOB.
// ---------------------------------------------------------------------------
TEST_CASE("allocateTmpBuf is sized by rndrW*rndrH when it is the largest dimension", "[imageBuffers]") {
    // Simulate a crop that produces a render area larger than the stored raw dims
    image img;
    img.nChannels = 3;
    img.fullIm    = false;
    img.width     = 100; img.height    = 100;
    img.rawWidth  = 100; img.rawHeight = 100;
    img.rndrW     = 200; img.rndrH     = 200;  // crop output upscaled

    img.allocateTmpBuf();
    REQUIRE(img.tmpOutData != nullptr);

    // Write to every float the buffer should cover — ASan will catch any OOB
    float* buf = img.tmpOutData;
    size_t count = (size_t)img.rndrW * img.rndrH * 4;
    for (size_t i = 0; i < count; ++i)
        buf[i] = static_cast<float>(i);

    img.clearTmpBuf();
}

// ---------------------------------------------------------------------------
// allocDispBuf / delDispBuf
// ---------------------------------------------------------------------------
TEST_CASE("allocDispBuf allocates a non-null uint8 buffer", "[imageBuffers]") {
    image img = makeImage(64, 64, 3);
    img.allocDispBuf();
    REQUIRE(img.dispImgData != nullptr);
    img.delDispBuf();
}

TEST_CASE("allocDispBuf does not re-allocate if already allocated", "[imageBuffers]") {
    image img = makeImage(64, 64, 3);
    img.allocDispBuf();
    uint8_t* first = img.dispImgData;
    img.allocDispBuf();
    CHECK(img.dispImgData == first);
    img.delDispBuf();
}

TEST_CASE("delDispBuf sets pointer to null", "[imageBuffers]") {
    image img = makeImage(64, 64, 3);
    img.allocDispBuf();
    img.delDispBuf();
    CHECK(img.dispImgData == nullptr);
}

// ---------------------------------------------------------------------------
// allocBlurBuf / delBlurBuf
// ---------------------------------------------------------------------------
TEST_CASE("allocBlurBuf allocates when null", "[imageBuffers]") {
    image img = makeImage(50, 50, 3);
    img.allocBlurBuf();
    REQUIRE(img.blurImgData != nullptr);
    img.delBlurBuf();
}

TEST_CASE("delBlurBuf clears pointer and resets blurReady", "[imageBuffers]") {
    image img = makeImage(50, 50, 3);
    img.blurReady = true;
    img.allocBlurBuf();
    img.delBlurBuf();
    CHECK(img.blurImgData == nullptr);
    CHECK(img.blurReady   == false);
}

// ---------------------------------------------------------------------------
// clearBuffers — comprehensive teardown
// ---------------------------------------------------------------------------
TEST_CASE("clearBuffers nulls all pointers for a normal RGB image", "[imageBuffers]") {
    image img = makeImage(80, 60, 3);
    img.rawImgData = new float[(size_t)80 * 60 * 3];

    img.allocProcBuf();
    img.allocateTmpBuf();
    img.allocDispBuf();
    img.allocBlurBuf();

    img.clearBuffers();

    CHECK(img.rawImgData  == nullptr);
    CHECK(img.procImgData == nullptr);
    CHECK(img.tmpOutData  == nullptr);
    CHECK(img.dispImgData == nullptr);
    CHECK(img.blurImgData == nullptr);
    CHECK(img.imageLoaded == false);
}

TEST_CASE("clearBuffers returns early when rawImgData is null", "[imageBuffers]") {
    // procImgData allocated, rawImgData null — clearBuffers should bail
    image img = makeImage(80, 60, 3);
    img.allocProcBuf();
    REQUIRE(img.rawImgData  == nullptr);
    REQUIRE(img.procImgData != nullptr);

    img.clearBuffers();  // should not crash; procImgData will be leaked (known limitation)
    // The early-return means procImgData is NOT freed here; just verify no crash.
    // Clean up manually to avoid leak in the test process.
    img.delProcBuf();
}
