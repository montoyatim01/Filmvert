#include "image.h"
#include "logger.h"
#include "preferences.h"




void image::allocBlurBuf() {
    if (!blurImgData)
        blurImgData = new float[width * height * 4];
}

void image::delBlurBuf() {
    blurReady = false;
    if (blurImgData)
    {
        delete [] blurImgData;
        blurImgData = nullptr;
    }

}

void image::allocateTmpBuf() {
    if (nChannels == 4 && fullIm)
    {
        // We already have an alpha channel
        tmpOutData = procImgData;
        return;
    }
    if (rawWidth * rawHeight > width * height)
        tmpOutData = new float[rawWidth * rawHeight * 4];
    else
        tmpOutData = new float[width * height * 4];
}
void image::clearTmpBuf() {
    if (tmpOutData && tmpOutData != procImgData) {
        delete[] tmpOutData;
        tmpOutData = nullptr;
    }

}

void image::allocProcBuf() {
    if (!procImgData) {
        if (rawWidth * rawHeight > width * height)
            procImgData = new float[rawWidth * rawHeight * 4];
        else
            procImgData = new float[width * height * 4];
    }
}
void image::delProcBuf() {
    if (procImgData) {
        delete [] procImgData;
        procImgData = nullptr;
    }
}

void image::allocDispBuf() {
    if (!dispImgData)
        dispImgData = new uint8_t[width * height * 4];
}

void image::delDispBuf() {
    if (dispImgData) {
        delete [] dispImgData;
        dispImgData = nullptr;
    }
}

//--- Clear Buffers ---//
/*
    Clear all buffers used by an image
    (other than the GL textures) to save
    on ram usage.
*/
void image::clearBuffers() {
    if (!rawImgData)
        return;
    // Delete raw buffer
    if (rawImgData) {
        delete [] rawImgData;
        rawImgData = nullptr;
    }
    // Delete proc buffer
    if (procImgData) {
        delete [] procImgData;
        procImgData = nullptr;
    }
    // Delete temp buffer
    if (tmpOutData) {
        delete [] tmpOutData;
        tmpOutData = nullptr;
    }
    // Delete Blur buff
    if (blurImgData) {
        delete [] blurImgData;
        blurImgData = nullptr;
    }
    // Delete disp buffer
    if (dispImgData) {
        delete [] dispImgData;
        dispImgData = nullptr;
    }
    imageLoaded = false;
}

//--- Load Buffers ---//
/*
    Re-allocate and re-load an image back
    from disk after it has been unloaded.
*/
void image::loadBuffers() {
    if (imageLoaded)
        return;
    reloading = true;
    allocProcBuf();
    allocDispBuf();
    if (isRawImage) {
        if(debayerImage(!appPrefs.prefs.perfMode, appPrefs.prefs.perfMode ? 2 : 11)) {
            imageLoaded = true;
        }
        else
            LOG_WARN("Unable to re-debayer image: {}", fullPath);
    } else if (isDataRaw) {
        if (dataReload()) {
            imageLoaded = true;
        } else {
            LOG_WARN("Unable to re-load image: {}", fullPath);
        }
    } else {
        if (oiioReload()) {
            imageLoaded = true;
        } else {
            LOG_WARN("Unable to re-load image: {}", fullPath);
        }
    }
    needRndr = true;
}

//--- Pad to RGBA ---//
/*
    Pad an RGB (Or R or RG) image to
    RGBA for all internal processing
    RGBA for better byte-alignment
*/
void image::padToRGBA() {
    if (nChannels == 4)
    {
        // We already have an alpha channel
        return;
    }
    allocProcBuf();
    if (!procImgData || !rawImgData) {
        LOG_ERROR("Either source or destination rgba buffer is null");
    }

    // Define the number of threads to use
    unsigned int numThreads = 2;//std::thread::hardware_concurrency();
    if (numThreads == 0) {
        numThreads = 2; // Default value if concurrent threads can't be determined
    }


    // Create a vector of threads
    std::vector<std::thread> threads(numThreads);

    // Divide the workload into equal parts for each thread
    int rowsPerThread = rawHeight / numThreads;

    auto processRows = [&](int startRow, int endRow) {
        for (int y=startRow; y<endRow; y++)
        {
            for (int x=0; x<rawWidth; x++)
            {
                int index = (y * rawWidth) + x;
                float rChan = nChannels >= 1 ? rawImgData[nChannels * index] : 0.0f;
                procImgData[4 * index] = rChan; // R channel
                procImgData[4 * index + 1] = nChannels >= 2 ? rawImgData[nChannels * index + 1] : rChan; // G channel
                procImgData[4 * index + 2] = nChannels >= 3 ? rawImgData[nChannels * index + 2] : rChan; // B channel
                procImgData[4 * index + 3] = nChannels >= 4 ? rawImgData[nChannels * index + 3] : 1.0f; // A channel
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
    memcpy(rawImgData, procImgData, rawWidth * rawHeight * 4 * sizeof(float));
    delProcBuf();
}

//--- Trim for Save ---//
/*
    Trim the output buffer to the same
    number of channels as the input
*/
void image::trimForSave() {
    if (nChannels == 4)
    {
        // We already have an alpha channel
        return;
    }
    if (!procImgData || !tmpOutData) {
        LOG_ERROR("Either source or destination rgba buffer is null");
        return;
    }

    // Define the number of threads to use
    unsigned int numThreads = 2;//std::thread::hardware_concurrency();
    if (numThreads == 0) {
        numThreads = 2; // Default value if concurrent threads can't be determined
    }


    // Create a vector of threads
    std::vector<std::thread> threads(numThreads);

    // Divide the workload into equal parts for each thread
    int rowsPerThread = rndrH / numThreads;

    auto processRows = [&](int startRow, int endRow) {
        for (int y=startRow; y<endRow; y++)
        {
            for (int x=0; x<rndrW; x++)
            {
                int index = (y * rndrW) + x;
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
        int endRow = (i == numThreads - 1) ? rndrH : (i + 1) * rowsPerThread;
        threads[i] = std::thread(processRows, startRow, endRow);
    }

        // Wait for all threads to finish
    for (auto& thread : threads) {
        thread.join();
    }
}
