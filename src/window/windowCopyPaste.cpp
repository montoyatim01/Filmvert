#include "window.h"


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

    }
}

void mainWindow::pasteIntoParams() {
    for (int i = 0; i < activeRollSize(); i++) {
        if (getImage(i)->selected) {
            getImage(i)->renderBypass = false;
            bool metaChg = false;
            if (pasteOptions.baseColor) {
                getImage(i)->imgParam.baseColor[0] = copyParams.baseColor[0];
                getImage(i)->imgParam.baseColor[1] = copyParams.baseColor[1];
                getImage(i)->imgParam.baseColor[2] = copyParams.baseColor[2];
                metaChg = true;
            }
            if (pasteOptions.analysisBlur) {
                getImage(i)->imgParam.blurAmount = copyParams.blurAmount;
                metaChg = true;
            }
            if (pasteOptions.temp) {
                getImage(i)->imgParam.temp = copyParams.temp;
                metaChg = true;
            }
            if (pasteOptions.tint) {
                getImage(i)->imgParam.tint = copyParams.tint;
                metaChg = true;
            }
            for (int j = 0; j < 4; j++) {
                if (pasteOptions.cropPoints) {
                    getImage(i)->imgParam.cropBoxX[j] = copyParams.cropBoxX[j];
                    getImage(i)->imgParam.cropBoxY[j] = copyParams.cropBoxY[j];
                    metaChg = true;
                }
                if (pasteOptions.analysis) {
                    getImage(i)->imgParam.blackPoint[j] = copyParams.blackPoint[j];
                    getImage(i)->imgParam.whitePoint[j] = copyParams.whitePoint[j];
                    metaChg = true;
                }
                if (pasteOptions.bp) {
                    getImage(i)->imgParam.g_blackpoint[j] = copyParams.g_blackpoint[j];
                    metaChg = true;
                }
                if (pasteOptions.wp) {
                    getImage(i)->imgParam.g_whitepoint[j] = copyParams.g_whitepoint[j];
                    metaChg = true;
                }
                if (pasteOptions.lift) {
                    getImage(i)->imgParam.g_lift[j] = copyParams.g_lift[j];
                    metaChg = true;
                }
                if (pasteOptions.gain) {
                    getImage(i)->imgParam.g_gain[j] = copyParams.g_gain[j];
                    metaChg = true;
                }
                if (pasteOptions.mult) {
                    getImage(i)->imgParam.g_mult[j] = copyParams.g_mult[j];
                    metaChg = true;
                }
                if (pasteOptions.offset) {
                    getImage(i)->imgParam.g_offset[j] = copyParams.g_offset[j];
                    metaChg = true;
                }
                if (pasteOptions.gamma) {
                    getImage(i)->imgParam.g_gamma[j] = copyParams.g_gamma[j];
                    metaChg = true;
                }
            }
            getImage(i)->needMetaWrite |= metaChg;
            imgRender(getImage(i));
        }
    }
}
