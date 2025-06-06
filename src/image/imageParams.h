#ifndef _imageParams_h
#define _imageParams_h

#include <cstring>
#include <nlohmann/json.hpp>

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
    float g_blackpoint[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    float g_whitepoint[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    float g_lift[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    float g_gain[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    float g_mult[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    float g_offset[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    float g_gamma[4] = {1.0f, 1.0f, 1.0f, 1.0f};

    float cropBoxX[4];
    float cropBoxY[4];

    int rotation = 1;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(imageParams, sampleX, sampleY,
        blurAmount, baseColor, whitePoint, blackPoint, minX, minY,
        maxX, maxY, temp, tint, g_blackpoint, g_whitepoint, g_lift,
        g_gain, g_mult, g_offset, g_gamma, cropBoxX, cropBoxY, rotation);

    // == operator for determing if re-render is needed
    // So only parameters affecting image
    bool operator==(const imageParams& other) const {
        return  std::memcmp(baseColor, other.baseColor, sizeof(baseColor)) == 0 &&
                std::memcmp(whitePoint, other.whitePoint, sizeof(whitePoint)) == 0 &&
                std::memcmp(blackPoint, other.blackPoint, sizeof(blackPoint)) == 0 &&
                temp == other.temp &&
                tint == other.tint &&
                std::memcmp(g_blackpoint, other.g_blackpoint, sizeof(g_blackpoint)) == 0 &&
                std::memcmp(g_whitepoint, other.g_whitepoint, sizeof(g_whitepoint)) == 0 &&
                std::memcmp(g_lift, other.g_lift, sizeof(g_lift)) == 0 &&
                std::memcmp(g_gain, other.g_gain, sizeof(g_gain)) == 0 &&
                std::memcmp(g_mult, other.g_mult, sizeof(g_mult)) == 0 &&
                std::memcmp(g_offset, other.g_offset, sizeof(g_offset)) == 0 &&
                std::memcmp(g_gamma, other.g_gamma, sizeof(g_gamma)) == 0;
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

    void rst_gBP();
    void rst_gWP();
    void rst_gLft();
    void rst_gGain();
    void rst_gMul();
    void rst_gOft();
    void rst_g_Gam();
};


#endif
