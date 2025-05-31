#ifndef _image_h
#define _image_h


#include <string>
#include <optional>
#include <variant>
#include <thread>
#include <mutex>
#include <OpenImageIO/imageio.h>
#include "nlohmann/json.hpp"
#include "renderParams.h"
#include "imageParams.h"
#include "imageMeta.h"
#include "state.h"
#include "structs.h"
#include <exiv2/exiv2.hpp>



struct image {
    // Buffers
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

    // Core information
    unsigned int nChannels;
    unsigned int width;
    unsigned int height;
    unsigned int rawWidth;
    unsigned int rawHeight;
    bool isRawImage = false;
    bool isDataRaw = false;

    // Filenames/paths
    std::string srcFilename;
    std::string srcPath;
    std::string fullPath;
    std::string expFullPath;

    // Analysis/Grade parameters
    imageParams imgParam;
    ocioSetting intOCIOSet;
    rawSetting intRawSet;

    // Undo/Redo state
    userState imgState;


    // Status flags
    bool blurReady = false;
    bool renderReady = false;
    bool imageLoaded = false;
    bool fullIm = false;
    bool analyzed = false;
    bool selected = false;
    bool inRndQueue = false;
    bool needRndr = false;
    bool imgRst = false;
    bool minSel = false;

    // Display/Render flags
    bool renderBypass;
    bool gradeBypass;


    // GL Display
    long long unsigned int glTexture;

    void* texture = nullptr;
    void* histTex = nullptr;
    int sdlRotation;
    bool sdlUpdate = false;



    // imageBuffers.cpp
    void allocBlurBuf();
    void delBlurBuf();
    void allocateTmpBuf();
    void clearTmpBuf();

    void allocProcBuf();
    void delProcBuf();

    void allocDispBuf();
    void delDispBuf();

    void clearBuffers();
    void loadBuffers();
    void padToRGBA();
    void trimForSave();



    // imageIO.cpp
    bool exportPreProcess(std::string outPath);
    void exportPostProcess();
    bool writeImg(const exportParam param, ocioSetting ocioSet);
    bool debayerImage(bool fullRes, int quality);
    bool oiioReload();
    bool dataReload();

    // imageMeta.cpp
    void readMetaFromFile();
    bool loadMetaFromStr(const std::string& j, bool loadMeta);
    std::optional<nlohmann::json> getJSONMeta();
    void updateMetaStr();
    void writeXMPFile();
    bool writeExpMeta(std::string filename);
    bool importImageMeta(std::string filename);

    // image.cpp
    void rotLeft();
    void rotRight();
    void setCrop();


    // imageProcessing.cpp
    void processBaseColor();
    void blurImage();
    void processMinMax();
    void setMinMax();
    void resizeProxy();

};

// imageIO.cpp
std::variant<image, std::string> readImage(std::string imagePath, rawSetting rawSet, ocioSetting ocioSet);
std::variant<image, std::string> readDataImage(std::string imagePath, rawSetting rawSet, ocioSetting ocioSet);
std::variant<image, std::string> readRawImage(std::string imagePath);
std::variant<image, std::string> readImageOIIO(std::string imagePath, ocioSetting ocioSet);

// image.cpp
renderParams img_to_param(image* _img);

#endif
