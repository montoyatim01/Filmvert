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
        copyParams.imageCropMinX = activeImage()->imgParam.imageCropMinX;
        copyParams.imageCropMaxX = activeImage()->imgParam.imageCropMaxX;
        copyParams.imageCropMinY = activeImage()->imgParam.imageCropMinY;
        copyParams.imageCropMaxY = activeImage()->imgParam.imageCropMaxY;
        copyParams.arbitraryRotation = activeImage()->imgParam.arbitraryRotation;
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
        copyMeta.cameraMake = activeImage()->imgMeta.cameraMake;
        copyMeta.cameraModel = activeImage()->imgMeta.cameraModel;
        copyMeta.lens = activeImage()->imgMeta.lens;
        copyMeta.filmStock = activeImage()->imgMeta.filmStock;
        copyMeta.focalLength = activeImage()->imgMeta.focalLength;
        copyMeta.fNumber = activeImage()->imgMeta.fNumber;
        copyMeta.exposureTime = activeImage()->imgMeta.exposureTime;
        copyMeta.dateTime = activeImage()->imgMeta.dateTime;
        copyMeta.location = activeImage()->imgMeta.location;
        copyMeta.gps = activeImage()->imgMeta.gps;
        copyMeta.notes = activeImage()->imgMeta.notes;
        copyMeta.devProcess = activeImage()->imgMeta.devProcess;
        copyMeta.chemMfg = activeImage()->imgMeta.chemMfg;
        copyMeta.devNotes = activeImage()->imgMeta.devNotes;
        copyMeta.scanner = activeImage()->imgMeta.scanner;
        copyMeta.scanNotes = activeImage()->imgMeta.scanNotes;


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
