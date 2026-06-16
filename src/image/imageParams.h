#ifndef _imageParams_h
#define _imageParams_h

#include <algorithm>
#include <cmath>
#include <cstring>
#include <vector>
#include <nlohmann/json.hpp>
#include "renderParams.h"

struct imageCurve {
    std::vector<float> px = {0.0f, 0.25f, 0.5f, 0.75f, 1.0f};
    std::vector<float> py = {0.0f, 0.25f, 0.5f, 0.75f, 1.0f};

    int n() const { return static_cast<int>(px.size()); }

    bool isIdentity() const {
        if (n() != 5) return false;
        const float ref[5] = {0.0f, 0.25f, 0.5f, 0.75f, 1.0f};
        for (int i = 0; i < 5; i++)
            if (px[i] != ref[i] || py[i] != ref[i]) return false;
        return true;
    }

    void reset() {
        px = {0.0f, 0.25f, 0.5f, 0.75f, 1.0f};
        py = {0.0f, 0.25f, 0.5f, 0.75f, 1.0f};
    }

    bool operator==(const imageCurve& o) const { return px == o.px && py == o.py; }
    bool operator!=(const imageCurve& o) const { return !(*this == o); }


    NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(imageCurve, px, py);
};

struct cmykParams {
    float cmy_A_whitePoint[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    float cmy_A_blackPoint[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    float cmy_blackpoint[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    float cmy_whitepoint[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    float cmy_lift[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    float cmy_gain[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    float cmy_mult[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    float cmy_offset[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    float cmy_gamma[4] = {1.0f, 1.0f, 1.0f, 1.0f};
};

struct imageParams {

    // Analysis
    float sampleX[2];
    float sampleY[2];

    float blurAmount = 10.0f;
    float baseColor[3] = {0.5f, 0.5f, 0.5f};
    float whitePoint[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    float blackPoint[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    float minX = 0;
    float minY = 0;
    float maxX = 0;
    float maxY = 0;

    // Correction
    float temp = 0.0f;
    float tint = 0.0f;
    float saturation = 0.0f;
    float g_blackpoint[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    float g_whitepoint[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    float g_lift[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    float g_gain[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    float g_mult[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    float g_offset[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    float g_gamma[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    float g_matrix[3][3] = {{1.0f, 0.0f, 0.0f},
                        {0.0f, 1.0f, 0.0f},
                        {0.0f, 0.0f, 1.0f}};
    float g_sharpen = 0.0f;
    float g_sharpenRadius = 1.0f;
    // Tone curves — [0]=Master/W, [1]=R, [2]=G, [3]=B
    imageCurve curves[4];

    cmykParams cmykParam;

    // This is for the analysis selection region
    float cropBoxX[4];
    float cropBoxY[4];

    // This is for actually cropping the image
    float imageCropMinX = 0.0f;
    float imageCropMinY = 0.0f;
    float imageCropMaxX = 1.0f;
    float imageCropMaxY = 1.0f;
    float imageCropAspect = 1.5f;
    bool lockAspect = false;
    int cropEnable = 0;
    int cropVisible = 0;

    // This is for an arbitrary rotation
    float arbitraryRotation = 0.0f;

    // This is the EXIF rotation value
    int rotation = 1;
    int writeRotation = 1;

    // OCIO Config Info (For replicating IDT processing)
    std::string ocioName;
    int ocioColor = -1;
    int ocioDisp = -1;
    int ocioView = -1;
    bool useDisplay = true;
    bool inverse = false;
    bool gamutComp = false;


    NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(imageParams, sampleX, sampleY,
        blurAmount, baseColor, whitePoint, blackPoint, minX, minY,
        maxX, maxY, temp, tint, saturation, g_blackpoint, g_whitepoint, g_lift,
        g_gain, g_mult, g_offset, g_gamma, g_matrix, g_sharpen, g_sharpenRadius,
        curves, cropBoxX, cropBoxY, imageCropMinX, imageCropMinY, imageCropMaxX,
        imageCropMaxY, imageCropAspect, lockAspect, cropEnable, arbitraryRotation, rotation,
        ocioName, ocioColor, ocioDisp, ocioView, useDisplay, inverse, gamutComp);

    // == operator for determing if re-render is needed
    // So only parameters affecting image
    bool operator==(const imageParams& other) const {
        return  std::memcmp(baseColor, other.baseColor, sizeof(baseColor)) == 0 &&
                std::memcmp(whitePoint, other.whitePoint, sizeof(whitePoint)) == 0 &&
                std::memcmp(blackPoint, other.blackPoint, sizeof(blackPoint)) == 0 &&
                std::abs(temp - other.temp) < 0.001f &&
                std::abs(tint - other.tint) < 0.001f &&
                std::abs(saturation - other.saturation) < 0.001f &&
                std::abs(g_sharpen - other.g_sharpen) < 0.001f &&
                std::abs(g_sharpenRadius - other.g_sharpenRadius) < 0.001f &&
                std::abs(imageCropAspect - other.imageCropAspect) < 0.001f &&
                std::abs(imageCropMinX - other.imageCropMinX) < 0.001f &&
                std::abs(imageCropMinY - other.imageCropMinY) < 0.001f &&
                std::abs(imageCropMaxX - other.imageCropMaxX) < 0.001f &&
                std::abs(imageCropMaxY - other.imageCropMaxY) < 0.001f &&
                std::abs(arbitraryRotation - other.arbitraryRotation) < 0.001f &&
                std::memcmp(g_blackpoint, other.g_blackpoint, sizeof(g_blackpoint)) == 0 &&
                std::memcmp(g_whitepoint, other.g_whitepoint, sizeof(g_whitepoint)) == 0 &&
                std::memcmp(g_lift, other.g_lift, sizeof(g_lift)) == 0 &&
                std::memcmp(g_gain, other.g_gain, sizeof(g_gain)) == 0 &&
                std::memcmp(g_mult, other.g_mult, sizeof(g_mult)) == 0 &&
                std::memcmp(g_offset, other.g_offset, sizeof(g_offset)) == 0 &&
                std::memcmp(g_gamma, other.g_gamma, sizeof(g_gamma)) == 0 &&
                std::memcmp(g_matrix, other.g_matrix, sizeof(g_matrix)) == 0 &&
                std::equal(std::begin(curves), std::end(curves), std::begin(other.curves));
    }

    bool operator!=(const imageParams& other) const {
        return !(*this == other);
    }

    void rstANA();

    void rstBP();
    void rstWP();
    void rstBC();
    void rstBLR();

    void rstTmp();
    void rstTnt();
    void rstSat();

    void rst_gBP();
    void rst_gWP();
    void rst_gLft();
    void rst_gGain();
    void rst_gMul();
    void rst_gOft();
    void rst_g_Gam();
    void rst_r_Matrix();
    void rst_g_Matrix();
    void rst_b_Matrix();
    void rst_gSharp();
    void rst_gSharpRad();
    void rst_curves();
    void rst_curve(int ch);

    void cmyk_to_rgb();
    void rgb_to_cmyk();
};




#endif
