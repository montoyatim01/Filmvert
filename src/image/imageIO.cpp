#include "image.h"
#include "structs.h"
#include "utils.h"
#include "preferences.h"
#include "lancir.h"

#include <OpenImageIO/imagebuf.h>
#include <OpenImageIO/imagebufalgo.h>
#include <OpenImageIO/imageio.h>
#include <OpenImageIO/filesystem.h>
#include <cstddef>
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
    (pref for libraw)

    Allocate necessary buffers for processing
*/
bool image::exportPreProcess(std::string outPath, int exportImgCount) {
    fullIm = true;
    expFullPath = outPath;
    renderReady = false;
    activeExpCount = exportImgCount;
    // Clear display flags
    showClip = false;
    channelView = 0;
    gradeBypass = false;
    secEnable = 0xFFFFFFFF;
    if (isRawImage) {
        if (imageLoaded){
            clearBuffers();
        }
        if(debayerImage(true, appPrefs.prefs.debayerMode)) {
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
    if (imageLoaded) {
        clearBuffers();
    } else {
        delProcBuf();
    }
    fullIm = false;
    renderReady = false;
    imgRst = true;
    needRndr = true;
    activeExpCount = 1;
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

    OIIO::ImageSpec inspec(rndrW, rndrH, nChannels, OIIO::TypeDesc::FLOAT);

    // Trim proc data for save
    allocateTmpBuf();
    trimForSave();

    imgParam.writeRotation = imgParam.rotation;

    // Apply the orientation to the image
    OIIO::ImageBuf srcBuf(inspec, tmpOutData);
    OIIO::ImageBuf* finalBuf = &srcBuf;
    OIIO::ImageBuf* writeBuf = &srcBuf;
    OIIO::ImageBuf reorientedBuf;
    OIIO::ImageBuf greyscaleBuf;

    if (applyCrops && imgParam.writeRotation != 1) {
        // Set the orientation metadata in the source buffer
        srcBuf.specmod()["Orientation"] = imgParam.writeRotation;
        // Apply reorientation
        if (!OIIO::ImageBufAlgo::reorient(reorientedBuf, srcBuf)) {
            LOG_ERROR("Failed to reorient image: {}", OIIO::geterror());
        } else {
            imgParam.writeRotation = 1;
        }
        finalBuf = &reorientedBuf;

    }

    OIIO::ImageBuf borderImg;
    if (param.border) {
        // Add a border to the rendered image
        int borderSize = (int)((float)finalBuf->spec().width * param.borderSize);
        int newWidth = finalBuf->spec().width + (2 * borderSize);
        int newHeight = finalBuf->spec().height + (2 * borderSize);
        imgMeta.borderPercentage = param.borderSize;

        OIIO::ImageSpec borderSpec = finalBuf->spec();
        borderSpec.width = newWidth;
        borderSpec.height = newHeight;
        borderImg = OIIO::ImageBuf(borderSpec);
        if (OIIO::ImageBufAlgo::fill(borderImg, param.borderColor)) {
            if(OIIO::ImageBufAlgo::paste(borderImg, borderSize, borderSize, 0, 0, *finalBuf)) {
                finalBuf = &borderImg;
            } else {
                LOG_ERROR("Unable to add image to border: {}", OIIO::geterror(true));
            }
        } else {
            LOG_ERROR("Unable to add image border: {}", OIIO::geterror(true));
        }
    }
    OIIO::ImageBuf resizeImg;
    if (param.resize) {
        int newWidth, newHeight;
        if (param.fixedSize) {
            // Scale image to fixed size
            if (param.longSide) {
                // Setting the long side
                if (finalBuf->spec().width > finalBuf->spec().height) {
                    // Width is the long side
                    newWidth = param.fixedSizePx;
                    newHeight = (int) ( ((float)newWidth / (float)finalBuf->spec().width) * (float)finalBuf->spec().height);
                } else {
                    // Height is the long side
                    newHeight = param.fixedSizePx;
                    newWidth = (int) ( ((float)newHeight / (float)finalBuf->spec().height) * (float)finalBuf->spec().width);
                }
            } else {
                // Setting the short side
                if (finalBuf->spec().width > finalBuf->spec().height) {
                    // Width is the long side, set the height to value
                    newHeight = param.fixedSizePx;
                    newWidth = (int) ( ((float)newHeight / (float)finalBuf->spec().height) * (float)finalBuf->spec().width);
                } else {
                    // Height is the long side, set the width to value
                    newWidth = param.fixedSizePx;
                    newHeight = (int) ( ((float)newWidth / (float)finalBuf->spec().width) * (float)finalBuf->spec().height);
                }
            }
        } else {
            // Scale image based on percentage
            newWidth = (int)((float)finalBuf->spec().width * (param.scaleSize * 0.01f));
            newHeight = (int)((float)finalBuf->spec().height * (param.scaleSize * 0.01f));
        }

        OIIO::ImageSpec resizeSpec(newWidth, newHeight,
                                   finalBuf->spec().nchannels,
                                   finalBuf->spec().format);
        resizeImg = OIIO::ImageBuf(resizeSpec);
        if (OIIO::ImageBufAlgo::resize(resizeImg, *finalBuf)) {
            finalBuf = &resizeImg;
        } else {
            LOG_ERROR("Unable to resize image {}: {}", srcFilename, OIIO::geterror(true));
        }
    }



    if (param.format == 2) {
        // Jpeg Compression
        std::string compression = "";
        compression = "jpeg:" + std::to_string(param.quality);
        finalBuf->specmod()["Compression"] = compression;
    } else if (param.format == 1) {
        // EXR Compression
        finalBuf->specmod()["Compression"] = "zip";
        updateMetaStr();
        finalBuf->specmod().attribute("filmvert", jsonMeta);
    } else if (param.format == 3) {
        finalBuf->specmod()["png:compressionLevel"] = param.compression;
    } else if (param.format == 4) {
        // Tiff Compression
        finalBuf->specmod()["tiff:zipquality"] = param.compression;
    }

    writeBuf = finalBuf;
    if (param.greyscale) {
        greyscaleBuf = OIIO::ImageBufAlgo::channels(*finalBuf, 1, {0}, {}, {"Y"});
        writeBuf = &greyscaleBuf;
    }


    // Write Image Data
    if (!writeBuf->write(filePath, outFormat)) {
        LOG_ERROR("Failed to write image: {}", OIIO::geterror());
        return false;
    }

    clearTmpBuf();

    // Write out the metadata
    writeExpMeta(filePath);

    LOG_INFO("Completed write for {}", srcFilename);

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

    // Open the raw file from buffer if loaded, otherwise file
    int result = -1;
    if (fileLoaded) {
        // Open from buffer
        result = rawProcessor->open_buffer(fileBuffer.data(), fileBuffer.size());
        if (result != LIBRAW_SUCCESS) {
            // We failed with the buffer, fallback to file
            rawProcessor->recycle();

            // Set parameters for linear output
            rawProcessor->imgdata.params.output_bps = 16;       // 16-bit output
            rawProcessor->imgdata.params.gamm[0] = 1;           // gamma 1.0 for linear output
            rawProcessor->imgdata.params.gamm[1] = 1;
            rawProcessor->imgdata.params.no_auto_bright = 1;    // Disable auto-brightening
            rawProcessor->imgdata.params.use_camera_wb = 1;     // Use camera white balance
            rawProcessor->imgdata.params.output_color = 6;      // Output color space: ACES
            rawProcessor->imgdata.params.user_qual = quality;
            rawProcessor->imgdata.params.half_size = !fullRes;

            result = rawProcessor->open_file(fullPath.c_str());
            if (result != LIBRAW_SUCCESS) {
                LOG_WARN("Error opening buffer and file: {}", fullPath);
                LOG_ERROR("{}", libraw_strerror(result));
                return false;
            }
        }
    } else {
        // Open from file
        result = rawProcessor->open_file(fullPath.c_str());
    }
    if (result != LIBRAW_SUCCESS) {
        LOG_WARN("Error opening file: {}", fullPath);
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
        return false;
    }

    // Fill raw buffer
    rawWidth = processedImage->width;
    rawHeight = processedImage->height;
    nChannels = processedImage->colors;
    if (rawImgData)
        delete[] rawImgData;
    rawImgData = new float[processedImage->width * processedImage->height * 4];
    rawBufSize = processedImage->width * processedImage->height * 4 * sizeof(float);

    // Convert to float (assuming 16-bit output)
    if (processedImage->bits == 16 && processedImage->type == LIBRAW_IMAGE_BITMAP) {
        unsigned int numThreads = 2;//std::thread::hardware_concurrency();
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
                    pIn[1] = processedImage->colors > 1 ? static_cast<float>(raw_data[index + 1]) / 65535.0f : pIn[0];
                    pIn[2] = processedImage->colors > 2 ? static_cast<float>(raw_data[index + 2]) / 65535.0f : pIn[0];
                    if (processedImage->colors == 3)
                        ap0_to_ap1(pIn, pOut);
                    else {
                        pOut[0] = pIn[0];
                        pOut[1] = pIn[1];
                        pOut[2] = pIn[2];
                    }
                    rawImgData[index + 0] = pOut[0] * 2.0f;
                    rawImgData[index + 1] = pOut[1] * 2.0f;
                    rawImgData[index + 2] = pOut[2] * 2.0f;
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
        needRndr = true;
        // Clean up
        LibRaw::dcraw_clear_mem(processedImage);
        rawProcessor->recycle();

        // Gamut Compression
        if (imgParam.gamutComp)
            ocioProc.refGamutCompress(rawImgData, rawWidth, rawHeight);
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
    std::unique_ptr<OIIO::Filesystem::IOMemReader> memReader;

    if (fileLoaded) {
        memReader = std::make_unique<OIIO::Filesystem::IOMemReader>(
                fileBuffer.data(),
                fileBuffer.size()
            );
        OIIO::ImageSpec memSpec;
        inputImage = OIIO::ImageInput::open(fullPath, &memSpec, memReader.get());
        if (!inputImage) {
            // We've failed loading the buffer, try from disk
            inputImage = OIIO::ImageInput::open(fullPath);
        }
    } else {
        inputImage = OIIO::ImageInput::open(fullPath);
    }

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
    if (rawImgData) {
        delete [] rawImgData;
        rawImgData = nullptr;
    }

    rawImgData = new float[rawWidth * rawHeight * 4];
    rawBufSize = rawWidth * rawHeight * 4 * sizeof(float);
    if(!inputImage->read_image(0, 0, 0, nChannels, OIIO::TypeDesc::FLOAT, (void*)rawImgData))
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

    if (appPrefs.prefs.perfMode && !fullIm)
        ocioProc.processImage(rawImgData, width, height, intOCIOSet);
    else
        ocioProc.processImage(rawImgData, rawWidth, rawHeight, intOCIOSet);
    // Gamut Compression
    if (imgParam.gamutComp)
        ocioProc.refGamutCompress(rawImgData, fullIm ? rawWidth : width, fullIm ? rawHeight : height);
    imageLoaded = true;
    needRndr = true;
    return true;

}

bool image::dataReload() {

    if (!fileLoaded) {
        loadFileintoBuffer();
    }
    if (!fileLoaded || fileBuffer.size() < 24) {
        LOG_ERROR("Unable to reload file: {}", srcFilename);
        return false;
    }
    // File is loaded
    size_t totalSize = fileBuffer.size();
    // Calculate data size (excluding header if present)
    size_t dataSize = intRawSet.pakonHeader ? totalSize - 16 : totalSize;

    // Seek to start of data (skip header if present)
    int offset = intRawSet.pakonHeader ? 16 : 0;
    char* raw_ptr = fileBuffer.data() + offset;

    float maxValue = static_cast<float>((1 << intRawSet.bitDepth) - 1);
    int bytesPerChannel = (intRawSet.bitDepth > 8) ? (intRawSet.bitDepth > 16 ? 4 : 2) : 1;
    int planeSize = rawWidth * rawHeight * bytesPerChannel;

    if (rawImgData) {
        delete [] rawImgData;
        rawImgData = nullptr;
    }

    rawImgData = new float[rawWidth * rawHeight * 4];
    rawBufSize = rawWidth * rawHeight * 4 * sizeof(float);

    // Load in the image and pre-process to Linear AP1
    unsigned int numThreads = 2;//std::thread::hardware_concurrency();
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
        int endRow = (i == numThreads - 1) ? rawHeight : (i + 1) * rowsPerThread;
        threads[i] = std::thread(processRows, startRow, endRow);
    }

    // Wait for all threads to finish
    for (auto& thread : threads) {
        thread.join();
    }

    padToRGBA();
    width = rawWidth;
    height = rawHeight;
    if (appPrefs.prefs.perfMode && !fullIm)
        resizeProxy();

    ocioProc.processImage(rawImgData, width, height, intOCIOSet);

    // Gamut Compression
    if (imgParam.gamutComp)
        ocioProc.refGamutCompress(rawImgData, width, height);

    imageLoaded = true;
    needRndr = true;
    if (!appPrefs.prefs.holdFilesinRAM)
        unloadFileBuffer();
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
std::variant<image, std::string> readImage(std::string imagePath, rawSetting rawSet, ocioSetting ocioSet, bool background) {
    // Attempt data raw
    auto drIm = readDataImage(imagePath, rawSet, ocioSet, background);
    if (std::holds_alternative<image>(drIm))
        return std::get<image>(drIm);

    // Attempt camera raw
    auto crIm = readRawImage(imagePath, background);
    if (std::holds_alternative<image>(crIm))
        return std::get<image>(crIm);

    // Finally, attempt OpenImageIO
    return readImageOIIO(imagePath, ocioSet, background);
}

//---Read File Raw Image---//
/*
    Attempt to load a raw-data file (Pakon)
    with the given image specifications
    (width, height, channels, bit depth)

    Applies a gamma 2.2 transform, and selected IDT
    returns an image object if successful, error string otherwise
*/

std::variant<image, std::string> readDataImage(std::string imagePath, rawSetting rawSet, ocioSetting ocioSet, bool background) {

    std::filesystem::path imgP(imagePath);
    if (imgP.extension().string() == ".raw" ||
        imgP.extension().string() == ".RAW") {

        image img;
        img.srcFilename = imgP.stem().string();
        img.srcPath = imgP.parent_path().string();
        img.fullPath = imagePath;
        img.imgMeta.fileName = img.srcFilename + imgP.extension().string();
        img.imgMeta.filePath = img.srcPath;

        try {
            long fileSize = std::filesystem::file_size(imagePath);
            checkRawFile(rawSet, fileSize);
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
            else if (std::filesystem::file_size(imgP) == expByteCount + 16) {
                // File has Pakon header
                rawSet.pakonHeader = true;
            } else {
                // We've got the wrong dimensions
                throw std::runtime_error("Error: Incorrect dimensions for RAW file! Skipping");
            }
            // Image size is good
            if (background) {
                // We don't want to actually load the image. Just
                // fill in the struct and return
                img.renderBypass = true;
                img.imageLoaded = false;
                if (appPrefs.prefs.perfMode)
                    img.calcProxyDim();
                img.setCrop();
                img.readMetaFromFile();
                img.intOCIOSet = ocioSet;
                img.isDataRaw = true;
                img.isRawImage = false;
                img.updateSaveState();
                return img;
            }

            img.rawImgData = new float[img.width * img.height * 4];
            img.rawBufSize = img.width * img.height * 4 * sizeof(float);
            // Load in the image and pre-process to Linear AP1
            unsigned int numThreads = 2;//std::thread::hardware_concurrency();
            numThreads = numThreads == 0 ? 2 : numThreads;
            // Create a vector of threads
            std::vector<std::thread> threads(numThreads);
            // Divide the workload into equal parts for each thread
            int rowsPerThread = img.height / numThreads;

            img.loadFileintoBuffer();
            if (!img.fileLoaded || img.fileBuffer.size() < 24) {
                delete [] img.rawImgData;
                img.rawImgData = nullptr;
                throw std::runtime_error("Error: Unable to open input file: " + imagePath);
            }
            img.calculateHash();
            img.intRawSet = rawSet;
            // Get total file size
            size_t totalSize = img.fileBuffer.size();

            // Calculate data size (excluding header if present)
            size_t dataSize = rawSet.pakonHeader ? totalSize - 16 : totalSize;

            // Seek to start of data (skip header if present)
            int offset = rawSet.pakonHeader ? 16 : 0;
            char* raw_ptr = img.fileBuffer.data() + offset;

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
            img.readMetaFromFile();
            img.intOCIOSet = ocioSet;
            if (!img.imgParam.ocioName.empty() && !ocioSet.impOverwrite) {
                // If there was ocio data in the image metadata
                bool goodConfig = false;
                std::vector<std::string> configList = ocioProc.getConfigNames();
                for (int i = 0; i < configList.size(); i++) {
                    if (img.imgParam.ocioName == configList[i]) {
                        goodConfig = true;
                        img.intOCIOSet.ocioConfig = i;
                    }
                }
                if (goodConfig) {
                    img.intOCIOSet.colorspace = img.imgParam.ocioColor;
                    img.intOCIOSet.display = img.imgParam.ocioDisp;
                    img.intOCIOSet.view = img.imgParam.ocioView;
                    img.intOCIOSet.useDisplay = img.imgParam.useDisplay;
                    img.intOCIOSet.inverse = img.imgParam.inverse;
                    img.intOCIOSet.gamutComp = img.imgParam.gamutComp;
                } else {
                    LOG_WARN("Unable to locate config used by image, falling back to current selected config!");
                }

            } else {
                img.imgParam.ocioName = ocioProc.activeConfig()->config->getName();
                img.imgParam.ocioColor = img.intOCIOSet.colorspace;
                img.imgParam.ocioDisp = img.intOCIOSet.display;
                img.imgParam.ocioView = img.intOCIOSet.view;
                img.imgParam.useDisplay = img.intOCIOSet.useDisplay;
                img.imgParam.inverse = img.intOCIOSet.inverse;
                img.imgParam.gamutComp = appPrefs.prefs.gamutComp;
            }
            ocioProc.processImage(img.rawImgData, img.width, img.height, img.intOCIOSet);

            // Gamut Compression
            if (img.imgParam.gamutComp)
                ocioProc.refGamutCompress(img.rawImgData, img.width, img.height);

            img.imageLoaded = true;
            img.isDataRaw = true;
            img.isRawImage = false;
            img.updateSaveState();
            if (!appPrefs.prefs.holdFilesinRAM)
                img.unloadFileBuffer();
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
std::variant<image, std::string> readRawImage(std::string imagePath, bool background) {
    auto start = std::chrono::steady_clock::now();
    image img;

    std::filesystem::path imgP = imagePath;
    img.srcFilename = imgP.stem().string();
    img.srcPath = imgP.parent_path().string();
    img.fullPath = imagePath;
    img.imgMeta.fileName = img.srcFilename + imgP.extension().string();
    img.imgMeta.filePath = img.srcPath;
    std::string fileExtension = imgP.extension().string();
    std::transform(fileExtension.begin(), fileExtension.end(), fileExtension.begin(),
        [](unsigned char c){ return std::tolower(c); });

    if (fileExtension == ".tif" || fileExtension == ".tiff" ||
        fileExtension == ".jpeg" || fileExtension == ".jpg" ||
        fileExtension == ".png" || fileExtension == ".dpx" ||
        fileExtension == ".exr" || fileExtension == ".bmp" ||
        fileExtension == ".cin" || fileExtension == ".heic" ||
        fileExtension == ".avif" || fileExtension == ".heif" ||
        fileExtension == ".tga" || fileExtension == ".webp") {
        // Ignore all of these non-raw image extensions
        return "non-raw image";
    }

    // Create a LibRaw processor instance
    std::unique_ptr<LibRaw> rawProcessor(new LibRaw);

    // Set parameters for linear output
    rawProcessor->imgdata.params.output_bps = 16;       // 16-bit output
    rawProcessor->imgdata.params.gamm[0] = 1;         // Set gamma to 1.0 for linear output
    rawProcessor->imgdata.params.gamm[1] = 1;
    rawProcessor->imgdata.params.no_auto_bright = 1;    // Disable auto-brightening
    rawProcessor->imgdata.params.use_camera_wb = 1;     // Use camera white balance
    rawProcessor->imgdata.params.output_color = 6;      // Output color space: 1 = sRGB
    rawProcessor->imgdata.params.user_qual = appPrefs.prefs.perfMode ? 2 : appPrefs.prefs.debayerMode;
    rawProcessor->imgdata.params.half_size = appPrefs.prefs.perfMode ? 1 : 0;

    // Open the raw file - check if valid
    int result = rawProcessor->open_file(imagePath.c_str());
    if (result != LIBRAW_SUCCESS) {
        LOG_ERROR("Error opening file: {}", imagePath);
        LOG_ERROR("{}", libraw_strerror(result));
        return "Error opening file";
    }
    // We have a valid raw file, load into memory
    // and calculate hash
    img.loadFileintoBuffer();
    if (!img.fileLoaded) {
        // We've failed to load the file into memory.
        // Skip hasing and raw processing from buffer.
    } else {
        rawProcessor->recycle();

        // Set parameters for linear output
        rawProcessor->imgdata.params.output_bps = 16;       // 16-bit output
        rawProcessor->imgdata.params.gamm[0] = 1;         // Set gamma to 1.0 for linear output
        rawProcessor->imgdata.params.gamm[1] = 1;
        rawProcessor->imgdata.params.no_auto_bright = 1;    // Disable auto-brightening
        rawProcessor->imgdata.params.use_camera_wb = 1;     // Use camera white balance
        rawProcessor->imgdata.params.output_color = 6;      // Output color space: 1 = sRGB
        rawProcessor->imgdata.params.user_qual = appPrefs.prefs.perfMode ? 2 : appPrefs.prefs.debayerMode;
        rawProcessor->imgdata.params.half_size = appPrefs.prefs.perfMode ? 1 : 0;

        result = rawProcessor->open_buffer(img.fileBuffer.data(), img.fileBuffer.size());
        if (result != LIBRAW_SUCCESS) {
            LOG_ERROR("Error opening file buffer: {}", imagePath);
            LOG_ERROR("{}", libraw_strerror(result));
            return "Error opening file buffer";
        }
        img.calculateHash();
    }
    if (background) {
        int rwidth = rawProcessor->imgdata.sizes.width;
        int rHeight = rawProcessor->imgdata.sizes.height;

        img.width = appPrefs.prefs.perfMode ? rwidth / 2 : rwidth;
        img.height = appPrefs.prefs.perfMode ? rHeight / 2 : rHeight;
        img.rawWidth = appPrefs.prefs.perfMode ? rwidth / 2 : rwidth;
        img.rawHeight = appPrefs.prefs.perfMode ? rHeight / 2 : rHeight;
        img.nChannels = rawProcessor->imgdata.idata.colors;
        if (appPrefs.prefs.perfMode)
            img.calcProxyDim();
        img.setCrop();
        // Read metadata
        img.readMetaFromFile();
        // Clean up
        rawProcessor->recycle();
        img.imageLoaded = false;
        img.isDataRaw = false;
        img.isRawImage = true;
        img.updateSaveState();
        if (!appPrefs.prefs.holdFilesinRAM)
            img.unloadFileBuffer();
        return img;
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
    img.rawBufSize = img.width * img.height * 4 * sizeof(float);
    // Convert to float (assuming 16-bit output)
    if (processedImage->bits == 16 && processedImage->type == LIBRAW_IMAGE_BITMAP) {
        unsigned int numThreads = 2;//std::thread::hardware_concurrency();
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
                    pIn[1] = processedImage->colors > 1 ? static_cast<float>(raw_data[index + 1]) / 65535.0f : pIn[0];
                    pIn[2] = processedImage->colors > 2 ? static_cast<float>(raw_data[index + 2]) / 65535.0f : pIn[0];
                    if (processedImage->colors == 3)
                        ap0_to_ap1(pIn, pOut);
                    else {
                        pOut[0] = pIn[0];
                        pOut[1] = pIn[1];
                        pOut[2] = pIn[2];
                    }
                    img.rawImgData[index + 0] = pOut[0] * 2.0f;
                    img.rawImgData[index + 1] = pOut[1] * 2.0f;
                    img.rawImgData[index + 2] = pOut[2] * 2.0f;
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

    // Gamut Compression
    if (img.imgParam.gamutComp)
        ocioProc.refGamutCompress(img.rawImgData, img.width, img.height);


    // Clean up
    LibRaw::dcraw_clear_mem(processedImage);
    rawProcessor->recycle();
    img.isDataRaw = false;
    img.isRawImage = true;
    img.updateSaveState();
    if (!appPrefs.prefs.holdFilesinRAM)
        img.unloadFileBuffer();
auto end = std::chrono::steady_clock::now();

auto durA = std::chrono::duration_cast<std::chrono::microseconds>(a1 - start);
auto durB = std::chrono::duration_cast<std::chrono::microseconds>(b1 - a1);
auto durC = std::chrono::duration_cast<std::chrono::microseconds>(c1 - b1);
auto durD = std::chrono::duration_cast<std::chrono::microseconds>(d1 - c1);
auto durE = std::chrono::duration_cast<std::chrono::microseconds>(e1 - d1);
auto durF = std::chrono::duration_cast<std::chrono::microseconds>(f1 - e1);
auto durG = std::chrono::duration_cast<std::chrono::microseconds>(end - f1);
auto durH = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
//LOG_INFO("-----------Debayer Time-----------");
//LOG_INFO("Open:     {:*>8}μs | {:*>8}ms", durA.count(), durA.count()/1000);
//LOG_INFO("Unpack:   {:*>8}μs | {:*>8}ms", durB.count(), durB.count()/1000);
//LOG_INFO("Process:  {:*>8}μs | {:*>8}ms", durC.count(), durC.count()/1000);
//LOG_INFO("Convert:  {:*>8}μs | {:*>8}ms", durD.count(), durD.count()/1000);
//LOG_INFO("Pad:      {:*>8}μs | {:*>8}ms", durE.count(), durE.count()/1000);
//LOG_INFO("Disp:     {:*>8}μs | {:*>8}ms", durF.count(), durF.count()/1000);
//LOG_INFO("Clear:    {:*>8}μs | {:*>8}ms", durG.count(), durG.count()/1000);
//LOG_INFO("Total:    {:*>8}μs | {:*>8}ms", durH.count(), durH.count()/1000);
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
std::variant<image, std::string> readImageOIIO(std::string imagePath, ocioSetting ocioSet, bool background) {
    image img;

    std::filesystem::path imgP = imagePath;
    img.srcFilename = imgP.stem().string();
    img.srcPath = imgP.parent_path().string();
    img.fullPath = imagePath;
    img.imgMeta.fileName = img.srcFilename + imgP.extension().string();
    img.imgMeta.filePath = img.srcPath;
    if (img.srcFilename == "" || img.srcPath == "") {
        LOG_ERROR("Unable to parse image: {}, empty paths", imagePath);
        return "Could not parse image path";
    }

    OIIO::ImageInput::unique_ptr inputImage;
    std::unique_ptr<OIIO::Filesystem::IOMemReader> memReader;

    img.loadFileintoBuffer();
    if (!img.fileLoaded) {
        //We've failed to load the image file into memory.
        // Skip hashing and OIIO loading from buffer
         inputImage = OIIO::ImageInput::open(imagePath);
    } else {
        img.calculateHash();
        memReader = std::make_unique<OIIO::Filesystem::IOMemReader>(
                img.fileBuffer.data(),
                img.fileBuffer.size()
            );
        OIIO::ImageSpec memSpec;
        inputImage = OIIO::ImageInput::open(imagePath, &memSpec, memReader.get());
    }



    if (!inputImage)
    {
        LOG_ERROR("[oiio] Could not read the image file: {}", imagePath);
        LOG_ERROR("[oiio] Error: {}", OIIO::geterror());
        return "Could not open image: " + imagePath;
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

    if (background) {
        if (appPrefs.prefs.perfMode)
            img.calcProxyDim();
        img.setCrop();
        img.readMetaFromFile();

        img.intOCIOSet = ocioSet;
        img.imageLoaded = false;
        img.isDataRaw = false;
        img.isRawImage = false;
        img.updateSaveState();
        if (!appPrefs.prefs.holdFilesinRAM)
            img.unloadFileBuffer();
        return img;
    }

    //LOG_INFO("Reading in image with size: {}x{}x{}", img.width, img.height, img.nChannels);

    img.rawImgData = new float[img.width * img.height * 4];
    img.rawBufSize = img.width * img.height * 4 * sizeof(float);
    if(!inputImage->read_image(0, 0, 0, inputSpec.nchannels, OIIO::TypeDesc::FLOAT, (void*)img.rawImgData))
    {
        LOG_ERROR("[oiio] Failed to read image: {}", imagePath);
        LOG_ERROR("[oiio] Error: {}", inputImage->geterror());
        delete [] img.rawImgData;
        img.rawImgData = nullptr;
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
    img.intOCIOSet = ocioSet;
    if (!img.imgParam.ocioName.empty() && !ocioSet.impOverwrite) {
        // If there was ocio data in the image metadata
        bool goodConfig = false;
        std::vector<std::string> configList = ocioProc.getConfigNames();
        for (int i = 0; i < configList.size(); i++) {
            if (img.imgParam.ocioName == configList[i]) {
                goodConfig = true;
                img.intOCIOSet.ocioConfig = i;
            }
        }
        if (goodConfig) {
            img.intOCIOSet.colorspace = img.imgParam.ocioColor;
            img.intOCIOSet.display = img.imgParam.ocioDisp;
            img.intOCIOSet.view = img.imgParam.ocioView;
            img.intOCIOSet.useDisplay = img.imgParam.useDisplay;
            img.intOCIOSet.inverse = img.imgParam.inverse;
            img.intOCIOSet.gamutComp = img.imgParam.gamutComp;
        } else {
            LOG_WARN("Unable to locate config used by image, falling back to current selected config!");
        }

    } else {
        img.imgParam.ocioName = ocioProc.activeConfig()->config->getName();
        img.imgParam.ocioColor = img.intOCIOSet.colorspace;
        img.imgParam.ocioDisp = img.intOCIOSet.display;
        img.imgParam.ocioView = img.intOCIOSet.view;
        img.imgParam.useDisplay = img.intOCIOSet.useDisplay;
        img.imgParam.inverse = img.intOCIOSet.inverse;
        img.imgParam.gamutComp = appPrefs.prefs.gamutComp;
    }
    ocioProc.processImage(img.rawImgData, img.width, img.height, img.intOCIOSet);

    // Gamut Compression
    if (img.imgParam.gamutComp)
        ocioProc.refGamutCompress(img.rawImgData, img.width, img.height);

    img.imageLoaded = true;
    img.isDataRaw = false;
    img.isRawImage = false;
    img.updateSaveState();
    if (!appPrefs.prefs.holdFilesinRAM)
        img.unloadFileBuffer();

    return img;
}
