#include "image.h"
#include "structs.h"
#include "utils.h"
#include "preferences.h"
#include "lancir.h"

#include <variant>
#include <filesystem>
#include <stdexcept>
#include <string>

#include "ocioProcessor.h"
#include <libraw/libraw.h>

//---Export Pre-Process---//
/*
    Image specific pre-processing step for
    prepping a full render and write out.

    Temporarily store the working width/height
    (for performance mode half-res debayers)

    Reload/Debayer full image in highest quality
    (11 for libraw)

    Allocate necessary buffers for processing
*/
bool image::exportPreProcess(std::string outPath) {
    fullIm = true;
    expFullPath = outPath;
    if (isRawImage) {
        if (imageLoaded){
            clearBuffers();
        }
        if(debayerImage(true, 11)) {
            //allocProcBuf();
            imageLoaded = true;
            return true;
        } else {
            LOG_WARN("Unable to re-debayer image: {}", fullPath);
            return false;
        }
    } else if (isDataRaw) {
        if (dataReload()) {
            //allocProcBuf();
            imageLoaded = true;
            return true;
        } else {
            LOG_WARN("Unable to re-load image: {}", fullPath);
            return false;
        }
    }  else {
        if (oiioReload()) {
            imageLoaded = true;
            //allocProcBuf();
            return true;
        } else {
            LOG_WARN("Unable to re-load image: {}", fullPath);
            return false;
        }
    }
}

//---Export Post-Process---//
/*
    Clear out buffers for performance mode
    Reset the width/height back to working values
*/
void image::exportPostProcess() {
    if (imageLoaded && isRawImage) {
        clearBuffers();
    } else {
        delProcBuf();
    }
    fullIm = false;
    renderReady = false;
}

