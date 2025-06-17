#ifndef _structs_h_
#define _structs_h_

#include <string>
#include <array>

#define VERMAJOR 1
#define VERMINOR 0
#define VERPATCH 0

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

    bool rotation = false;
    bool imageCrop = false;

    void analysisGlobal(){
        if (baseColor && cropPoints &&
            analysisBlur && analysis)
            baseColor = cropPoints = analysisBlur = analysis = !analysis;
        else
            baseColor = cropPoints = analysisBlur = analysis = true;
    }

    void gradeGlobal(){
        if (temp && tint && bp && wp && lift &&
            gain && mult && offset && gamma)
            temp = tint = bp = wp = lift = gain = mult = offset = gamma = !gamma;
        else
            temp = tint = bp = wp = lift = gain = mult = offset = gamma = true;
    }

    void metaGlobal() {
        if (make && model && lens && stock && focal &&
            fstop && exposure && date && location &&
            gps && notes && dev && chem && devnote && scanner && scannotes)
                make = model = lens = stock = focal =
                fstop = exposure = date = location =
                gps = notes = dev = chem = devnote =
                scanner = scannotes = rotation = !devnote;
        else
            make = model = lens = stock = focal =
            fstop = exposure = date = location =
            gps = notes = dev = chem = devnote =
            scanner = scannotes = rotation = true;
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
    int colorspace = 0;
    int display = 0;
    int view = 0;
    bool inverse = false;
    bool useDisplay = true;
    bool ext = false;

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
                ext == other.ext &&
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

#endif
