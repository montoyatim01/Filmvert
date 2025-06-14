#include "logger.h"
//#include "metalGPU.h"
#include "window.h"


//--- Image Render ---//
/*
    Add the active image to the GPU render queue
*/
void mainWindow::imgRender() {
    if (validIm())
        gpu->addToRender(activeImage(), r_sdt, dispOCIO);
}

//--- Image Render ---//
/*
    Add the provided image to the GPU render queue
*/
void mainWindow::imgRender(image *img, renderType rType) {
    if (img)
        gpu->addToRender(img, rType, dispOCIO);
}

//--- Roll Render ---//
/*
    Queue up the entire active roll for GPU rendering
*/
void mainWindow::rollRender() {
    if (validRoll()) {
        for (int i = 0; i < activeRollSize(); i++) {
            imgRender(getImage(selRoll, i), r_bg);
        }
    }
}

//--- Roll Render Check ---//
/*
    Loop through all rolls/images to determine
    if any have been rendered and are in need of
    GL texture updating.

    Also check through all of the rolls to
    see if any are in need of dumping for
    performance mode
*/
void mainWindow::rollRenderCheck() {

    // Scan through all images needing GL updates
    // after being rendered (queued by import)
    for (int r = 0; r < activeRolls.size(); r++) {
        for (int i = 0; i < activeRolls[r].rollSize(); i++) {
            image *img = getImage(r, i);
            if (img && img->imageLoaded && img->needRndr) {
                imgRender(img);
                img->needRndr = false;
            }

        }
    }

    // Scan through all rolls checking if an GL update is queued
    // If no, and if we're not on the selected roll
    // Dump the roll
    for (int r = 0; r < activeRolls.size(); r++) {
        if (!activeRolls[r].rollLoaded || activeRolls[r].imagesLoading) {
            continue; // We don't want to inturrupt unloaded, or active rolls
        }

        bool imRendering = false;
        for (int i = 0; i < activeRolls[r].rollSize(); i++) {
            image* im = getImage(r, i);
            if (!im)
                continue;
            if (im->glUpdate || gpu->isInQueue(im)) {
                imRendering = true;
            }

        }
        if (!imRendering && r != selRoll &&
            activeRolls[r].rollLoaded &&
            !activeRolls[r].imagesLoading) {
                // If there are no GL updates needed
                // And this is not the active roll
                // And this roll is fully loaded
                // And there are not images loading
                if (appPrefs.prefs.perfMode) {
                    clearRoll(&activeRolls[r]);
                }

            }
    }

}

//--- State Render ---//
/*
    For all images in the current roll
    check if a re-render is needed based
    on the state change, queue them up
*/
void mainWindow::stateRender() {
    if (validRoll()) {
        if (validIm()) {
            imgRender();
            activeImage()->needRndr = false;
        }
        for (auto &img : activeRoll()->images) {
            if (img.needRndr) {
                imgRender(&img);
                img.needRndr = false;
            }
        }
    }
}


//--- Analyze Image ---//
/*
    Call up the GPU render for the blur pass
    and run the min/max on the resulting image
*/
void mainWindow::analyzeImage() {

    if (validIm()) {
        if (!activeImage()->imageLoaded) {
            std::strcpy(ackMsg, "Cannot analyze while image is being loaded!\nWait for image to finish loading.");
            ackPopTrig = true;
            return;
        }
        LOG_INFO("Analyzing {}", activeImage()->srcFilename);
        anaPopTrig = true;

    std::thread analyzeThread([this]{
        if (!activeImage()->blurImgData)
            activeImage()->allocBlurBuf();
        activeImage()->blurImage();
        activeImage()->processMinMax();
        anaPopTrig = false;
        activeImage()->needMetaWrite = true;
        metaRefresh = true;
        activeImage()->delBlurBuf();
        activeImage()->renderBypass = false;
        imgRender();
    });

    analyzeThread.detach();

    }
}
