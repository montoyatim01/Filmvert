#include "utils.h"
#include <cmath>
#include <chrono>



int iDivUp(int a, int b) { return (a % b != 0) ? (a / b + 1) : (a / b); }


bool isPointInBox(const unsigned int& x, const unsigned int& y, const unsigned int* xPoints, const unsigned int* yPoints) {
    // For a non-axis-aligned box (arbitrary quadrilateral)
    // We'll use the winding number algorithm
    int winding_number = 0;

    // Create points from array values
    // Loop through all edges of the polygon
    for (size_t i = 0; i < 4; ++i) {
        // Get the current point and the next point (wrapping around to the first point)
        xyPoint current(static_cast<float>(xPoints[i]), static_cast<float>(yPoints[i]));
        xyPoint next(static_cast<float>(xPoints[(i + 1) % 4]), static_cast<float>(yPoints[(i + 1) % 4]));

        // Check if the edge crosses the ray from the test point to the right
        if (current.y <= y) {
            if (next.y > y &&
                ((next.x - current.x) * (y - current.y) -
                 (next.y - current.y) * (x - current.x)) > 0) {
                // Ray crosses upward edge
                ++winding_number;
            }
        } else {
            if (next.y <= y &&
                ((next.x - current.x) * (y - current.y) -
                 (next.y - current.y) * (x - current.x)) < 0) {
                // Ray crosses downward edge
                --winding_number;
            }
        }
    }

    // If winding number is non-zero, the point is inside
    return winding_number != 0;
}


float Luma(float R, float G, float B)
{
    //float lumaRec709 = R * 0.2126f + G * 0.7152f + B * 0.0722f;
    //float lumaRec2020 = R * 0.2627f + G * 0.6780f + B * 0.0593f;
    //float lumaDCIP3 = R * 0.209492f + G * 0.721595f + B * 0.0689131f;
    //float lumaACESAP0 = R * 0.3439664498f + G * 0.7281660966f + B * -0.0721325464f;
    float lumaACESAP1 = R * 0.2722287168f + G * 0.6740817658f + B * 0.0536895174f;
    //float lumaAvg = (R + G + B) / 3.0f;
    //float lumaMax = fmax(fmax(R, G), B);
    //float Lu = L == 0 ? lumaRec709 : L == 1 ? lumaRec2020 : L == 2 ? lumaDCIP3 : L == 3 ? lumaACESAP0 : L == 4 ? lumaACESAP1 : L == 5 ? lumaAvg : lumaMax;
    return lumaACESAP1;
}

void ap0_to_ap1(float* in, float* out) {

    out[0] = in[0] * 1.451439316072 + in[1] * -0.236510746889 + in[2] * -0.214928569308;
    out[1] = in[0] * -0.076553773314 + in[1] * 1.176229699812 + in[2] * -0.099675926450;
    out[2] = in[0] * 0.008316148425 + in[1] * -0.006032449791 + in[2] * 0.997716301413;

    return;
}

float oklab_Chroma(float r, float g, float b) {
    // Convert pixel from AP1 to sRGB primaries
    float s_r = 1.7070626733 * r + -0.6199595404 * g + -0.0872598502 * b;
    float s_g = -0.1309768295 * r + 1.1390322752 * g + -0.0079562968 * b;
    float s_b = -0.0245106012 * r + -0.1248109317 * g + 1.1493959705 * b;

    // Convert to oklab
    // Sample code from: https://bottosson.github.io/posts/oklab/
    float l = 0.4122214708f * s_r + 0.5363325363f * s_g + 0.0514459929f * s_b;
	float m = 0.2119034982f * s_r + 0.6806995451f * s_g + 0.1073969566f * s_b;
	float s = 0.0883024619f * s_r + 0.2817188376f * s_g + 0.6299787005f * s_b;

    float l_ = cbrtf(l);
    float m_ = cbrtf(m);
    float s_ = cbrtf(s);

    float la = 1.9779984951f*l_ - 2.4285922050f*m_ + 0.4505937099f*s_;
    float lb = 0.0259040371f*l_ + 0.7827717662f*m_ - 0.8086757660f*s_;
    return std::sqrt(std::pow(la, 2.0f) + std::pow(lb, 2.0f));
}


