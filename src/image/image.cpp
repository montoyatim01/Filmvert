#include "image.h"

#include "imageParams.h"
#include "logger.h"
#include "renderParams.h"
#include "utils.h"



//---Rotate Right---//
/*
    Set valid exif rotation values
*/
void image::rotRight() {
    switch(imRot) {
        case 1: // Upright
            imRot = 6; break;
        case 6: // Left
            imRot = 3; break;
        case 3: // Upside
            imRot = 8; break;
        case 8: // Right
            imRot = 1; break;
    }
}

//---Rotate Left---//
/*
    Set valid exif rotation values
*/
void image::rotLeft() {
    switch(imRot) {
        case 1: // Upright
            imRot = 8; break;
        case 8: // Right
            imRot = 3; break;
        case 3: // Upside
            imRot = 6; break;
        case 6: // Left
            imRot = 1; break;
    }
}

void image::setCrop() {
    imgParam.cropBoxX[0] = 0.1;
    imgParam.cropBoxY[0] = 0.1;

    imgParam.cropBoxX[1] = 0.9;
    imgParam.cropBoxY[1] = 0.1;

    imgParam.cropBoxX[2] = 0.9;
    imgParam.cropBoxY[2] = 0.9;

    imgParam.cropBoxX[3] = 0.1;
    imgParam.cropBoxY[3] = 0.9;
}

//---Image to Param---//
/*
    Fill out the render param struct with the values
    set for the image.
    The "alpha" values in the UI get used as a global multiplier
    (add/sub for bp, lift, offset)
*/

renderParams img_to_param(image* _img) {
    renderParams params;

    params.width = _img->width;
    params.height = _img->height;

    params.sigmaFilter = _img->imgParam.blurAmount;
    params.temp = _img->imgParam.temp;
    params.tint = _img->imgParam.tint;

    params.bypass = _img->renderBypass ? 1 : 0;
    params.gradeBypass = _img->gradeBypass ? 1 : 0;

    for (int i = 0; i < 4; i++) {
        params.baseColor[i] = i == 3 ? 0.0f : _img->imgParam.baseColor[i];
        params.blackPoint[i] = i == 3 ? 0.0f : _img->imgParam.blackPoint[i] + _img->imgParam.blackPoint[3];
        params.whitePoint[i] = i == 3 ? 1.0f : _img->imgParam.whitePoint[i] * _img->imgParam.whitePoint[3];
        params.G_blackpoint[i] = i == 3 ? 0.0f : _img->imgParam.g_blackpoint[i] + _img->imgParam.g_blackpoint[3];
        params.G_whitepoint[i] = i == 3 ? 1.0f : _img->imgParam.g_whitepoint[i] * _img->imgParam.g_whitepoint[3];
        params.G_lift[i] = i == 3 ? 0.0f : _img->imgParam.g_lift[i] + _img->imgParam.g_lift[3];
        params.G_gain[i] = i == 3 ? 1.0f : _img->imgParam.g_gain[i] * _img->imgParam.g_gain[3];
        params.G_mult[i] = i == 3 ? 1.0f : _img->imgParam.g_mult[i] * _img->imgParam.g_mult[3];
        params.G_offset[i] = i == 3 ? 0.0f : _img->imgParam.g_offset[i] + _img->imgParam.g_offset[3];
        params.G_gamma[i] = i == 3 ? 1.0f : _img->imgParam.g_gamma[i] * _img->imgParam.g_gamma[3];
    }

    return params;


}
