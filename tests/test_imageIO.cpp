#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <OpenImageIO/imageio.h>
#include <filesystem>
#include <vector>
#include "image.h"

using Catch::Matchers::WithinAbs;
namespace fs = std::filesystem;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

// Build an image with a pre-allocated rawImgData buffer.
// Always sized for 4 channels — this matches how readImageOIIO allocates
// rawImgData, and is required because padToRGBA's final memcpy writes
// rawWidth * rawHeight * 4 floats back into rawImgData regardless of nChannels.
static image makeRawImage(unsigned int w, unsigned int h, unsigned int ch) {
    image img;
    img.width     = w;
    img.height    = h;
    img.rawWidth  = w;
    img.rawHeight = h;
    img.nChannels = ch;
    img.rndrW     = w;
    img.rndrH     = h;
    img.fullIm    = false;
    img.rawImgData = new float[w * h * 4](); // always 4ch, zero-initialised
    return img;
}

// RAII temp file — cross-platform, uses std::filesystem throughout.
// A process-local counter makes each name unique within a test run.
struct TempFile {
    fs::path path;
    explicit TempFile(const std::string& ext) {
        static int seq = 0;
        std::error_code ec;
        path = fs::temp_directory_path(ec) /
               ("fvc_test_" + std::to_string(++seq) + ext);
    }
    ~TempFile() {
        std::error_code ec;
        if (fs::exists(path, ec)) fs::remove(path, ec);
    }
    TempFile(const TempFile&)            = delete;
    TempFile& operator=(const TempFile&) = delete;
};

// ---------------------------------------------------------------------------
// padToRGBA
// ---------------------------------------------------------------------------
TEST_CASE("padToRGBA expands RGB to RGBA with alpha 1.0", "[imageIO]") {
    const unsigned int W = 4, H = 4;
    image img = makeRawImage(W, H, 3);

    // Fill source as tightly-packed RGB (3 floats per pixel)
    for (unsigned int i = 0; i < W * H; ++i) {
        img.rawImgData[3 * i + 0] = 0.25f;
        img.rawImgData[3 * i + 1] = 0.50f;
        img.rawImgData[3 * i + 2] = 0.75f;
    }

    img.padToRGBA();

    // padToRGBA allocates procImgData internally and frees it before returning
    REQUIRE(img.procImgData == nullptr);

    // rawImgData is now RGBA — 4 floats per pixel
    for (unsigned int i = 0; i < W * H; ++i) {
        CHECK_THAT(img.rawImgData[4 * i + 0], WithinAbs(0.25f, 1e-6f));
        CHECK_THAT(img.rawImgData[4 * i + 1], WithinAbs(0.50f, 1e-6f));
        CHECK_THAT(img.rawImgData[4 * i + 2], WithinAbs(0.75f, 1e-6f));
        CHECK_THAT(img.rawImgData[4 * i + 3], WithinAbs(1.0f,  1e-6f));
    }

    delete[] img.rawImgData;
    img.rawImgData = nullptr;
}

TEST_CASE("padToRGBA replicates greyscale into all RGB channels", "[imageIO]") {
    const unsigned int W = 4, H = 4;
    image img = makeRawImage(W, H, 1);

    for (unsigned int i = 0; i < W * H; ++i)
        img.rawImgData[i] = 0.6f; // 1 float per pixel

    img.padToRGBA();

    REQUIRE(img.procImgData == nullptr);

    for (unsigned int i = 0; i < W * H; ++i) {
        CHECK_THAT(img.rawImgData[4 * i + 0], WithinAbs(0.6f, 1e-6f)); // R
        CHECK_THAT(img.rawImgData[4 * i + 1], WithinAbs(0.6f, 1e-6f)); // G replicated
        CHECK_THAT(img.rawImgData[4 * i + 2], WithinAbs(0.6f, 1e-6f)); // B replicated
        CHECK_THAT(img.rawImgData[4 * i + 3], WithinAbs(1.0f, 1e-6f)); // A
    }

    delete[] img.rawImgData;
    img.rawImgData = nullptr;
}

