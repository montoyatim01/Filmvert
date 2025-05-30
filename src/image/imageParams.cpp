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
