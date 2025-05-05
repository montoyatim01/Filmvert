#ifndef _ocioprocessor_h
#define _ocioprocessor_h

#include <string>
#include <sstream>
#include <thread>

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
    void processImageGPU(float *img, unsigned int width, unsigned int height);

    private:

    OCIO::ConstConfigRcPtr OCIOconfig;
};

// OCIO
extern ocioProcessor ocioProc;


#endif
