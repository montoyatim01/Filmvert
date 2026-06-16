#ifndef _utils_h
#define _utils_h
#include "structs.h"
#include <stdint.h>
#include <csignal>

#define KERNELSIZE 6

struct xyPoint {
    float x;
    float y;

    xyPoint(float x_val = 0, float y_val = 0) : x(x_val), y(y_val) {}
};


int iDivUp(int a, int b);

bool isPointInBox(const unsigned int& x, const unsigned int& y, const unsigned int* xPoints, const unsigned int* yPoints);
float Luma(float R, float G, float B);


void ap0_to_ap1(float* in, float* out);
float oklab_Chroma(float r, float g, float b);

uint16_t swapBytes16(uint16_t value);
uint32_t swapBytes32(uint32_t value);

void computeKernels(float strength, float* kernels);

void checkRawFile(rawSetting& rawSet, long fileSize);

uint64_t currentEpoch();
#endif
