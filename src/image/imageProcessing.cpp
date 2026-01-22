#include "image.h"
#include "imageParams.h"
#include "logger.h"
#include "ocioProcessor.h"
#include "preferences.h"
#include "renderParams.h"
#include "utils.h"
#include "lancir.h"
#include <string>
#include <numbers>

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
    if (rawImgData) {
        delete [] rawImgData;
        rawImgData = nullptr;
    }

    rawImgData = new float[newWidth * newHeight * 4];

    // Copy back from the temp buffer to the raw image buffer
    memcpy(rawImgData, tmpOutData, newWidth * newHeight * 4 * sizeof(float));
    width = newWidth;
    height = newHeight;
    clearTmpBuf();
}

//---- CPU Processing Functions ----//

// Helper function to calculate sample UV from output UV
// Returns the input texture coordinates for a given output pixel coordinate
void getCroppedRotatedUV(float u, float v, float& outU, float& outV,
                         const renderParams& imgParam, int inputWidth, int inputHeight,
                         int outputWidth, int outputHeight) {
    outU = u;
    outV = v;

    if (!imgParam.cropEnable && !imgParam.cropVisible) {
        return;
    }

    float aspectRatio = (float)inputWidth / (float)inputHeight;

    float workingU = u;
    float workingV = v;

    if (imgParam.cropEnable) {
        float cropSizeX = imgParam.imageCropMaxX - imgParam.imageCropMinX;
        float cropSizeY = imgParam.imageCropMaxY - imgParam.imageCropMinY;
        workingU = imgParam.imageCropMinX + cropSizeX * u;
        workingV = imgParam.imageCropMinY + cropSizeY * v;
    }

    float imageCenter = 0.5f;

    // Center the coordinates
    float centeredU = workingU - imageCenter;
    float centeredV = workingV - imageCenter;

    // Scale by aspect ratio before rotation
    centeredU *= aspectRatio;

    // Apply rotation matrix
    float cosR = std::cos(imgParam.arbitraryRotation);
    float sinR = std::sin(imgParam.arbitraryRotation);

    float rotatedU = centeredU * cosR - centeredV * sinR;
    float rotatedV = centeredU * sinR + centeredV * cosR;

    // Scale back by inverse aspect ratio after rotation
    rotatedU /= aspectRatio;

    // Convert back to texture coordinates
    outU = rotatedU + imageCenter;
    outV = rotatedV + imageCenter;
}

// Mitchell-Netravali filter kernel
// B and C parameters: B=1/3, C=1/3 gives the standard Mitchell filter
// B=1, C=0 gives B-spline, B=0, C=0.5 gives Catmull-Rom
inline float mitchell(float x, float B = 1.0f/3.0f, float C = 1.0f/3.0f) {
    x = fabs(x);
    if (x < 1.0f) {
        return ((12.0f - 9.0f * B - 6.0f * C) * x * x * x +
                (-18.0f + 12.0f * B + 6.0f * C) * x * x +
                (6.0f - 2.0f * B)) / 6.0f;
    } else if (x < 2.0f) {
        return ((-B - 6.0f * C) * x * x * x +
                (6.0f * B + 30.0f * C) * x * x +
                (-12.0f * B - 48.0f * C) * x +
                (8.0f * B + 24.0f * C)) / 6.0f;
    }
    return 0.0f;
}

// Sample image with Mitchell-Netravali filtering
float4 sampleImageMitchell(const float* rawImgData, int inputWidth, int inputHeight,
                          float u, float v) {


    // Convert normalized UV to pixel coordinates
    float srcX = u * (float)inputWidth - 0.5f;
    float srcY = v * (float)inputHeight - 0.5f;

    int centerX = (int)floor(srcX);
    int centerY = (int)floor(srcY);

    float fx = srcX - centerX;
    float fy = srcY - centerY;

    float4 result = float4(0.0f, 0.0f, 0.0f, 0.0f);
    float weightSum = 0.0f;

    // Sample 4x4 neighborhood for Mitchell filter
    for (int dy = -1; dy <= 2; dy++) {
        for (int dx = -1; dx <= 2; dx++) {
            int sampleX = centerX + dx;
            int sampleY = centerY + dy;

            // Clamp to valid range
            sampleX = std::max(0, std::min(sampleX, inputWidth - 1));
            sampleY = std::max(0, std::min(sampleY, inputHeight - 1));

            // Calculate Mitchell weights
            float wx = mitchell((float)dx - fx);
            float wy = mitchell((float)dy - fy);
            float weight = wx * wy;

            int idx = ((sampleY * inputWidth) + sampleX) * 4;

            result.x += rawImgData[idx + 0] * weight;
            result.y += rawImgData[idx + 1] * weight;
            result.z += rawImgData[idx + 2] * weight;

            weightSum += weight;
        }
    }

    // Normalize by total weight
    if (weightSum > 0.0f) {
        result.x /= weightSum;
        result.y /= weightSum;
        result.z /= weightSum;
    }
    result.w = 1.0f;

    return result;
}


