#include "preferences.h"
#include "roll.h"
#include "gpu.h"
#include <OpenImageIO/imagebuf.h>
#include <OpenImageIO/imageio.h>
#include <OpenImageIO/imagebufalgo.h>
#include <vector>
#include <cstddef>
#include <csignal>
/*
-- Parameters in:
- Images wide
- Format

- (One at a time)
- Copy from gpu proxy buffer
- Border image (extra bottom)
- Add image text
- Calculate final width/height
- Create full image
- Add roll name
- Arrange images

*/

// TODO:
// - Fix buffer sizes/borders text pos
// - Make sure contact sheet doesn't import (blacklist name?)
// - Double check how crop operates (Should be fine?)
// - Apply EXIF rotations

void filmRoll::generateContactSheet(int imageWidth, exportParam expParam) {
    // Only using this as a way to get the
    // image data from the buffers. No processing
    openglGPU tmpGPU;
    float imageScale = appPrefs.prefs.proxyRes;


    std::vector<OIIO::ImageBuf> bdrOIIOBuf;

    for (size_t im = 0; im < images.size(); im++) {
        // If using full buffer instead of proxy
        // change this out to just dispW and dispH
        int imgWidth = (int)((float)images[im].dispW * imageScale);
        int imgHeight = (int)((float)images[im].dispH * imageScale);

        float* rawImgBuffer = nullptr;

        // Allocate image buffer
        rawImgBuffer = new float[imgWidth * imgHeight * 4];

        // Copy image buffer from texture
        tmpGPU.copyFromTexFull(images[im].glTextureSm, imgWidth, imgHeight, rawImgBuffer);

        if (rawImgBuffer == nullptr) {
            // Something went wrong fetching the image buffer
            // move to next image
            LOG_ERROR("[CS] Unable to copy image data from GPU");
            continue;
        }
        // Create the image spec and image buf from the buffer
        OIIO::ImageSpec rawImgSpec(imgWidth, imgHeight, 4, OIIO::TypeDesc::FLOAT);
        OIIO::ImageBuf rawOIIOBuf(rawImgSpec, rawImgBuffer);

        OIIO::ImageBuf* oiioImage;

        // Apply the image's orientation
        if (expParam.csBakeRot == 0) {
            // Don't bake anything
            oiioImage = &rawOIIOBuf;
        } else if (expParam.csBakeRot == 1) {
            // Bake only operations that don't modify width/height
            if (images[im].imgParam.rotation == 2 ||
                images[im].imgParam.rotation == 3 ||
                images[im].imgParam.rotation == 4) {
                    rawOIIOBuf.specmod()["Orientation"] = images[im].imgParam.rotation;
                    OIIO::ImageBuf orientedOIIOBuf;
                    if (!OIIO::ImageBufAlgo::reorient(orientedOIIOBuf, rawOIIOBuf)) {
                        LOG_ERROR("[CS] Failed to reorient image: {}", OIIO::geterror());
                        oiioImage = &rawOIIOBuf;
                    } else {
                        oiioImage = &orientedOIIOBuf;
                    }
                } else {
                    oiioImage = &rawOIIOBuf;
                }
        } else {
            // Bake everything
            rawOIIOBuf.specmod()["Orientation"] = images[im].imgParam.rotation;
            OIIO::ImageBuf orientedOIIOBuf;
            if (!OIIO::ImageBufAlgo::reorient(orientedOIIOBuf, rawOIIOBuf)) {
                LOG_ERROR("[CS] Failed to reorient image: {}", OIIO::geterror());
                oiioImage = &rawOIIOBuf;
            } else {
                oiioImage = &orientedOIIOBuf;
            }
        }


        // Calculate the border size
        int rotImWidth = oiioImage->spec().width;
        int rotImHeight = oiioImage->spec().height;
        int borderSize = (int)((float)rotImWidth * appPrefs.prefs.contactSheetBorder);
        int bottomBorderSize = (int)((float)rotImWidth * (appPrefs.prefs.contactSheetBorder * 0.125f) + ((float)imgWidth * 0.02f));
        int bdrWidth = rotImWidth + (2 * borderSize);
        int bdrHeight = rotImHeight + (6 * bottomBorderSize);
        // Create the larger image buffer
        OIIO::ImageSpec bdrSpec(bdrWidth, bdrHeight, 4, OIIO::TypeDesc::FLOAT);
        bdrOIIOBuf.emplace_back(OIIO::ImageBuf(bdrSpec));

        // Fill our border buffer and paste our image into it
        if (OIIO::ImageBufAlgo::fill(bdrOIIOBuf[im], {0.0f, 0.0f, 0.0f, 1.0f})) {
            // Success filling the border buffer
            if (OIIO::ImageBufAlgo::paste(bdrOIIOBuf[im], borderSize, borderSize, 0, 0, *oiioImage)) {
                // Success copying our image over
            } else {
                LOG_ERROR("[CS] Failed to paste image to border-buffer! {}", OIIO::geterror());
                if (rawImgBuffer)
                    delete [] rawImgBuffer;
                continue;
            }
        } else {
            LOG_ERROR("[CS] Failed to fill border-buffer! {}", OIIO::geterror());
            if (rawImgBuffer)
                delete [] rawImgBuffer;
            continue;
        }

        // Add text
        OIIO::ImageBufAlgo::render_text(bdrOIIOBuf[im], bdrWidth/2, rotImHeight + (3 * bottomBorderSize),
            #ifdef linux
            images[im].srcFilename, 52, "JetBrainsMono", 1.0f,
            #elif defined WIN32
            images[im].srcFilename, 52, "C:\\Windows\\Fonts\\arial.ttf", 1.0f,
            #else
            images[im].srcFilename, 52, "Arial", 1.0f,
            #endif
            OIIO::ImageBufAlgo::TextAlignX::Center,
            OIIO::ImageBufAlgo::TextAlignY::Center);
    } // Single image loop

    int rowCount = images.size() / imageWidth;
    rowCount = rowCount * imageWidth >= images.size() ? rowCount : rowCount + 1;

    // Calculate the final image block width/height
    int imgBlkWidth = 0;
    int imgBlkHeight = 0;
    for (size_t row = 0; row < rowCount; row++) {
        int rowWidth = 0;
        int rowHeight = 0;
        for (size_t im = 0; im < imageWidth; im++) {
            int index = (row * imageWidth) + im;
            if (index >= images.size())
                break;
            rowWidth += bdrOIIOBuf[index].spec().width;
            rowHeight = std::max(rowHeight, bdrOIIOBuf[index].spec().height);
        }
        imgBlkWidth = std::max(imgBlkWidth, rowWidth);
        imgBlkHeight += rowHeight;
    }

    // Title height
    int titleHeight = 260;

    // Final image
    OIIO::ImageSpec finalSpec(imgBlkWidth, imgBlkHeight + titleHeight, 4, OIIO::TypeDesc::FLOAT);
    std::string rollMeta = getRollMetaString();
    //std::raise(SIGINT);
    finalSpec.attribute("ImageDescription", rollMeta);
    OIIO::ImageBuf finalBuf(finalSpec);

    if (!OIIO::ImageBufAlgo::fill(finalBuf, {0.0f, 0.0f, 0.0f, 1.0f})) {
        LOG_ERROR("[CS] Failed to fill final image! {}", OIIO::geterror());
        return;
    }

    // Add title
    OIIO::ImageBufAlgo::render_text(finalBuf, imgBlkWidth/2, titleHeight/2,
        #ifdef linux
        rollName, 140, "JetBrainsMono", 1.0f,
        #elif defined WIN32
        rollName, 140, "C:\\Windows\\Fonts\\arial.ttf", 1.0f,
        #else
        rollName, 140, "Arial", 1.0f,
        #endif
        OIIO::ImageBufAlgo::TextAlignX::Center,
        OIIO::ImageBufAlgo::TextAlignY::Center);

    // Paste in the images
    int yOffset = titleHeight;
    for (size_t row = 0; row < rowCount; row++) {
        // Cycle through each row
        int xOffset = 0;
        int maxHeight = 0;
        for (size_t im = 0; im < imageWidth; im++) {
            size_t index = (row * imageWidth) + im;
            if (index >= images.size())
                continue;
            // Paste in the image
            if (!OIIO::ImageBufAlgo::paste(finalBuf, xOffset, yOffset, 0, 0, bdrOIIOBuf[index])) {
                LOG_ERROR("[CS] Failed to paste to final image. Bailing out! {}", OIIO::geterror());
                return;
            }
            // Up our xOffset based on the width
            xOffset += bdrOIIOBuf[index].spec().width;
            // Max the image heights for yOffset
            maxHeight = std::max(maxHeight, bdrOIIOBuf[index].spec().height);
        } // Image loop
        yOffset += maxHeight;
    } // Row loop

    OIIO::TypeDesc outFormat;
    switch (expParam.bitDepth) {
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
    switch (expParam.format) {
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
    std::string filePath = rollPath + "/" + rollName + fileExt;
    // Write final image
    if (!finalBuf.write(filePath, outFormat)) {
        LOG_ERROR("Failed to write image: {}", OIIO::geterror());
        return;
    }
}
