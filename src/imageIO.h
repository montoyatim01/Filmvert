#ifndef _imageio_h
#define _imageio_h


#include <string>
#include <optional>
#include <variant>
#include <thread>
#include <OpenImageIO/imageio.h>
#include "grainParams.h"
#include "renderParams.h"

#define THUMBWIDTH 600
#define THUMBHEIGHT 400

struct exportParam {
  std::string outPath;
  int format;
  int bitDepth;
  int quality;
  bool overwrite;

};

struct minMaxPoint {
    unsigned int minX = 0;
    unsigned int minY = 0;
    float minLuma = 1.0f;
    float minRGB[3] = {1.0f, 1.0f, 1.0f};

    unsigned int maxX = 0;
    unsigned int maxY = 0;
    float maxLuma = 0.0f;
    float maxRGB[3] = {0.0f, 0.0f, 0.0f};
};
struct imageParams {
    float blurAmount = 5.0f;
    float baseColor[3] = {0.5f, 0.5f, 0.5f};
    float whitePoint[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    float blackPoint[4] = {0.0f, 0.0f, 0.0f, 0.0f};


    // Correction
    float temp = 0.0f;
    float tint = 0.0f;
    float g_blackpoint[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    float g_whitepoint[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    float g_lift[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    float g_gain[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    float g_mult[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    float g_offset[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    float g_gamma[4] = {1.0f, 1.0f, 1.0f, 1.0f};

    unsigned int cropBoxX[4];
    unsigned int cropBoxY[4];
};

struct image {
    float* rawImgData = nullptr;
    float* procImgData = nullptr;
    float* tmpOutData = nullptr;
    float* blurImgData = nullptr;
    uint8_t* dispImgData;
    uint8_t* thumbData;

    unsigned int nChannels;
    unsigned int width;
    unsigned int height;

    float blurAmount = 2.25f;

    // Inversion
    float baseColor[3] = {0.5f, 0.5f, 0.5f};
    float whitePoint[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    float blackPoint[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    unsigned int minX = 0;
    unsigned int minY = 0;

    unsigned int maxX = 0;
    unsigned int maxY = 0;

    float temp = 0.0f; //TODO: Implement in kernel (which step? Grade node math?)
    float tint = 0.0f;

    // Correction
    float g_blackpoint[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    float g_whitepoint[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    float g_lift[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    float g_gain[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    float g_mult[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    float g_offset[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    float g_gamma[4] = {1.0f, 1.0f, 1.0f, 1.0f};

    bool renderBypass;
    bool analyzed = false;

    std::string srcFilename;
    std::string srcPath;
    std::string fullPath;

    void* texture = nullptr;
    void* thumbTexture = nullptr;

    std::string imgMeta;

    unsigned int sampleX[2];
    unsigned int sampleY[2];
    bool sampleVisible;

    unsigned int cropBoxX[4];
    unsigned int cropBoxY[4];

    bool selected = false;

    bool sdlUpdate = false;

    void padToRGBA();
    void trimForSave();
    void procDispImg();
    void allocateTmpBuf();
    void clearTmpBuf();
    bool writeImg(const exportParam param);

    bool debayerImage(bool fullRes, int quality);

    void allocBlurBuf();
    void delBlurBuf();

    void processBaseColor();
    void processMinMax();

};


std::variant<image, std::string> readRawImage(std::string imagePath);
std::variant<image, std::string> readImage(std::string imagePath);

renderParams img_to_param(image* _img);

#endif