//---Write Image---//
/*
    Given the provided parameters write
    out the final image from the proc buffer

    Called after metal render is queued up.
    Will wait for render to finish, and then apply
    selected ODT.

    Write metadata to final file after OIIO close
*/
bool image::writeImg(const exportParam param, ocioSetting ocioSet) {
    // Wait for the metal render to finish
    auto start = std::chrono::steady_clock::now();
    while (!renderReady) {
        auto end = std::chrono::steady_clock::now();
        auto dur = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

        if (dur.count() > 15000) {
            // Bailing out after waiting 15 seconds for Metal to finish rendering..
            // At avg of 20-30fps it should never take this long
            LOG_ERROR("Stuck waiting for Metal GPU render. Cannot export file: {}!", srcFilename);
            return false;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
    renderReady = false;

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

    std::string filePath = expFullPath + "/" + srcFilename + fileExt;
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
    OIIO::ImageSpec spec(rawWidth, rawHeight, nChannels, outFormat);


    if (param.format == 2) {
        // Jpeg Compression
        std::string compression = "";
        compression = "jpeg:" + std::to_string(param.quality);
        spec["Compression"] = compression;
    } else if (param.format == 1) {
        // EXR Compression
        spec["Compression"] = "zip";
    } else if (param.format == 3) {
        spec["png:compressionLevel"] = param.compression;
    } else if (param.format == 4) {
        // Tiff Compression
        spec["tiff:zipquality"] = param.compression;
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

    // Write out the metadata
    writeExpMeta(filePath);
    return true;

}


//---Debayer Image---//
/*
    Reload/debayer the image using the given
    quality settings.
*/
bool image::debayerImage(bool fullRes, int quality) {
    imageLoaded = false;
    std::unique_ptr<LibRaw> rawProcessor(new LibRaw);

    // Set parameters for linear output
    rawProcessor->imgdata.params.output_bps = 16;       // 16-bit output
    rawProcessor->imgdata.params.gamm[0] = 1;           // gamma 1.0 for linear output
    rawProcessor->imgdata.params.gamm[1] = 1;
    rawProcessor->imgdata.params.no_auto_bright = 1;    // Disable auto-brightening
    rawProcessor->imgdata.params.use_camera_wb = 1;     // Use camera white balance
    rawProcessor->imgdata.params.output_color = 6;      // Output color space: ACES
    rawProcessor->imgdata.params.user_qual = quality;
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
    rawWidth = processedImage->width;
    rawHeight = processedImage->height;
    nChannels = processedImage->colors;
    if (rawImgData)
        delete[] rawImgData;
    rawImgData = new float[processedImage->width * processedImage->height * 4];

    // Convert to float (assuming 16-bit output)
    if (processedImage->bits == 16 && processedImage->type == LIBRAW_IMAGE_BITMAP && processedImage->colors == 3) {
        unsigned int numThreads = std::thread::hardware_concurrency();
        numThreads = numThreads == 0 ? 2 : numThreads;
        // Create a vector of threads
        std::vector<std::thread> threads(numThreads);
        // Divide the workload into equal parts for each thread
        int rowsPerThread = processedImage->height / numThreads;

        uint16_t* raw_data = reinterpret_cast<uint16_t*>(processedImage->data);
        auto processRows = [&](int startRow, int endRow) {
            float pIn[3] = {0};
            float pOut[3] = {0};
            for (int y=startRow; y<endRow; y++)
            {
                for (int x=0; x<processedImage->width; x++)
                {
                    int index = ((y * processedImage->width) + x) * processedImage->colors;
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
        };
        // Launch the threads
        for (int i=0; i<numThreads; ++i) {
            int startRow = i * rowsPerThread;
            int endRow = (i == numThreads - 1) ? processedImage->height : (i + 1) * rowsPerThread;
            threads[i] = std::thread(processRows, startRow, endRow);
        }

            // Wait for all threads to finish
        for (auto& thread : threads) {
            thread.join();
        }
        padToRGBA();
        if (appPrefs.prefs.perfMode && !fullIm)
            resizeProxy();

        imageLoaded = true;
        // Clean up
        LibRaw::dcraw_clear_mem(processedImage);
        rawProcessor->recycle();
        return true;
    }
    LibRaw::dcraw_clear_mem(processedImage);
    rawProcessor->recycle();
    return false;
}

//---OpenImageIO Reload---//
/*
    Reload the image from disk into the raw buffer
    This needs to occur if performance mode is enabled
    and the image has been unloaded for memory savings.
*/
bool image::oiioReload() {
    OIIO::ImageInput::unique_ptr inputImage;
    inputImage = OIIO::ImageInput::open(fullPath);

    if (!inputImage)
    {
        LOG_ERROR("[oiio] Could not read the image file: {}", srcFilename);
        LOG_ERROR("[oiio] Error: {}", OIIO::geterror());
        return false;
    }

    const OIIO::ImageSpec &inputSpec = inputImage->spec();
    // We don't want to overwrite if image from disk changed
    //width = inputSpec.width;
    //height = inputSpec.height;
    //nChannels = inputSpec.nchannels;
    if (rawImgData)
        delete [] rawImgData;
    rawImgData = new float[rawWidth * rawHeight * 4];
    if(!inputImage->read_image(OIIO::TypeDesc::FLOAT, (void*)rawImgData))
    {
        LOG_ERROR("[oiio] Failed to read image: {}", srcFilename);
        LOG_ERROR("[oiio] Error: {}", inputImage->geterror());
        delete [] rawImgData;
        rawImgData = nullptr;
        return false;
    }

    inputImage->close();

    // Pad to RGBA
    padToRGBA();
    if (appPrefs.prefs.perfMode && !fullIm)
        resizeProxy();


    ocioProc.processImage(rawImgData, width, height, intOCIOSet);
    imageLoaded = true;
    return true;

}

bool image::dataReload() {

    std::ifstream file(fullPath, std::ios::binary | std::ios::ate);
    if (!file) {
        return false;
    }
    // Get total file size
    file.seekg(0, std::ios::end);
    size_t totalSize = file.tellg();
    // Calculate data size (excluding header if present)
    size_t dataSize = intRawSet.pakonHeader ? totalSize - 16 : totalSize;

    // Seek to start of data (skip header if present)
    if (intRawSet.pakonHeader)
        file.seekg(16, std::ios::beg);
    else
        file.seekg(0, std::ios::beg);

    // Allocate buffer for data only
    std::unique_ptr<char[]> buffer(new char[dataSize]);

    // Read the data
    file.read(buffer.get(), dataSize);
    file.close();

    char* raw_ptr = buffer.get();

    float maxValue = static_cast<float>((1 << intRawSet.bitDepth) - 1);
    int bytesPerChannel = (intRawSet.bitDepth > 8) ? (intRawSet.bitDepth > 16 ? 4 : 2) : 1;
    int planeSize = rawWidth * rawHeight * bytesPerChannel;

    if (rawImgData)
        delete [] rawImgData;
    rawImgData = new float[rawWidth * rawHeight * 4];

    // Load in the image and pre-process to Linear AP1
    unsigned int numThreads = std::thread::hardware_concurrency();
    numThreads = numThreads == 0 ? 2 : numThreads;
    // Create a vector of threads
    std::vector<std::thread> threads(numThreads);
    // Divide the workload into equal parts for each thread
    int rowsPerThread = rawHeight / numThreads;
    auto processRows = [&](int startRow, int endRow) {
        float pIn[3] = {0};
        float pOut[3] = {0};

        for (int y = startRow; y < endRow; y++) {
            for (int x = 0; x < rawWidth; x++) {
                // Calculate the starting byte position for this pixel
                // Calculate pixel position
                int pixelIndex = y * rawWidth + x;

                // For interleaved data, calculate the byte offset
                int pixelByteOffset = pixelIndex * nChannels * bytesPerChannel;

                // For float output array
                int floatIndex = pixelIndex * nChannels;


                for (int c = 0; c < nChannels && c < 3; c++) {
                    // Calculate the byte offset for this specific channel
                    int channelByteOffset;
                    channelByteOffset = intRawSet.planar ? (c * planeSize) + (pixelIndex * bytesPerChannel) : pixelByteOffset + (c * bytesPerChannel);

                    if (intRawSet.bitDepth <= 8) {
                        pIn[c] = static_cast<float>(static_cast<unsigned char>(raw_ptr[channelByteOffset])) / maxValue;
                    } else if (intRawSet.bitDepth <= 16) {
                        // For 16-bit, we read from the correct channel offset
                        uint16_t value = *reinterpret_cast<uint16_t*>(&raw_ptr[channelByteOffset]);
                        value = !intRawSet.littleE ? swapBytes16(value) : value;
                        pIn[c] = static_cast<float>(value) / maxValue;
                    } else if (intRawSet.bitDepth <= 32) {
                        // For 32-bit, we read from the correct channel offset
                        uint32_t value = *reinterpret_cast<uint32_t*>(&raw_ptr[channelByteOffset]);
                        value = !intRawSet.littleE ? swapBytes32(value) : value;
                        pIn[c] = static_cast<float>(value) / maxValue;
                    }
                }

                // Apply 2.2 gamma correction
                for (int c = 0; c < nChannels; c++) {
                    pOut[c] = std::pow(pIn[c], 1.0f/2.2f);
                }


                // Write to output
                for (int c = 0; c < nChannels; c++) {
                    rawImgData[floatIndex + c] = pOut[c];
                }

                // Set alpha channel if it exists
                if (nChannels == 4) {
                    rawImgData[floatIndex + 3] = 1.0f;
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

    padToRGBA();
    if (appPrefs.prefs.perfMode && !fullIm)
        resizeProxy();

    ocioProc.processImage(rawImgData, width, height, intOCIOSet);
    imageLoaded = true;
    return true;
}



//--------------IMAGE LOADER FUNCTIONS-----------------//

//---Read Image---//
/*
    Read in an image with a given path.
    Will attempt data-raw first, then camera-raw,
    then OpenImageIO after.

    This prevents OpenImageIO from reading just the
    thumbnail image that may be present in a file.

    Function returns a valid image object set up based
    on the type of image loaded, otherwise an error string
*/
std::variant<image, std::string> readImage(std::string imagePath, rawSetting rawSet, ocioSetting ocioSet) {
    // Attempt data raw
    auto drIm = readDataImage(imagePath, rawSet, ocioSet);
    if (std::holds_alternative<image>(drIm))
        return std::get<image>(drIm);

    // Attempt camera raw
    auto crIm = readRawImage(imagePath);
    if (std::holds_alternative<image>(crIm))
        return std::get<image>(crIm);

    // Finally, attempt OpenImageIO
    return readImageOIIO(imagePath, ocioSet);
}

//---Read File Raw Image---//
/*
    Attempt to load a raw-data file (Pakon)
    with the given image specifications
    (width, height, channels, bit depth)

    Applies a gamma 2.2 transform, and selected IDT
    returns an image object if successful, error string otherwise
*/

std::variant<image, std::string> readDataImage(std::string imagePath, rawSetting rawSet, ocioSetting ocioSet) {

    std::filesystem::path imgP(imagePath);
    if (imgP.extension().string() == ".raw" ||
        imgP.extension().string() == ".RAW") {

        image img;
        img.srcFilename = imgP.stem().string();
        img.srcPath = imgP.parent_path().string();
        img.fullPath = imagePath;

        try {
            img.width = rawSet.width;
            img.height = rawSet.height;
            img.rawWidth = rawSet.width;
            img.rawHeight = rawSet.height;
            img.nChannels = rawSet.channels;
            long expByteCount = img.width * img.height * img.nChannels;
            expByteCount *= (rawSet.bitDepth / 8);

            if (std::filesystem::file_size(imgP) < expByteCount) {
                // File is too small for dimensions
                throw std::runtime_error("Error: Filesize less than expected! Skipping");
            }
            if (std::filesystem::file_size(imgP) == expByteCount) {
                // File is exactly right (without header)
                rawSet.pakonHeader = false;
            }
            if (std::filesystem::file_size(imgP) == expByteCount + 16) {
                // File has Pakon header
                rawSet.pakonHeader = true;
            } else {
                // We've got the wrong dimensions
                throw std::runtime_error("Error: Incorrect dimensions for RAW file! Skipping");
            }

            img.rawImgData = new float[img.width * img.height * 4];
            // Load in the image and pre-process to Linear AP1
            unsigned int numThreads = std::thread::hardware_concurrency();
            numThreads = numThreads == 0 ? 2 : numThreads;
            // Create a vector of threads
            std::vector<std::thread> threads(numThreads);
            // Divide the workload into equal parts for each thread
            int rowsPerThread = img.height / numThreads;

            std::ifstream file(imagePath, std::ios::binary | std::ios::ate);
            if (!file) {
                delete [] img.rawImgData;
                throw std::runtime_error("Error: Unable to open input file: " + imagePath);
            }

            // Get total file size
            file.seekg(0, std::ios::end);
            size_t totalSize = file.tellg();

            // Calculate data size (excluding header if present)
            size_t dataSize = rawSet.pakonHeader ? totalSize - 16 : totalSize;

            // Seek to start of data (skip header if present)
            if (rawSet.pakonHeader)
                file.seekg(16, std::ios::beg);
            else
                file.seekg(0, std::ios::beg);

            // Allocate buffer for data only
            std::unique_ptr<char[]> buffer(new char[dataSize]);

            // Read the data
            file.read(buffer.get(), dataSize);
            file.close();

            char* raw_ptr = buffer.get();

            float maxValue = static_cast<float>((1 << rawSet.bitDepth) - 1);
            int bytesPerChannel = (rawSet.bitDepth > 8) ? (rawSet.bitDepth > 16 ? 4 : 2) : 1;
            int planeSize = img.width * img.height * bytesPerChannel;

            auto processRows = [&](int startRow, int endRow) {
                float pIn[3] = {0};
                float pOut[3] = {0};

                for (int y = startRow; y < endRow; y++) {
                    for (int x = 0; x < img.width; x++) {
                        // Calculate the starting byte position for this pixel
                        // Calculate pixel position
                        int pixelIndex = y * img.width + x;

                        // For interleaved data, calculate the byte offset
                        int pixelByteOffset = pixelIndex * img.nChannels * bytesPerChannel;

                        // For float output array
                        int floatIndex = pixelIndex * img.nChannels;


                        for (int c = 0; c < img.nChannels && c < 3; c++) {
                            // Calculate the byte offset for this specific channel
                            int channelByteOffset;
                            channelByteOffset = rawSet.planar ? (c * planeSize) + (pixelIndex * bytesPerChannel) : pixelByteOffset + (c * bytesPerChannel);

                            if (rawSet.bitDepth <= 8) {
                                pIn[c] = static_cast<float>(static_cast<unsigned char>(raw_ptr[channelByteOffset])) / maxValue;
                            } else if (rawSet.bitDepth <= 16) {
                                // For 16-bit, we read from the correct channel offset
                                uint16_t value = *reinterpret_cast<uint16_t*>(&raw_ptr[channelByteOffset]);
                                value = !rawSet.littleE ? swapBytes16(value) : value;
                                pIn[c] = static_cast<float>(value) / maxValue;
                            } else if (rawSet.bitDepth <= 32) {
                                // For 32-bit, we read from the correct channel offset
                                uint32_t value = *reinterpret_cast<uint32_t*>(&raw_ptr[channelByteOffset]);
                                value = !rawSet.littleE ? swapBytes32(value) : value;
                                pIn[c] = static_cast<float>(value) / maxValue;
                            }
                        }
                        if (x == 0 && y == 0) {
                            LOG_INFO("First pixel values:");
                            for (int c = 0; c < img.nChannels; c++) {
                                LOG_INFO("Channel {}: {}", c, pIn[c]);
                            }
                        }

                        // Apply 2.2 gamma correction
                        for (int c = 0; c < img.nChannels; c++) {
                            pOut[c] = std::pow(pIn[c], 1.0f/2.2f);
                        }


                        // Write to output
                        for (int c = 0; c < img.nChannels; c++) {
                            img.rawImgData[floatIndex + c] = pOut[c];
                        }

                        // Set alpha channel if it exists
                        if (img.nChannels == 4) {
                            img.rawImgData[floatIndex + 3] = 1.0f;
                        }
                    }
                }
            };
            // Launch the threads
            for (int i=0; i<numThreads; ++i) {
                int startRow = i * rowsPerThread;
                int endRow = (i == numThreads - 1) ? img.height : (i + 1) * rowsPerThread;
                threads[i] = std::thread(processRows, startRow, endRow);
            }

            // Wait for all threads to finish
            for (auto& thread : threads) {
                thread.join();
            }

            // Image is good
            img.renderBypass = true;
            img.imageLoaded = true;
            img.padToRGBA();
            if (appPrefs.prefs.perfMode)
                img.resizeProxy();
            img.setCrop();
            img.intOCIOSet = ocioSet;
            ocioProc.processImage(img.rawImgData, img.width, img.height, img.intOCIOSet);
            img.imageLoaded = true;
            img.isDataRaw = true;
            img.isRawImage = false;
            return img;


        } catch(const std::exception& e) {
            LOG_ERROR("Error decoding raw image: {}", e.what());
            return "Unable to decode raw-data image";
        }

    }
    return "Not a data-raw image, continuing";

}




//---Read Camera Raw Image---//
/*
    Attempt to read in a camera raw file and debayer it
    using the LibRaw library.
    Will read half-size and lower quality if performance
    mode is enabled.

    Returns an image object if successful, error string otherwise
*/
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
    rawProcessor->imgdata.params.user_qual = appPrefs.prefs.perfMode ? 2 : 11;
    rawProcessor->imgdata.params.half_size = appPrefs.prefs.perfMode ? 1 : 0;

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
    img.rawWidth = processedImage->width;
    img.rawHeight = processedImage->height;
    img.nChannels = processedImage->colors;
    img.rawImgData = new float[img.width * img.height * 4];

    // Convert to float (assuming 16-bit output)
    if (processedImage->bits == 16 && processedImage->type == LIBRAW_IMAGE_BITMAP && processedImage->colors == 3) {
        unsigned int numThreads = std::thread::hardware_concurrency();
        numThreads = numThreads == 0 ? 2 : numThreads;
        // Create a vector of threads
        std::vector<std::thread> threads(numThreads);
        // Divide the workload into equal parts for each thread
        int rowsPerThread = processedImage->height / numThreads;

        uint16_t* raw_data = reinterpret_cast<uint16_t*>(processedImage->data);
        auto processRows = [&](int startRow, int endRow) {
            float pIn[3] = {0};
            float pOut[3] = {0};
            for (int y=startRow; y<endRow; y++)
            {
                for (int x=0; x<processedImage->width; x++)
                {
                    int index = ((y * processedImage->width) + x) * processedImage->colors;
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
        };
        // Launch the threads
        for (int i=0; i<numThreads; ++i) {
            int startRow = i * rowsPerThread;
            int endRow = (i == numThreads - 1) ? processedImage->height : (i + 1) * rowsPerThread;
            threads[i] = std::thread(processRows, startRow, endRow);
        }

        // Wait for all threads to finish
        for (auto& thread : threads) {
            thread.join();
        }

    }
auto d1 = std::chrono::steady_clock::now();

    img.imgParam.cropBoxX[0] = img.width * 0.1;
    img.imgParam.cropBoxY[0] = img.height * 0.1;

    img.imgParam.cropBoxX[1] = img.width * 0.9;
    img.imgParam.cropBoxY[1] = img.height * 0.1;

    img.imgParam.cropBoxX[2] = img.width * 0.9;
    img.imgParam.cropBoxY[2] = img.height * 0.9;

    img.imgParam.cropBoxX[3] = img.width * 0.1;
    img.imgParam.cropBoxY[3] = img.height * 0.9;
    img.renderBypass = true;
    img.imageLoaded = true;

    // Pad to RGBA
    img.padToRGBA();
    if (appPrefs.prefs.perfMode)
        img.resizeProxy();
    img.setCrop();
auto e1 = std::chrono::steady_clock::now();

auto f1 = std::chrono::steady_clock::now();

    // Read metadata
    img.readMetaFromFile();


    // Clean up
    LibRaw::dcraw_clear_mem(processedImage);
    rawProcessor->recycle();
    img.isDataRaw = false;
    img.isRawImage = true;
auto end = std::chrono::steady_clock::now();

auto durA = std::chrono::duration_cast<std::chrono::microseconds>(a1 - start);
auto durB = std::chrono::duration_cast<std::chrono::microseconds>(b1 - a1);
auto durC = std::chrono::duration_cast<std::chrono::microseconds>(c1 - b1);
auto durD = std::chrono::duration_cast<std::chrono::microseconds>(d1 - c1);
auto durE = std::chrono::duration_cast<std::chrono::microseconds>(e1 - d1);
auto durF = std::chrono::duration_cast<std::chrono::microseconds>(f1 - e1);
auto durG = std::chrono::duration_cast<std::chrono::microseconds>(end - f1);
auto durH = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
LOG_INFO("-----------Debayer Time-----------");
//LOG_INFO("Open:     {:*>8}μs | {:*>8}ms", durA.count(), durA.count()/1000);
//LOG_INFO("Unpack:   {:*>8}μs | {:*>8}ms", durB.count(), durB.count()/1000);
//LOG_INFO("Process:  {:*>8}μs | {:*>8}ms", durC.count(), durC.count()/1000);
//LOG_INFO("Convert:  {:*>8}μs | {:*>8}ms", durD.count(), durD.count()/1000);
//LOG_INFO("Pad:      {:*>8}μs | {:*>8}ms", durE.count(), durE.count()/1000);
//LOG_INFO("Disp:     {:*>8}μs | {:*>8}ms", durF.count(), durF.count()/1000);
//LOG_INFO("Clear:    {:*>8}μs | {:*>8}ms", durG.count(), durG.count()/1000);
LOG_INFO("Total:    {:*>8}μs | {:*>8}ms", durH.count(), durH.count()/1000);
//LOG_INFO("----------------------------------");
    return img;
}


//---Read Image OpenImageIO---//
/*
    Attempts to read in the selected file using
    OpenImageIO. Applies the user-selected IDT settings
    to the image during opening.

    Returns an image object if successful, error string otherwise
*/
std::variant<image, std::string> readImageOIIO(std::string imagePath, ocioSetting ocioSet) {
    image img;

    OIIO::ImageInput::unique_ptr inputImage;
    inputImage = OIIO::ImageInput::open(imagePath);

    if (!inputImage)
    {
        LOG_ERROR("[oiio] Could not read the image file: {}", imagePath);
        LOG_ERROR("[oiio] Error: {}", OIIO::geterror());
        return "Could not open image: " + imagePath;
    }

    std::filesystem::path imgP = imagePath;
    img.srcFilename = imgP.stem().string();
    img.srcPath = imgP.parent_path().string();
    img.fullPath = imagePath;
    if (img.srcFilename == "" || img.srcPath == "") {
        LOG_ERROR("Unable to parse image: {}, empty paths", imagePath);
        return "Could not parse image path";
    }

    const OIIO::ImageSpec &inputSpec = inputImage->spec();
    img.width = inputSpec.width;
    img.height = inputSpec.height;
    img.rawWidth = inputSpec.width;
    img.rawHeight = inputSpec.height;
    img.nChannels = inputSpec.nchannels;

    img.imgParam.cropBoxX[0] = img.width * 0.1;
    img.imgParam.cropBoxY[0] = img.height * 0.1;

    img.imgParam.cropBoxX[1] = img.width * 0.9;
    img.imgParam.cropBoxY[1] = img.height * 0.1;

    img.imgParam.cropBoxX[2] = img.width * 0.9;
    img.imgParam.cropBoxY[2] = img.height * 0.9;

    img.imgParam.cropBoxX[3] = img.width * 0.1;
    img.imgParam.cropBoxY[3] = img.height * 0.9;
    img.renderBypass = true;

    LOG_INFO("Reading in image with size: {}x{}x{}", img.width, img.height, img.nChannels);

    img.rawImgData = new float[img.width * img.height * 4];
    if(!inputImage->read_image(OIIO::TypeDesc::FLOAT, (void*)img.rawImgData))
    {
        LOG_ERROR("[oiio] Failed to read image: {}", imagePath);
        LOG_ERROR("[oiio] Error: {}", inputImage->geterror());
        delete [] img.rawImgData;
        return "Could not read image";
    }

    inputImage->close();

    // Pad to RGBA
    img.padToRGBA();
    if (appPrefs.prefs.perfMode)
        img.resizeProxy();
    img.setCrop();

    // Read metadata
    img.readMetaFromFile();

    // Process IDT
    img.intOCIOSet = ocioSet;
    ocioProc.processImage(img.rawImgData, img.width, img.height, img.intOCIOSet);
    img.imageLoaded = true;
    img.isDataRaw = false;
    img.isRawImage = false;

    return img;
}
