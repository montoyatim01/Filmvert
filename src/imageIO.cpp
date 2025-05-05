#include "imageIO.h"
#include "OpenColorIO/OpenColorTransforms.h"
#include "OpenColorIO/OpenColorTypes.h"
#include "grainParams.h"
#include "logger.h"
#include "renderParams.h"
#include "utils.h"
#include "ocioProcessor.h"
#include <OpenImageIO/imageio.h>
#include <OpenImageIO/typedesc.h>
#include <libraw/libraw.h>
#include <cstdlib>
#include <cstring>
#include <csignal>
#include <filesystem>
#include <string>
#include <math.h>
#include <chrono>



void image::padToRGBA() {
    if (nChannels == 4)
    {
        // We already have an alpha channel
        return;
    }
    if (!procImgData || !rawImgData) {
        LOG_ERROR("Either source or destination rgba buffer is null");
    }

    // Define the number of threads to use
    unsigned int numThreads = std::thread::hardware_concurrency();
    if (numThreads == 0) {
        numThreads = 2; // Default value if concurrent threads can't be determined
    }


    // Create a vector of threads
    std::vector<std::thread> threads(numThreads);

    // Divide the workload into equal parts for each thread
    int rowsPerThread = height / numThreads;

    auto processRows = [&](int startRow, int endRow) {
        for (int y=startRow; y<endRow; y++)
        {
            for (int x=0; x<width; x++)
            {
                int index = (y * width) + x;
                procImgData[4 * index] = nChannels >= 1 ? rawImgData[nChannels * index] : 0.0f; // R channel
                procImgData[4 * index + 1] = nChannels >= 2 ? rawImgData[nChannels * index + 1] : 0.0f; // G channel
                procImgData[4 * index + 2] = nChannels >= 3 ? rawImgData[nChannels * index + 2] : 0.0f; // B channel
                procImgData[4 * index + 3] = nChannels >= 4 ? rawImgData[nChannels * index + 3] : 1.0f; // A channel
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
    memcpy(rawImgData, procImgData, width * height * 4 * sizeof(float));
}

void image::trimForSave() {
    if (nChannels == 4)
    {
        // We already have an alpha channel
        return;
    }
    if (!procImgData || !tmpOutData) {
        LOG_ERROR("Either source or destination rgba buffer is null");
    }

    // Define the number of threads to use
    unsigned int numThreads = std::thread::hardware_concurrency();
    if (numThreads == 0) {
        numThreads = 2; // Default value if concurrent threads can't be determined
    }


    // Create a vector of threads
    std::vector<std::thread> threads(numThreads);

    // Divide the workload into equal parts for each thread
    int rowsPerThread = height / numThreads;

    auto processRows = [&](int startRow, int endRow) {
        for (int y=startRow; y<endRow; y++)
        {
            for (int x=0; x<width; x++)
            {
                int index = (y * width) + x;
                tmpOutData[nChannels * index] = procImgData[4 * index]; // R channel
                if (nChannels > 1)
                    tmpOutData[nChannels * index + 1] = nChannels >= 2 ? procImgData[4 * index + 1] : 0.0f; // G channel
                if (nChannels > 2)
                    tmpOutData[nChannels * index + 2] = nChannels >= 3 ? procImgData[4 * index + 2] : 0.0f; // B channel
                if (nChannels > 3)
                    tmpOutData[nChannels * index + 3] = nChannels >= 4 ? procImgData[4 * index + 3] : 1.0f; // A channel
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
}
void image::allocateTmpBuf() {
    if (nChannels == 4)
    {
        // We already have an alpha channel
        tmpOutData = procImgData;
        return;
    }
    tmpOutData = new float[width * height * nChannels];
}
void image::clearTmpBuf() {
    if (tmpOutData && tmpOutData != procImgData) {
        delete[] tmpOutData;
        tmpOutData = nullptr;
    }

}

void image::procDispImg() {

    if (!procImgData || !dispImgData || !rawImgData) {
        LOG_ERROR("Either source or destination buffer is null");
    }
    // Define the number of threads to use
    unsigned int numThreads = std::thread::hardware_concurrency();
    if (numThreads == 0) {
        numThreads = 2; // Default value if concurrent threads can't be determined
    }

    float* buffer = procImgData; //procImgData

    // OCIO Display Process
    //ocioProc.processImageGPU(buffer, width, height);

    // Create a vector of threads
    std::vector<std::thread> threads(numThreads);

    // Divide the workload into equal parts for each thread
    int rowsPerThread = height / numThreads;

    auto processRows = [&](int startRow, int endRow) {
        for (int y=startRow; y<endRow; y++)
        {
            for (int x=0; x<width; x++)
            {
                int index = (y * width) + x;
                dispImgData[4 * index + 0] = (uint8_t)(std::clamp(buffer[4 * index + 0] * 255.0f, 0.0f, 255.0f)); // R channel
                dispImgData[4 * index + 1] = (uint8_t)(std::clamp(buffer[4 * index + 1] * 255.0f, 0.0f, 255.0f)); // G channel
                dispImgData[4 * index + 2] = (uint8_t)(std::clamp(buffer[4 * index + 2] * 255.0f, 0.0f, 255.0f)); // B channel
                dispImgData[4 * index + 3] = (uint8_t)(std::clamp(buffer[4 * index + 3] * 255.0f, 0.0f, 255.0f)); // A channel

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
}

bool image::writeImg(const exportParam param) {
    OIIO::TypeDesc format = OIIO::TypeDesc::FLOAT;
    OIIO::TypeDesc outFormat;
    switch (param.bitDepth) {
        case 0:
            outFormat = OIIO::TypeDesc::UINT8;
            break;
        case 1:
            outFormat = OIIO::TypeDesc::UINT16;
            break;
        case 2:
            outFormat = OIIO::TypeDesc::FLOAT;
            break;
   }

    std::string fileExt = "";
    switch (param.format) {
        case 0:
            fileExt = ".dpx";
            break;
        case 1:
            fileExt = ".exr";
            break;
        case 2:
            fileExt = ".jpg";
            break;
        case 3:
            fileExt = ".png";
            break;
        case 4:
            fileExt = ".tiff";
            break;
    }

    std::string filePath = param.outPath + "/" + srcFilename + fileExt;
    LOG_INFO("Exporting to: {}", filePath);
    if (std::filesystem::exists(filePath) && !param.overwrite) {
        LOG_INFO("Skipping file: {}", filePath);
        return false;
    }
    std::unique_ptr<OIIO::ImageOutput> out = OIIO::ImageOutput::create(filePath);
    if (!out) {
        LOG_ERROR("Unable to write image");
            return false;
    }
    OIIO::ImageSpec spec(width, height, nChannels, outFormat);
    //spec.from_xml(imgMeta.c_str());
    //spec.erase_attribute("Exif:LensSpecification");


    if (param.format == 2) {
        // Jpeg Compression
        std::string compression = "";
        compression = "jpeg:" + std::to_string(param.quality);
        spec["Compression"] = compression;
    } else if (param.format == 1) {
        // EXR Compression
        spec["Compression"] = "zip";
    }

    // Open the destination file
    if (!out->open(filePath, spec)) {
        LOG_ERROR("Unable to open output file: {}", OIIO::geterror());
            return false;
    }

    // Trim proc data for save
    allocateTmpBuf();
    trimForSave();
    // Write the image data
    if (!out->write_image(format, tmpOutData)) {
        LOG_ERROR("Failed to write image data: {}", OIIO::geterror());
        return false;
    }

    // Close the file
    out->close();
    clearTmpBuf();
    return true;

}

void image::processBaseColor() {

    unsigned int x0, x1, y0, y1;
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
        LOG_ERROR("No base average!");
        return;
    }

    baseColor[0] = rTotal / (float)pixCount;
    baseColor[1] = gTotal / (float)pixCount;
    baseColor[2] = bTotal / (float)pixCount;

}

void image::allocBlurBuf() {
    if (!blurImgData)
        blurImgData = new float[width * height * 4];
}

void image::delBlurBuf() {
    if (blurImgData)
    {
        delete [] blurImgData;
        blurImgData = nullptr;
    }

}

void image::processMinMax() {

    if (!blurImgData) {
            LOG_ERROR("No Blur Buffer!");
            return; //TODO: Error status return
        }

        unsigned int numThreads = std::thread::hardware_concurrency();
        if (numThreads == 0) {
            numThreads = 2; // Default value if concurrent threads can't be determined
        }

        float* buffer = blurImgData;

        // Create a vector of threads
       /*  std::vector<std::thread> threads(numThreads);
        std::vector<minMaxPoint> points(numThreads);

        // Initialize points with default values
        for (auto& point : points) {
            point.minLuma = 1.0f;
            point.maxLuma = 0.0f;
        }

        // Divide the workload into equal parts for each thread
        int rowsPerThread = height / numThreads;

        auto processRows = [&](int startRow, int endRow, int threadPos) {
            // Initialize with appropriate starting values
            float minLuma = 1.0f;  // Max possible value
            float maxLuma = 0.0f;  // Min possible value

            for (unsigned int y = startRow; y < endRow; y++) {
                for (unsigned int x = 0; x < width; x++) {
                    if (isPointInBox(x, y, cropBoxX, cropBoxY)) {
                        int index = ((y * width) + x) * 4;
                        float lum = Luma(buffer[index + 0], buffer[index + 1], buffer[index + 2]);

                        if (lum < minLuma) {
                            minLuma = lum;
                            points[threadPos].minLuma = lum;
                            points[threadPos].minX = x;
                            points[threadPos].minY = y;
                            points[threadPos].minRGB[0] = buffer[index + 0];
                            points[threadPos].minRGB[1] = buffer[index + 1]; // Fixed index
                            points[threadPos].minRGB[2] = buffer[index + 2]; // Fixed index
                        }

                        if (lum > maxLuma) {
                            maxLuma = lum;
                            points[threadPos].maxLuma = lum;
                            points[threadPos].maxX = x;
                            points[threadPos].maxY = y;
                            points[threadPos].maxRGB[0] = buffer[index + 0];
                            points[threadPos].maxRGB[1] = buffer[index + 1]; // Fixed index
                            points[threadPos].maxRGB[2] = buffer[index + 2]; // Fixed index
                        }
                    }
                }
            }
        };

        // Launch the threads
        for (int i = 0; i < numThreads; ++i) {
            int startRow = i * rowsPerThread;
            int endRow = (i == numThreads - 1) ? height : (i + 1) * rowsPerThread;
            threads[i] = std::thread(processRows, startRow, endRow, i);
        }

        // Wait for all threads to finish
        for (auto& thread : threads) {
            thread.join();
            }*/
        float maxLuma = -100.0f;
        float minLuma = 100.0f;
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                if (isPointInBox(x, y, cropBoxX, cropBoxY)) {
                    unsigned int index = (y * width) + x;
                    float luma = Luma(buffer[4 * index + 0], buffer[4 * index + 1], buffer[4 * index + 2]);
                    if (luma < minLuma) {
                        minLuma = luma;
                        minX = x;
                        minY = y;
                        blackPoint[0] = buffer[4 * index + 0];
                        blackPoint[1] = buffer[4 * index + 1];
                        blackPoint[2] = buffer[4 * index + 2];
                    }
                    if (luma > maxLuma) {
                        maxLuma = luma;
                        maxX = x;
                        maxY = y;
                        whitePoint[0] = buffer[4 * index + 0];
                        whitePoint[1] = buffer[4 * index + 1];
                        whitePoint[2] = buffer[4 * index + 2];
                    }
                }
            }
        }

        // If you need to debug, uncomment the line below
        // std::raise(SIGINT);  // THIS IS A KEY ISSUE - it interrupts execution

        /*float minLum = 1.0f;
        float maxLum = 0.0f;

        for (int i = 0; i < points.size(); i++) {
            if (points[i].minLuma < minLum) {
                minLum = points[i].minLuma;
                blackPoint[0] = points[i].minRGB[0];
                blackPoint[1] = points[i].minRGB[1];
                blackPoint[2] = points[i].minRGB[2];

                minX = points[i].minX;
                minY = points[i].minY;
            }

            if (points[i].maxLuma > maxLum) {
                maxLum = points[i].maxLuma;
                whitePoint[0] = points[i].maxRGB[0];
                whitePoint[1] = points[i].maxRGB[1];
                whitePoint[2] = points[i].maxRGB[2];

                maxX = points[i].maxX;
                maxY = points[i].maxY;
            }
            }*/

        LOG_INFO("Analysis finished!");
        LOG_INFO("Min Pixel: {},{}", minX, minY);
        LOG_INFO("Min Values: {}, {}, {}", blackPoint[0], blackPoint[1], blackPoint[2]);
        LOG_INFO("Max Pixel: {},{}", maxX, maxY);
        LOG_INFO("Max Values: {}, {}, {}", whitePoint[0], whitePoint[1], whitePoint[2]);
    //std::raise(SIGINT);
}

bool image::debayerImage(bool fullRes, int quality) {
    std::unique_ptr<LibRaw> rawProcessor(new LibRaw);

    // Set parameters for linear output
    rawProcessor->imgdata.params.output_bps = 16;       // 16-bit output
    rawProcessor->imgdata.params.gamm[0] = 1;           // Set gamma to 1.0 for linear output
    rawProcessor->imgdata.params.gamm[1] = 1;
    rawProcessor->imgdata.params.no_auto_bright = 1;    // Disable auto-brightening
    rawProcessor->imgdata.params.use_camera_wb = 1;     // Use camera white balance
    rawProcessor->imgdata.params.output_color = 6;      // Output color space: 1 = sRGB
    rawProcessor->imgdata.params.user_qual = quality;   // 0 for draft, 3 for good
    rawProcessor->imgdata.params.half_size = !fullRes;

    // Open the raw file
    int result = rawProcessor->open_file(fullPath.c_str());
    if (result != LIBRAW_SUCCESS) {
        LOG_ERROR("Error opening file: {}", fullPath);
        LOG_ERROR("{}", libraw_strerror(result));
        return false;
    }

    // Unpack the raw data
    result = rawProcessor->unpack();
    if (result != LIBRAW_SUCCESS) {
        LOG_ERROR("Error unpacking file: {}", libraw_strerror(result));
        return false;
    }

    // Process (demosaic/debayer) the raw data
    result = rawProcessor->dcraw_process();
    if (result != LIBRAW_SUCCESS) {
        LOG_ERROR("Error processing file: {}", libraw_strerror(result));
        return false;
    }

    // Convert to image
    libraw_processed_image_t* processedImage = rawProcessor->dcraw_make_mem_image(&result);
    if (!processedImage || result != LIBRAW_SUCCESS) {
        LOG_ERROR("Error creating image: {}",libraw_strerror(result) );
        return "Error creating image";
    }

    // Fill raw buffer
    width = processedImage->width;
    height = processedImage->height;
    nChannels = processedImage->colors;
    if (rawImgData)
        delete[] rawImgData;
    rawImgData = new float[width * height * 4];

    // Convert to float (assuming 16-bit output)
        if (processedImage->bits == 16 && processedImage->type == LIBRAW_IMAGE_BITMAP && processedImage->colors == 3) {
            uint16_t* raw_data = reinterpret_cast<uint16_t*>(processedImage->data);
            for (int y = 0; y < processedImage->height; y++) {
                for (int x = 0; x < processedImage->width; x++) {
                    int index = ((y * processedImage->width) + x) * processedImage->colors;
                    float* pIn = new float[3];
                    float* pOut = new float[3];
                    pIn[0] = static_cast<float>(raw_data[index + 0]) / 65535.0f;
                    pIn[1] = static_cast<float>(raw_data[index + 1]) / 65535.0f;
                    pIn[2] = static_cast<float>(raw_data[index + 2]) / 65535.0f;
                    ap0_to_ap1(pIn, pOut);
                    rawImgData[index + 0] = pOut[0] * 2.0f;
                    rawImgData[index + 1] = pOut[1] * 2.0f;
                    rawImgData[index + 2] = pOut[2] * 2.0f;
                    rawImgData[index + 3] = 1.0f;
                }
            }
        }
    return true;
}




std::variant<image, std::string> readRawImage(std::string imagePath) {
    auto start = std::chrono::steady_clock::now();
    image img;

    std::filesystem::path imgP = imagePath;
    img.srcFilename = imgP.stem().string();
    img.srcPath = imgP.parent_path().string();
    img.fullPath = imagePath;

    // Create a LibRaw processor instance
    std::unique_ptr<LibRaw> rawProcessor(new LibRaw);

    // Set parameters for linear output
    rawProcessor->imgdata.params.output_bps = 16;       // 16-bit output
    rawProcessor->imgdata.params.gamm[0] = 1;         // Set gamma to 1.0 for linear output
    rawProcessor->imgdata.params.gamm[1] = 1;
    rawProcessor->imgdata.params.no_auto_bright = 1;    // Disable auto-brightening
    rawProcessor->imgdata.params.use_camera_wb = 1;     // Use camera white balance
    rawProcessor->imgdata.params.output_color = 6;      // Output color space: 1 = sRGB
    rawProcessor->imgdata.params.user_qual = 0;
    rawProcessor->imgdata.params.half_size = 1;

    // Open the raw file
    int result = rawProcessor->open_file(imagePath.c_str());
    if (result != LIBRAW_SUCCESS) {
        LOG_ERROR("Error opening file: {}", imagePath);
        LOG_ERROR("{}", libraw_strerror(result));
        return "Error opening file";
    }
auto a1 = std::chrono::steady_clock::now();
    // Unpack the raw data
    result = rawProcessor->unpack();
    if (result != LIBRAW_SUCCESS) {
        LOG_ERROR("Error unpacking file: {}", libraw_strerror(result));
        return "Error unpacking file";
    }
auto b1 = std::chrono::steady_clock::now();
    // Process (demosaic/debayer) the raw data
    result = rawProcessor->dcraw_process();
    if (result != LIBRAW_SUCCESS) {
        LOG_ERROR("Error processing file: {}", libraw_strerror(result));
        return "Error processing file";
    }
auto c1 = std::chrono::steady_clock::now();
    // Convert to image
    libraw_processed_image_t* processedImage = rawProcessor->dcraw_make_mem_image(&result);
    if (!processedImage || result != LIBRAW_SUCCESS) {
        LOG_ERROR("Error creating image: {}",libraw_strerror(result) );
        return "Error creating image";
    }

    img.width = processedImage->width;
    img.height = processedImage->height;
    img.nChannels = processedImage->colors;
    LOG_INFO("CR3 Channels: {}", img.nChannels);
    LOG_INFO("Width: {}, Height: {}", img.width, img.height);
    img.rawImgData = new float[img.width * img.height * 4];
    //img.allocBlurBuf();

    // Convert to float (assuming 16-bit output)
        if (processedImage->bits == 16 && processedImage->type == LIBRAW_IMAGE_BITMAP && processedImage->colors == 3) {
            uint16_t* raw_data = reinterpret_cast<uint16_t*>(processedImage->data);
            for (int y = 0; y < processedImage->height; y++) {
                for (int x = 0; x < processedImage->width; x++) {
                    int index = ((y * processedImage->width) + x) * processedImage->colors;
                    float* pIn = new float[3];
                    float* pOut = new float[3];
                    pIn[0] = static_cast<float>(raw_data[index + 0]) / 65535.0f;
                    pIn[1] = static_cast<float>(raw_data[index + 1]) / 65535.0f;
                    pIn[2] = static_cast<float>(raw_data[index + 2]) / 65535.0f;
                    ap0_to_ap1(pIn, pOut);
                    img.rawImgData[index + 0] = pOut[0] * 2.0f;
                    img.rawImgData[index + 1] = pOut[1] * 2.0f;
                    img.rawImgData[index + 2] = pOut[2] * 2.0f;
                    img.rawImgData[index + 3] = 1.0f;
                }
            }
        }
auto d1 = std::chrono::steady_clock::now();

    img.procImgData = new float[img.width * img.height * 4];
    img.dispImgData = new uint8_t[img.width * img.height * 4];
    img.sampleVisible = false;
    img.cropBoxX[0] = img.width * 0.1;
    img.cropBoxY[0] = img.height * 0.1;

    img.cropBoxX[1] = img.width * 0.9;
    img.cropBoxY[1] = img.height * 0.1;

    img.cropBoxX[2] = img.width * 0.9;
    img.cropBoxY[2] = img.height * 0.9;

    img.cropBoxX[3] = img.width * 0.1;
    img.cropBoxY[3] = img.height * 0.9;
    img.renderBypass = true;

    // Pad to RGBA
    img.padToRGBA();
auto e1 = std::chrono::steady_clock::now();
    // Process Disp Img
    img.procDispImg();
auto f1 = std::chrono::steady_clock::now();

    // Clean up
    LibRaw::dcraw_clear_mem(processedImage);
    rawProcessor->recycle();
auto end = std::chrono::steady_clock::now();

auto durA = std::chrono::duration_cast<std::chrono::microseconds>(a1 - start);
auto durB = std::chrono::duration_cast<std::chrono::microseconds>(b1 - a1);
auto durC = std::chrono::duration_cast<std::chrono::microseconds>(c1 - b1);
auto durD = std::chrono::duration_cast<std::chrono::microseconds>(d1 - c1);
auto durE = std::chrono::duration_cast<std::chrono::microseconds>(e1 - d1);
auto durF = std::chrono::duration_cast<std::chrono::microseconds>(f1 - e1);
auto durG = std::chrono::duration_cast<std::chrono::microseconds>(end - f1);

LOG_INFO("-----------Debayer Time-----------");
LOG_INFO("A1:  {:*>8}μs | {:*>8}ms", durA.count(), durA.count()/1000);
LOG_INFO("B1:  {:*>8}μs | {:*>8}ms", durB.count(), durB.count()/1000);
LOG_INFO("C1:  {:*>8}μs | {:*>8}ms", durC.count(), durC.count()/1000);
LOG_INFO("D1:  {:*>8}μs | {:*>8}ms", durD.count(), durD.count()/1000);
LOG_INFO("E1:  {:*>8}μs | {:*>8}ms", durE.count(), durE.count()/1000);
LOG_INFO("F1:  {:*>8}μs | {:*>8}ms", durF.count(), durF.count()/1000);
LOG_INFO("G1:  {:*>8}μs | {:*>8}ms", durG.count(), durG.count()/1000);
LOG_INFO("----------------------------------");
    return img;
}

std::variant<image, std::string> readImage(std::string imagePath) {
    image newImg;
    LOG_INFO("reading image: {}", imagePath);

    OIIO::ImageInput::unique_ptr inputImage;
    inputImage = OIIO::ImageInput::open(imagePath);

    if (!inputImage)
    {
        LOG_ERROR("[oiio] Could not read the image file: {}", imagePath);
        LOG_ERROR("[oiio] Error: {}", OIIO::geterror());
        return "Could not open image";
    }
    std::filesystem::path imgP = imagePath;
    newImg.srcFilename = imgP.stem().string();
    newImg.srcPath = imgP.parent_path().string();
    if (newImg.srcFilename == "" || newImg.srcPath == "") {
        LOG_ERROR("Unable to parse image: {}, empty paths", imagePath);
        return "Could not parse image path";
    }

    const OIIO::ImageSpec &inputSpec = inputImage->spec();
    newImg.width = inputSpec.width;
    newImg.height = inputSpec.height;
    newImg.nChannels = inputSpec.nchannels;
    newImg.imgMeta = inputSpec.to_xml();
    newImg.sampleVisible = false;

    newImg.cropBoxX[0] = newImg.width * 0.1;
    newImg.cropBoxY[0] = newImg.height * 0.1;

    newImg.cropBoxX[1] = newImg.width * 0.9;
    newImg.cropBoxY[1] = newImg.height * 0.1;

    newImg.cropBoxX[2] = newImg.width * 0.9;
    newImg.cropBoxY[2] = newImg.height * 0.9;

    newImg.cropBoxX[3] = newImg.width * 0.1;
    newImg.cropBoxY[3] = newImg.height * 0.9;
    newImg.renderBypass = true;

    //readMetadata(inputSpec, &newImg, imgP.stem().string(), imgP.extension().string());
    LOG_INFO("Reading in image with size: {}x{}x{}", newImg.width, newImg.height, newImg.nChannels);

    newImg.rawImgData = new float[newImg.width * newImg.height * 4];
    newImg.procImgData = new float[newImg.width * newImg.height * 4];
    newImg.dispImgData = new uint8_t[newImg.width * newImg.height * 4];
    if(!inputImage->read_image(OIIO::TypeDesc::FLOAT, (void*)newImg.rawImgData))
    {
        LOG_ERROR("[oiio] Failed to read image: {}", imagePath);
        LOG_ERROR("[oiio] Error: {}", inputImage->geterror());
        return "Could not read image";
    }

    inputImage->close();

    // Pad to RGBA
    newImg.padToRGBA();
    // Process Disp Img
    newImg.procDispImg();

    return newImg;
}


renderParams img_to_param(image* _img) {
    renderParams params;

    params.width = _img->width;
    params.height = _img->height;

    params.sigmaFilter = _img->blurAmount;
    params.temp = _img->temp;
    params.tint = _img->tint;

    params.bypass = _img->renderBypass ? 1 : 0;

    for (int i = 0; i < 4; i++) {
        params.baseColor[i] = i == 3 ? 0.0f : _img->baseColor[i];
        params.blackPoint[i] = i == 3 ? 0.0f : _img->blackPoint[i] + _img->blackPoint[3];
        params.whitePoint[i] = i == 3 ? 1.0f : _img->whitePoint[i] * _img->whitePoint[3];
        params.G_blackpoint[i] = i == 3 ? 0.0f : _img->g_blackpoint[i] + _img->g_blackpoint[3];
        params.G_whitepoint[i] = i == 3 ? 1.0f : _img->g_whitepoint[i] * _img->g_whitepoint[3];
        params.G_lift[i] = i == 3 ? 0.0f : _img->g_lift[i] + _img->g_lift[3];
        params.G_gain[i] = i == 3 ? 1.0f : _img->g_gain[i] * _img->g_gain[3];
        params.G_mult[i] = i == 3 ? 1.0f : _img->g_mult[i] * _img->g_mult[3];
        params.G_offset[i] = i == 3 ? 0.0f : _img->g_offset[i] + _img->g_offset[3];
        params.G_gamma[i] = i == 3 ? 1.0f : _img->g_gamma[i] * _img->g_gamma[3];
    }

    //params.baseColor[0] = 0.01f;
    //params.baseColor[1] = 0.01f;
    //params.baseColor[2] = 0.01f;

    //LOG_INFO("Render Params:");
    //LOG_INFO("Base Color: {}, {}, {}, {}", params.baseColor[0], params.baseColor[1], params.baseColor[2], params.baseColor[3]);
    //LOG_INFO("Width: {}, Height: {}", params.width, params.height);
    return params;


}
