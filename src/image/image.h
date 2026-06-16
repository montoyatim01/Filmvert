#ifndef _image_h
#define _image_h


#include <string>
#include <optional>
#include <variant>
#include <thread>
#include <mutex>
#include <vector>
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
    std::vector<char> fileBuffer;
    float* rawImgData = nullptr;
    float* procImgData = nullptr;
    float* tmpOutData = nullptr;
    float* blurImgData = nullptr;
    uint8_t* dispImgData = nullptr;  // Deprecated - remove
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

    // Buffer Sizes
    uint64_t rawBufSize = 0;
    uint64_t blurBufSize = 0;
    uint64_t procBufSize = 0;
    uint64_t tmpBufSize = 0;
    uint64_t glBufSize = 0;
    uint64_t glSmBufSize = 0;

    // Resolution used for display
    // This is modified by the crop settings
    unsigned int dispW = 0;
    unsigned int dispH = 0;

    // Resolution used for file output
    // This is modified by the crop settings
    unsigned int rndrW = 0;
    unsigned int rndrH = 0;

    bool fileLoaded = false;
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
    imageParams prevSaveParam;
    imageMetadata prevSaveMeta;


    // Status flags
    bool blurReady = false;
    bool renderReady = false;
    bool cpuRender = false;
    bool imageLoaded = false;
    bool fullIm = false;
    bool analyzed = false;
    bool selected = false;
    bool visible = false;
    bool inRndQueue = false;
    bool needRndr = false;
    bool imgRst = false;
    bool minSel = false;
    bool reloading = false;
    uint64_t histQueue = 0;
    int activeExpCount = 1;

    // Display/Render flags
    bool renderBypass = true;
    bool gradeBypass = false;
    bool showClip = false;
    int channelView = 0;
    uint32_t secEnable = 0xFFFFFFFF; // Everything on
    bool applyCrops = false;


    // GL Display
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
    void unloadFileBuffer();
    uint64_t ramUsage();
    uint64_t vramUsage();



    // imageIO.cpp
    bool exportPreProcess(std::string outPath, int exportImgCount);
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
    void calculateHash();
    bool imageUnsaved();

    // image.cpp
    void rotLeft();
    void rotRight();
    void flipV();
    void flipH();
    void setCrop(float buffer = 0.1f);
    void loadFileintoBuffer();
    void updateSaveState();


    // imageProcessing.cpp
    void processBaseColor();
    void blurImage();
    void processMinMax(ocioSetting ocioSet);
    void setMinMax(ocioSetting ocioSet);
    void calcProxyDim();
    void resizeProxy();
    void processCPU(ocioSetting ocioSet);

};

// imageIO.cpp
std::variant<image, std::string> readImage(std::string imagePath, rawSetting rawSet, ocioSetting ocioSet, bool background = false);
std::variant<image, std::string> readDataImage(std::string imagePath, rawSetting rawSet, ocioSetting ocioSet, bool background = false);
std::variant<image, std::string> readRawImage(std::string imagePath, bool background = false);
std::variant<image, std::string> readImageOIIO(std::string imagePath, ocioSetting ocioSet, bool background = false);

// image.cpp
renderParams img_to_param(image* _img);

struct float4 {
    float x;
    float y;
    float z;
    float w;

    float4(){}
    float4(float x) : x(x), y(x), z(x), w(x){}
    float4(float x, float y, float z) : x(x), y(y), z(z){}
    //float4(const float arr[3]) : x(arr[0]), y(arr[1]), z(arr[2]), w(0.0f) {}
    float4(const float arr[4]) : x(arr[0]), y(arr[1]), z(arr[2]), w(arr[3]) {}
    float4(float x, float y, float z, float w) : x(x), y(y), z(z), w(w){}

    // Component-wise addition
    float4 operator+(const float4& other) const {
        return float4(x + other.x, y + other.y, z + other.z, w + other.w);
    }

    // Scalar addition
    float4 operator+(float scalar) const {
        return float4(x + scalar, y + scalar, z + scalar, w + scalar);
    }

    // Component-wise subtraction
    float4 operator-(const float4& other) const {
        return float4(x - other.x, y - other.y, z - other.z, w - other.w);
    }

    // Scalar subtraction
    float4 operator-(float scalar) const {
        return float4(x - scalar, y - scalar, z - scalar, w - scalar);
    }

    // Component-wise multiplication
    float4 operator*(const float4& other) const {
        return float4(x * other.x, y * other.y, z * other.z, w * other.w);
    }

    // Scalar multiplication
    float4 operator*(float scalar) const {
        return float4(x * scalar, y * scalar, z * scalar, w * scalar);
    }

    // Component-wise division
    float4 operator/(const float4& other) const {
        return float4(x / other.x, y / other.y, z / other.z, w / other.w);
    }

    // Scalar division
    float4 operator/(float scalar) const {
        return float4(x / scalar, y / scalar, z / scalar, w / scalar);
    }

    // Assignment operators for convenience
    float4& operator+=(const float4& other) {
        x += other.x; y += other.y; z += other.z; w += other.w;
        return *this;
    }

    float4& operator+=(float scalar) {
        x += scalar; y += scalar; z += scalar; w += scalar;
        return *this;
    }

    float4& operator-=(const float4& other) {
        x -= other.x; y -= other.y; z -= other.z; w -= other.w;
        return *this;
    }

    float4& operator-=(float scalar) {
        x -= scalar; y -= scalar; z -= scalar; w -= scalar;
        return *this;
    }

    float4& operator*=(const float4& other) {
        x *= other.x; y *= other.y; z *= other.z; w *= other.w;
        return *this;
    }

    float4& operator*=(float scalar) {
        x *= scalar; y *= scalar; z *= scalar; w *= scalar;
        return *this;
    }

    float4& operator/=(const float4& other) {
        x /= other.x; y /= other.y; z /= other.z; w /= other.w;
        return *this;
    }

    float4& operator/=(float scalar) {
        x /= scalar; y /= scalar; z /= scalar; w /= scalar;
        return *this;
    }

};

// Free functions for scalar operations where scalar is on the left side
inline float4 operator+(float scalar, const float4& vec) {
    return vec + scalar;
}

inline float4 operator-(float scalar, const float4& vec) {
    return float4(scalar - vec.x, scalar - vec.y, scalar - vec.z, scalar - vec.w);
}

inline float4 operator*(float scalar, const float4& vec) {
    return vec * scalar;
}

inline float4 operator/(float scalar, const float4& vec) {
    return float4(scalar / vec.x, scalar / vec.y, scalar / vec.z, scalar / vec.w);
}

inline float4 max(float4 a, float4 b) {
    return float4(a.x > b.x ? a.x : b.x,
        a.y > b.y ? a.y : b.y,
        a.z > b.z ? a.z : b.z,
        a.w > b.w ? a.w : b.w);
}

inline float4 pow(float4 a, float4 b) {
    return float4(pow(a.x, b.x),
    pow(a.y, b.y),
    pow(a.z, b.z),
    pow(a.w, b.w));
}

inline float4 clamp(float4 a, float4 b, float4 c) {
    return float4(a.x < b.x ? b.x : a.x > c.x ? c.x : a.x,
        a.y < b.y ? b.y : a.y > c.y ? c.y : a.y,
        a.z < b.z ? b.z : a.z > c.z ? c.z : a.z,
        a.w < b.w ? b.w : a.w > c.w ? c.w : a.w
    );
}

#endif