TEST_CASE("padToRGBA is a no-op for 4-channel images", "[imageIO]") {
    const unsigned int W = 4, H = 4;
    image img = makeRawImage(W, H, 4);

    for (unsigned int i = 0; i < W * H; ++i) {
        img.rawImgData[4 * i + 0] = 0.1f;
        img.rawImgData[4 * i + 1] = 0.2f;
        img.rawImgData[4 * i + 2] = 0.3f;
        img.rawImgData[4 * i + 3] = 0.8f;
    }

    float* original = img.rawImgData;
    img.padToRGBA(); // should return immediately — nChannels == 4

    CHECK(img.rawImgData == original);
    CHECK_THAT(img.rawImgData[3], WithinAbs(0.8f, 1e-6f)); // alpha unchanged

    delete[] img.rawImgData;
    img.rawImgData = nullptr;
}

// ---------------------------------------------------------------------------
// trimForSave
// ---------------------------------------------------------------------------
TEST_CASE("trimForSave strips alpha and writes correct RGB values", "[imageIO]") {
    const unsigned int W = 4, H = 4;
    image img;
    img.nChannels = 3;
    img.rndrW     = W;
    img.rndrH     = H;

    img.procImgData = new float[W * H * 4];
    for (unsigned int i = 0; i < W * H; ++i) {
        img.procImgData[4 * i + 0] = 0.1f;
        img.procImgData[4 * i + 1] = 0.2f;
        img.procImgData[4 * i + 2] = 0.3f;
        img.procImgData[4 * i + 3] = 1.0f; // alpha — must not appear in output
    }

    img.tmpOutData = new float[W * H * 3]();

    img.trimForSave();

    for (unsigned int i = 0; i < W * H; ++i) {
        CHECK_THAT(img.tmpOutData[3 * i + 0], WithinAbs(0.1f, 1e-6f));
        CHECK_THAT(img.tmpOutData[3 * i + 1], WithinAbs(0.2f, 1e-6f));
        CHECK_THAT(img.tmpOutData[3 * i + 2], WithinAbs(0.3f, 1e-6f));
    }

    delete[] img.procImgData; img.procImgData = nullptr;
    delete[] img.tmpOutData;  img.tmpOutData  = nullptr;
}

TEST_CASE("trimForSave is a no-op for 4-channel images", "[imageIO]") {
    const unsigned int W = 2, H = 2;
    image img;
    img.nChannels = 4;
    img.rndrW     = W;
    img.rndrH     = H;

    img.procImgData = new float[W * H * 4]();
    img.tmpOutData  = new float[W * H * 4]();
    float* sentinel = img.tmpOutData;

    img.trimForSave(); // should return immediately — nChannels == 4

    // tmpOutData still zero (nothing written)
    for (unsigned int i = 0; i < W * H * 4; ++i)
        CHECK(sentinel[i] == 0.0f);

    delete[] img.procImgData; img.procImgData = nullptr;
    delete[] img.tmpOutData;  img.tmpOutData  = nullptr;
}

// ---------------------------------------------------------------------------
// padToRGBA → trimForSave round-trip
// ---------------------------------------------------------------------------
TEST_CASE("padToRGBA then trimForSave round-trips RGB pixel values", "[imageIO]") {
    const unsigned int W = 8, H = 8;
    image img = makeRawImage(W, H, 3);

    // Fill source RGB
    for (unsigned int i = 0; i < W * H; ++i) {
        img.rawImgData[3 * i + 0] = static_cast<float>(i) / (W * H);
        img.rawImgData[3 * i + 1] = static_cast<float>(i) / (W * H) * 0.5f;
        img.rawImgData[3 * i + 2] = 1.0f - static_cast<float>(i) / (W * H);
    }

    // Save original values before padToRGBA modifies rawImgData
    std::vector<float> origR(W * H), origG(W * H), origB(W * H);
    for (unsigned int i = 0; i < W * H; ++i) {
        origR[i] = img.rawImgData[3 * i + 0];
        origG[i] = img.rawImgData[3 * i + 1];
        origB[i] = img.rawImgData[3 * i + 2];
    }

    img.padToRGBA(); // rawImgData now RGBA

    // Set up for trimForSave: copy RGBA into procImgData
    img.rndrW = W;
    img.rndrH = H;
    img.procImgData = new float[W * H * 4];
    std::memcpy(img.procImgData, img.rawImgData, W * H * 4 * sizeof(float));
    img.tmpOutData = new float[W * H * 3]();

    img.trimForSave();

    for (unsigned int i = 0; i < W * H; ++i) {
        CHECK_THAT(img.tmpOutData[3 * i + 0], WithinAbs(origR[i], 1e-6f));
        CHECK_THAT(img.tmpOutData[3 * i + 1], WithinAbs(origG[i], 1e-6f));
        CHECK_THAT(img.tmpOutData[3 * i + 2], WithinAbs(origB[i], 1e-6f));
    }

    delete[] img.rawImgData;  img.rawImgData  = nullptr;
    delete[] img.procImgData; img.procImgData = nullptr;
    delete[] img.tmpOutData;  img.tmpOutData  = nullptr;
}

