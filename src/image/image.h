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
    imageMetadata imgMeta;

    // Core information
    unsigned int nChannels = 0;
    unsigned int width = 0;
    unsigned int height = 0;
    unsigned int rawWidth = 0;
    unsigned int rawHeight = 0;

    // Resolution used for display
    // This is modified by the crop settings
    unsigned int dispW = 0;
    unsigned int dispH = 0;

    // Resolution used for file output
    // This is modified by the crop settings
    unsigned int rndrW = 0;
    unsigned int rndrH = 0;

    bool isRawImage = false;
    bool isDataRaw = false;

    // Filenames/paths
    std::string srcFilename;
    std::string srcPath;
    std::string fullPath;
    std::string expFullPath;
    std::string rollPath;

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
    bool reloading = false;
    uint64_t histQueue = 0;

    // Display/Render flags
    bool renderBypass = true;
    bool gradeBypass = false;
    bool applyCrops = false;


    // GL Display
    long long unsigned int glTexture = 0;
    long long unsigned int glTextureSm = 0;

    void* histTex = nullptr;
    bool glUpdate = false;



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
    bool loadMetaFromStr(const std::string& j, copyPaste* impOpt = nullptr, bool init = false);
    std::optional<nlohmann::json> getJSONMeta();
    void updateMetaStr();
    void writeXMPFile();
    bool writeJSONFile();
    bool writeExpMeta(std::string filename);
    bool importImageMeta(std::string filename, copyPaste* impOpt = nullptr);
    void metaPaste(copyPaste selectons, imageParams* params, imageMetadata* meta, bool init = false);
    void loadParamJSONObj(imageParams* imgParam, copyPaste *&pasteOpts, nlohmann::json obj);

    // image.cpp
    void rotLeft();
    void rotRight();
    void flipV();
    void flipH();
    void setCrop();


    // imageProcessing.cpp
    void processBaseColor();
    void blurImage();
    void processMinMax();
    void setMinMax();
    void calcProxyDim();
    void resizeProxy();

};

// imageIO.cpp
std::variant<image, std::string> readImage(std::string imagePath, rawSetting rawSet, ocioSetting ocioSet, bool background = false);
std::variant<image, std::string> readDataImage(std::string imagePath, rawSetting rawSet, ocioSetting ocioSet, bool background = false);
std::variant<image, std::string> readRawImage(std::string imagePath, bool background = false);
std::variant<image, std::string> readImageOIIO(std::string imagePath, ocioSetting ocioSet, bool background = false);

// image.cpp
renderParams img_to_param(image* _img);

#endif
