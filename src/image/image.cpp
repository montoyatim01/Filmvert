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
    switch(imgParam.rotation) {
        case 1: // Normal -> 90° CW
            imgParam.rotation = 6; break;
        case 2: // H flip -> 90° CW + H flip
            imgParam.rotation = 7; break;
        case 3: // 180° -> 90° CCW
            imgParam.rotation = 8; break;
        case 4: // V flip -> 90° CCW + H flip
            imgParam.rotation = 5; break;
        case 5: // 90° CCW + H flip -> Normal + H flip
            imgParam.rotation = 2; break;
        case 6: // 90° CW -> 180°
            imgParam.rotation = 3; break;
        case 7: // 90° CW + H flip -> V flip
            imgParam.rotation = 4; break;
        case 8: // 90° CCW -> Normal
            imgParam.rotation = 1; break;
    }
}

//---Rotate Left---//
/*
    Set valid exif rotation values
*/
void image::rotLeft() {
    switch(imgParam.rotation) {
        case 1: // Normal -> 90° CCW
            imgParam.rotation = 8; break;
        case 2: // H flip -> 90° CCW + H flip
            imgParam.rotation = 5; break;
        case 3: // 180° -> 90° CW
            imgParam.rotation = 6; break;
        case 4: // V flip -> 90° CW + H flip
            imgParam.rotation = 7; break;
        case 5: // 90° CCW + H flip -> V flip
            imgParam.rotation = 4; break;
        case 6: // 90° CW -> Normal
            imgParam.rotation = 1; break;
        case 7: // 90° CW + H flip -> H flip
            imgParam.rotation = 2; break;
        case 8: // 90° CCW -> 180°
            imgParam.rotation = 3; break;
    }
}

//---Flip Vertical---//
/*
    Set valid exif rotation values
*/
void image::flipV() {
    switch(imgParam.rotation) {
        case 1: // Normal -> Vertical flip
            imgParam.rotation = 4; break;
        case 2: // Horizontal flip -> 180°
            imgParam.rotation = 3; break;
        case 3: // 180° -> Horizontal flip
            imgParam.rotation = 2; break;
        case 4: // Vertical flip -> Normal
            imgParam.rotation = 1; break;
        case 5: // 90° CCW + H flip -> 90° CW + H flip
            imgParam.rotation = 7; break;
        case 6: // 90° CW -> 90° CCW
            imgParam.rotation = 8; break;
        case 7: // 90° CW + H flip -> 90° CCW + H flip
            imgParam.rotation = 5; break;
        case 8: // 90° CCW -> 90° CW
            imgParam.rotation = 6; break;
    }
}

//---Flip Horizontal---//
/*
    Set valid exif rotation values
*/
void image::flipH() {
    switch(imgParam.rotation) {
        case 1: // Normal -> Horizontal flip
            imgParam.rotation = 2; break;
        case 2: // Horizontal flip -> Normal
            imgParam.rotation = 1; break;
        case 3: // 180° -> Vertical flip
            imgParam.rotation = 4; break;
        case 4: // Vertical flip -> 180°
            imgParam.rotation = 3; break;
        case 5: // 90° CCW + H flip -> 90° CCW
            imgParam.rotation = 8; break;
        case 6: // 90° CW -> 90° CW + H flip
            imgParam.rotation = 7; break;
        case 7: // 90° CW + H flip -> 90° CW
            imgParam.rotation = 6; break;
        case 8: // 90° CCW -> 90° CCW + H flip
            imgParam.rotation = 5; break;
    }
}

//--- Set Crop ---//
/*
    Set the initial crop state for an image
*/
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

    params.width = _img->fullIm ? _img->rawWidth : _img->width;
    params.height = _img->fullIm ? _img->rawHeight : _img->height;

    params.sigmaFilter = _img->imgParam.blurAmount;
    params.temp = _img->imgParam.temp;
    params.tint = _img->imgParam.tint;
    params.saturation = _img->imgParam.saturation;

    params.bypass = _img->renderBypass ? 1 : 0;
    params.gradeBypass = _img->gradeBypass ? 1 : 0;

    params.arbitraryRotation = _img->imgParam.arbitraryRotation;
    params.cropEnable = _img->imgParam.cropEnable;
    params.cropVisible = _img->imgParam.cropVisible;
    params.imageCropMinX = _img->imgParam.imageCropMinX;
    params.imageCropMinY = _img->imgParam.imageCropMinY;
    params.imageCropMaxX = _img->imgParam.imageCropMaxX;
    params.imageCropMaxY = _img->imgParam.imageCropMaxY;

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
