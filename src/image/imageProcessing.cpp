#include "image.h"
#include "logger.h"
#include "preferences.h"
#include "utils.h"
#include "lancir.h"
#include <strings.h>

//---Process Base Color---//
/*
    Based on the selection points made in the GUI,
    average all of the pixels in the rectangle
    together to get the base color
*/

void image::processBaseColor() {

    unsigned int x0, x1, y0, y1;
    unsigned int sampleX[2];
    unsigned int sampleY[2];
    for (int i = 0; i < 2; i++) {
        sampleX[i] = imgParam.sampleX[i] * width;
        sampleY[i] = imgParam.sampleY[i] * height;
    }
    x0 = sampleX[0] < sampleX[1] ? sampleX[0] : sampleX[1];
    x1 = sampleX[0] > sampleX[1] ? sampleX[0] : sampleX[1];

    x0 = std::clamp(x0, 0u, width - 2);
    x1 = std::clamp(x0, 0u, width - 2);

    y0 = sampleY[0] < sampleY[1] ? sampleY[0] : sampleY[1];
    y1 = sampleY[0] > sampleY[1] ? sampleY[0] : sampleY[1];

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
    unsigned int minX, minY, maxX, maxY;
    unsigned int cropBoxX[4];
    unsigned int cropBoxY[4];
    for (int i = 0; i < 4; i++) {
        cropBoxX[i] = imgParam.cropBoxX[i] * width;
        cropBoxY[i] = imgParam.cropBoxY[i] * height;
    }
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            if (isPointInBox(x, y, cropBoxX, cropBoxY)) {
                unsigned int index = (y * width) + x;
                float luma = Luma(buffer[4 * index + 0], buffer[4 * index + 1], buffer[4 * index + 2]);
                if (luma < minLuma) {
                    minLuma = luma;
                    minX = x;
                    minY = y;
                    imgParam.blackPoint[0] = buffer[4 * index + 0];
                    imgParam.blackPoint[1] = buffer[4 * index + 1];
                    imgParam.blackPoint[2] = buffer[4 * index + 2];
                }
                if (luma > maxLuma) {
                    maxLuma = luma;
                    maxX = x;
                    maxY = y;
                    imgParam.whitePoint[0] = buffer[4 * index + 0];
                    imgParam.whitePoint[1] = buffer[4 * index + 1];
                    imgParam.whitePoint[2] = buffer[4 * index + 2];
                }
            }
        }
    }

    imgParam.minX = (float)minX / width;
    imgParam.minY = (float)minY / height;
    imgParam.maxX = (float)maxX / width;
    imgParam.maxY = (float)maxY / height;

    LOG_INFO("Analysis finished for {}!", srcFilename);
    LOG_INFO("Min Pixel: {},{}", imgParam.minX, imgParam.minY);
    LOG_INFO("Min Values: {}, {}, {}", imgParam.blackPoint[0], imgParam.blackPoint[1], imgParam.blackPoint[2]);
    LOG_INFO("Max Pixel: {},{}", imgParam.maxX, imgParam.maxY);
    LOG_INFO("Max Values: {}, {}, {}", imgParam.whitePoint[0], imgParam.whitePoint[1], imgParam.whitePoint[2]);
}

//--- Resize Proxy ---//
/*
    Resize the input image to the maximum
    long-side dimension as set in the preferences.
*/
void image::resizeProxy() {

    // Calculate the dimensions based on the max side
    // Allocate temp buffer
    // Resize from raw to tmp
    // Copy back
    //
    if (std::max(rawWidth, rawHeight) < appPrefs.prefs.maxRes)
        return; // Our image is small enough, don't resize
    unsigned int newWidth, newHeight;
    float ratio = rawWidth > rawHeight ? (float)appPrefs.prefs.maxRes / (float)rawWidth : (float)appPrefs.prefs.maxRes / (float)rawHeight;

    newWidth = rawWidth * ratio;
    newHeight = rawHeight * ratio;

    allocateTmpBuf();

    avir::CLancIR imageResizer;
    imageResizer.resizeImage<float, float>(
        rawImgData, rawWidth, rawHeight, 0,
        tmpOutData, newWidth, newHeight, 0, 4, 0, 0, 0, 0);

    if (rawImgData)
        delete [] rawImgData;
    rawImgData = new float[newWidth * newHeight * 4];

    memcpy(rawImgData, tmpOutData, newWidth * newHeight * 4 * sizeof(float));
    width = newWidth;
    height = newHeight;
    clearTmpBuf();
}