// JPLog Functions
float4 JPLogtoLin(float4 inputPixel)
{
    float4 outPixel;

    const float ALOGSM1_LIN_BRKPNT = 0.006801176276;
    const float ALOGSM1_LOG_BRKPNT = 0.16129032258064516129;
    const float ALOGSM1_LINTOLOG_SLOPE = 10.36773919972907075549;
    const float ALOGSM1_LINTOLOG_YINT = 0.09077750069969257965;

    outPixel.x = inputPixel.x <= ALOGSM1_LOG_BRKPNT ? (inputPixel.x - ALOGSM1_LINTOLOG_YINT) / ALOGSM1_LINTOLOG_SLOPE : pow(2.0f, inputPixel.x * 20.46f - 10.5f);
    outPixel.y = inputPixel.y <= ALOGSM1_LOG_BRKPNT ? (inputPixel.y - ALOGSM1_LINTOLOG_YINT) / ALOGSM1_LINTOLOG_SLOPE : pow(2.0f, inputPixel.y * 20.46f - 10.5f);
    outPixel.z = inputPixel.z <= ALOGSM1_LOG_BRKPNT ? (inputPixel.z - ALOGSM1_LINTOLOG_YINT) / ALOGSM1_LINTOLOG_SLOPE : pow(2.0f, inputPixel.z * 20.46f - 10.5f);

    return outPixel;
}

float4 LintoJPLog(float4 inputPixel)
{
    float4 outPixel;

    const float ALOGSM1_LIN_BRKPNT = 0.006801176276;
    const float ALOGSM1_LOG_BRKPNT = 0.16129032258064516129;
    const float ALOGSM1_LINTOLOG_SLOPE = 10.36773919972907075549;
    const float ALOGSM1_LINTOLOG_YINT = 0.09077750069969257965;

    outPixel.x = inputPixel.x <= ALOGSM1_LIN_BRKPNT ? ALOGSM1_LINTOLOG_SLOPE * inputPixel.x + ALOGSM1_LINTOLOG_YINT : (log(inputPixel.x)/log(2.0) + 10.5) / 20.46;
    outPixel.y = inputPixel.y <= ALOGSM1_LIN_BRKPNT ? ALOGSM1_LINTOLOG_SLOPE * inputPixel.y + ALOGSM1_LINTOLOG_YINT : (log(inputPixel.y)/log(2.0) + 10.5) / 20.46;
    outPixel.z = inputPixel.z <= ALOGSM1_LIN_BRKPNT ? ALOGSM1_LINTOLOG_SLOPE * inputPixel.z + ALOGSM1_LINTOLOG_YINT : (log(inputPixel.z)/log(2.0) + 10.5) / 20.46;

    return outPixel;
}
float luma(float4 inputPixel)
{
    return inputPixel.x * 0.3439664498 + inputPixel.y * 0.7281660966 + inputPixel.z * -0.0721325464;
}

