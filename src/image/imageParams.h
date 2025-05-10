#ifndef _imageParams_h
#define _imageParams_h

struct imageParams {

    // Analysis
    unsigned int sampleX[2];
    unsigned int sampleY[2];

    float blurAmount = 2.5f;
    float baseColor[3] = {0.5f, 0.5f, 0.5f};
    float whitePoint[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    float blackPoint[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    unsigned int minX = 0;
    unsigned int minY = 0;
    unsigned int maxX = 0;
    unsigned int maxY = 0;

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

    unsigned int cropBoxX[4];
    unsigned int cropBoxY[4];

    void rstBP();
    void rstWP();
    void rstBC();

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
