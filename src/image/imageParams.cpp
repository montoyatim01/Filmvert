#include "imageParams.h"
#include <algorithm>

//--- Reset functions ---//
/*
    Quick functions to reset the parameters to
    their default values
*/
void imageParams::rstANA() {
    rstBP();
    rstWP();
    minX = 0.0f;
    minY = 0.0f;
    maxX = 0.0f;
    maxY = 0.0f;
}

void imageParams::rstBLR() {
    blurAmount = 10.0f;
}
void imageParams::rstBP() {
    std::fill(std::begin(blackPoint), std::end(blackPoint), 0.0f);
}
void imageParams::rstWP() {
    std::fill(std::begin(whitePoint), std::end(whitePoint), 1.0f);
}
void imageParams::rstBC() {
    std::fill(std::begin(baseColor), std::end(baseColor), 0.5f);
}

void imageParams::rstTmp() {
    temp = 0.0f;
}
void imageParams::rstTnt() {
    tint = 0.0f;
}
void imageParams::rstSat() {
    saturation = 0.0f;
}

void imageParams::rst_gBP() {
    std::fill(std::begin(g_blackpoint), std::end(g_blackpoint), 0.0f);
}
void imageParams::rst_gWP() {
    std::fill(std::begin(g_whitepoint), std::end(g_whitepoint), 1.0f);
}
void imageParams::rst_gLft() {
    std::fill(std::begin(g_lift), std::end(g_lift), 0.0f);
}
void imageParams::rst_gGain() {
    std::fill(std::begin(g_gain), std::end(g_gain), 1.0f);
}
void imageParams::rst_gMul() {
    std::fill(std::begin(g_mult), std::end(g_mult), 1.0f);
}
void imageParams::rst_gOft() {
    std::fill(std::begin(g_offset), std::end(g_offset), 0.0f);
}
void imageParams::rst_g_Gam() {
    std::fill(std::begin(g_gamma), std::end(g_gamma), 1.0f);
}
void imageParams::rst_r_Matrix() {
    g_matrix[0][0] = 1.0f;
    g_matrix[0][1] = 0.0f;
    g_matrix[0][2] = 0.0f;
}
void imageParams::rst_g_Matrix() {
    g_matrix[1][0] = 0.0f;
    g_matrix[1][1] = 1.0f;
    g_matrix[1][2] = 0.0f;
}
void imageParams::rst_b_Matrix() {
    g_matrix[2][0] = 0.0f;
    g_matrix[2][1] = 0.0f;
    g_matrix[2][2] = 1.0f;
}

void imageParams::rst_gSharp() {
    g_sharpen = 0.0f;
}

void imageParams::rst_gSharpRad() {
    g_sharpenRadius = 1.0f;
}

void imageParams::rst_curves() {
    for (int ch = 0; ch < 4; ch++)
        curves[ch].reset();
}

void imageParams::rst_curve(int ch) {
    if (ch >= 0 && ch < 4)
        curves[ch].reset();
}

void imageParams::cmyk_to_rgb() {
    for (int i = 0; i < 3; i ++) {
        blackPoint[i] = 1.0f - (cmykParam.cmy_A_blackPoint[i] + 1.0f);
        whitePoint[i] = (1.0f - cmykParam.cmy_A_whitePoint[i]) + 1.0f;
        g_blackpoint[i] = 1.0f - (cmykParam.cmy_blackpoint[i] + 1.0f);
        g_whitepoint[i] = (1.0f - cmykParam.cmy_whitepoint[i]) + 1.0f;
        g_lift[i] = (1.0f - cmykParam.cmy_lift[i])- 1.0f;
        g_gamma[i] = (1.0f - cmykParam.cmy_gamma[i]) + 1.0f;
        g_gain[i] = (1.0f - cmykParam.cmy_gain[i]) + 1.0f;
        g_mult[i] = (1.0f - cmykParam.cmy_mult[i]) + 1.0f;
        g_offset[i] = (1.0f - cmykParam.cmy_offset[i]) - 1.0f;
    }
    blackPoint[3] = cmykParam.cmy_A_blackPoint[3];
    whitePoint[3] = cmykParam.cmy_A_whitePoint[3];
    g_blackpoint[3] = cmykParam.cmy_blackpoint[3];
    g_whitepoint[3] = cmykParam.cmy_whitepoint[3];
    g_lift[3] = cmykParam.cmy_lift[3];
    g_gamma[3] = cmykParam.cmy_gamma[3];
    g_gain[3] = cmykParam.cmy_gain[3];
    g_mult[3] = cmykParam.cmy_mult[3];
    g_offset[3] = cmykParam.cmy_offset[3];

}

void imageParams::rgb_to_cmyk() {
    for (int i = 0; i < 3; i ++) {
        cmykParam.cmy_A_blackPoint[i] = (1.0f - blackPoint[i]) - 1.0f;
        cmykParam.cmy_A_whitePoint[i] = 1.0f - (whitePoint[i] - 1.0f);
        cmykParam.cmy_blackpoint[i] = (1.0f - g_blackpoint[i]) - 1.0f;
        cmykParam.cmy_whitepoint[i] = 1.0f - (g_whitepoint[i] - 1.0f);
        cmykParam.cmy_lift[i] = 1.0f - (g_lift[i] + 1.0f);
        cmykParam.cmy_gamma[i] = 1.0f - (g_gamma[i] - 1.0f);
        cmykParam.cmy_gain[i] = 1.0f - (g_gain[i] - 1.0f);
        cmykParam.cmy_mult[i] = 1.0f - (g_mult[i] - 1.0f);
        cmykParam.cmy_offset[i] = 1.0f - (g_offset[i] + 1.0f);
    }
    cmykParam.cmy_A_blackPoint[3] = blackPoint[3];
    cmykParam.cmy_A_whitePoint[3] = whitePoint[3];
    cmykParam.cmy_blackpoint[3] = g_blackpoint[3];
    cmykParam.cmy_whitepoint[3] = g_whitepoint[3];
    cmykParam.cmy_lift[3] = g_lift[3];
    cmykParam.cmy_gamma[3] = g_gamma[3];
    cmykParam.cmy_gain[3] = g_gain[3];
    cmykParam.cmy_mult[3] = g_mult[3];
    cmykParam.cmy_offset[3] = g_offset[3];
}
