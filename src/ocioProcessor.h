#ifndef _ocioprocessor_h
#define _ocioprocessor_h

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
    void processImage(float* img, unsigned int width, unsigned int height);
    std::string getMetalKernel();
    void processImageGPU(float *img, unsigned int width, unsigned int height);

    std::vector<char*> displays;
    std::vector<std::vector<char*>> views;

    int displayOp;
    int viewOp;

    private:

    OCIO::ConstConfigRcPtr OCIOconfig;
};

// OCIO
extern ocioProcessor ocioProc;


#endif
