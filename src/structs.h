#ifndef _structs_h_
#define _structs_h_

#include <string>
#include <array>

struct copyPaste {

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
            gps && notes && dev && chem && devnote)
                make = model = lens = stock = focal =
                fstop = exposure = date = location =
                gps = notes = dev = chem = devnote = !devnote;
        else
            make = model = lens = stock = focal =
            fstop = exposure = date = location =
            gps = notes = dev = chem = devnote = true;
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
    const float* texture;
    unsigned int texWidth = 0;
    unsigned int texHeight = 0;

    bool operator==(const ocioSetting& other) const {
        return colorspace == other.colorspace &&
                display == other.display &&
                 view == other.view &&
                inverse == other.inverse &&
                useDisplay == other.useDisplay &&
                ext == other.ext &&
                texCount == other.texCount &&
                texture == other.texture &&
                texWidth == other.texWidth &&
                texHeight == other.texHeight;
    }


    bool operator!=(const ocioSetting& other) const {
        return !(*this == other);
    }
};


struct HistogramData {
    std::array<int, 256> r_hist;
    std::array<int, 256> g_hist;
    std::array<int, 256> b_hist;
    std::array<int, 256> luminance_hist;

    HistogramData() {
        r_hist.fill(0);
        g_hist.fill(0);
        b_hist.fill(0);
        luminance_hist.fill(0);
    }
};

#endif
