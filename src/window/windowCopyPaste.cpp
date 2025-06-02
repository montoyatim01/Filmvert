//#include "metalGPU.h"
#include "window.h"

//--- Copy Into Params ---//
/*
    Copy active image's params and metadata into
    the copyParams struct for later pasting
*/
void mainWindow::copyIntoParams() {
    if (validIm()) {
        copyParams.blurAmount = activeImage()->imgParam.blurAmount;
        copyParams.temp = activeImage()->imgParam.temp;
        copyParams.tint = activeImage()->imgParam.tint;
        copyParams.baseColor[0] = activeImage()->imgParam.baseColor[0];
        copyParams.baseColor[1] = activeImage()->imgParam.baseColor[1];
        copyParams.baseColor[2] = activeImage()->imgParam.baseColor[2];
        for (int i = 0; i < 4; i++) {
            copyParams.whitePoint[i] = activeImage()->imgParam.whitePoint[i];
            copyParams.blackPoint[i] = activeImage()->imgParam.blackPoint[i];
            copyParams.g_blackpoint[i] = activeImage()->imgParam.g_blackpoint[i];
            copyParams.g_whitepoint[i] = activeImage()->imgParam.g_whitepoint[i];
            copyParams.g_lift[i] = activeImage()->imgParam.g_lift[i];
            copyParams.g_gain[i] = activeImage()->imgParam.g_gain[i];
            copyParams.g_mult[i] = activeImage()->imgParam.g_mult[i];
            copyParams.g_offset[i] = activeImage()->imgParam.g_offset[i];
            copyParams.g_gamma[i] = activeImage()->imgParam.g_gamma[i];
            copyParams.cropBoxX[i] = activeImage()->imgParam.cropBoxX[i];
            copyParams.cropBoxY[i] = activeImage()->imgParam.cropBoxY[i];
        }

        //---Metadata
        copyMeta.cameraMake = activeImage()->imMeta.cameraMake;
        copyMeta.cameraModel = activeImage()->imMeta.cameraModel;
        copyMeta.lens = activeImage()->imMeta.lens;
        copyMeta.filmStock = activeImage()->imMeta.filmStock;
        copyMeta.focalLength = activeImage()->imMeta.focalLength;
        copyMeta.fNumber = activeImage()->imMeta.fNumber;
        copyMeta.exposureTime = activeImage()->imMeta.exposureTime;
        copyMeta.dateTime = activeImage()->imMeta.dateTime;
        copyMeta.location = activeImage()->imMeta.location;
        copyMeta.gps = activeImage()->imMeta.gps;
        copyMeta.notes = activeImage()->imMeta.notes;
        copyMeta.devProcess = activeImage()->imMeta.devProcess;
        copyMeta.chemMfg = activeImage()->imMeta.chemMfg;
        copyMeta.devNotes = activeImage()->imMeta.devNotes;
        copyMeta.scanner = activeImage()->imMeta.scanner;
        copyMeta.scanNotes = activeImage()->imMeta.scanNotes;


    }
}

//--- Paste Into Params ---//
/*
    For each image selected, paste the selected params
    from the copyParams struct into the image's params
*/
void mainWindow::pasteIntoParams() {
    for (int i = 0; i < activeRollSize(); i++) {
        if (getImage(i)->selected) {
            getImage(i)->metaPaste(pasteOptions, &copyParams, &copyMeta);
            imgRender(getImage(i), r_sdt);
        }
    }
}
