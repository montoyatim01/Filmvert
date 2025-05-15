#ifndef _utils_h
#define _utils_h
#include <stdint.h>
struct xyPoint {
    float x;
    float y;

    xyPoint(float x_val = 0, float y_val = 0) : x(x_val), y(y_val) {}
};


int iDivUp(int a, int b);

bool isPointInBox(const unsigned int& x, const unsigned int& y, const unsigned int* xPoints, const unsigned int* yPoints);
float Luma(float R, float G, float B);


void ap0_to_ap1(float* in, float* out);

uint16_t swapBytes16(uint16_t value);
uint32_t swapBytes32(uint32_t value);
#endif
