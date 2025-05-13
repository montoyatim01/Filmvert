#include "image.h"
#include "logger.h"
#include "utils.h"

//---Process Base Color---//
/*
    Based on the selection points made in the GUI,
    average all of the pixels in the rectangle
    together to get the base color
*/

void image::processBaseColor() {

    unsigned int x0, x1, y0, y1;
    x0 = imgParam.sampleX[0] < imgParam.sampleX[1] ? imgParam.sampleX[0] : imgParam.sampleX[1];
    x1 = imgParam.sampleX[0] > imgParam.sampleX[1] ? imgParam.sampleX[0] : imgParam.sampleX[1];

    x0 = std::clamp(x0, 0u, width - 2);
    x1 = std::clamp(x0, 0u, width - 2);

    y0 = imgParam.sampleY[0] < imgParam.sampleY[1] ? imgParam.sampleY[0] : imgParam.sampleY[1];
    y1 = imgParam.sampleY[0] > imgParam.sampleY[1] ? imgParam.sampleY[0] : imgParam.sampleY[1];

    y0 = std::clamp(y0, 0u, height - 2);
    y1 = std::clamp(y0, 0u, height - 2);

    unsigned int pixCount = 0;
    float rTotal = 0.0f;
    float gTotal = 0.0f;
    float bTotal = 0.0f;
    for (int y = y0; y <= y1; y++) {
        for (int x = x0; x <= x1; x++) {
            int index = ((y * width) + x) * 4;
            rTotal += rawImgData[index + 0];
            gTotal += rawImgData[index + 1];
            bTotal += rawImgData[index + 2];

            pixCount++;
        }
    }
    if (pixCount == 0) {
        LOG_ERROR("Cannot average base! No pixels selected");
        return;
    }

    imgParam.baseColor[0] = rTotal / (float)pixCount;
    imgParam.baseColor[1] = gTotal / (float)pixCount;
    imgParam.baseColor[2] = bTotal / (float)pixCount;

}


//--- Process Min/Max values ---//
/*
    Loop through all pixels in the image looking
    for the brightest and darkest pixel.
    The brightest becomes the analysed white point.
    The darkest becomes the analysed black point.
*/
void image::processMinMax() {

    if (!blurImgData) {
            LOG_ERROR("Cannot analyze {}. No Blur Buffer!", srcFilename);
            return; //TODO: Error status return
        }

    float* buffer = blurImgData;

    float maxLuma = -100.0f;
    float minLuma = 100.0f;
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            if (isPointInBox(x, y, imgParam.cropBoxX, imgParam.cropBoxY)) {
                unsigned int index = (y * width) + x;
                float luma = Luma(buffer[4 * index + 0], buffer[4 * index + 1], buffer[4 * index + 2]);
                if (luma < minLuma) {
                    minLuma = luma;
                    imgParam.minX = x;
                    imgParam.minY = y;
                    imgParam.blackPoint[0] = buffer[4 * index + 0];
                    imgParam.blackPoint[1] = buffer[4 * index + 1];
                    imgParam.blackPoint[2] = buffer[4 * index + 2];
                }
                if (luma > maxLuma) {
                    maxLuma = luma;
                    imgParam.maxX = x;
                    imgParam.maxY = y;
                    imgParam.whitePoint[0] = buffer[4 * index + 0];
                    imgParam.whitePoint[1] = buffer[4 * index + 1];
                    imgParam.whitePoint[2] = buffer[4 * index + 2];
                }
            }
        }
    }

    LOG_INFO("Analysis finished for {}!", srcFilename);
    LOG_INFO("Min Pixel: {},{}", imgParam.minX, imgParam.minY);
    LOG_INFO("Min Values: {}, {}, {}", imgParam.blackPoint[0], imgParam.blackPoint[1], imgParam.blackPoint[2]);
    LOG_INFO("Max Pixel: {},{}", imgParam.maxX, imgParam.maxY);
    LOG_INFO("Max Values: {}, {}, {}", imgParam.whitePoint[0], imgParam.whitePoint[1], imgParam.whitePoint[2]);
}
