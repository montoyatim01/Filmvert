#include "image.h"
#include "logger.h"
#include "preferences.h"
#include "utils.h"
#include "lancir.h"
#include <string>

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

//--- Blur Image ---//
/*
    Blur the image based on the bias
    setting set in the UI
*/
void image::blurImage() {
    allocateTmpBuf();

    int kernelSize = (int)(imgParam.blurAmount * KERNELSIZE) + 1;
    kernelSize = kernelSize % 2 == 0 ? kernelSize + 1 : kernelSize;

    int halfSize = kernelSize / 2;

    float* blurKern = new float[kernelSize];

    computeKernels(imgParam.blurAmount, blurKern);

    unsigned int numThreads = std::thread::hardware_concurrency();
    numThreads = numThreads == 0 ? 2 : numThreads;
    // Create a vector of threads
    std::vector<std::thread> threads(numThreads);

    int rowsPerThread = height / numThreads;
    int colsPerThread = width / numThreads;

    // Horizontal Blur
    auto processRows = [&](int startRow, int endRow) {
        for (int y=startRow; y<endRow; y++)
        {
            for (int x=0; x < width; x++) {
                for (int ch = 0; ch < 3; ch++) {
                    float outPix = 0.0f;
                    int kernelCounterx = 0;
                    for (int i = -halfSize; i <= halfSize; i++)
                    {
                        outPix += blurKern[kernelCounterx] * rawImgData[(((y * width) + std::clamp(x + i, 0, (int)width - 1)) * 4 )+ch];
                        kernelCounterx++;
                    }
                    tmpOutData[(((y * width) + x) * 4 ) + ch] = outPix;
                }
            }
        }
    };
    // Launch the threads
    for (int i=0; i<numThreads; ++i) {
        int startRow = i * rowsPerThread;
        int endRow = (i == numThreads - 1) ? height : (i + 1) * rowsPerThread;
        threads[i] = std::thread(processRows, startRow, endRow);
    }

    // Wait for all threads to finish
    for (auto& thread : threads) {
        thread.join();
    }

    // Vertical Blur
    auto processCols = [&](int startCol, int endCol) {
        for (int x = startCol; x < endCol; x++) {
            for (int y = 0; y < height; y++) {
                for (int ch = 0; ch < 3; ch++) {
                    float outPix = 0.0f;
                    int kernelCountery = 0;
                    for (int i = -halfSize; i <= halfSize; i++) {
                        outPix += blurKern[kernelCountery] * tmpOutData[(((std::clamp(y + i, 0, (int)height - 1) * width) + x) * 4)+ch];
                        kernelCountery++;
                    }
                    blurImgData[(((y * width) + x) * 4 ) + ch] = (imgParam.baseColor[ch] / outPix) * 0.1f;
                }
            }
        }
    };
    // Launch the threads
    for (int i=0; i<numThreads; ++i) {
        int startCol = i * colsPerThread;
        int endCol = (i == numThreads - 1) ? width : (i + 1) * colsPerThread;
        threads[i] = std::thread(processCols, startCol, endCol);
    }

    // Wait for all threads to finish
    for (auto& thread : threads) {
        thread.join();
    }

    clearTmpBuf();
    delete [] blurKern;
    blurReady = true;
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

//--- Set MinMax ---//
/*
    Blur the area around the selected min/max
    point based on the set blur amount. Set the
    min/max point based on the blur
*/
void image::setMinMax() {

    // Kernel size and half-kernel size
    int kernelSize = (int)(imgParam.blurAmount * KERNELSIZE) + 1;
    kernelSize = kernelSize % 2 == 0 ? kernelSize + 1 : kernelSize;
    int halfSize = kernelSize / 2;

    // Point to use for processing
    int pointX = minSel ? imgParam.minX * width : imgParam.maxX * width;
    int pointY = minSel ? imgParam.minY * height : imgParam.maxY * height;

    float sumWeights = 0.0;
    float outPix[3] = {0.0f, 0.0f, 0.0f};
    int ySq, xSq;

    for (int y = -halfSize; y <= halfSize; y++) {
        ySq = y * y;
        for (int x = -halfSize; x<= halfSize; x++) {
            xSq = x * x;
            int xPos = std::clamp(pointX + x, 0, (int)width - 1);
            int yPos = std::clamp(pointY + y, 0, (int)height - 1);
            float weight = exp(-((float)(xSq + ySq)) / (2.0f * imgParam.blurAmount * imgParam.blurAmount));
            for (int ch = 0; ch < 3; ch ++) {
                outPix[ch] += weight * rawImgData[(((yPos * width) + xPos) * 4) + ch];
            }
            sumWeights += weight;
        }
    }
    if (minSel) {
        imgParam.blackPoint[0] = (imgParam.baseColor[0] / (outPix[0] / sumWeights)) * 0.1f;
        imgParam.blackPoint[1] = (imgParam.baseColor[1] / (outPix[1] / sumWeights)) * 0.1f;
        imgParam.blackPoint[2] = (imgParam.baseColor[2] / (outPix[2] / sumWeights)) * 0.1f;
    } else {
        imgParam.whitePoint[0] = (imgParam.baseColor[0] / (outPix[0] / sumWeights)) * 0.1f;
        imgParam.whitePoint[1] = (imgParam.baseColor[1] / (outPix[1] / sumWeights)) * 0.1f;
        imgParam.whitePoint[2] = (imgParam.baseColor[2] / (outPix[2] / sumWeights)) * 0.1f;
    }
}

//--- Calculate Proxy ---//
/*
    Helper function to calculate image
    dimensions if performance mode is enabled
*/
void image::calcProxyDim() {
    if (std::max(rawWidth, rawHeight) < appPrefs.prefs.maxRes)
        return; // Our image is small enough, don't resize
    unsigned int newWidth, newHeight;
    float ratio = rawWidth > rawHeight ? (float)appPrefs.prefs.maxRes / (float)rawWidth : (float)appPrefs.prefs.maxRes / (float)rawHeight;

    newWidth = rawWidth * ratio;
    newHeight = rawHeight * ratio;
    return;
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

    // We want to resize the raw image data buffer
    // to only be as big as the new smaller image
    if (rawImgData)
        delete [] rawImgData;
    rawImgData = new float[newWidth * newHeight * 4];

    // Copy back from the temp buffer to the raw image buffer
    memcpy(rawImgData, tmpOutData, newWidth * newHeight * 4 * sizeof(float));
    width = newWidth;
    height = newHeight;
    clearTmpBuf();
}
