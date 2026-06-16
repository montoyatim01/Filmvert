#ifndef _structs_h_
#define _structs_h_

#include <cstdint>
#include <string>
#include <array>

#ifdef _WIN32
#define M_PI 3.141592653
#endif

// ---------------------------------------------------------------------------
// Application version — edit these three lines and VERSTAGE only.
// All four are validated at compile time immediately below.
// ---------------------------------------------------------------------------
#define VERMAJOR 1
#define VERMINOR 2
#define VERPATCH 0

// Release stage for the current build.
// Must be exactly one of: ReleaseStage::Alpha, ReleaseStage::Beta,
//                         ReleaseStage::Release
enum class ReleaseStage : uint8_t {
    Alpha   = 0,
    Beta    = 1,
    Release = 2
};

#define VERSTAGE ReleaseStage::Release

// ---------------------------------------------------------------------------
// Compile-time validation
//   VERMAJOR / VERMINOR / VERPATCH — must be integers in [0, 65535]
//     If any is set to a string literal the >= comparison is an illegal
//     pointer-to-integer conversion and the compiler errors immediately.
//     If any is a float the range check fires the static_assert message.
//   VERSTAGE — enum comparison is type-safe; only the three ReleaseStage
//     values compile; anything else is a type error.
// ---------------------------------------------------------------------------
static_assert(VERMAJOR >= 0 && VERMAJOR <= 65535,
    "VERMAJOR must be an integer in [0, 65535]");
static_assert(VERMINOR >= 0 && VERMINOR <= 65535,
    "VERMINOR must be an integer in [0, 65535]");
static_assert(VERPATCH >= 0 && VERPATCH <= 65535,
    "VERPATCH must be an integer in [0, 65535]");
static_assert(
    VERSTAGE == ReleaseStage::Alpha   ||
    VERSTAGE == ReleaseStage::Beta    ||
    VERSTAGE == ReleaseStage::Release,
    "VERSTAGE must be ReleaseStage::Alpha, ReleaseStage::Beta, or ReleaseStage::Release");

struct copyPaste {

    bool fromLoad = false;

    //---Analysis
    bool baseColor = false;
    bool cropPoints = false;
    bool analysisBlur = false;
    bool analysis = false;

    //---Grade
    bool temp = false;
    bool tint = false;
    bool bp = false;
    bool wp = false;
    bool lift = false;
    bool gain = false;
    bool mult = false;
    bool offset = false;
    bool gamma = false;
    bool saturation = false;
    bool matrix = false;
    bool curves = false;

    //---Metadata
    bool make = false;
    bool model = false;
    bool lens = false;
    bool stock = false;
    bool focal = false;
    bool fstop = false;
    bool exposure = false;
    bool date = false;
    bool location = false;
    bool gps = false;
    bool notes = false;
    bool dev = false;
    bool chem = false;
    bool devnote = false;
    bool scanner = false;
    bool scannotes = false;
    bool frameNum = false;
    bool hash = false;

    bool rotation = false;
    bool imageCrop = false;

    bool ocio = false;

    void analysisGlobal(){
        if (baseColor && cropPoints &&
            analysisBlur && analysis)
            baseColor = cropPoints = analysisBlur = analysis = !analysis;
        else
            baseColor = cropPoints = analysisBlur = analysis = true;
    }

    void gradeGlobal(){
        if (temp && tint && bp && wp && lift &&
            gain && mult && offset && gamma && saturation && matrix)
            temp = tint = bp = wp = lift = gain = mult = offset = gamma = matrix = curves = saturation = !gamma;
        else
            temp = tint = bp = wp = lift = gain = mult = offset = gamma = matrix = curves = saturation = true;
    }

    void metaGlobal() {
        if (make && model && lens && stock && focal &&
            fstop && exposure && date && location &&
            gps && notes && dev && chem && devnote && scanner && scannotes)
                make = model = lens = stock = focal =
                fstop = exposure = date = location =
                gps = notes = dev = chem = devnote =
                scanner = scannotes = rotation = imageCrop = !devnote;
        else
            make = model = lens = stock = focal =
            fstop = exposure = date = location =
            gps = notes = dev = chem = devnote =
            scanner = scannotes = rotation = imageCrop = true;
    }

    void enableAll() {
        baseColor = cropPoints = analysisBlur = analysis = temp = tint =
            bp = wp = lift = gain = offset = gamma = saturation = matrix =
            curves = make = model = lens = stock = focal = fstop = exposure =
            date = location = gps = notes = dev = chem = devnote = scanner =
            scannotes = frameNum = hash = rotation = imageCrop = ocio = true;
    }
};


struct exportParam {
  std::string outPath;
  int format = 4;
  int bitDepth = 1;
  int quality = 85;
  int compression = 8;
  bool overwrite = false;
  int colorspaceOpt = 1;
  bool bakeRotation = true;
  bool border = false;
  float borderSize = 0.0f;
  float borderColor[3] = {0.0f, 0.0f, 0.0f};
  int csBakeRot = 1;
  bool greyscale = false;

  bool resize = false;
  bool fixedSize = false;
  bool longSide = true;

  uint32_t fixedSizePx = 3000;
  float scaleSize = 100.0f;
};

struct minMaxPoint {
    unsigned int minX = 0;
    unsigned int minY = 0;
    float minLuma = 1.0f;
    float minRGB[3] = {1.0f, 1.0f, 1.0f};

    unsigned int maxX = 0;
    unsigned int maxY = 0;
    float maxLuma = 0.0f;
    float maxRGB[3] = {0.0f, 0.0f, 0.0f};
};

struct rawSetting {
    int width = 3000;
    int height = 2000;
    int channels = 3;
    int bitDepth = 16;
    bool littleE = true;
    bool pakonHeader = false;
    bool planar = true;
};

struct ocioSetting {
    int ocioConfig = 0;
    int colorspace = 0;
    int display = 0;
    int view = 0;
    bool inverse = false;
    bool useDisplay = true;
    bool gamutComp = false;
    bool impOverwrite = false;

    int texCount = 0;
    const float* texture[3];

    unsigned int texWidth[3];
    unsigned int texHeight[3];

    bool operator==(const ocioSetting& other) const {
        return colorspace == other.colorspace &&
                display == other.display &&
                 view == other.view &&
                inverse == other.inverse &&
                useDisplay == other.useDisplay &&
                gamutComp == other.gamutComp &&
                texCount == other.texCount &&
                texture[0] == other.texture[0] &&
                texWidth[0] == other.texWidth[0] &&
                texHeight[0] == other.texHeight[0];
    }


    bool operator!=(const ocioSetting& other) const {
        return !(*this == other);
    }
};


struct HistogramData {
    std::array<int, 512> r_hist;
    std::array<int, 512> g_hist;
    std::array<int, 512> b_hist;
    std::array<int, 512> luminance_hist;

    HistogramData() {
        r_hist.fill(0);
        g_hist.fill(0);
        b_hist.fill(0);
        luminance_hist.fill(0);
    }
};

enum closeMode {
    c_app = 0,
    c_roll = 1,
    c_selIm = 2
};

enum gradeSection {
    grade_wb = 1 << 0,
    grade_tone = 1 << 1,
    grade_curves = 1 << 2,
    grade_matrix = 1 << 3
};

#endif