// Helper functions for endian conversion
uint16_t swapBytes16(uint16_t value) {
    return (value << 8) | (value >> 8);
}

uint32_t swapBytes32(uint32_t value) {
    return ((value & 0xFF000000) >> 24) |
           ((value & 0x00FF0000) >> 8)  |
           ((value & 0x0000FF00) << 8)  |
           ((value & 0x000000FF) << 24);
}


void computeKernels(float strength, float* kernels)
{
  float h_PI = 3.14159265359;
  int kernelSize = (int)(strength * KERNELSIZE) + 1;
  kernelSize = kernelSize % 2 == 0 ? kernelSize + 1 : kernelSize;
  int posI = 0;
  float kernelSum = 0.0f;
  // Loop through the kernel and apply the Gaussian blur
  for (int i = -kernelSize / 2; i <= kernelSize / 2; i++)
  {
          float weight = exp(-0.5f * pow(i / strength, 2)) / (strength * sqrt(2 * h_PI));
          kernels[posI] = weight;
          kernelSum += weight;
          posI++;
  }

  // Normalize the kernel
  for (int i = 0; i < posI; i++)
  {
      kernels[i] /= kernelSum;
  }

}

void checkRawFile(rawSetting& rawSet, long fileSize) {
    switch (fileSize) {
        case 36000000:
            // 3000x2000 Base 16
            rawSet.width = 3000;
            rawSet.height = 2000;
            rawSet.channels = 3;
            rawSet.bitDepth = 16;
            rawSet.littleE = true;
            break;
        case 36000016:
            // 3000x2000 Base 16 w/header
            rawSet.width = 3000;
            rawSet.height = 2000;
            rawSet.channels = 3;
            rawSet.bitDepth = 16;
            rawSet.littleE = true;
            rawSet.pakonHeader = true;
            break;
        case 20250000:
            // 2250x1500 Base 8
            rawSet.width = 2250;
            rawSet.height = 1500;
            rawSet.channels = 3;
            rawSet.bitDepth = 16;
            rawSet.littleE = true;
            break;
        case 20250016:
            // 2250x1500 Base 8 w/header
            rawSet.width = 2250;
            rawSet.height = 1500;
            rawSet.channels = 3;
            rawSet.bitDepth = 16;
            rawSet.littleE = true;
            rawSet.pakonHeader = true;
            break;
        case 18000000:
            // 1500x2000 Half-frame Base 16
            rawSet.width = 1500;
            rawSet.height = 2000;
            rawSet.channels = 3;
            rawSet.bitDepth = 16;
            rawSet.littleE = true;
            break;
        case 18000016:
            // 1500x2000 Half-frame Base 16 w/header
            rawSet.width = 1500;
            rawSet.height = 2000;
            rawSet.channels = 3;
            rawSet.bitDepth = 16;
            rawSet.littleE = true;
            rawSet.pakonHeader = true;
            break;
        case 9000000:
            // 1500x1000 Base 4
            rawSet.width = 1500;
            rawSet.height = 1000;
            rawSet.channels = 3;
            rawSet.bitDepth = 16;
            rawSet.littleE = true;
            break;
        case 9000016:
            // 1500x1000 Base 4 w/header
            rawSet.width = 1500;
            rawSet.height = 1000;
            rawSet.channels = 3;
            rawSet.bitDepth = 16;
            rawSet.littleE = true;
            rawSet.pakonHeader = true;
            break;
        default:
            // We don't know!
            break;
    }
}


uint64_t currentEpoch() {
    auto now = std::chrono::system_clock::now();
    return std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
}