// ---------------------------------------------------------------------------
// OIIO round-trip — verifies the library works correctly in the test environment
// ---------------------------------------------------------------------------
TEST_CASE("OIIO EXR round-trip preserves float pixel values", "[imageIO]") {
    TempFile tmp(".exr");
    const int W = 8, H = 8, CH = 3;

    std::vector<float> pixels(W * H * CH);
    for (int i = 0; i < W * H * CH; ++i)
        pixels[i] = static_cast<float>(i) / (W * H * CH);

    {
        OIIO::ImageSpec spec(W, H, CH, OIIO::TypeDesc::FLOAT);
        auto out = OIIO::ImageOutput::create(tmp.path.string());
        REQUIRE(out != nullptr);
        REQUIRE(out->open(tmp.path.string(), spec));
        REQUIRE(out->write_image(OIIO::TypeDesc::FLOAT, pixels.data()));
        out->close();
    }

    REQUIRE(fs::exists(tmp.path));

    std::vector<float> readback(W * H * CH);
    {
        auto in = OIIO::ImageInput::open(tmp.path.string());
        REQUIRE(in != nullptr);
        const OIIO::ImageSpec& spec = in->spec();
        CHECK(spec.width     == W);
        CHECK(spec.height    == H);
        CHECK(spec.nchannels == CH);
        REQUIRE(in->read_image(0, 0, 0, CH, OIIO::TypeDesc::FLOAT, readback.data()));
        in->close();
    }

    for (int i = 0; i < W * H * CH; ++i)
        CHECK_THAT(readback[i], WithinAbs(pixels[i], 1e-5f));
}

TEST_CASE("OIIO TIFF round-trip preserves uint16 pixel values", "[imageIO]") {
    TempFile tmp(".tiff");
    const int W = 8, H = 8, CH = 3;

    std::vector<uint16_t> pixels(W * H * CH);
    for (int i = 0; i < W * H * CH; ++i)
        pixels[i] = static_cast<uint16_t>(i * 17);

    {
        OIIO::ImageSpec spec(W, H, CH, OIIO::TypeDesc::UINT16);
        auto out = OIIO::ImageOutput::create(tmp.path.string());
        REQUIRE(out != nullptr);
        REQUIRE(out->open(tmp.path.string(), spec));
        REQUIRE(out->write_image(OIIO::TypeDesc::UINT16, pixels.data()));
        out->close();
    }

    REQUIRE(fs::exists(tmp.path));

    std::vector<uint16_t> readback(W * H * CH);
    {
        auto in = OIIO::ImageInput::open(tmp.path.string());
        REQUIRE(in != nullptr);
        CHECK(in->spec().width     == W);
        CHECK(in->spec().height    == H);
        CHECK(in->spec().nchannels == CH);
        REQUIRE(in->read_image(0, 0, 0, CH, OIIO::TypeDesc::UINT16, readback.data()));
        in->close();
    }

    for (int i = 0; i < W * H * CH; ++i)
        CHECK(readback[i] == pixels[i]);
}

TEST_CASE("OIIO reports correct dimensions for a written PNG", "[imageIO]") {
    TempFile tmp(".png");
    const int W = 16, H = 12, CH = 3;

    std::vector<uint8_t> pixels(W * H * CH, 128);
    {
        OIIO::ImageSpec spec(W, H, CH, OIIO::TypeDesc::UINT8);
        auto out = OIIO::ImageOutput::create(tmp.path.string());
        REQUIRE(out != nullptr);
        REQUIRE(out->open(tmp.path.string(), spec));
        REQUIRE(out->write_image(OIIO::TypeDesc::UINT8, pixels.data()));
        out->close();
    }

    auto in = OIIO::ImageInput::open(tmp.path.string());
    REQUIRE(in != nullptr);
    CHECK(in->spec().width     == W);
    CHECK(in->spec().height    == H);
    CHECK(in->spec().nchannels == CH);
    in->close();
}
