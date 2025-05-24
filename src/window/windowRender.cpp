#include "logger.h"
#include "metalGPU.h"
#include "window.h"


//--- Image Render ---//
/*
    Add the active image to the GPU render queue
*/
void mainWindow::imgRender() {
    if (validIm())
        mtlGPU->addToRender(activeImage(), r_sdt, dispOCIO);
}

//--- Image Render ---//
/*
    Add the provided image to the GPU render queue
*/
void mainWindow::imgRender(image *img, renderType rType) {
    if (img)
        mtlGPU->addToRender(img, rType, dispOCIO);
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
    SDL texture updating.

    Also check through all of the rolls to
    see if any are in need of dumping for
    performance mode
*/
void mainWindow::rollRenderCheck() {

    // Scan through all images needing SDL updates
    // after being rendered (queued by import)
    for (int r = 0; r < activeRolls.size(); r++) {
        for (int i = 0; i < activeRolls[r].rollSize(); i++) {
            image *img = getImage(r, i);
            if (img && img->imageLoaded && img->sdlUpdate)
                updateSDLTexture(img);
        }
    }

    // Scan through all rolls checking if an SDL update is queued
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
            if (im->sdlUpdate || mtlGPU->isInQueue(im)) {
                imRendering = true;
            }

        }
        if (!imRendering && r != selRoll &&
            activeRolls[r].rollLoaded &&
            !activeRolls[r].imagesLoading) {
                // If there are no SDL updates needed
                // And this is not the active roll
                // And this roll is fully loaded
                // And there are not images loading
                if (appPrefs.prefs.perfMode) {
                    activeRolls[r].clearBuffers();
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
        LOG_INFO("Analyzing");
        //isRendering = true;
        if (!activeImage()->blurImgData)
            activeImage()->allocBlurBuf();
        mtlGPU->addToRender(activeImage(), r_blr, dispOCIO);
        while (!activeImage()->blurReady){}
        activeImage()->processMinMax();
        activeImage()->needMetaWrite = true;
        metaRefresh = true;
        //activeImage()->procDispImg();
        //activeImage()->sdlUpdate = true;
        activeImage()->delBlurBuf();
        activeImage()->renderBypass = false;

        imgRender();
    }
}
