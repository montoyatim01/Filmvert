#ifndef _imageio_h
#define _imageio_h


#include <string>
#include <optional>
#include <variant>
#include <thread>
#include <OpenImageIO/imageio.h>
#include "grainParams.h"
#include "nlohmann/json_fwd.hpp"
#include "renderParams.h"
#include "imageParams.h"
#include "imageMeta.h"
#include <exiv2/exiv2.hpp>

#define THUMBWIDTH 600
#define THUMBHEIGHT 400

struct exportParam {
  std::string outPath;
  int format = 4;
  int bitDepth = 1;
  int quality = 85;
  int compression = 8;
  bool overwrite = false;
  int colorspaceOpt = 1;
  int colorspace = 0;
  int display = 0;
  int view = 0;
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


struct image {
    float* rawImgData = nullptr;
    float* procImgData = nullptr;
    float* tmpOutData = nullptr;
    float* blurImgData = nullptr;
    uint8_t* dispImgData = nullptr;
    //uint8_t* thumbData;

    // Metadata
    Exiv2::ExifData exifData;
    Exiv2::XmpData intxmpData;
    Exiv2::XmpData scxmpData;
    bool hasSCXMP = false;
    bool needMetaWrite = false;
    std::string jsonMeta;
    int imRot = 1;
    imageMetadata imMeta;


    unsigned int nChannels;
    unsigned int width;
    unsigned int height;
    unsigned int workWidth;
    unsigned int workHeight;

    std::string expFullPath;

    bool blurReady = false;
    bool imageLoaded = false;

    imageParams imgParam;

    bool renderBypass;
    bool gradeBypass;
    bool fullIm = false;
    bool analyzed = false;

    std::string srcFilename;
    std::string srcPath;
    std::string fullPath;

    void* texture = nullptr;
    int sdlRotation;
    //void* thumbTexture = nullptr;



    bool selected = false;

    bool sdlUpdate = false;

    void padToRGBA();
    void trimForSave();
    void procDispImg();
    void allocateTmpBuf();
    void clearTmpBuf();

    bool exportPreProcess(std::string outPath);
    void exportPostProcess();

    bool writeImg(const exportParam param);

    bool debayerImage(bool fullRes, int quality);
    void readMetaFromFile();
    bool loadMetaFromStr(const std::string& j);
    std::optional<nlohmann::json> getJSONMeta();
    void updateMetaStr();
    void writeXMPFile();
    bool writeExpMeta(std::string filename);


    void rotLeft();
    void rotRight();

    void allocBlurBuf();
    void delBlurBuf();

    void allocProcBuf();
    void delProcBuf();

    void allocDispBuf();
    void delDispBuf();

    void clearBuffers();
    void loadBuffers();

    void processBaseColor();
    void processMinMax();

};


std::variant<image, std::string> readRawImage(std::string imagePath);
std::variant<image, std::string> readImage(std::string imagePath);

renderParams img_to_param(image* _img);

#endif