//--- CPU Render ---//
/*
    Function to process images on CPU
    rather than on GPU
*/
void image::processCPU(ocioSetting ocioSet) {
    auto start = std::chrono::steady_clock::now();
    cpuRender = true;
    // Generate RenderParams struct
    renderParams _renderParams = img_to_param(this);

    int procWidth = fullIm ? rawWidth : width;
    int procHeight = fullIm ? rawHeight : height;

    int outputWidth = procWidth;
    int outputHeight = procHeight;
    if (imgParam.cropEnable) {
        // Calculate crop rectangle dimensions
        outputWidth = (imgParam.imageCropMaxX - imgParam.imageCropMinX) * procWidth;
        outputHeight = (imgParam.imageCropMaxY - imgParam.imageCropMinY) * procHeight;

        // Ensure minimum size
        if (outputWidth < 1) outputWidth = 1;
        if (outputHeight < 1) outputHeight = 1;
    }
    allocProcBuf();
    float4 baseColor = float4(_renderParams.baseColor);
    float4 blackPoint = float4(_renderParams.blackPoint);
    float4 whitePoint = float4(_renderParams.whitePoint);
    float G_temp = _renderParams.temp;
    float G_tint = _renderParams.tint;
    float4 G_blackpoint = float4(_renderParams.G_blackpoint);
    float4 G_whitepoint = float4(_renderParams.G_whitepoint);
    float4 G_mult = float4(_renderParams.G_mult);
    float4 G_gain = float4(_renderParams.G_gain);
    float4 G_lift = float4(_renderParams.G_lift);
    float4 G_offset = float4(_renderParams.G_offset);
    float4 G_gamma = float4(_renderParams.G_gamma);
    _renderParams.arbitraryRotation = imgParam.arbitraryRotation * (std::numbers::pi_v<float> / 180.0f);

    // Get system thread count
    int threadCount = std::thread::hardware_concurrency();
    threadCount = threadCount < 1 ? 1 : threadCount; // Ensure minimum

    // Get minimum of active images/max export count
    unsigned int numThreads = threadCount / std::min(appPrefs.prefs.maxSimExports, activeExpCount);
    numThreads = numThreads < 1 ? 1 : numThreads; // Ensure minimum

    LOG_INFO("Processing image {} on CPU with {} threads!", srcFilename, numThreads);

    // Thread vector
    std::vector<std::thread> threads(numThreads);
    int rowsPerThread = outputHeight / numThreads;

    auto processRows = [&](int startRow, int endRow) {
        for (int y = startRow; y < endRow; y++) {
            for (int x = 0; x < outputWidth; x++) {
                float4 pixIn;
                uint32_t index;
                if (imgParam.cropEnable) {
                    // Convert output pixel to normalized UV coordinates [0, 1]
                    float u = ((float)x + 0.5f) / (float)outputWidth;
                    float v = ((float)y + 0.5f) / (float)outputHeight;

                    // Calculate input UV coordinates
                    float sampleU, sampleV;
                    getCroppedRotatedUV(u, v, sampleU, sampleV, _renderParams,
                                        procWidth, procHeight,      // input dimensions
                                        outputWidth, outputHeight);  // output dimensions
                    // Check if sample position is outside valid texture bounds
                    index = ((y * outputWidth) + x) * 4;
                    if (sampleU < 0.0f || sampleU > 1.0f || sampleV < 0.0f || sampleV > 1.0f) {
                        continue;
                    }

                    pixIn = sampleImageMitchell(rawImgData, procWidth, procHeight,
                                                       sampleU, sampleV);
                } else {
                    index = ((y * outputWidth) + x) * 4;
                    pixIn.x = rawImgData[index + 0];
                    pixIn.y = rawImgData[index + 1];
                    pixIn.z = rawImgData[index + 2];
                    pixIn.w = rawImgData[index + 3];
                }

                // Base Color
                float4 pixOut = (baseColor / max(pixIn, float4(0.0001))) * 0.1;
                pixOut.w = 1.0;

                // Set White/Black Points
                pixOut = (pixOut - blackPoint) / (whitePoint - blackPoint);

                // Temp/Tint
                float4 tempPix = pixOut;
                float4 warm = float4(2.0, 1.0, 0.0, 1.0);
                float4 cool = float4(0.0, 1.0, 2.0, 1.0);
                float4 green = float4(0.0, 1.5, 0.0, 1.0);
                float4 mag = float4(1.5, 0.0, 1.5, 1.0);
                float temp = (-1.0 * _renderParams.temp);
                float tint = (0.75 * _renderParams.tint);

                // WB
                tempPix = temp >= 0.0 ?
                    (tempPix * cool * temp) + ((1.0 - temp) * tempPix) :
                    (tempPix * warm * (-1.0 * temp)) + ((1.0 - (-1.0 * temp)) * tempPix);
                // Tint
                tempPix = tint >= 0.0 ?
                    (tempPix * mag * tint) + ((1.0 - tint) * tempPix) :
                    (tempPix * green * (-1.0 * tint)) + ((1.0 - (-1.0 * tint)) * tempPix);

                // Process grade bp/wp in linear
                tempPix = (tempPix - G_blackpoint) / (G_whitepoint - G_blackpoint);

                // Lin to Log for grading
                tempPix = LintoJPLog(tempPix);

                // Grade node operation
                float4 aGrade = G_mult * (G_gain - G_lift) / (1.0 - 0.0);
                float4 bGrade = G_offset + G_lift - aGrade * 0.0;
                float4 powBase = aGrade * tempPix + bGrade;
                powBase = max(powBase, float4(0.0001));
                tempPix = pow(powBase, 1.0/G_gamma);
                tempPix = clamp(tempPix, 0.0, 100.0);

                // Back to lin for output
                tempPix = JPLogtoLin(tempPix);

                // Perform saturation operation
                float lumaPix = luma(tempPix);
                tempPix = lumaPix + (_renderParams.saturation + 1.0) * (tempPix - lumaPix);

                procImgData[index + 0] = (renderBypass == 1 ? pixIn.x : gradeBypass == 1 ? pixOut.x : tempPix.x);
                procImgData[index + 1] = (renderBypass == 1 ? pixIn.y : gradeBypass == 1 ? pixOut.y : tempPix.y);
                procImgData[index + 2] = (renderBypass == 1 ? pixIn.z : gradeBypass == 1 ? pixOut.z : tempPix.z);
                procImgData[index + 3] = 1.0f;
            }
        }
    };

    for (size_t i = 0; i < numThreads; i++) {
        int startRow = i * rowsPerThread;
        int endRow = (i == numThreads - 1) ? outputHeight : (i + 1) * rowsPerThread;
        threads[i] = std::thread(processRows, startRow, endRow);
    }

    for (auto & thread : threads) {
        thread.join();
    }

    ocioProc.processImage(procImgData, outputWidth, outputHeight, ocioSet);

    rndrW = outputWidth;
    rndrH = outputHeight;

    renderReady = true;
    cpuRender = false;
    auto end = std::chrono::steady_clock::now();
    auto dur = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    LOG_INFO("Finished CPU processing {} in {}ms", srcFilename, dur.count()/1000);
}
