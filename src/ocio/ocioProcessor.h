#ifndef _ocioprocessor_h
#define _ocioprocessor_h

#include "structs.h"
#include <string>
#include <sstream>
#include <thread>
#include <vector>

// OCIO
#include <OpenColorIO/OpenColorIO.h>
namespace OCIO = OCIO_NAMESPACE;


#include "logger.h"

class ocioProcessor {
    public:
    ocioProcessor(){};
    ~ocioProcessor(){};

    bool initialize(std::string &configFile);
    bool initAltConfig(std::string path);
    void setExtActive();
    void setIntActive();

    void processImage(float* img, unsigned int width, unsigned int height, ocioSetting &ocioSet);
    std::string getMetalKernel(ocioSetting ocioSet);
    void processImageGPU(float *img, unsigned int width, unsigned int height);


    std::vector<char*> colorspaces;
    std::vector<char*> displays;
    std::vector<std::vector<char*>> views;


    bool validExternal = false;

    private:
    bool useExt = false;
    std::string internalConfig;
    std::string externalConfigPath;

    OCIO::ConstConfigRcPtr OCIOconfig;
    OCIO::ConstConfigRcPtr extOCIOconfig;
};

// OCIO
extern ocioProcessor ocioProc;


#endif
