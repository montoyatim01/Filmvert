#include "window.h"


void mainWindow::initRender(int start, int end) {
    std::thread renThread = std::thread{ [this, start, end]() {
        //isRendering = true;
        for (int i = start; i < end; i++) {
            if (i >= activeRollSize())
                break;
            mtlGPU->addToRender(getImage(i), r_sdt);
            //getImage(i)->procDispImg();
           //getImage(i)->sdlUpdate = true;
            //updateSDLTexture(getImage(i));
        }

    }};
    renThread.detach();

}

void mainWindow::imgRender() {

    if (validIm()) {

        //std::thread renThread = std::thread{ [this]() {
            //isRendering = true;
            mtlGPU->addToRender(activeImage(), r_sdt);
            //activeImage()->procDispImg();
            //activeImage()->sdlUpdate = true;
            //updateSDLTexture(activeImage());

            //} };
       // renThread.detach();

    }
}

void mainWindow::imgRender(image *img) {


    if (img) {
        //isRendering = true;
        mtlGPU->addToRender(img, r_sdt);
        //img->procDispImg();
        //img->sdlUpdate = true;


        //renderCall = false;
	}
}
void mainWindow::rollRender() {
    if (validRoll()) {
        for (int i = 0; i < activeRolls[selRoll].images.size(); i++) {
            imgRender(getImage(selRoll, i));
        }
    }
}

void mainWindow::rollRenderCheck() {

    // Scan through all images needing SDL updates
    // after being rendered (queued by import)
    for (int r = 0; r < activeRolls.size(); r++) {
        for (int i = 0; i < activeRolls[r].images.size(); i++) {
            image *img = getImage(r, i);
            if (img && img->imageLoaded && img->sdlUpdate)
                updateSDLTexture(img);
        }
    }

    // Scan through all rolls checking if an SDL update is queued
    // If no, and if we're not on the selected roll
    // Dump the roll
    for (int r = 0; r < activeRolls.size(); r++) {
        if (!activeRolls[r].rollLoaded || activeRolls[r].imagesLoading)
            continue; // We don't want to inturrupt unloaded, or active rolls
        bool imRendering = false;
        for (int i = 0; i < activeRolls[r].images.size(); i++) {
            image* im = getImage(r, i);
            if (!im)
                continue;
            if (im->sdlUpdate || mtlGPU->isInQueue(im))
                imRendering = true;
        }
        if (!imRendering && r != selRoll &&
            activeRolls[r].rollLoaded &&
            !activeRolls[r].imagesLoading) {
                // If there are no SDL updates needed
                // And this is not the active roll
                // And this roll is fully loaded
                // And there are not images loading
                activeRolls[r].clearBuffers();
            }
    }

}

void mainWindow::analyzeImage() {

    if (validIm()) {
        LOG_INFO("Analyzing");
        //isRendering = true;
        if (!activeImage()->blurImgData)
            activeImage()->allocBlurBuf();
        mtlGPU->addToRender(activeImage(), r_blr);
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
