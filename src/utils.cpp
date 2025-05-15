#include "utils.h"
#include <cmath>



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
