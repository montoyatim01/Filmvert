#ifndef _gpustructs_h
#define _gpustructs_h

#include "image.h"
#include "structs.h"


struct gpuTimer {
    float renderTime;
    float fps;
};

enum renderType {
    r_sdt = 0,
    r_blr = 1,
    r_full = 2,
    r_bg = 3
};

struct gpuQueue {
    image* _img;
    renderType _type;
    ocioSetting _ocioSet;


    gpuQueue(image* img, renderType type, ocioSetting ocioSet)
    {_img = img;
    _type = type;
    _ocioSet = ocioSet;}


};

#endif
